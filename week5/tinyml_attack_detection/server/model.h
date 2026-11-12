// Eğitim öncesi model durumu: çıkarım sonucu henüz kullanılamaz.
// train.py, ölçüm CSV dosyasından öğrendiği karar ağacını bu başlığa yazar.
#pragma once
#include <stdint.h>
constexpr bool MODEL_READY=false;
constexpr char MODEL_TAG[]="none";
inline int predict(const int16_t*) { return -1; }
