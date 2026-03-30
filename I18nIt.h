/**
 * @file  I18nIt.h
 * @brief Italiano — i18n string table.
 */

#pragma once
#include "I18n.h"

static const I18nStrings __in_flash("i18n") i18n_it = {
    // Boot
    "APERTO (acquisizione in corso)",
    "BLOCCATO \xE2\x80\x94 LOGIN <password>",
    "SBLOCCATO AUTOMATICAMENTE",
    "Digita HELP per i comandi.",

    // Errori
    "[ERRORE] Bloccato. Prima LOGIN.",
    "[ERRORE] Password errata.",
    "[ERRORE] PIN errato.",
    "[ERRORE] Sistema non sbloccato.",
    "[ERRORE] Comando troppo lungo, scartato.",
    "Comando sconosciuto. Digita HELP.",
    "[ERRORE] Password troppo lunga (max %u caratteri).",
    "[ERRORE] Il PIN deve avere esattamente 4 cifre.",
    "[ERRORE] Il PIN deve contenere solo cifre 0-9.",
    "[INFO] Gia bloccato. Usa PASS o UNLOCK.",
    "[INFO] Gia aperto.",
    "[ERRORE] Fallito. Il sistema potrebbe essere gia bloccato.",
    "[ERRORE] Vecchia password errata o non sbloccato.",
    "[ERRORE] PIN errato o nessun PIN impostato.",

    // Operazioni
    "--- BLOCCO DEL SISTEMA CON PASSWORD ---",
    "--- SISTEMA BLOCCATO ---",
    "[INFO] DEK cancellata dalla RAM. Acquisizione IN PAUSA.",
    "[INFO] Il prossimo avvio richiedera LOGIN.",
    "[INFO] Usa LOGIN per riprendere l'acquisizione.",
    "Sblocco in corso... (KDF, ~200 ms)",
    "SBLOCCATO \xE2\x80\x94 Acquisizione attiva",
    "--- RIMOZIONE PASSWORD UTENTE ---",
    "--- SISTEMA SBLOCCATO (senza password) ---",
    "--- CAMBIO PASSWORD ---",
    "--- PASSWORD CAMBIATA ---",
    "--- CANCELLAZIONE LOG FLASH ---",
    "--- CANCELLAZIONE COMPLETATA ---",
    "[INFO] Password rimossa. Modalita aperta.",
    "--- PIN DI LETTURA IMPOSTATO ---",
    "--- PIN DI LETTURA RIMOSSO ---",
    "[INFO] Calcolo hash PIN (KDF, ~10 ms)...",
    "[INFO] DUMP, HEX, EXPORT, RAWDUMP richiedono PIN.",
    "\n--- ANNULLATO ---",
    "[OCCUPATO] Operazione in corso. Invia STOP.",

    // Info
    "[INFO] Gia sbloccato. Scrittura su flash.",
    "[INFO] Nessuna password impostata. Sistema aperto.",
    "[INFO] Tastiera ABNT2 rilevata \xE2\x80\x94 layout cambiato.",
    "[AVVISO] L'acquisizione si fermera fino al LOGIN.",

    // Lockout
    "[BLOCCO] Troppi tentativi falliti. Attendi %lu secondi.",

    // LIVE
    "--- MODALITA LIVE ATTIVATA ---",
    "--- MODALITA LIVE DISATTIVATA ---",
    "[AVVISO] Bloccato \xE2\x80\x94 LIVE dopo LOGIN.",

    // HELP
    "--- COMANDI (Modalita Bloccata) ---",
    "--- COMANDI (Modalita Aperta) ---",

    // LANG
    "Lingua cambiata in %s.",
    "[ERRORE] Lingua sconosciuta. Digita LANG.",

    // Dump/Export headers
    "--- INIZIO DUMP TESTO ---",
    "--- INIZIO DUMP HEX ---",
    "--- INIZIO RAWDUMP ---",
    "[ERRORE] Impossibile impostare il PIN di lettura.",

    // Help descriptions, usage, layout/lang UI, format messages
    "Sblocca per acquisizione",
    "Dump testo",
    "Dump esadecimale",
    "Esporta CSV (ultimi N opzionale)",
    "Pagine cifrate grezze",
    "Cambia password",
    "Rimuovi password",
    "Imposta password",
    "Imposta PIN lettura 4 cifre",
    "Rimuovi PIN lettura",
    "Mostra/cambia layout tastiera",
    "Mostra/cambia lingua",
    "Attiva/disattiva streaming",
    "Imposta timestamp Unix",
    "Cancella tutti i dati + reset",
    "Annulla operazione attiva",
    "Statistiche di sistema",
    "Questo messaggio di aiuto",
    "<password>",
    "<pin>",
    "<vecchia> <nuova>",
    "--- LAYOUT TASTIERA ---",
    "--- Usa: LAYOUT <nome> per cambiare ---",
    "--- LAYOUT CAMBIATO IN %s ---",
    "[ERRORE] Layout sconosciuto: '%s'",
    "--- LINGUE ---",
    "[ERRORE] PIN errato. (%u/%u)",
    "[ERRORE] Password errata. (%u/%u)",
    "[INFO] Salto %lu eventi...",
    "[INFO] Epoch attuale: %lu",
    "  %lu / %lu settori...",
    "--- ORA IMPOSTATA: epoch=%lu ---",
    "[USO] RPIN SET <pin> | RPIN CLEAR <pin>",
    "[INFO] PIN lettura: %s",
    "[USO] TIME <secondi_epoch_unix>",
    "[SICUREZZA] Cancella TUTTI i dati. Digita 'DEL CONFIRM'."
};
