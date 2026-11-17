// Roda uma sequencia fixa de eventos na logica do ESP32 (firmware/fila.h) e
// imprime as estimativas. testes/coerencia.py roda a mesma sequencia no
// simulador Python e confere se os numeros batem.
#include <cstdio>
#include "fila.h"
static Fila f;
int main() {
    // (tempo em ms, operacao): c = chegada, a = atendido
    const struct { unsigned ms; char op; } seq[] = {
        {0,'a'}, {1000,'c'}, {2000,'c'}, {3500,'c'}, {9000,'a'}, {9500,'c'}, {16000,'a'},
        {17000,'c'}, {21000,'a'}, {30000,'a'}, {31000,'a'}, {32000,'a'}, {33000,'c'}, {40000,'c'}};
    fila_iniciar(&f);
    for (auto &s : seq) {
        if (s.op == 'c') fila_chegada(&f, s.ms, 0); else fila_atendido(&f, s.ms, 0);
        printf("%u %c %u %.4f %.4f\n", s.ms, s.op, f.aguardando,
               (double)fila_espera_convencional(&f), (double)fila_espera_ia(&f));
    }
}
