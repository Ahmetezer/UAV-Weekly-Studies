# İki ESP8266 ile ölçüm

1. client/client.ino dosyasını erişim noktası olacak karta yükleyin.
2. server/server.ino dosyasını ölçüm kartına yükleyin.
3. Depo kökünde Python ortamını hazırlayın, seri monitörleri kapatın.
4. Gerçek portları seçerek çalıştırın:

    python week3/data_collector/logger.py --client COM3 --server COM5

Akış 1000 pencere sonunda durur; 500 normal ve 500 kontrollü drop denemesi toplar.
Çıktılar bu klasörün data altına CSV ve deney kimliği içeren JSON olarak yazılır.
Var olan ölçüm dosyası üzerine yazılmaz.

## Ölçüm nasıl yapılır?

Server önce TCP kontrol kanalıyla pencereyi açar. Client 18 ms aralıklarla 10 UDP
slotu kullanır. Slot 0 ve 9 referanstır; atakta 1–8 arasındaki bir slot gönderilmez.
Server 250 ms boyunca gelen slotların varışını ve 0/90/180 ms zamanlarında bağlı
AP'nin RSSI değerini ölçer. Geç veya farklı deneyden gelen paketler ayıklanır.

| CSV alanı | Anlamı |
|---|---|
| run_id, trial | Deney ve pencere kimliği; ML girdisi değildir |
| label, dropped_slot | Client'ın gerçek deney etiketi; ölçümle PC'de birleştirilir |
| rssi_0..2 | Bağlı AP'nin üç gerçek RSSI örneği (dBm) |
| arrival_0..9 | Pencere başlangıcından itibaren ms; gelmeyen paket için -1 |
| missing | Sekiz iç slottaki gelmeyen paket sayısı |
| refs_missing | İki referans slottaki kayıp sayısı |
| ref_span_ms | Son ve ilk referans arasındaki süre; referans eksikse -1 |
| rssi_mean, rssi_range | Üç RSSI örneğinin tamsayı ortalaması ve açıklığı |
| prediction | Bu haftada -1: model çalıştırılmaz |
| inference_us | Ölçüm fonksiyonunun çıkarım çağrısı süresi; modelsiz gerçek ML süresi sayılmaz |

AP bağlantı RSSI'si her UDP çerçevesinin ayrı RSSI'si olarak yorumlanmaz.
Bu uygulamada noise floor ve SNR ölçümü bulunmaz.

## Pasif çerçeve gözlemi

Ayrı deney olarak sniffer/sniffer.ino dosyasını tek ESP8266'ya yükleyin.
OBSERVE_CHANNEL değerini gözlenecek kanala göre seçin. Kayıt:

    python week3/data_collector/sniffer_logger.py --port COM3 --seconds 60

RSSI, yönetim/veri tipi, alt tip, retry biti, 12 bit MAC sıra numarası,
gönderen/alıcı MAC ve callback boyutu CSV'ye yazılır. Callback boyutu,
gerçek havadaki paket uzunluğu değildir. Birden çok alt paket içeren SDK
callback'leri ayrı paket sayısına genişletilmez; bu çıktı trafik toplamı sayacı değildir.
Yoğun trafikte kuyruktan atılan gözlemler discarded_callback_records ile bildirilir.
SNR veya gürültü tabanı ölçülmez. İki modun CSV şemaları farklıdır.
