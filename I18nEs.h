/**
 * @file  I18nEs.h
 * @brief Espanol — i18n string table.
 */

#pragma once
#include "I18n.h"

static const I18nStrings __in_flash("i18n") i18n_es = {
    // Boot
    "ABIERTO (capturando)",
    "BLOQUEADO \xE2\x80\x94 LOGIN <contrasena>",
    "DESBLOQUEADO AUTOMATICAMENTE",
    "Escriba HELP para ver comandos.",

    // Errores
    "[ERROR] Bloqueado. Haga LOGIN primero.",
    "[ERROR] Contrasena incorrecta.",
    "[ERROR] PIN incorrecto.",
    "[ERROR] Sistema no desbloqueado.",
    "[ERROR] Comando demasiado largo, descartado.",
    "Comando desconocido. Escriba HELP.",
    "[ERROR] Contrasena muy larga (max %u caracteres).",
    "[ERROR] El PIN debe tener exactamente 4 digitos.",
    "[ERROR] El PIN solo puede contener digitos 0-9.",
    "[INFO] Ya esta bloqueado. Use PASS o UNLOCK.",
    "[INFO] Ya esta abierto.",
    "[ERROR] Fallo. El sistema puede estar ya bloqueado.",
    "[ERROR] Contrasena antigua incorrecta o no desbloqueado.",
    "[ERROR] PIN incorrecto o ningun PIN definido.",

    // Operaciones
    "--- BLOQUEANDO SISTEMA CON CONTRASENA ---",
    "--- SISTEMA BLOQUEADO ---",
    "[INFO] DEK borrada de RAM. Captura PAUSADA.",
    "[INFO] El proximo arranque requerira LOGIN.",
    "[INFO] Use LOGIN para reanudar la captura.",
    "Desbloqueando... (KDF, ~200 ms)",
    "DESBLOQUEADO \xE2\x80\x94 Captura activa",
    "--- ELIMINANDO CONTRASENA ---",
    "--- SISTEMA DESBLOQUEADO (sin contrasena) ---",
    "--- CAMBIANDO CONTRASENA ---",
    "--- CONTRASENA CAMBIADA ---",
    "--- BORRANDO LOG DEL FLASH ---",
    "--- BORRADO COMPLETO ---",
    "[INFO] Contrasena eliminada. Modo abierto.",
    "--- PIN DE LECTURA DEFINIDO ---",
    "--- PIN DE LECTURA ELIMINADO ---",
    "[INFO] Calculando hash del PIN (KDF, ~10 ms)...",
    "[INFO] DUMP, HEX, EXPORT, RAWDUMP requieren PIN.",
    "\n--- CANCELADO ---",
    "[OCUPADO] Operacion en curso. Envie STOP.",

    // Info
    "[INFO] Ya desbloqueado. Capturando al flash.",
    "[INFO] Sin contrasena. Sistema abierto.",
    "[INFO] Teclado ABNT2 detectado \xE2\x80\x94 layout cambiado.",
    "[AVISO] La captura se pausara hasta LOGIN.",

    // Lockout
    "[BLOQUEO] Demasiados fallos. Espere %lu segundos.",

    // LIVE
    "--- MODO EN VIVO ACTIVADO ---",
    "--- MODO EN VIVO DESACTIVADO ---",
    "[AVISO] Bloqueado \xE2\x80\x94 LIVE se activara tras LOGIN.",

    // HELP
    "--- COMANDOS (Modo Bloqueado) ---",
    "--- COMANDOS (Modo Abierto) ---",

    // LANG
    "Idioma cambiado a %s.",
    "[ERROR] Idioma desconocido. Escriba LANG.",

    // Dump/Export headers
    "--- INICIO DEL VOLCADO DE TEXTO ---",
    "--- INICIO DEL VOLCADO HEX ---",
    "--- INICIO DEL RAWDUMP ---",
    "[ERROR] Fallo al establecer el PIN de lectura.",

    // Help descriptions, usage, layout/lang UI, format messages
    "Desbloquear para captura",
    "Volcado de texto",
    "Volcado hexadecimal",
    "Exportar CSV (ultimos N opcional)",
    "Paginas cifradas sin procesar",
    "Cambiar contrasena",
    "Eliminar contrasena",
    "Establecer contrasena",
    "Establecer PIN de 4 digitos",
    "Eliminar PIN de lectura",
    "Mostrar/cambiar distribucion del teclado",
    "Mostrar/cambiar idioma",
    "Activar/desactivar transmision en vivo",
    "Establecer marca de tiempo Unix",
    "Borrar todos los datos + reiniciar",
    "Cancelar operacion activa",
    "Estadisticas del sistema",
    "Este mensaje de ayuda",
    "<contrasena>",
    "<pin>",
    "<antigua> <nueva>",
    "--- DISTRIBUCIONES DE TECLADO ---",
    "--- Use: LAYOUT <nombre> para cambiar ---",
    "--- LAYOUT CAMBIADO A %s ---",
    "[ERROR] Layout desconocido: '%s'",
    "--- IDIOMAS ---",
    "[ERROR] PIN incorrecto. (%u/%u)",
    "[ERROR] Contrasena incorrecta. (%u/%u)",
    "[INFO] Saltando %lu eventos...",
    "[INFO] Epoch actual: %lu",
    "  %lu / %lu sectores...",
    "--- HORA ESTABLECIDA: epoch=%lu ---",
    "[USO] RPIN SET <pin> | RPIN CLEAR <pin>",
    "[INFO] PIN de lectura: %s",
    "[USO] TIME <segundos_epoch_unix>",
    "[SEGURIDAD] Borra TODOS los datos. Escriba 'DEL CONFIRM'."
};
