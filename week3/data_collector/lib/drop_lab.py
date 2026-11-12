"""İki seri portun ölçüm ve gerçek etiketlerini deney/deneme kimliğiyle birleştirir.

Client etiket üretir; server etiketi bilmeden RF ölçümü yapar ve tahmin üretir.
Eğitim dosyasındaki etiket, ölçümle ancak bilgisayarda eşleştirilir.
"""
import argparse
import csv
from datetime import datetime, timezone
import json
from pathlib import Path
import secrets
import time

from lib.tree_model import FEATURES, evaluate

ROOT = Path(__file__).resolve().parents[1]
DATA_COLUMNS = ("run_id", "trial", "label", "dropped_slot", "rssi_0", "rssi_1", "rssi_2",
                *(f"arrival_{i}" for i in range(10)), *FEATURES, "prediction", "inference_us")


class LinePort:
    """Seri porttaki kısmi satırları kaybetmeden tamponlar."""
    def __init__(self, name):
        import serial
        self.port = serial.Serial(port=None, baudrate=115200, timeout=0, write_timeout=2)
        self.port.port, self.port.dtr, self.port.rts = name, False, False
        self.port.open()
        self.buffer = bytearray()
    def lines(self):
        self.buffer.extend(self.port.read(self.port.in_waiting or 1))
        output = []
        while b"\n" in self.buffer:
            line, _, self.buffer = self.buffer.partition(b"\n")
            output.append(line.decode("utf-8", errors="replace").strip())
        if len(self.buffer) > 8192:
            raise RuntimeError("Seri satır 8192 baytı aştı; baud ve firmware sürümünü kontrol edin.")
        return output
    def send(self, text):
        self.port.write(text.encode("ascii"))
        self.port.flush()
    def close(self):
        self.port.close()


def decode_server(line):
    fields = line.split(",")
    if len(fields) != 23 or fields[0] != "S":
        raise ValueError("Server satır şeması beklenen 23 alanla uyuşmuyor.")
    values = list(map(int, fields[1:]))
    run, trial = values[:2]
    rssis, arrivals, features = values[2:5], values[5:15], values[15:20]
    if not all(-127 <= r < 0 for r in rssis):
        raise ValueError("Geçersiz RSSI ölçümü.")
    if not all(-1 <= t < 250 for t in arrivals):
        raise ValueError("Geçersiz paket varış zamanı.")
    # Hesaplayıcı ile kartın aynı özellikleri ürettiğini gerçek ham ölçümden kontrol et.
    # C/C++ negatif bölmeyi sıfıra doğru keser; Python // aşağı yuvarlar.
    expected = [sum(t < 0 for t in arrivals[1:9]),
                int(arrivals[0] < 0) + int(arrivals[9] < 0),
                -1 if arrivals[0] < 0 or arrivals[9] < 0 else arrivals[9]-arrivals[0],
                int(sum(rssis) / 3), max(rssis)-min(rssis)]
    if features != expected:
        raise ValueError(f"Firmware/PC özellik sırası uyuşmuyor: {features} != {expected}")
    row = dict(zip(DATA_COLUMNS[4:7], rssis))
    row.update({f"arrival_{i}": t for i, t in enumerate(arrivals)})
    row.update(zip(FEATURES, features))
    row.update(run_id=run, trial=trial, prediction=values[20], inference_us=values[21])
    return row


def merge_rows(observations, labels, run_id, count):
    if set(observations) != set(range(count)) or set(labels) != set(range(count)):
        raise RuntimeError("Deney eksik veya tekrarlı satır içeriyor; sonuç oluşturulmadı.")
    rows = []
    for trial in range(count):
        row = dict(observations[trial])
        label, dropped = labels[trial]
        if row["run_id"] != run_id or label not in (0, 1):
            raise RuntimeError("Deney kimliği veya etiketi geçersiz.")
        if (label == 0 and dropped != -1) or (label == 1 and not 1 <= dropped <= 8):
            raise RuntimeError("Normal/atak etiketi düşürülen paketle tutarsız.")
        row.update(label=label, dropped_slot=dropped)
        rows.append(row)
    if sum(r["label"] for r in rows) != count // 2:
        raise RuntimeError("500/500 veya 100/100 sınıf dengesi sağlanamadı.")
    return rows


