/**
 * @file  CryptoEngine.cpp
 * @brief Flash encryption engine — implementation.
 *
 * Cipher operations go through an abstraction layer (cipherBlockEncrypt /
 * cipherBlockDecrypt) to facilitate future migration to AES-128.
 * PBKDF2 uses a dual CBC-MAC PRF producing 128 bits per iteration.
 * Read PIN is hashed with a lightweight KDF (1000 rounds).
 */

#include "CryptoEngine.h"
#include "KeylogConfig.h"
#include "I18n.h"
#include "BuildSeed.h"
#include <pico/unique_id.h>
#include <hardware/structs/rosc.h>
#include <hardware/timer.h>

// =============================================================================
// INTERNAL STATE
// =============================================================================

/** @brief Data Encryption Key — loaded into RAM only after successful unlock. */
static uint32_t dek[4] = {0};

/** @brief Salt from the current crypto header (cached for verification). */
static uint8_t stored_salt[8] = {0};

/** @brief Password check hash from the current crypto header. */
static uint8_t stored_pwd_check[16] = {0};

/** @brief Encrypted DEK from the current crypto header. */
static uint8_t stored_encrypted_dek[16] = {0};

/** @brief True only after successful password verification + DEK decryption. */
static bool unlocked = false;

/** @brief True once a valid header has been read (even if not unlocked). */
static bool header_valid = false;

/** @brief Flags from the crypto header (bit 0 = user pwd, bit 1 = read PIN). */
static uint8_t stored_flags = 0;

/** @brief Active keyboard layout ID, persisted in the crypto header. */
static uint8_t stored_layout_id = 0;

/** @brief Active interface language ID, persisted in the crypto header. */
static uint8_t stored_lang_id = 0;

/** @brief KDF-derived PIN hash from the crypto header (8 bytes). */
static uint8_t stored_pin_hash[PIN_HASH_LEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/** @brief Firmware CRC-32 from the crypto header. */
static uint32_t stored_fw_crc32 = 0xFFFFFFFF;

// =============================================================================
// =============================================================================
//
// All cryptographic block operations go through these wrappers.
// To migrate to AES-128 (e.g. on RP2350 with hardware accelerator):
//   1. Replace cipherBlockEncrypt/cipherBlockDecrypt with AES equivalents.
//   2. Update key schedule (AES uses different expansion).
//   3. Adjust block size if needed (AES = 128-bit, XTEA = 64-bit).
//
// Current implementation: XTEA with 32 Feistel rounds, 128-bit key.
//

/**
 * @brief Encrypt a 64-bit block in place with a 128-bit key.
 * @param v    Two uint32_t values (64-bit block).
 * @param key  Four uint32_t values (128-bit key).
 */
static void cipherBlockEncrypt(uint32_t v[2], const uint32_t key[4]) {
    uint32_t v0  = v[0];
    uint32_t v1  = v[1];
    uint32_t sum = 0;
    const uint32_t delta = 0x9E3779B9;

    for (int i = 0; i < 32; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
    }

    v[0] = v0;
    v[1] = v1;
}

/**
 * @brief Decrypt a 64-bit block in place with a 128-bit key.
 * @param v    Two uint32_t values (64-bit block).
 * @param key  Four uint32_t values (128-bit key).
 */
static void cipherBlockDecrypt(uint32_t v[2], const uint32_t key[4]) {
    uint32_t v0  = v[0];
    uint32_t v1  = v[1];
    const uint32_t delta = 0x9E3779B9;
    uint32_t sum = delta * 32;

    for (int i = 0; i < 32; i++) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }

    v[0] = v0;
    v[1] = v1;
}

// =============================================================================
// ECB MODE — Encrypt/decrypt a 128-bit value (two 64-bit blocks)
// =============================================================================

static void cipherECBEncrypt16(uint8_t data[16], const uint32_t key[4]) {
    uint32_t* blocks = (uint32_t*)data;
    cipherBlockEncrypt(&blocks[0], key);
    cipherBlockEncrypt(&blocks[2], key);
}

