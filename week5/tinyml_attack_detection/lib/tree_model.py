"""Beş tamsayı ölçümünden küçük bir CART karar ağacı öğrenir.

Eğitim bilgisayarda yapılır. Üretilen model.h yalnızca karşılaştırma ve dal
seçimi içerir; mikrodenetleyiciye Python veya bir ML çalışma zamanı yüklenmez.
Etiket, deney kimliği ve deneme sırası hiçbir zaman modele girdi yapılmaz.
"""
from dataclasses import dataclass
import hashlib
import json
import random

FEATURES = ("missing", "refs_missing", "ref_span_ms", "rssi_mean", "rssi_range")


def impurity(labels):
    """İki sınıflı Gini safsızlığı: saf düğüm 0, dengeli düğüm 0.5."""
    if not labels:
        return 0.0
    p = sum(labels) / len(labels)
    return 2 * p * (1 - p)


def train(rows, max_depth=4, min_leaf=8):
    """Tüm olası tamsayı eşiklerini karşılaştırarak en iyi bölünmeyi bulur."""
    if not rows or any(r["label"] not in (0, 1) for r in rows):
        raise ValueError("Eğitim için 0/1 etiketli ölçümler gerekli.")
    def grow(samples, depth):
        labels = [r["label"] for r in samples]
        prediction = int(sum(labels) * 2 >= len(labels))
        node = {"prediction": prediction, "samples": len(samples)}
        if depth >= max_depth or len(set(labels)) == 1 or len(samples) < 2 * min_leaf:
            return node
        best = None
        base = impurity(labels)
        for feature in FEATURES:
            ordered = sorted(samples, key=lambda r: r[feature])
            total_one = sum(labels)
            left_one = 0
            for i in range(1, len(ordered)):
                left_one += ordered[i - 1]["label"]
                if i < min_leaf or len(ordered) - i < min_leaf:
                    continue
                a, b = ordered[i - 1][feature], ordered[i][feature]
                if a == b:
                    continue
                lp, rp = left_one / i, (total_one - left_one) / (len(ordered) - i)
                weighted = (i * 2 * lp * (1-lp) +
                            (len(ordered)-i) * 2 * rp * (1-rp)) / len(ordered)
                gain = base - weighted
                if gain > 1e-12 and (best is None or gain > best[0]):
                    # Girdiler tamsayı: iki örnek arasındaki tam sayı eşiği
                    # C ve Python tarafında tam olarak aynı bölünmeyi üretir.
                    best = gain, feature, (a + b) // 2
        if best is not None:
            _, feature, threshold = best
            left = [r for r in samples if r[feature] <= threshold]
            right = [r for r in samples if r[feature] > threshold]
            node.update(feature=feature, threshold=threshold,
                        left=grow(left, depth+1), right=grow(right, depth+1))
        return node
    return {"schema": 1, "features": list(FEATURES), "tree": grow(rows, 0)}


def predict(model, row):
    node = model["tree"]
    while "feature" in node:
        node = node["left"] if row[node["feature"]] <= node["threshold"] else node["right"]
    return node["prediction"]


def split_holdout(rows, fraction=0.2, seed=42):
    """Sınıf oranını koruyan, sabit tohumlu eğitim/ayrılmış test bölmesi."""
    rng = random.Random(seed)
    training, holdout = [], []
    for label in (0, 1):
        group = [r for r in rows if r["label"] == label]
        if len(group) < 10:
            raise ValueError("Her sınıfta en az 10 gerçek ölçüm gerekli.")
        rng.shuffle(group)
        cut = max(1, round(len(group) * fraction))
        holdout.extend(group[:cut])
        training.extend(group[cut:])
    return training, holdout


def evaluate(model, rows):
    counts = {"tn": 0, "fp": 0, "fn": 0, "tp": 0}
    for row in rows:
        truth, answer = row["label"], predict(model, row)
        counts[{(0,0): "tn", (0,1): "fp", (1,0): "fn", (1,1): "tp"}[truth, answer]] += 1
    total = len(rows)
    return {**counts, "samples": total,
            "accuracy_percent": 100 * (counts["tn"] + counts["tp"]) / total if total else None}


def flatten(model):
    """Ağacı ESP8266 üzerinde sabit bir C dizisi olarak saklamak için düzleştirir."""
    nodes = []
    def visit(node):
        index = len(nodes)
        nodes.append(None)
        if "feature" not in node:
            nodes[index] = (-1, 0, -1, -1, node["prediction"])
        else:
            left, right = visit(node["left"]), visit(node["right"])
            nodes[index] = (FEATURES.index(node["feature"]), node["threshold"], left, right,
                            node["prediction"])
        return index
    visit(model["tree"])
    return nodes


def export_header(model):
    """Model kimliği modelin içeriğinden türetilir; örnek sayısı kimliğe eklenmez."""
    tag = hashlib.sha256(json.dumps(model, sort_keys=True).encode()).hexdigest()[:12]
    values = ",\n".join("  {%d, %d, %d, %d, %d}" % node for node in flatten(model))
    return f"""// train.py tarafından GERÇEK ölçümlerden üretilir. Elle düzenlemeyin.
#pragma once
#include <stdint.h>
constexpr bool MODEL_READY = true;
constexpr char MODEL_TAG[] = "{tag}";
struct ModelNode {{ int8_t feature; int16_t threshold; int16_t left, right; int8_t prediction; }};
const ModelNode MODEL_NODES[] = {{
{values}
}};
inline int predict(const int16_t* features) {{
  int16_t index = 0;
  while (MODEL_NODES[index].feature >= 0) {{
    const ModelNode& node = MODEL_NODES[index];
    index = features[node.feature] <= node.threshold ? node.left : node.right;
  }}
  return MODEL_NODES[index].prediction;
}}
"""
