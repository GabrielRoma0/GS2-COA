// fila.h - logica da fila virtual, sem nada de Arduino.
//
// O MESMO arquivo e usado pelo ESP32 (sketch.ino) e pelos testes no PC
// (testes/teste_fila.cpp). Assim o que foi testado e exatamente o que roda
// na placa.
//
// Tempos: 'agora_ms' e o relogio interno (millis() no ESP32). 'instante' e a
// hora real em segundos (epoch, vinda do NTP) so para o registro de eventos.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include "modelo.h"

// ---- Configuracao -------------------------------------------------------
#define FILA_MAXIMA      99    // limite de pessoas (o campo do evento e 8 bits)
#define CAPACIDADE_LOG   256   // eventos guardados na RAM (buffer circular)
#define JANELA_MEDIA     5     // "media dos ultimos 5 atendimentos" do modelo
#define PESO_HISTORICO   20    // a media historica comeca valendo 20 atendimentos
// Modo demonstracao: 1 segundo de botao = FATOR_TEMPO segundos de posto.
// Com 60, um atendimento de 6 s no Wokwi conta como 6 min. Use 1 num posto real.
#ifndef FATOR_TEMPO
#define FATOR_TEMPO      60
#endif

enum TipoEvento : uint8_t {
    EV_CHEGADA = 1,
    EV_ATENDIDO = 2,
    EV_ERRO_FILA_VAZIA = 3,   // apertou "atendido" sem ninguem na fila
    EV_ERRO_FILA_CHEIA = 4    // chegada recusada: fila no limite
};

// 8 bytes por evento: 256 eventos = 2 KB de RAM.
struct Evento {
    uint32_t instante;     // hora (epoch, s) ou segundos desde o boot sem NTP
    uint16_t seq;          // numero do evento (para o painel nao repetir)
    uint8_t  tipo;         // TipoEvento
    uint8_t  fila;         // pessoas no posto depois do evento
};

struct Fila {
    // contadores
    uint8_t  aguardando;   // pessoas no posto (esperando + sendo atendida)
    uint32_t chegadas;
    uint32_t concluidos;
    uint32_t recusas;      // operacoes invalidas bloqueadas
    // registro de eventos (buffer circular: o mais antigo e sobrescrito)
    Evento   log[CAPACIDADE_LOG];
    uint16_t log_inicio;   // posicao do evento mais antigo
    uint16_t log_qtd;
    uint16_t seq;
    // medicao dos atendimentos
    uint32_t inicio_atual_ms;          // quando o atendimento atual comecou
    float    ult_dur[JANELA_MEDIA];    // ultimas duracoes (min)
    uint8_t  ult_qtd, ult_pos;
    float    soma_dur;                 // soma das duracoes medidas (min)
    uint32_t n_dur;
};

// ---- Funcoes ------------------------------------------------------------

inline void fila_iniciar(Fila *f) {
    *f = Fila();   // zera tudo
}

inline void fila_registrar(Fila *f, uint8_t tipo, uint32_t instante) {
    uint16_t pos = (uint16_t)((f->log_inicio + f->log_qtd) % CAPACIDADE_LOG);
    if (f->log_qtd == CAPACIDADE_LOG) {           // cheio: descarta o mais antigo
        pos = f->log_inicio;
        f->log_inicio = (uint16_t)((f->log_inicio + 1) % CAPACIDADE_LOG);
    } else {
        f->log_qtd++;
    }
    f->seq++;
    f->log[pos].instante = instante;
    f->log[pos].seq = f->seq;
    f->log[pos].tipo = tipo;
    f->log[pos].fila = f->aguardando;
}

// i = 0 e o evento mais antigo ainda guardado.
inline const Evento *fila_evento(const Fila *f, uint16_t i) {
    if (i >= f->log_qtd) return nullptr;
    return &f->log[(f->log_inicio + i) % CAPACIDADE_LOG];
}

inline const Evento *fila_ultimo_evento(const Fila *f) {
    return f->log_qtd ? fila_evento(f, (uint16_t)(f->log_qtd - 1)) : nullptr;
}

// Botao "chegou". Devolve true se registrou; false se a fila esta no limite.
inline bool fila_chegada(Fila *f, uint32_t agora_ms, uint32_t instante) {
    if (f->aguardando >= FILA_MAXIMA) {
        f->recusas++;
        fila_registrar(f, EV_ERRO_FILA_CHEIA, instante);
        return false;
    }
    if (f->aguardando == 0) f->inicio_atual_ms = agora_ms;   // atendimento comeca ja
    f->aguardando++;
    f->chegadas++;
    fila_registrar(f, EV_CHEGADA, instante);
    return true;
}

