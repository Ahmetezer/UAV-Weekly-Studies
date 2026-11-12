# ESP8266 üzerinde TinyML ile drop sınıflandırması

Amaç, kablosuz bağlantıdan elde edilen ölçümlerle normal ve kontrollü paket düşürme senaryolarını ayırmaktır. İki ESP8266 kullanılır. Client paket gönderimini, server ölçüm ve model çıkarımını yürütür. Eğitim bilgisayarda yapılır.

## Dosyalar

| Dosya | Görevi |
|---|---|
| client/client.ino | Erişim noktası, UDP gönderimi ve normal/drop denemeleri |
| server/server.ino | Özellik çıkarma ve sınıflandırma |
| server/model.h | train.py tarafından üretilen C karar ağacı |
| logger.py | İki seri porttan ölçüm ve etiketleri birleştirir |
| train.py | Veriyi ayırır, karar ağacını eğitir ve C başlığını üretir |
| test.py | Karta yüklenen modeli 200 yeni denemede değerlendirir |
| lib | Seri veri işleme ve karar ağacı modülleri |

## Ölçüm ve eğitim

1000 pencerelik toplama akışında 500 normal ve 500 kontrollü drop denemesi vardır. Her pencere 250 ms sürer; 10 UDP paket fırsatından ilk ve sonuncusu referans olarak kullanılır. Drop denemesinde aradaki sekiz paketten biri gönderilmez.

Model girdileri: kayıp iç paket sayısı, kayıp referans sayısı, referanslar arasındaki süre, ortalama RSSI ve RSSI açıklığı. Etiket ve deneme numarası model girdisi değildir.

Verinin 800 kaydı eğitim, 200 kaydı ayrılmış değerlendirme için kullanılır. CART ağacının en fazla dört seviyesi ve yaprak başına en az sekiz örneği vardır. Ağaç, ESP8266 üzerinde C karşılaştırmalarıyla çalışır.

## Çalıştırma

İki kartı USB ile bağlayın. client ve server sketchlerini Arduino IDE ile ilgili kartlara yükleyin. NodeMCU 1.0 kartını seçin. Seri monitörleri kapatıp depo kökünden gerçek portlarla çalıştırın:

    python week5/tinyml_attack_detection/logger.py --client COM3 --server COM5
    python week5/tinyml_attack_detection/train.py week5/tinyml_attack_detection/data/train_TARIH.csv

COM3/COM5 ve train_TARIH.csv örnektir; oluşan dosya ve bağlı portlar seçilir. Eğitimden sonra server/model.h değişir. Server sketchini bu başlıkla yeniden yükleyin:

    python week5/tinyml_attack_detection/test.py --client COM3 --server COM5

Wi-Fi ayarları iki kartın LabWire.h dosyasında aynı olmalıdır: AE-UAV-LAB, kanal 6, erişim noktası 192.168.4.1, ölçüm kartı 192.168.4.2.

## Değerlendirme

Yeni testte doğruluk, 100 × (TP + TN) / 200 olarak hesaplanır. CSV ve JSON çıktıları kaydedilir; yanlış pozitif ve yanlış negatif sayıları ayrıca verilir. Ayrılmış veri başarısı ile kart üzerindeki yeni deney başarısı farklı ölçümlerdir.

Depodaki model.h eğitim öncesi başlıktır; MODEL_READY=false iken tahmin -1/NA olur. Ölçüm dosyaları, model ve deney raporu yukarıdaki akışla üretilir. Paket kaybı doğal kanal etkilerinden de kaynaklanabileceğinden çıktı, tek başına saldırgan varlığını kanıtlamaz.