static void cipherECBDecrypt16(uint8_t data[16], const uint32_t key[4]) {
    uint32_t* blocks = (uint32_t*)data;
    cipherBlockDecrypt(&blocks[0], key);
    cipherBlockDecrypt(&blocks[2], key);
}

// =============================================================================
// CTR MODE — Stream encryption for data pages
// =============================================================================

static void cipherCTR(
    uint8_t* data, uint16_t len,
    const uint32_t key[4], uint32_t nonce
) {
    for (uint16_t offset = 0; offset < len; offset += 8) {
        uint32_t ctr_block[2];
        ctr_block[0] = nonce;
        ctr_block[1] = offset / 8;
        cipherBlockEncrypt(ctr_block, key);

        uint8_t* keystream = (uint8_t*)ctr_block;
        uint16_t block_len = ((len - offset) >= 8) ? 8 : (len - offset);

        for (uint16_t j = 0; j < block_len; j++) {
            data[offset + j] ^= keystream[j];
        }
    }
}

// =============================================================================
// DUAL CBC-MAC — 128-bit PRF output for PBKDF2
// =============================================================================

/**
 * @brief Compute dual CBC-MAC over an input buffer, producing 16 bytes.
 *
 * @details Two independent CBC-MAC passes with different IVs:
 *          - Pass 1: IV = {0x00000000, 0x00000000}
 *          - Pass 2: IV = {0xFFFFFFFF, 0xFFFFFFFF}
 *          Concatenation yields 16 bytes (128 bits).
 *
 * @param key        Cipher key (4 × uint32_t).
 * @param input      Input data to MAC.
 * @param input_len  Length of input in bytes.
 * @param output     16-byte output buffer.
 */
static void dualCbcMac(
    const uint32_t key[4],
    const uint8_t* input, uint16_t input_len,
    uint8_t output[16]
) {
    // Pass 1: IV = {0, 0}
    uint32_t s1[2] = {0, 0};
    for (uint16_t i = 0; i < input_len; i += 8) {
        uint32_t block[2] = {0, 0};
        uint16_t chunk = ((input_len - i) >= 8) ? 8 : (input_len - i);
        memcpy(block, &input[i], chunk);
        s1[0] ^= block[0];
        s1[1] ^= block[1];
        cipherBlockEncrypt(s1, key);
    }

    // Pass 2: IV = {0xFFFFFFFF, 0xFFFFFFFF}
    uint32_t s2[2] = {0xFFFFFFFF, 0xFFFFFFFF};
    for (uint16_t i = 0; i < input_len; i += 8) {
        uint32_t block[2] = {0, 0};
        uint16_t chunk = ((input_len - i) >= 8) ? 8 : (input_len - i);
        memcpy(block, &input[i], chunk);
        s2[0] ^= block[0];
        s2[1] ^= block[1];
        cipherBlockEncrypt(s2, key);
    }

    // Concatenate: [s1 || s2] = 16 bytes
    memcpy(output, s1, 8);
    memcpy(output + 8, s2, 8);
}

// =============================================================================
// PASSWORD TO CIPHER KEY — Deterministic folding of arbitrary-length passwords
// =============================================================================

/**
 * @brief Derive a 128-bit cipher key from a password string.
 *
 * @param password  Null-terminated password string.
 * @param key       Output: 4 × uint32_t key.
 *
 * * @note Uses size_t for length to avoid uint8_t overflow on long inputs.
 */
static void passwordToKey(const char* password, uint32_t key[4]) {
    key[0] = 0x67452301;
    key[1] = 0xEFCDAB89;
    key[2] = 0x98BADCFE;
    key[3] = 0x10325476;

    size_t len = strlen(password);
    if (len == 0) len = 1;

    // Clamp to PASSWORD_MAX_LEN to prevent excessively long inputs
    if (len > PASSWORD_MAX_LEN) len = PASSWORD_MAX_LEN;

    for (size_t i = 0; i < len; i++) {
        uint8_t idx = (uint8_t)(i % 4);
        key[idx] ^= ((uint32_t)(uint8_t)password[i]) << ((i % 4) * 8);
        key[idx] *= 0x5BD1E995;
        key[idx] ^= key[idx] >> 15;
    }

    for (int i = 0; i < 4; i++) {
        key[i] ^= key[(i + 1) % 4] >> 13;
        key[i] *= 0xCC9E2D51;
        key[i] ^= key[i] >> 16;
    }
}

