/**
 * @file  I18nPtBr.h
 * @brief Portugues (Brasil) — i18n string table.
 */

#pragma once
#include "I18n.h"

static const I18nStrings __in_flash("i18n") i18n_ptbr = {
    // Boot
    "ABERTO (capturando)",
    "TRANCADO \xE2\x80\x94 LOGIN <senha>",
    "DESBLOQUEADO AUTOMATICAMENTE",
    "Digite HELP para ver os comandos.",

    // Erros
    "[ERRO] Trancado. Faca LOGIN primeiro.",
    "[ERRO] Senha incorreta.",
    "[ERRO] PIN incorreto.",
    "[ERRO] Sistema nao desbloqueado.",
    "[ERRO] Comando longo demais, descartado.",
    "Comando desconhecido. Digite HELP.",
    "[ERRO] Senha longa demais (max %u caracteres).",
    "[ERRO] PIN deve ter exatamente 4 digitos.",
    "[ERRO] PIN deve conter apenas digitos 0-9.",
    "[INFO] Ja esta trancado. Use PASS ou UNLOCK.",
    "[INFO] Ja esta aberto.",
    "[ERRO] Falha. Sistema pode ja estar trancado.",
    "[ERRO] Senha antiga incorreta ou nao desbloqueado.",
    "[ERRO] PIN incorreto ou nenhum PIN definido.",

    // Operacoes
    "--- TRANCANDO SISTEMA COM SENHA ---",
    "--- SISTEMA TRANCADO ---",
    "[INFO] DEK apagada da RAM. Captura PAUSADA.",
    "[INFO] Proximo boot exigira LOGIN <senha>.",
    "[INFO] Use LOGIN <senha> para retomar a captura.",
    "Desbloqueando... (KDF, ~200 ms)",
    "DESBLOQUEADO \xE2\x80\x94 Captura ativa",
    "--- REMOVENDO SENHA DO USUARIO ---",
    "--- SISTEMA DESBLOQUEADO (sem senha) ---",
    "--- ALTERANDO SENHA ---",
    "--- SENHA ALTERADA COM SUCESSO ---",
    "--- APAGANDO LOG DO FLASH ---",
    "--- APAGAMENTO COMPLETO ---",
    "[INFO] Senha removida. Modo aberto. Captura ativa.",
    "--- PIN DE LEITURA DEFINIDO ---",
    "--- PIN DE LEITURA REMOVIDO ---",
    "[INFO] Calculando hash do PIN (KDF, ~10 ms)...",
    "[INFO] DUMP, HEX, EXPORT, RAWDUMP agora exigem PIN.",
    "\n--- CANCELADO ---",
    "[OCUPADO] Operacao em andamento. Envie STOP.",

    // Info
    "[INFO] Ja desbloqueado. Capturando para o flash.",
    "[INFO] Nenhuma senha definida. Sistema aberto.",
    "[INFO] Teclado ABNT2 detectado \xE2\x80\x94 layout alterado.",
    "[AVISO] Captura pausara ate LOGIN.",

    // Lockout
    "[BLOQUEIO] Muitas falhas. Aguarde %lu segundos.",

    // LIVE
    "--- MODO AO VIVO LIGADO ---",
    "--- MODO AO VIVO DESLIGADO ---",
    "[AVISO] Sistema trancado \xE2\x80\x94 LIVE ativara apos LOGIN.",

    // HELP
    "--- COMANDOS (Modo Trancado) ---",
    "--- COMANDOS (Modo Aberto) ---",

    // LANG
    "Idioma alterado para %s.",
    "[ERRO] Idioma desconhecido. Digite LANG para listar.",

    // Dump/Export headers
    "--- INICIO DO DUMP DE TEXTO ---",
    "--- INICIO DO DUMP HEX ---",
    "--- INICIO DO RAWDUMP ---",
    "[ERRO] Falha ao definir PIN de leitura.",

    // Help descriptions, usage, layout/lang UI, format messages
    "Desbloquear para captura",
    "Dump de texto",
    "Dump hexadecimal",
    "Exportar CSV (ultimos N opcional)",
    "Paginas criptografadas brutas",
    "Alterar senha",
    "Remover senha",
    "Definir senha",
    "Definir PIN de 4 digitos",
    "Remover PIN de leitura",
    "Mostrar/alterar layout do teclado",
    "Mostrar/alterar idioma",
    "Alternar streaming em tempo real",
    "Definir timestamp Unix",
    "Apagar todos os dados + resetar",
    "Cancelar operacao ativa",
    "Estatisticas do sistema",
    "Esta mensagem de ajuda",
    "<senha>",
    "<pin>",
    "<antiga> <nova>",
    "--- LAYOUTS DE TECLADO ---",
    "--- Use: LAYOUT <nome> para alterar ---",
    "--- LAYOUT ALTERADO PARA %s ---",
    "[ERRO] Layout desconhecido: '%s'",
    "--- IDIOMAS ---",
    "[ERRO] PIN incorreto. (%u/%u)",
    "[ERRO] Senha incorreta. (%u/%u)",
    "[INFO] Pulando %lu eventos...",
    "[INFO] Epoch atual: %lu",
    "  %lu / %lu setores...",
    "--- HORA DEFINIDA: epoch=%lu ---",
    "[USO] RPIN SET <pin> | RPIN CLEAR <pin>",
    "[INFO] PIN de leitura: %s",
    "[USO] TIME <segundos_epoch_unix>",
    "[SEGURANCA] Apaga TODOS os dados. Digite 'DEL CONFIRM'."
};
