# İnsansız Hava Araçları Teknolojisi

Ahmet Ezer

Bu depo, ders kapsamındaki kablosuz haberleşme, şifreleme, frekans atlama ve İHA tespiti çalışmalarını içerir.

| Hafta | Çalışma | Kullanılan donanım / araç |
|---|---|---|
| [2. hafta](week2) | nRF24 haberleşmesi; AES-128 ve AES-256 | Arduino Uno, Mega 2560, nRF24L01 |
| [3. hafta](week3) | Wi-Fi kanal gözlemi, RSSI ve paket kaybı kaydı | İki ESP8266, Python |
| [4. hafta](week4) | AES-256 ile frekans atlamalı çift yönlü haberleşme | Arduino Uno, Mega 2560, nRF24L01 |
| [5. hafta](week5) | TinyML ile paket düşürme sınıflandırması; YOLO ile İHA tespiti | ESP8266, Python, Ultralytics |

Her haftanın bağlantıları, parametreleri ve çalıştırma adımları kendi README dosyasında bulunur. Python yardımcı modülleri ilgili çalışmanın lib klasöründedir.

## Kurulum

Python 3.11 veya 3.12 ile sanal ortam oluşturup bağımlılıkları kurun:

    py -3.12 -m venv .venv
    .venv\Scripts\python.exe -m pip install -r requirements.txt

Komutlardaki python yerine sanal ortamdaki .venv\Scripts\python.exe kullanılabilir. Yalnız seri veri toplamak için pyserial yeterlidir.

Arduino IDE kütüphaneleri: RF24 ve Crypto. Kart paketleri: Arduino AVR Boards ve ESP8266 Community. ESP8266 kart seçimi NodeMCU 1.0 (ESP-12E Module), seri port hızı 115200 baud.

ESP8266 kart yöneticisi adresi: https://arduino.esp8266.com/stable/package_esp8266com_index.json

## Teknik dokümanlar

- [RF24](https://nrf24.github.io/RF24/)
- [Arduino Crypto](https://rweather.github.io/arduinolibs/)
- [ESP8266 Arduino Core](https://arduino-esp8266.readthedocs.io/)
- [Ultralytics YOLO](https://docs.ultralytics.com/)