def run_experiment(client_port, server_port, count, output, require_model=False):
    if client_port.lower() == server_port.lower():
        raise ValueError("Client ve server için farklı seri portlar seçin.")
    if count not in (1000, 200):
        raise ValueError("Firmware 1000 toplama veya 200 yeni test denemesi kabul eder.")
    output = Path(output)
    if output.exists():
        raise FileExistsError(f"Mevcut ölçüm dosyası korunuyor: {output}")
    client = LinePort(client_port)
    server = None
    try:
        server = LinePort(server_port)
        ready, last_query, deadline = {}, 0, time.monotonic()+45
        while len(ready) < 2:
            now = time.monotonic()
            if now > deadline:
                raise TimeoutError("Kartlar hazır değil. Port, kart kodu ve Wi-Fi bağlantısını kontrol edin.")
            if now-last_query > 1:
                client.send("?\n"); server.send("?\n")
                last_query = now
            for role, port in (("C", client), ("S", server)):
                for line in port.lines():
                    if line.startswith("ERROR,"):
                        raise RuntimeError(line)
                    if line.startswith("READY,"):
                        fields = line.split(",")
                        if len(fields) < 2 or fields[1] != role:
                            raise RuntimeError("Client ve server portları ters seçilmiş.")
                        ready[role] = fields[2] if len(fields) > 2 else "collector"
            time.sleep(0.01)
        model_tag = ready["S"]
        if require_model and model_tag in ("none", "collector"):
            raise RuntimeError("Önce gerçek veriden train.py ile model üretip server'a yükleyin.")
        run_id = secrets.randbelow(2**32-1)+1
        observations, labels, done, result = {}, {}, set(), None
        client.send(f"R {run_id} {count}\n")
        deadline = time.monotonic()+count*0.25+90
        last_report = 0
        while len(done) < 2:
            if time.monotonic() > deadline:
                raise TimeoutError(f"Deney tamamlanmadı: {len(observations)} ölçüm/{len(labels)} etiket.")
            for role, port in (("C", client), ("S", server)):
                for line in port.lines():
                    if line.startswith("ERROR,"):
                        raise RuntimeError(line)
                    if line.startswith("S,"):
                        row = decode_server(line)
                        if row["run_id"] != run_id or row["trial"] in observations:
                            raise RuntimeError("Yabancı veya tekrarlı server satırı.")
                        observations[row["trial"]] = row
                    elif line.startswith("C,"):
                        fields = list(map(int, line.split(",")[1:]))
                        if len(fields) != 4:
                            raise RuntimeError("Client şeması geçersiz.")
                        run, trial, label, dropped = fields
                        if run != run_id or trial in labels:
                            raise RuntimeError("Yabancı veya tekrarlı etiket.")
                        labels[trial] = (label, dropped)
                    elif line.startswith("DONE,"):
                        f = line.split(",")
                        if len(f) != 3 or f[1] != role or int(f[2]) != run_id:
                            raise RuntimeError("Bitiş mesajı yanlış deneyden geldi.")
                        done.add(role)
                    elif line.startswith("RESULT,"):
                        result = line
            if len(observations)-last_report >= 50:
                last_report = len(observations)
                print(f"Gerçek ölçüm: {last_report}/{count}", flush=True)
            time.sleep(0.005)
        rows = merge_rows(observations, labels, run_id, count)
        output.parent.mkdir(parents=True, exist_ok=True)
        with output.open("x", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=DATA_COLUMNS)
            writer.writeheader(); writer.writerows(rows)
        report = {"created_utc": datetime.now(timezone.utc).isoformat(), "run_id": run_id,
                  "model_tag": model_tag, "samples": count, "normal": count//2, "attack": count//2,
                  "device_result": result, "hardware_data": True}
        if require_model:
            if any(r["prediction"] not in (0, 1) for r in rows):
                raise RuntimeError("Kart geçerli model tahmini üretmedi.")
            matrix = {"tn":0, "fp":0, "fn":0, "tp":0}
            for row in rows:
                matrix[{(0,0):"tn",(0,1):"fp",(1,0):"fn",(1,1):"tp"}[
                    row["label"],row["prediction"]]] += 1
            report.update(matrix, accuracy_percent=100*(matrix["tn"]+matrix["tp"])/count)
            print(f"YENİ DONANIM TESTİ: %{report['accuracy_percent']:.2f}  {matrix}")
        output.with_suffix(".json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"Kaydedildi: {output}", flush=True)
        return output
    finally:
        client.close()
        if server is not None:
            server.close()


def collect_main(test=False, directory=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", required=True, help="AP kuran gönderici kartın COM portu")
    parser.add_argument("--server", required=True, help="Ölçüm alan kartın COM portu")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    folder = Path(directory) if directory else ROOT
    output = args.out or folder/("results" if test else "data")/f"{'test' if test else 'train'}_{stamp}.csv"
    run_experiment(args.client, args.server, 200 if test else 1000, output, require_model=test)
