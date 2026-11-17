"""Confere se o simulador Python (plano B) calcula igual ao firmware (fila.h).
Uso: python3 testes/coerencia.py   (precisa do g++ e do paho-mqtt instalado)"""
import os, subprocess, sys, tempfile
RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
sys.path.insert(0, os.path.join(RAIZ, "simulador"))
from posto_simulado import Fila  # noqa: E402

binario = os.path.join(tempfile.mkdtemp(), "sequencia")
subprocess.run(["g++", "-std=c++17", "-I", os.path.join(RAIZ, "firmware"),
                os.path.join(RAIZ, "testes", "sequencia.cpp"), "-o", binario], check=True)
saida_c = subprocess.run([binario], capture_output=True, text=True, check=True).stdout.split("\n")

f, falhas = Fila(), 0
for linha in filter(None, saida_c):
    ms, op, fila_c, conv_c, ia_c = linha.split()
    agora = int(ms) / 1000
    f.chegada(agora) if op == "c" else f.atendido(agora)
    ok = (f.aguardando == int(fila_c) and abs(f.espera_convencional() - float(conv_c)) < 0.01
          and abs(f.espera_ia() - float(ia_c)) < 0.01)
    falhas += not ok
    print(f"{'ok  ' if ok else 'ERRO'} t={ms:>5} ms {op}  fila C++={fila_c} Py={f.aguardando}  "
          f"conv {float(conv_c):6.2f}/{f.espera_convencional():6.2f}  IA {float(ia_c):6.2f}/{f.espera_ia():6.2f}")
print("firmware e simulador COERENTES" if not falhas else f"{falhas} diferença(s)!")
sys.exit(1 if falhas else 0)
