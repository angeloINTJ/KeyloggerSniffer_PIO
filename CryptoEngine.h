/**
 * @file  CryptoEngine.h
 * @brief Flash encryption engine — public API.
 *
 * XTEA-ECB key envelope + XTEA-CTR per-page data encryption.
 * Key hierarchy: KEK (from PBKDF2) wraps a random DEK.
 * Board-unique ID is mixed into the KDF salt.
 */

#pragma once
#include <Arduino.h>
#include "KeylogConfig.h"

// =============================================================================
// CRYPTO HEADER — Stored in Flash Crypto Sector
// =============================================================================

#define CRYPTO_MAGIC   0x43525950
#define CRYPTO_VERSION 7

/**
 * @struct CryptoHeader
 * @brief  Persistent crypto state stored in the flash crypto sector.
 *
 * @details Layout (64 bytes, padded to 256 in flash page):
 *          [0..3]   magic          4B
 *          [4]      version        1B
 *          [5]      flags          1B
 *          [6]      layout_id      1B
 *          [7]      lang_id        1B  (interface language)
 *          [8..15]  salt           8B
 *          [16..31] pwd_check      16B
 *          [32..47] encrypted_dek  16B
 *          [48..55] pin_hash       8B  (KDF-derived, 0xFF if unset)
 *          [56..59] fw_crc32       4B
 *          [60..61] crc16          2B  (CRC-16 of bytes [0..59])
 *          [62..63] _reserved2     2B
 */
struct CryptoHeader {
    uint32_t magic;
    uint8_t  version;
    uint8_t  flags;
    uint8_t  layout_id;
    uint8_t  lang_id;         ///< Active interface language (LANG_ID_*)
    uint8_t  salt[8];
    uint8_t  pwd_check[16];
    uint8_t  encrypted_dek[16];
    uint8_t  pin_hash[8];
    uint32_t fw_crc32;
    uint16_t crc16;
    uint8_t  _reserved2[2];
};
// sizeof(CryptoHeader) = 64 bytes

#define CRYPTO_HEADER_CRC_SPAN 60

#define CRYPTO_FLAG_USER_PWD  0x01
#define CRYPTO_FLAG_READ_PIN  0x02
#define LAYOUT_ID_MAX         9

// =============================================================================
// KDF CONFIGURATION
// =============================================================================

#define KDF_ROUNDS     12000
#define KDF_PIN_ROUNDS 1000
#define READ_PIN_LEN   4
#define PIN_HASH_LEN   8

// =============================================================================
// PUBLIC API — Core crypto operations
// =============================================================================

bool cryptoInit(void (*flash_op)(bool, uint32_t, const uint8_t*));
bool cryptoUnlock(const char* password);
bool cryptoVerifyPassword(const char* password);
bool cryptoChangePassword(
    const char* old_password, const char* new_password,
    void (*flash_op)(bool, uint32_t, const uint8_t*));
bool cryptoSetUserPassword(
    const char* new_password,
    void (*flash_op)(bool, uint32_t, const uint8_t*));
bool cryptoRemoveUserPassword(
    const char* current_password,
    void (*flash_op)(bool, uint32_t, const uint8_t*));
void cryptoReset(void (*flash_op)(bool, uint32_t, const uint8_t*));
bool cryptoHasUserPassword();
void cryptoEncryptPage(uint8_t* page_data, uint32_t nonce);
void cryptoDecryptPage(uint8_t* page_data, uint32_t nonce);
bool cryptoIsUnlocked();

#define CRYPTO_BUILD_PWD_LEN 16

void cryptoGetBuildPassword(char* out);
uint8_t cryptoGetLayoutId();
void cryptoSetLayoutId(uint8_t layout_id,
    void (*flash_op)(bool, uint32_t, const uint8_t*));

// =============================================================================
// PUBLIC API — Language ID (persisted alongside layout)
// =============================================================================

uint8_t cryptoGetLangId();
void cryptoSetLangId(uint8_t lang_id,
    void (*flash_op)(bool, uint32_t, const uint8_t*));

// =============================================================================
// PUBLIC API — DEK Zeroing (S1)
// =============================================================================

void cryptoZeroDek();

// =============================================================================
// PUBLIC API — Read PIN with KDF
// =============================================================================

bool cryptoSetReadPin(const char* pin,
    void (*flash_op)(bool, uint32_t, const uint8_t*));
bool cryptoClearReadPin(const char* pin,
    void (*flash_op)(bool, uint32_t, const uint8_t*));
bool cryptoVerifyReadPin(const char* pin);
bool cryptoHasReadPin();

// =============================================================================
// PUBLIC API — Firmware CRC
// =============================================================================

void cryptoSetFwCrc(uint32_t crc,
    void (*flash_op)(bool, uint32_t, const uint8_t*));
uint32_t cryptoGetFwCrc();

// =============================================================================
// PUBLIC API — RAWDUMP support
// =============================================================================

void cryptoGetRawDumpHeader(uint8_t out_enc_dek[16],
    uint8_t out_salt[8], uint8_t* out_flags);
