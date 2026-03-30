/**
 * @file  I18n.cpp
 * @brief Language registry — single definition of all language tables.
 */

#include "I18n.h"
#include "I18nEn.h"
#include "I18nPtBr.h"
#include "I18nEs.h"
#include "I18nFr.h"
#include "I18nDe.h"
#include "I18nIt.h"
#include "I18nJa.h"
#include "I18nZh.h"
#include "I18nKo.h"
#include "I18nRu.h"

// =============================================================================
// REGISTRY TABLE — Indexed by LANG_ID_*
// =============================================================================

const LangDescriptor __in_flash("i18n") lang_registry[LANG_COUNT] = {
    /* 0 */ { "EN",    "English",               &i18n_en   },
    /* 1 */ { "PT-BR", "Portugu\xC3\xAAs (BR)", &i18n_ptbr },
    /* 2 */ { "ES",    "Espa\xC3\xB1ol",        &i18n_es   },
    /* 3 */ { "FR",    "Fran\xC3\xA7""ais",     &i18n_fr   },
    /* 4 */ { "DE",    "Deutsch",               &i18n_de   },
    /* 5 */ { "IT",    "Italiano",              &i18n_it   },
    /* 6 */ { "JA",    "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E",  &i18n_ja },
    /* 7 */ { "ZH",    "\xE4\xB8\xAD\xE6\x96\x87",               &i18n_zh },
    /* 8 */ { "KO",    "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4",  &i18n_ko },
    /* 9 */ { "RU",    "\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9", &i18n_ru },
};

// =============================================================================
// ACTIVE LANGUAGE STATE
// =============================================================================

/** @brief Active language ID. */
static uint8_t active_lang_id = LANG_ID_DEFAULT;

/** @brief Pointer to the active string table (global accessor). */
const I18nStrings* i18n = &i18n_en;

void setActiveLanguage(uint8_t lang_id) {
    if (lang_id >= LANG_COUNT) {
        lang_id = LANG_ID_DEFAULT;
    }
    active_lang_id = lang_id;
    i18n = lang_registry[lang_id].strings;
}

uint8_t getActiveLangId() {
    return active_lang_id;
}