// =============================================================================
// PBKDF2 — Password + Salt + Board ID → 256-bit Output
// =============================================================================

static void readBoardId(uint8_t out[8]) {
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    memcpy(out, board_id.id, 8);
}

/**
 * @brief Derive N bytes from password + salt + board_id via PBKDF2.
 *
 * @details Parameterized round count allows reuse for both the main
 *          password KDF (12000 rounds) and the lightweight PIN KDF
 *          (1000 rounds).
 *
 * @param password    Null-terminated password string.
 * @param salt        Random salt buffer (8 bytes).
 * @param rounds      Number of PBKDF2 iterations per block.
 * @param out         Output buffer.
 * @param out_blocks  Number of 16-byte blocks to produce (1 or 2).
 */
static void kdf_internal(
    const char* password, const uint8_t* salt,
    uint32_t rounds, uint8_t* out, uint8_t out_blocks
) {
    // Derive cipher key from password
    uint32_t prf_key[4];
    passwordToKey(password, prf_key);

    // Build extended salt: salt[8] || board_id[8] = 16 bytes
    uint8_t ext_salt[20];
    memcpy(ext_salt, salt, 8);

    uint8_t board_id[8];
    readBoardId(board_id);
    memcpy(&ext_salt[8], board_id, 8);

    for (uint8_t block_idx = 1; block_idx <= out_blocks; block_idx++) {
        // Append block index (big-endian) to extended salt
        ext_salt[16] = 0;
        ext_salt[17] = 0;
        ext_salt[18] = 0;
        ext_salt[19] = block_idx;

        // U_1 = PRF(key, ext_salt || INT(block_idx))
        uint8_t u_prev[16];
        dualCbcMac(prf_key, ext_salt, 20, u_prev);

        // T = U_1
        uint8_t t_accum[16];
        memcpy(t_accum, u_prev, 16);

        // Iterate: U_i = PRF(key, U_{i-1}), T ^= U_i
        for (uint32_t r = 1; r < rounds; r++) {
            uint8_t u_next[16];
            dualCbcMac(prf_key, u_prev, 16, u_next);

            for (uint8_t j = 0; j < 16; j++) {
                t_accum[j] ^= u_next[j];
            }

            memcpy(u_prev, u_next, 16);
        }

        // Copy block output
        memcpy(&out[(block_idx - 1) * 16], t_accum, 16);
    }

    // Scrub intermediaries
    secure_memzero(prf_key, sizeof(prf_key));
    secure_memzero(ext_salt, sizeof(ext_salt));
}

/**
 * @brief Full-strength KDF for user passwords (12000 rounds, 2 blocks).
 */
static void kdf(const char* password, const uint8_t* salt, uint32_t out[8]) {
    kdf_internal(password, salt, KDF_ROUNDS, (uint8_t*)out, 2);
}

/**
 * @brief Lightweight KDF for PIN hashing (1000 rounds, 1 block).
 *
 * @details Produces 16 bytes but only the first PIN_HASH_LEN (8) are
 *          stored. This makes offline brute-force of 10000 PINs take
 *          ~10 seconds on RP2040 instead of microseconds.
 */
static void kdf_pin(const char* pin, const uint8_t* salt, uint8_t out[16]) {
    kdf_internal(pin, salt, KDF_PIN_ROUNDS, out, 1);
}

// =============================================================================
// RANDOM NUMBER GENERATION — ROSC + von Neumann debiasing + timer jitter
// =============================================================================

static uint8_t getDebiasedBit() {
    while (true) {
        uint8_t a = rosc_hw->randombit & 0x01;
        for (volatile int j = 0; j < 5; j++) { /* spin */ }
        uint8_t b = rosc_hw->randombit & 0x01;
        for (volatile int j = 0; j < 5; j++) { /* spin */ }
        if (a != b) {
            return a;
        }
    }
}

