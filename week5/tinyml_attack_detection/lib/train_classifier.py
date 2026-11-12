"""Gerçek ölçüm CSV'sini ayırır, modeli eğitir ve ESP8266 başlığını üretir."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
from lib.tree_model import FEATURES, train, split_holdout, evaluate, export_header

ROOT = Path(__file__).resolve().parents[1]


def train_file(csv_path, output_dir):
    csv_path, output_dir = Path(csv_path), Path(output_dir)
    metadata_path = csv_path.with_suffix(".json")
    if not metadata_path.exists():
        raise ValueError("logger.py'nin gerçek deney metadata JSON dosyası gerekli.")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    if metadata.get("hardware_data") is not True or metadata.get("samples") != 1000:
        raise ValueError("1000 örneklik gerçek veri toplama deneyi gerekli.")
    with csv_path.open(newline="", encoding="utf-8-sig") as f:
        raw = list(csv.DictReader(f))
    if len(raw) != 1000:
        raise ValueError(f"1000 kayıt bekleniyordu; {len(raw)} bulundu.")
    rows = [{key: int(row[key]) for key in (*FEATURES, "label", "run_id", "trial")} for row in raw]
    if any(r["run_id"] != metadata["run_id"] for r in rows):
        raise ValueError("CSV ve metadata deney kimlikleri farklı.")
    if set(r["trial"] for r in rows) != set(range(1000)):
        raise ValueError("Deneme numaraları eksik/tekrarlı.")
    if sum(r["label"] for r in rows) != 500 or any(r["label"] not in (0,1) for r in rows):
        raise ValueError("500 normal + 500 atak gerekli.")
    training, holdout = split_holdout(rows)
    model = train(training)
    model["source_sha256"] = hashlib.sha256(csv_path.read_bytes()).hexdigest()
    model["source_run"] = metadata["run_id"]
    header = export_header(model)
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir/"model.h").write_text(header, encoding="utf-8", newline="\n")
    (output_dir/"model.json").write_text(json.dumps(model, indent=2), encoding="utf-8")
    report = {"hardware_test": False, "evaluation": "1000 gerçek kayıttan ayrılmış 200 kayıt",
              "training_samples":len(training), "holdout":evaluate(model, holdout),
              "source_sha256":model["source_sha256"], "features":list(FEATURES)}
    (output_dir/"training_report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2),
                                                 encoding="utf-8")
    print(f"Ayrılmış veri doğruluğu: %{report['holdout']['accuracy_percent']:.2f}")
    print("Bu değer yeni donanım deneyi sonucu değildir. Sonraki adım: modeli yükleyip test.py.")
    print(f"Model: {output_dir/'model.h'}")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path)
    parser.add_argument("--out", type=Path, default=ROOT/"server")
    args = parser.parse_args()
    train_file(args.csv, args.out)
