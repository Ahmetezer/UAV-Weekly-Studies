# Kriptografi notları

| Yöntem | Temel özellik | Kullanım |
|---|---|---|
| AES | Şifreleme ve çözmede aynı gizli anahtar kullanılır | Veri gizliliği |
| RSA / ECC | Açık ve özel anahtar çifti kullanılır | İmza, anahtar anlaşması veya anahtar taşıma; seçilen algoritmaya bağlıdır |
| SHA-256 | Tek yönlü, sabit uzunluklu özet üretir | Veri bütünlüğü kontrolü |
| HMAC | Gizli anahtarla özet üretir | Bütünlük ve kaynak doğrulama |

AES blok boyutu 128 bittir. AES-128 ve AES-256 adlarındaki sayı anahtar uzunluğunu belirtir. Bu çalışmada 32 baytlık nRF24 paketi iki adet 16 bayt blok olarak işlenir.

ECB modunda her blok bağımsız şifrelenir; aynı açık metin ve anahtar aynı çıktıyı üretir. ECB, paket kimlik doğrulaması veya yeniden oynatma koruması sağlamaz. Ders uygulamasında şifreleme ve çözme adımları incelenir; doğrulamalı bir bağlantı için AEAD ve paket sayacı denetimi ayrıca tasarlanmalıdır.