static uint8_t getRandomByte() {
    uint8_t byte_val = 0;
    for (int i = 0; i < 8; i++) {
        uint32_t jitter = time_us_32();
        byte_val = (byte_val << 1) | getDebiasedBit();
        byte_val ^= (uint8_t)(jitter & 0x01);
    }
    return byte_val;
}

static void fillRandom(uint8_t* buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = getRandomByte();
    }
}

// =============================================================================
// CRYPTO HEADER PERSISTENCE
// =============================================================================

/**
 * @brief Build and program a CryptoHeader to the flash crypto sector.
 *
 * @details Includes all current fields: pin_hash, fw_crc32.
 *          nonce_floor moved to FlashMeta (no longer in this header).
 */
static void writeCryptoHeader(
    const uint32_t raw_dek[4],
    const uint8_t salt[8],
    const uint32_t kek[4],
    const uint8_t pwd_check[16],
    uint8_t flags,
    void (*flash_op)(bool, uint32_t, const uint8_t*)
) {
    // Encrypt the DEK with KEK
    uint8_t enc_dek[16];
    memcpy(enc_dek, raw_dek, 16);
    cipherECBEncrypt16(enc_dek, kek);

    // Build the page
    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, FLASH_PAGE_SIZE);

    CryptoHeader* h = (CryptoHeader*)page;
    h->magic        = CRYPTO_MAGIC;
    h->version      = CRYPTO_VERSION;
    h->flags        = flags;
    h->layout_id    = stored_layout_id;
    h->lang_id      = stored_lang_id;
    memcpy(h->salt, salt, 8);
    memcpy(h->pwd_check, pwd_check, 16);
    memcpy(h->encrypted_dek, enc_dek, 16);
    memcpy(h->pin_hash, stored_pin_hash, PIN_HASH_LEN);
    h->fw_crc32     = stored_fw_crc32;
    memset(h->_reserved2, 0, 2);

    // CRC over bytes [0..63]
    h->crc16 = calcCRC16(page, CRYPTO_HEADER_CRC_SPAN);

    // Erase the crypto sector and program the header page
    flash_op(true, FLASH_CRYPTO_START, nullptr);
    flash_op(false, FLASH_CRYPTO_START, page);

    // Update cached copies
    memcpy(stored_salt, salt, 8);
    memcpy(stored_pwd_check, pwd_check, 16);
    memcpy(stored_encrypted_dek, enc_dek, 16);
    stored_flags = flags;
    header_valid = true;
}

/**
 * @brief Rewrite the crypto header from cached state (no key changes).
 *
 * @details Used when only metadata fields change (layout_id, pin_hash,
 *          fw_crc32, flags).
 */
static void rewriteHeaderInPlace(
    void (*flash_op)(bool, uint32_t, const uint8_t*)
) {
    if (!header_valid) return;

    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, FLASH_PAGE_SIZE);

    CryptoHeader* h = (CryptoHeader*)page;
    h->magic        = CRYPTO_MAGIC;
    h->version      = CRYPTO_VERSION;
    h->flags        = stored_flags;
    h->layout_id    = stored_layout_id;
    h->lang_id      = stored_lang_id;
    memcpy(h->salt, stored_salt, 8);
    memcpy(h->pwd_check, stored_pwd_check, 16);
    memcpy(h->encrypted_dek, stored_encrypted_dek, 16);
    memcpy(h->pin_hash, stored_pin_hash, PIN_HASH_LEN);
    h->fw_crc32     = stored_fw_crc32;
    memset(h->_reserved2, 0, 2);

    h->crc16 = calcCRC16(page, CRYPTO_HEADER_CRC_SPAN);

    flash_op(true, FLASH_CRYPTO_START, nullptr);
    flash_op(false, FLASH_CRYPTO_START, page);
}

// =============================================================================
// PUBLIC API — Core crypto operations
// =============================================================================

