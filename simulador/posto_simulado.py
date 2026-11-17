"""
Posto simulado em Python (PLANO B do protótipo e ferramenta de teste do painel).

Faz o mesmo papel do ESP32: mantém a fila, aplica as mesmas regras
(nunca negativa, limite de 99, registro de eventos), calcula as duas
estimativas com os MESMOS coeficientes (ia/modelo.json) e publica no mesmo
tópico MQTT. O painel não sabe se do outro lado está a placa ou este script.

Uso:
  python3 simulador/posto_simulado.py            # comandos pelo teclado: c, a, q
  python3 simulador/posto_simulado.py --auto 60  # gera chegadas/atendimentos sozinho por 60 s

Precisa de:  pip install paho-mqtt==2.1.0
"""
import argparse
import json
import os
import random
import sys
import threading
import time

import paho.mqtt.client as mqtt

RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
MODELO = json.load(open(os.path.join(RAIZ, "ia", "modelo.json"), encoding="utf-8"))

FILA_MAXIMA = 99
CAPACIDADE_LOG = 256
JANELA_MEDIA = 5
PESO_HISTORICO = 20
FATOR_TEMPO = 60


class Fila:
    """Espelho em Python de firmware/fila.h (mesmas regras e fórmulas)."""

    def __init__(self):
        self.aguardando = self.chegadas = self.concluidos = self.recusas = 0
        self.log, self.seq = [], 0
        self.inicio_atual = 0.0
        self.ult_dur, self.soma_dur, self.n_dur = [], 0.0, 0

    def _registrar(self, tipo):
        self.seq += 1
        ev = {"seq": self.seq, "tipo": tipo, "fila": self.aguardando, "instante": int(time.time())}
        self.log.append(ev)
        del self.log[:-CAPACIDADE_LOG]
        return ev

    def chegada(self, agora):
        if self.aguardando >= FILA_MAXIMA:
            self.recusas += 1
            return False, self._registrar("erro_fila_cheia")
        if self.aguardando == 0:
            self.inicio_atual = agora
        self.aguardando += 1
        self.chegadas += 1
        return True, self._registrar("chegada")

    def atendido(self, agora):
        if self.aguardando == 0:
            self.recusas += 1
            return False, self._registrar("erro_fila_vazia")
        dur = (agora - self.inicio_atual) / 60 * FATOR_TEMPO
        self.ult_dur = (self.ult_dur + [dur])[-JANELA_MEDIA:]
        self.soma_dur += dur
        self.n_dur += 1
        self.aguardando -= 1
        self.concluidos += 1
        if self.aguardando > 0:
            self.inicio_atual = agora
        return True, self._registrar("atendido")

    def media_historica(self):
        return (MODELO["media_historica_min"] * PESO_HISTORICO + self.soma_dur) / (PESO_HISTORICO + self.n_dur)

    def media_ult5(self):
        return sum(self.ult_dur) / len(self.ult_dur) if self.ult_dur else self.media_historica()

    def espera_convencional(self):
        return self.aguardando * self.media_historica()

    def espera_ia(self):
        if self.aguardando == 0:
            return 0.0
        n = self.aguardando
        return max(0.0, MODELO["b0"] + MODELO["b1"] * n + MODELO["b2"] * n * self.media_ult5())

    def estado(self, posto, inicio):
        u = self.log[-1] if self.log else {"seq": 0, "tipo": "nenhum", "instante": 0}
        return {
            "posto": posto, "fila": self.aguardando, "chegadas": self.chegadas,
            "concluidos": self.concluidos, "recusas": self.recusas,
            "espera_conv_min": round(self.espera_convencional(), 1),
            "espera_ia_min": round(self.espera_ia(), 1),
            "media_hist_min": round(self.media_historica(), 2),
            "media_ult5_min": round(self.media_ult5(), 2),
            "atend_medidos": self.n_dur, "fator_tempo": FATOR_TEMPO,
            "ultimo": {"seq": u["seq"], "tipo": u["tipo"], "instante": u["instante"]},
            "uptime_s": int(time.time() - inicio), "ntp": True, "simulador": True,
        }


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--posto", default="mapiii-posto-pcd-570589")
    ap.add_argument("--broker", default="broker.hivemq.com")
    ap.add_argument("--auto", type=int, default=0, help="segundos de simulação automática")
    args = ap.parse_args()

    base = f"fiap-gs2026/mapiii-fila/{args.posto}/"
    fila, inicio, trava = Fila(), time.time(), threading.Lock()
    cli = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"simulador-{random.randrange(16**6):06x}")

    def publicar(ev, origem):
        cli.publish(base + "evento", json.dumps({**ev, "origem": origem}))
        cli.publish(base + "estado", json.dumps(fila.estado(args.posto, inicio)), retain=True)
        print(f"[{ev['tipo']:<16}] fila={fila.aguardando:2d}  IA={fila.espera_ia():5.1f} min  "
              f"média={fila.espera_convencional():5.1f} min  ({origem})", flush=True)

    def operar(cmd, origem):
        with trava:
            if cmd == "chegada":
                _, ev = fila.chegada(time.time())
            elif cmd == "atendido":
                _, ev = fila.atendido(time.time())
            else:
                return
            publicar(ev, origem)

    def ao_conectar(c, *_):
        c.subscribe(base + "comando")
        print(f"conectado a {args.broker}; tópicos {base}#", flush=True)

    cli.on_connect = ao_conectar
    cli.on_message = lambda c, u, msg: operar(msg.payload.decode(errors="ignore"), "painel")
    cli.connect(args.broker, 1883, keepalive=30)
    cli.loop_start()
    time.sleep(1.5)

    def batimento():
        while True:
            time.sleep(5)
            with trava:
                cli.publish(base + "estado", json.dumps(fila.estado(args.posto, inicio)), retain=True)

    threading.Thread(target=batimento, daemon=True).start()

    try:
        if args.auto:
            rng = random.Random(7)
            fim = time.time() + args.auto
            operar("atendido", "auto")            # teste mínimo 3: saída com fila vazia
            while time.time() < fim:
                time.sleep(rng.uniform(1.0, 3.0))
                # mais chegadas no começo, mais atendimentos no fim
                p = 0.65 if time.time() < fim - args.auto / 3 else 0.3
                operar("chegada" if rng.random() < p else "atendido", "auto")
        else:
            print("c = chegada, a = atendido, q = sair")
            for linha in sys.stdin:
                cmd = {"c": "chegada", "a": "atendido"}.get(linha.strip().lower())
                if linha.strip().lower() == "q":
                    break
                if cmd:
                    operar(cmd, "teclado")
    finally:
        cli.loop_stop()
        cli.disconnect()


if __name__ == "__main__":
    main()
