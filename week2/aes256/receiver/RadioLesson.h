#pragma once
#include "RadioConfig.h"
// uint32_t gibi sabit genişlikli türler, Uno ve Mega'da aynı paketi oluşturur.
struct __attribute__((packed)) Message {
  uint16_t magic; uint8_t revision, mode;
  uint32_t sequence, uptime;
  int32_t value;
  char text[16];
};
static_assert(sizeof(Message)==32,"Message must fit nRF24 payload");
#if LESSON_BITS==128
AESSmall128 cipher;
#elif LESSON_BITS==256
AESSmall256 cipher;
#endif

void encodeMessage(const Message& input,uint8_t* output) {
#if LESSON_BITS==0
  memcpy(output,&input,32);
#else
  cipher.encryptBlock(output,reinterpret_cast<const uint8_t*>(&input));
  cipher.encryptBlock(output+16,reinterpret_cast<const uint8_t*>(&input)+16);
#endif
}
void decodeMessage(const uint8_t* input,Message& output) {
#if LESSON_BITS==0
  memcpy(&output,input,32);
#else
  cipher.decryptBlock(reinterpret_cast<uint8_t*>(&output),input);
  cipher.decryptBlock(reinterpret_cast<uint8_t*>(&output)+16,input+16);
#endif
}
void setup() {
  Serial.begin(115200); setupRadio(46);
#if LESSON_BITS!=0
  cipher.setKey(DEMO_KEY,LESSON_BITS/8);
#endif
#if LESSON_TX
  radio.openWritingPipe(RADIO_ADDRESS); radio.stopListening();
  Serial.println(F("TX: saniyede bir 32 bayt gonderim"));
#else
  radio.openReadingPipe(1,RADIO_ADDRESS); radio.startListening();
  Serial.println(F("RX: sifreli ve cozulmus veri gozlemi"));
#endif
}
uint32_t previousSend=0,sequence=0,lastReceived=0,received=0,lost=0;
bool haveSequence=false;
void loop() {
#if LESSON_TX
  const uint32_t now=millis();
  if(now-previousSend<1000) return;
  previousSend=now;
  Message message{};
  message.magic=0xAE22; message.revision=1; message.mode=LESSON_BITS/8;
  message.sequence=sequence++; message.uptime=now; message.value=message.sequence;
  strcpy(message.text,"Merhaba IHA");
  uint8_t encrypted[32]; encodeMessage(message,encrypted);
  const bool acknowledged=radio.write(encrypted,sizeof(encrypted));
  Serial.print(F("seq="));Serial.print(message.sequence);
  Serial.print(F(" donanim_ACK="));Serial.println(acknowledged?F("var"):F("yok"));
#else
  while(radio.available()) {
    const bool aboveThreshold=radio.testRPD();
    uint8_t encrypted[32]; Message message{};
    radio.read(encrypted,sizeof(encrypted)); decodeMessage(encrypted,message);
    if(message.magic!=0xAE22 || message.revision!=1 || message.mode!=LESSON_BITS/8) {
      Serial.println(F("Paket formati/anahtar/deney modu uyusmuyor")); continue;
    }
    // Geri giden sayaç, vericinin yeniden başladığı anlamına gelebilir.
    // Yalnız ileri yöndeki sıra boşlukları kayıp sayılır; ilk paket öncesi kayıp bilinmez.
    if(haveSequence && int32_t(message.sequence-lastReceived)>0)
      lost+=message.sequence-lastReceived-1;
    lastReceived=message.sequence; haveSequence=true; ++received;
    message.text[15]='\0';
    Serial.print(F("HEX="));
    for(uint8_t b:encrypted) { if(b<16)Serial.print('0');Serial.print(b,HEX);Serial.print(' '); }
    Serial.print(F(" | seq="));Serial.print(message.sequence);
    Serial.print(F(" mesaj="));Serial.print(message.text);
    Serial.print(F(" | RPD="));Serial.print(aboveThreshold?F("esik_ustu"):F("esik_alti"));
    Serial.print(F(" | kayip_%="));Serial.println(100.0*lost/(received+lost),2);
  }
#endif
}