bool cryptoInit(void (*flash_op)(bool, uint32_t, const uint8_t*)) {
    unlocked     = false;
    header_valid = false;

    const CryptoHeader* h = (const CryptoHeader*)(
        XIP_BASE + FLASH_CRYPTO_START);

    if (h->magic == CRYPTO_MAGIC && h->version == CRYPTO_VERSION) {
        uint16_t crc = calcCRC16(
            (const uint8_t*)h, CRYPTO_HEADER_CRC_SPAN);

        if (crc == h->crc16) {
            memcpy(stored_salt, h->salt, 8);
            memcpy(stored_pwd_check, h->pwd_check, 16);
            memcpy(stored_encrypted_dek, h->encrypted_dek, 16);
            memcpy(stored_pin_hash, h->pin_hash, PIN_HASH_LEN);
            stored_fw_crc32    = h->fw_crc32;
            stored_flags       = h->flags;
            stored_layout_id   = (h->layout_id <= LAYOUT_ID_MAX) ? h->layout_id : 0;
            stored_lang_id     = (h->lang_id <= LANG_ID_MAX) ? h->lang_id : 0;
            header_valid       = true;
            return false;  // LOGIN required
        }
    }

    // No valid header — first boot or version mismatch. Fresh crypto state.
    cryptoReset(flash_op);
    return true;
}

bool cryptoUnlock(const char* password) {
    if (!header_valid) return false;
    if (unlocked) return true;

    uint32_t derived[8];
    kdf(password, stored_salt, derived);

    if (!timing_safe_equal(&derived[4], stored_pwd_check, 16)) {
        secure_memzero(derived, sizeof(derived));
        return false;
    }

    uint32_t kek[4];
    memcpy(kek, derived, 16);

    uint8_t dec_dek[16];
    memcpy(dec_dek, stored_encrypted_dek, 16);
    cipherECBDecrypt16(dec_dek, kek);

    memcpy(dek, dec_dek, 16);
    unlocked = true;

    secure_memzero(kek, sizeof(kek));
    secure_memzero(derived, sizeof(derived));
    secure_memzero(dec_dek, sizeof(dec_dek));

    return true;
}

bool cryptoVerifyPassword(const char* password) {
    if (!header_valid) return false;

    uint32_t derived[8];
    kdf(password, stored_salt, derived);

    bool match = timing_safe_equal(&derived[4], stored_pwd_check, 16);

    secure_memzero(derived, sizeof(derived));
    return match;
}

bool cryptoChangePassword(
    const char* old_password,
    const char* new_password,
    void (*flash_op)(bool, uint32_t, const uint8_t*)
) {
    if (!unlocked) return false;
    if (!cryptoVerifyPassword(old_password)) return false;

    uint8_t new_salt[8];
    fillRandom(new_salt, 8);

    uint32_t derived[8];
    kdf(new_password, new_salt, derived);

    uint32_t new_kek[4];
    memcpy(new_kek, derived, 16);

    uint8_t new_check[16];
    memcpy(new_check, &derived[4], 16);

    writeCryptoHeader(dek, new_salt, new_kek, new_check, stored_flags, flash_op);

    secure_memzero(new_kek, sizeof(new_kek));
    secure_memzero(derived, sizeof(derived));

    return true;
}

void cryptoReset(void (*flash_op)(bool, uint32_t, const uint8_t*)) {
    uint32_t new_dek[4];
    fillRandom((uint8_t*)new_dek, 16);

    uint8_t new_salt[8];
    fillRandom(new_salt, 8);

    char build_pwd[BUILD_PWD_DERIVED_LEN + 1];
    deriveBuildPassword(build_pwd);

    uint32_t derived[8];
    kdf(build_pwd, new_salt, derived);
    secure_memzero(build_pwd, sizeof(build_pwd));

    uint32_t kek[4];
    memcpy(kek, derived, 16);

    uint8_t check[16];
    memcpy(check, &derived[4], 16);

    // Clear PIN hash, reset fw_crc, language on reset
    memset(stored_pin_hash, 0xFF, PIN_HASH_LEN);
    stored_fw_crc32    = 0xFFFFFFFF;
    stored_lang_id     = 0;

    writeCryptoHeader(new_dek, new_salt, kek, check, 0, flash_op);

    memcpy(dek, new_dek, 16);
    unlocked = true;

    secure_memzero(kek, sizeof(kek));
    secure_memzero(derived, sizeof(derived));
    secure_memzero(new_dek, sizeof(new_dek));
}

