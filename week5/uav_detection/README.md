# YOLO ile İHA tespiti

Video veya kamera görüntüsünde quadcopter ve fixed-wing sınıflarını tespit eden uygulama. models/drone_model.pt, YOLOv8n tabanlı iki sınıflı modeldir. videos klasöründe örnek video bulunur.

## Çalıştırma

Depo kökünden:

    python week5/uav_detection/main.py
    python week5/uav_detection/main.py --source 0
    python week5/uav_detection/main.py --save week5/uav_detection/runs/sonuc.mp4

İlk komut örnek videoyu, ikinci komut kamerayı kullanır. Yan panel tespit sayısını, güven eşiğini, işlem süresini ve FPS değerini gösterir. Q/ESC çıkış, boşluk duraklatma içindir. Varsayılan cihaz CPU'dur; CUDA ortamında --device 0 seçilebilir.

Farklı model seçmek için --model kullanılır. İHA sınıfı içermeyen modellerde uygulama hata verir; --all-classes seçeneği genel nesne tespitini modelin kendi sınıf adlarıyla açar. models/baseline içindeki genel modeller bu amaçla bulunur.

## Model eğitimi

Etiketli bir YOLO veri setiyle:

    python week5/uav_detection/train_model.py --data C:/veri/data.yaml --base week5/uav_detection/models/baseline/yolo11n.pt --epochs 100

Eğitim çıktısı runs/iha_training altına yazılır. Modelin güven skoru, tespit doğruluğu yüzdesi değildir; doğruluk değerlendirmesi etiketli ve eğitimden ayrı test verisi gerektirir. Model ve video kullanımlarında ilgili kaynak lisansları geçerlidir.
