# FHSS uygulama protokolü

## Temel davranış

Master 15 kanallık havuzu 5 saniyelik slotlarla gezer. Yer istasyonu SEARCH modunda
80 ms/kanal tarar. Geçerli telemetri geldiğinde master'ın uptime değeri ile kendi
millis saati arasında ofset hesaplar ve FOLLOW durumuna geçer. 500 ms sessizlikte
COAST ile son saat ofsetini kullanarak takibi sürdürür; 15 saniyede yeniden arar.

| Parametre | Değer |
|---|---|
| Kanallar | 5, 10, 15, …, 75 |
| Frekans | 2400 + kanal MHz |
| Kanal başına süre | 5000 ms |
| Veri fırsatı | 50 ms aralık, yaklaşık 20 Hz |
| Slot sınırı koruması | Her iki tarafta 40 ms |
| Uygulama cevabı bekleme | 25 ms |
| Radyonun gönderim sonuç süresi | En fazla 12 ms |
| Şifreleme | AES-256 ECB, iki 16 bayt blok |
| RF ayarları | 1 Mbps, CRC-16, otomatik donanım ACK |

## İki yönlü veri

Master DATA gönderir, RX'e döner. Yer istasyonu paketi çözüp 1.5 ms bekler,
aynı kanalda kendi 32 bayt uygulama cevabını gönderir ve RX'e döner.
Cevapta alınan DATA'nın sıra numarası echo alanına konur. Master bu echo ve
oturum kimliğini doğrulayarak cevabı önceki veya farklı denemelerden ayırır.

Bu tasarım ACK payload kullanmaz; ters yön verisi ayrı bir uygulama paketidir.
Boş radyo ACK'si ve GS_OK uygulama cevabı ayrı sayaçlarda gösterilir.
Bağlantı kesildiğinde master saatini veya atlamalarını durdurmaz.

## Paket biçimi

magic(2), kind(1), channelIndex(1), session(4), sequence(4), uptime(4),
echo(4), reading(4), text(8): toplam 32 bayt. Sabit genişlikli türler ve
static_assert, Uno/Mega üzerinde aynı boyutu doğrular.

## İstatistikler

Master girişim, donanım ACK ve uygulama cevabı sayılarını ayrı gösterir.
Slave telemetri sıra numaralarındaki boşlukları kayıp sayar. Gönderim hızı
değişirse kayıp hesabında sabit paket/s katsayısı değiştirmek gerekmez.
İlk yakalama öncesi kayıp hesaplanmaz; yeni oturumda önceki sıra karşılaştırması sıfırlanır.

## Daha sonra fiziksel olarak doğrulanacaklar

İki farklı zamanda açılma, tek kartın yeniden başlatılması, kısa/uzun RF kesintisi,
slot sınırında trafik ve menzil değişimi denenmelidir. Saat ofseti RF/AES gecikmesini
tam ölçmez; koruma aralığı tasarım payıdır, sahada doğrulanmış saat doğruluğu değildir.
Demo oturum kimliği kriptografik nonce değildir. ECB paketleri için bütünlük,
kimlik doğrulama ve yeniden oynatma güvenliği iddia edilmez.