void cryptoEncryptPage(uint8_t* page_data, uint32_t nonce) {
    if (!unlocked) return;
    cipherCTR(page_data, PAGE_COUNT_OFFSET, dek, nonce);
}

void cryptoDecryptPage(uint8_t* page_data, uint32_t nonce) {
    // CTR mode: encrypt == decrypt
    cryptoEncryptPage(page_data, nonce);
}

bool cryptoIsUnlocked() {
    return unlocked;
}

bool cryptoHasUserPassword() {
    return (stored_flags & CRYPTO_FLAG_USER_PWD) != 0;
}

bool cryptoSetUserPassword(
    const char* new_password,
    void (*flash_op)(bool, uint32_t, const uint8_t*)
) {
    if (!unlocked) return false;
    if (stored_flags & CRYPTO_FLAG_USER_PWD) return false;

    uint8_t new_salt[8];
    fillRandom(new_salt, 8);

    uint32_t derived[8];
    kdf(new_password, new_salt, derived);

    uint32_t new_kek[4];
    memcpy(new_kek, derived, 16);

    uint8_t new_check[16];
    memcpy(new_check, &derived[4], 16);

    writeCryptoHeader(dek, new_salt, new_kek, new_check,
                      stored_flags | CRYPTO_FLAG_USER_PWD, flash_op);

    secure_memzero(new_kek, sizeof(new_kek));
    secure_memzero(derived, sizeof(derived));

    return true;
}

bool cryptoRemoveUserPassword(
    const char* current_password,
    void (*flash_op)(bool, uint32_t, const uint8_t*)
) {
    if (!unlocked) return false;
    if (!(stored_flags & CRYPTO_FLAG_USER_PWD)) return false;
    if (!cryptoVerifyPassword(current_password)) return false;

    uint8_t new_salt[8];
    fillRandom(new_salt, 8);

    char build_pwd[BUILD_PWD_DERIVED_LEN + 1];
    deriveBuildPassword(build_pwd);

    uint32_t derived[8];
    kdf(build_pwd, new_salt, derived);
    secure_memzero(build_pwd, sizeof(build_pwd));

    uint32_t new_kek[4];
    memcpy(new_kek, derived, 16);

    uint8_t new_check[16];
    memcpy(new_check, &derived[4], 16);

    // Remove user password flag but preserve read PIN flag
    uint8_t new_flags = stored_flags & ~CRYPTO_FLAG_USER_PWD;
    writeCryptoHeader(dek, new_salt, new_kek, new_check, new_flags, flash_op);

    secure_memzero(new_kek, sizeof(new_kek));
    secure_memzero(derived, sizeof(derived));

    return true;
}

void cryptoGetBuildPassword(char* out) {
    deriveBuildPassword(out);
}

uint8_t cryptoGetLayoutId() {
    return stored_layout_id;
}

void cryptoSetLayoutId(uint8_t layout_id,
    void (*flash_op)(bool, uint32_t, const uint8_t*))
{
    if (layout_id > LAYOUT_ID_MAX) return;
    if (layout_id == stored_layout_id) return;

    stored_layout_id = layout_id;

    if (!header_valid || !unlocked) return;

    rewriteHeaderInPlace(flash_op);
}

// =============================================================================
// PUBLIC API — Language ID (persisted alongside layout)
// =============================================================================

uint8_t cryptoGetLangId() {
    return stored_lang_id;
}

