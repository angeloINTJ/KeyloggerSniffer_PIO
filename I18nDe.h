/**
 * @file  I18nDe.h
 * @brief Deutsch — i18n string table.
 */

#pragma once
#include "I18n.h"

static const I18nStrings __in_flash("i18n") i18n_de = {
    // Boot
    "OFFEN (Aufzeichnung laeuft)",
    "GESPERRT \xE2\x80\x94 LOGIN <Passwort>",
    "AUTOMATISCH ENTSPERRT",
    "Geben Sie HELP fuer Befehle ein.",

    // Fehler
    "[FEHLER] Gesperrt. Zuerst LOGIN.",
    "[FEHLER] Falsches Passwort.",
    "[FEHLER] Falsche PIN.",
    "[FEHLER] System nicht entsperrt.",
    "[FEHLER] Befehl zu lang, verworfen.",
    "Unbekannter Befehl. Geben Sie HELP ein.",
    "[FEHLER] Passwort zu lang (max %u Zeichen).",
    "[FEHLER] PIN muss genau 4 Ziffern haben.",
    "[FEHLER] PIN darf nur Ziffern 0-9 enthalten.",
    "[INFO] Bereits gesperrt. Nutzen Sie PASS oder UNLOCK.",
    "[INFO] Bereits offen.",
    "[FEHLER] Fehlgeschlagen. System ist moeglicherweise bereits gesperrt.",
    "[FEHLER] Altes Passwort falsch oder nicht entsperrt.",
    "[FEHLER] Falsche PIN oder keine PIN gesetzt.",

    // Operationen
    "--- SYSTEM WIRD GESPERRT ---",
    "--- SYSTEM GESPERRT ---",
    "[INFO] DEK aus RAM geloescht. Aufzeichnung PAUSIERT.",
    "[INFO] Naechster Start erfordert LOGIN.",
    "[INFO] Nutzen Sie LOGIN um Aufzeichnung fortzusetzen.",
    "Entsperre... (KDF-Berechnung, ~200 ms)",
    "ENTSPERRT \xE2\x80\x94 Aufzeichnung aktiv",
    "--- PASSWORT WIRD ENTFERNT ---",
    "--- SYSTEM ENTSPERRT (ohne Passwort) ---",
    "--- PASSWORT WIRD GEAENDERT ---",
    "--- PASSWORT GEAENDERT ---",
    "--- FLASH-LOG WIRD GELOESCHT ---",
    "--- LOESCHUNG ABGESCHLOSSEN ---",
    "[INFO] Passwort entfernt. Offener Modus.",
    "--- LESE-PIN GESETZT ---",
    "--- LESE-PIN ENTFERNT ---",
    "[INFO] PIN-Hash wird berechnet (KDF, ~10 ms)...",
    "[INFO] DUMP, HEX, EXPORT, RAWDUMP erfordern PIN.",
    "\n--- ABGEBROCHEN ---",
    "[BELEGT] Operation laeuft. Senden Sie STOP.",

    // Info
    "[INFO] Bereits entsperrt. Schreibe auf Flash.",
    "[INFO] Kein Passwort gesetzt. System ist offen.",
    "[INFO] ABNT2-Tastatur erkannt \xE2\x80\x94 Layout gewechselt.",
    "[WARNUNG] Aufzeichnung pausiert bis LOGIN.",

    // Lockout
    "[SPERRE] Zu viele Fehlversuche. Warten Sie %lu Sekunden.",

    // LIVE
    "--- LIVE-MODUS EIN ---",
    "--- LIVE-MODUS AUS ---",
    "[WARNUNG] Gesperrt \xE2\x80\x94 LIVE nach LOGIN.",

    // HELP
    "--- BEFEHLE (Gesperrter Modus) ---",
    "--- BEFEHLE (Offener Modus) ---",

    // LANG
    "Sprache geaendert auf %s.",
    "[FEHLER] Unbekannte Sprache. Geben Sie LANG ein.",

    // Dump/Export headers
    "--- TEXTAUSGABE BEGINNT ---",
    "--- HEX-AUSGABE BEGINNT ---",
    "--- RAWDUMP BEGINNT ---",
    "[FEHLER] PIN konnte nicht gesetzt werden.",

    // Help descriptions, usage, layout/lang UI, format messages
    "Entsperren fuer Aufzeichnung",
    "Textausgabe",
    "Hex-Ausgabe",
    "CSV-Export (letzte N optional)",
    "Rohe verschluesselte Seiten",
    "Passwort aendern",
    "Passwort entfernen",
    "Passwort setzen",
    "4-stellige Lese-PIN setzen",
    "Lese-PIN entfernen",
    "Tastaturlayout anzeigen/aendern",
    "Sprache anzeigen/aendern",
    "Echtzeit-Streaming umschalten",
    "Unix-Zeitstempel setzen",
    "Alle Daten loeschen + Reset",
    "Aktive Operation abbrechen",
    "Systemstatistiken",
    "Diese Hilfemeldung",
    "<Passwort>",
    "<PIN>",
    "<alt> <neu>",
    "--- TASTATURLAYOUTS ---",
    "--- Nutzen Sie: LAYOUT <Name> ---",
    "--- LAYOUT GEAENDERT AUF %s ---",
    "[FEHLER] Unbekanntes Layout: '%s'",
    "--- SPRACHEN ---",
    "[FEHLER] Falsche PIN. (%u/%u)",
    "[FEHLER] Falsches Passwort. (%u/%u)",
    "[INFO] Ueberspringe %lu Ereignisse...",
    "[INFO] Aktueller Epoch: %lu",
    "  %lu / %lu Sektoren...",
    "--- ZEIT GESETZT: epoch=%lu ---",
    "[NUTZUNG] RPIN SET <PIN> | RPIN CLEAR <PIN>",
    "[INFO] Lese-PIN: %s",
    "[NUTZUNG] TIME <Unix_Epoch_Sekunden>",
    "[SICHERHEIT] Loescht ALLE Daten. Geben Sie 'DEL CONFIRM' ein."
};
