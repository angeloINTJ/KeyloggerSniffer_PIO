/**
 * @file  I18nFr.h
 * @brief Francais — i18n string table.
 */

#pragma once
#include "I18n.h"

static const I18nStrings __in_flash("i18n") i18n_fr = {
    // Boot
    "OUVERT (capture en cours)",
    "VERROUILLE \xE2\x80\x94 LOGIN <mot de passe>",
    "DEVERROUILLE AUTOMATIQUEMENT",
    "Tapez HELP pour les commandes.",

    // Erreurs
    "[ERREUR] Verrouille. Faites LOGIN d'abord.",
    "[ERREUR] Mot de passe incorrect.",
    "[ERREUR] PIN incorrect.",
    "[ERREUR] Systeme non deverrouille.",
    "[ERREUR] Commande trop longue, ignoree.",
    "Commande inconnue. Tapez HELP.",
    "[ERREUR] Mot de passe trop long (max %u caracteres).",
    "[ERREUR] Le PIN doit avoir exactement 4 chiffres.",
    "[ERREUR] Le PIN ne doit contenir que des chiffres 0-9.",
    "[INFO] Deja verrouille. Utilisez PASS ou UNLOCK.",
    "[INFO] Deja ouvert.",
    "[ERREUR] Echec. Le systeme est peut-etre deja verrouille.",
    "[ERREUR] Ancien mot de passe incorrect.",
    "[ERREUR] PIN incorrect ou aucun PIN defini.",

    // Operations
    "--- VERROUILLAGE DU SYSTEME ---",
    "--- SYSTEME VERROUILLE ---",
    "[INFO] DEK effacee de la RAM. Capture EN PAUSE.",
    "[INFO] Le prochain demarrage necessitera LOGIN.",
    "[INFO] Utilisez LOGIN pour reprendre la capture.",
    "Deverrouillage... (KDF, ~200 ms)",
    "DEVERROUILLE \xE2\x80\x94 Capture active",
    "--- SUPPRESSION DU MOT DE PASSE ---",
    "--- SYSTEME DEVERROUILLE (sans mot de passe) ---",
    "--- CHANGEMENT DE MOT DE PASSE ---",
    "--- MOT DE PASSE CHANGE ---",
    "--- EFFACEMENT DU LOG FLASH ---",
    "--- EFFACEMENT TERMINE ---",
    "[INFO] Mot de passe supprime. Mode ouvert.",
    "--- PIN DE LECTURE DEFINI ---",
    "--- PIN DE LECTURE SUPPRIME ---",
    "[INFO] Calcul du hash PIN (KDF, ~10 ms)...",
    "[INFO] DUMP, HEX, EXPORT, RAWDUMP exigent le PIN.",
    "\n--- ANNULE ---",
    "[OCCUPE] Operation en cours. Envoyez STOP.",

    // Info
    "[INFO] Deja deverrouille. Capture vers flash.",
    "[INFO] Aucun mot de passe. Systeme ouvert.",
    "[INFO] Clavier ABNT2 detecte \xE2\x80\x94 layout change.",
    "[AVERT] La capture sera en pause jusqu'a LOGIN.",

    // Lockout
    "[BLOCAGE] Trop de tentatives. Attendez %lu secondes.",

    // LIVE
    "--- MODE EN DIRECT ACTIVE ---",
    "--- MODE EN DIRECT DESACTIVE ---",
    "[AVERT] Verrouille \xE2\x80\x94 LIVE apres LOGIN.",

    // HELP
    "--- COMMANDES (Mode Verrouille) ---",
    "--- COMMANDES (Mode Ouvert) ---",

    // LANG
    "Langue changee en %s.",
    "[ERREUR] Langue inconnue. Tapez LANG.",

    // Dump/Export headers
    "--- DEBUT DU DUMP TEXTE ---",
    "--- DEBUT DU DUMP HEX ---",
    "--- DEBUT DU RAWDUMP ---",
    "[ERREUR] Echec de la definition du PIN.",

    // Help descriptions, usage, layout/lang UI, format messages
    "Deverrouiller pour capture",
    "Dump texte",
    "Dump hexadecimal",
    "Export CSV (derniers N optionnel)",
    "Pages chiffrees brutes",
    "Changer le mot de passe",
    "Supprimer le mot de passe",
    "Definir mot de passe",
    "Definir PIN de lecture 4 chiffres",
    "Supprimer PIN de lecture",
    "Afficher/changer la disposition du clavier",
    "Afficher/changer la langue",
    "Activer/desactiver le flux en direct",
    "Definir horodatage Unix",
    "Effacer toutes les donnees + reinit",
    "Annuler l'operation en cours",
    "Statistiques du systeme",
    "Ce message d'aide",
    "<mot_de_passe>",
    "<pin>",
    "<ancien> <nouveau>",
    "--- DISPOSITIONS CLAVIER ---",
    "--- Utilisez: LAYOUT <nom> pour changer ---",
    "--- DISPOSITION CHANGEE EN %s ---",
    "[ERREUR] Disposition inconnue: '%s'",
    "--- LANGUES ---",
    "[ERREUR] PIN incorrect. (%u/%u)",
    "[ERREUR] Mot de passe incorrect. (%u/%u)",
    "[INFO] Saut de %lu evenements...",
    "[INFO] Epoch actuel: %lu",
    "  %lu / %lu secteurs...",
    "--- HEURE DEFINIE: epoch=%lu ---",
    "[USAGE] RPIN SET <pin> | RPIN CLEAR <pin>",
    "[INFO] PIN de lecture: %s",
    "[USAGE] TIME <secondes_epoch_unix>",
    "[SECURITE] Efface TOUTES les donnees. Tapez 'DEL CONFIRM'."
};