void cryptoSetLangId(uint8_t lang_id,
    void (*flash_op)(bool, uint32_t, const uint8_t*))
{
    if (lang_id > LANG_ID_MAX) return;
    if (lang_id == stored_lang_id) return;

    stored_lang_id = lang_id;

    if (!header_valid || !unlocked) return;

    rewriteHeaderInPlace(flash_op);
}

// =============================================================================
// PUBLIC API — DEK Zeroing
// =============================================================================

void cryptoZeroDek() {
    secure_memzero(dek, sizeof(dek));
    unlocked = false;
}

// =============================================================================
// PUBLIC API — Read PIN with KDF
// =============================================================================

/**
 * @brief Compute a KDF-derived hash of a PIN.
 *
 * @details Uses PBKDF2 with KDF_PIN_ROUNDS (1000) to produce 16 bytes,
 *          truncated to PIN_HASH_LEN (8). This makes offline brute-force
 *          of the 10000 PIN space take ~10 seconds on RP2040.
 *
 * @param pin   4-digit ASCII PIN string.
 * @param hash  Output: PIN_HASH_LEN-byte hash buffer.
 */
static void computePinHash(const char* pin, uint8_t hash[PIN_HASH_LEN]) {
    uint8_t full_hash[16];
    kdf_pin(pin, stored_salt, full_hash);
    memcpy(hash, full_hash, PIN_HASH_LEN);
    secure_memzero(full_hash, sizeof(full_hash));
}

bool cryptoSetReadPin(const char* pin,
    void (*flash_op)(bool, uint32_t, const uint8_t*))
{
    if (!unlocked) return false;
    if (!pin || strlen(pin) != READ_PIN_LEN) return false;

    // Validate: all characters must be ASCII digits
    for (uint8_t i = 0; i < READ_PIN_LEN; i++) {
        if (pin[i] < '0' || pin[i] > '9') return false;
    }

    // Store KDF-derived hash instead of plaintext
    computePinHash(pin, stored_pin_hash);
    stored_flags |= CRYPTO_FLAG_READ_PIN;

    rewriteHeaderInPlace(flash_op);
    return true;
}

bool cryptoClearReadPin(const char* pin,
    void (*flash_op)(bool, uint32_t, const uint8_t*))
{
    if (!unlocked) return false;
    if (!(stored_flags & CRYPTO_FLAG_READ_PIN)) return false;
    if (!cryptoVerifyReadPin(pin)) return false;

    memset(stored_pin_hash, 0xFF, PIN_HASH_LEN);
    stored_flags &= ~CRYPTO_FLAG_READ_PIN;

    rewriteHeaderInPlace(flash_op);
    return true;
}

bool cryptoVerifyReadPin(const char* pin) {
    if (!(stored_flags & CRYPTO_FLAG_READ_PIN)) return false;
    if (!pin || strlen(pin) != READ_PIN_LEN) return false;

    // Compute KDF hash and compare against stored hash
    uint8_t candidate_hash[PIN_HASH_LEN];
    computePinHash(pin, candidate_hash);

    bool match = timing_safe_equal(candidate_hash, stored_pin_hash, PIN_HASH_LEN);
    secure_memzero(candidate_hash, sizeof(candidate_hash));
    return match;
}

bool cryptoHasReadPin() {
    return (stored_flags & CRYPTO_FLAG_READ_PIN) != 0;
}

// =============================================================================
// PUBLIC API — Firmware CRC
// =============================================================================

void cryptoSetFwCrc(uint32_t crc,
    void (*flash_op)(bool, uint32_t, const uint8_t*))
{
    stored_fw_crc32 = crc;

    if (!header_valid || !unlocked) return;

    rewriteHeaderInPlace(flash_op);
}

uint32_t cryptoGetFwCrc() {
    return stored_fw_crc32;
}

// =============================================================================
// PUBLIC API — RAWDUMP support
// =============================================================================

void cryptoGetRawDumpHeader(uint8_t out_enc_dek[16],
    uint8_t out_salt[8], uint8_t* out_flags)
{
    memcpy(out_enc_dek, stored_encrypted_dek, 16);
    memcpy(out_salt, stored_salt, 8);
    *out_flags = stored_flags;
}