// Botao "atendido". Devolve true se registrou; false se a fila estava vazia
// (a contagem NUNCA fica negativa: a operacao e bloqueada e vira evento de erro).
inline bool fila_atendido(Fila *f, uint32_t agora_ms, uint32_t instante) {
    if (f->aguardando == 0) {
        f->recusas++;
        fila_registrar(f, EV_ERRO_FILA_VAZIA, instante);
        return false;
    }
    // duracao do atendimento que terminou (subtracao sem sinal: funciona
    // mesmo quando o millis() da a volta, a cada ~49 dias)
    float dur_min = (float)(uint32_t)(agora_ms - f->inicio_atual_ms) / 60000.0f * FATOR_TEMPO;
    f->ult_dur[f->ult_pos] = dur_min;
    f->ult_pos = (uint8_t)((f->ult_pos + 1) % JANELA_MEDIA);
    if (f->ult_qtd < JANELA_MEDIA) f->ult_qtd++;
    f->soma_dur += dur_min;
    f->n_dur++;

    f->aguardando--;
    f->concluidos++;
    if (f->aguardando > 0) f->inicio_atual_ms = agora_ms;   // proxima pessoa e chamada
    fila_registrar(f, EV_ATENDIDO, instante);
    return true;
}

// Media historica: a do treino (dados simulados), atualizada com cada
// atendimento medido. Comeca valendo PESO_HISTORICO atendimentos.
inline float fila_media_historica(const Fila *f) {
    return (MODELO_MEDIA_HISTORICA_MIN * PESO_HISTORICO + f->soma_dur) /
           (float)(PESO_HISTORICO + f->n_dur);
}

// Media dos ultimos JANELA_MEDIA atendimentos (ou a historica, se nenhum ainda).
inline float fila_media_ult5(const Fila *f) {
    if (f->ult_qtd == 0) return fila_media_historica(f);
    float s = 0;
    for (uint8_t i = 0; i < f->ult_qtd; i++) s += f->ult_dur[i];
    return s / f->ult_qtd;
}

// Espera (min) para quem chega agora - metodo convencional.
inline float fila_espera_convencional(const Fila *f) {
    return f->aguardando * fila_media_historica(f);
}

// Espera (min) para quem chega agora - regressao linear (ia/treinar_regressao.py).
inline float fila_espera_ia(const Fila *f) {
    if (f->aguardando == 0) return 0.0f;
    float n = (float)f->aguardando;
    float e = MODELO_B0 + MODELO_B1 * n + MODELO_B2 * n * fila_media_ult5(f);
    return e > 0.0f ? e : 0.0f;
}

inline const char *fila_nome_evento(uint8_t tipo) {
    switch (tipo) {
        case EV_CHEGADA:         return "chegada";
        case EV_ATENDIDO:        return "atendido";
        case EV_ERRO_FILA_VAZIA: return "erro_fila_vazia";
        case EV_ERRO_FILA_CHEIA: return "erro_fila_cheia";
    }
    return "desconhecido";
}

// Estado em JSON (o que vai para o painel). 'extra' sao campos a mais ja
// formatados (ex.: ",\"heap_livre\":1234") ou "". Devolve o tamanho escrito.
inline int fila_json_estado(const Fila *f, const char *posto, const char *extra,
                            char *buf, size_t tam) {
    const Evento *u = fila_ultimo_evento(f);
    return snprintf(buf, tam,
        "{\"posto\":\"%s\",\"fila\":%u,\"chegadas\":%lu,\"concluidos\":%lu,\"recusas\":%lu,"
        "\"espera_conv_min\":%.1f,\"espera_ia_min\":%.1f,\"media_hist_min\":%.2f,"
        "\"media_ult5_min\":%.2f,\"atend_medidos\":%lu,\"fator_tempo\":%d,"
        "\"ultimo\":{\"seq\":%u,\"tipo\":\"%s\",\"instante\":%lu}%s}",
        posto, (unsigned)f->aguardando, (unsigned long)f->chegadas,
        (unsigned long)f->concluidos, (unsigned long)f->recusas,
        (double)fila_espera_convencional(f), (double)fila_espera_ia(f),
        (double)fila_media_historica(f), (double)fila_media_ult5(f),
        (unsigned long)f->n_dur, (int)FATOR_TEMPO,
        u ? (unsigned)u->seq : 0u, u ? fila_nome_evento(u->tipo) : "nenhum",
        u ? (unsigned long)u->instante : 0ul, extra);
}

inline int fila_json_evento(const Evento *e, const char *origem, char *buf, size_t tam) {
    return snprintf(buf, tam,
        "{\"seq\":%u,\"tipo\":\"%s\",\"fila\":%u,\"instante\":%lu,\"origem\":\"%s\"}",
        (unsigned)e->seq, fila_nome_evento(e->tipo), (unsigned)e->fila,
        (unsigned long)e->instante, origem);
}
