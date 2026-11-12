# Hafta 4 — Frekans atlamalı haberleşme

Amaç, nRF24L01 modülleri arasında AES-256 ile şifrelenmiş veriyi değişen RF kanallarında çift yönlü iletmektir.

sender/sender.ino verici Uno kartına, receiver/receiver.ino alıcı Mega kartına yüklenir. Seri monitörler 115200 baud ile açılır. Her iki taraftaki RadioConfig.h ve LinkProtocol.h ayarları eşleşmelidir.

| Parametre | Değer |
|---|---|
| Kanal listesi | 5, 10, 15, …, 75 |
| Kanalda kalma süresi | 5000 ms |
| Veri gönderim aralığı | 50 ms |
| Veri hızı | 1 Mbps |
| Şifreleme | AES-256 ECB |
| Paket uzunluğu | 32 bayt |

Alıcı önce kanalları tarar. Geçerli paket alındığında vericinin zamanına göre kanal atlamasını takip eder. Kısa kesintide son saat farkıyla takibe devam eder; uzun kesintide yeniden kanal arar. Ters yöndeki uygulama cevabı, donanım ACK bilgisinden ayrı sayılır. Ayrıntılar [PROTOKOL.md](PROTOKOL.md) dosyasındadır.

## Bağlantılar

| nRF24L01 | Arduino Uno | Arduino Mega 2560 |
|---|---|---|
| VCC | 3.3 V | 3.3 V |
| GND | GND | GND |
| CE | D7 | D9 |
| CSN | D8 | D53 |
| SCK | D13 | D52 |
| MOSI | D11 | D51 |
| MISO | D12 | D50 |

CE ve CSN ayarları RadioConfig.h içindedir. Modüle yakın 10–100 µF kondansatör kullanılabilir; nRF24 beslemesine doğrudan 5 V uygulanmaz.
