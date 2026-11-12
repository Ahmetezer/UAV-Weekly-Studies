// Hafta 3: yalnız ölçüm; herhangi bir ML tahmini yapılmaz.

// Eğitim öncesi model durumu: çıkarım sonucu henüz kullanılamaz.
// train.py, ölçüm CSV dosyasından öğrendiği karar ağacını bu başlığa yazar.
#include <stdint.h>
constexpr bool MODEL_READY=false;
constexpr char MODEL_TAG[]="collector";
inline int predict(const int16_t*) { return -1; }

#include "LabServer.h"
