# Hafta 2 — nRF24 ve AES deneyi

Önce plain, ardından ana sender/receiver AES-128 ve isteğe bağlı aes256 çiftini kullanın.
Verici ve alıcıda aynı mod, kanal (46 → 2446 MHz), adres (AEUAV), 1 Mbps,
CRC-16 ve 32 bayt sabit payload seçilmiştir. Seri çıktı 115200 baud'dur.
Bağlantılar aşağıdaki tabloda verilmiştir.

## Paket

| Alan | Bayt | Amaç |
|---|---:|---|
| magic | 2 | Beklenen uygulama paketini ayırır |
| revision + mode | 1 + 1 | Sürüm ve şifreleme modunu belirtir |
| sequence | 4 | Artan paket numarası |
| uptime | 4 | Vericinin açılıştan beri geçen ms zamanı |
| value | 4 | Örnek tamsayı telemetri |
| text | 16 | Sıfır sonlandırıcılı kısa mesaj |
| Toplam | 32 | static_assert ile derlemede doğrulanır |

AES'in blok boyutu hem AES-128 hem AES-256 için 16 bayttır. Buradaki 32 bayt paket
iki blok halinde ECB şifrelenir. 128/256 sayısı blok boyutunu değil anahtarı belirtir.
Verici yalnız saniyede bir paket üretir. Alıcı ham HEX'i, çözülmüş mesajı, sıra
boşluğundan bulunan kaybı ve RPD göstergesini yazar.

RPD güçlü/zayıf eşiğidir; sürekli sayısal RSSI ölçümü değildir. Donanım ACK'si
RF paketinin karşı modüle ulaştığını bildirir; AES metninin uygulama tarafından
doğru yorumlandığını tek başına doğrulamaz.

Kayıp hesabı birim zaman başına varsayılan paket sayısından değil sıra numarasından
yapılır. İlk duyulan paketten önceki kayıplar bilinmez.

## Kriptografi araştırması

AES, ortak anahtarlı blok şifredir; RSA/ECC anahtar değişimi ve imza gibi farklı
rollerde kullanılır. Hash işlevi tek yönlü özet üretir ve tek başına göndereni
doğrulamaz. MAC/HMAC anahtarla bütünlük ve kaynak doğrulama sağlar. Bu ders örneğindeki
ECB aynı açık metin bloklarında aynı şifreli çıktıyı üretir ve bir doğrulama etiketi içermez; güvenli
telemetri tasarımında doğrulamalı şifreleme ve yeniden oynatma koruması ayrıca gerekir.
[Kriptografi notları](kriptografi.md) temel yöntemleri karşılaştırır.

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
