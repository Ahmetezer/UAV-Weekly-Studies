#pragma once
#include "RadioConfig.h"
// 15 kanal, 5 saniyelik zaman dilimleri. Tüm kanallar 2.4 GHz deney bandında seçildi.
const uint8_t HOP_CHANNELS[]={5,10,15,20,25,30,35,40,45,50,55,60,65,70,75};
constexpr uint8_t HOP_COUNT=sizeof(HOP_CHANNELS);
constexpr uint32_t HOP_MS=5000, SEND_MS=50, GUARD_MS=40;
constexpr uint32_t REPLY_MS=25, SEARCH_DWELL_MS=80, HOLD_AFTER_MS=500, SEARCH_AFTER_MS=15000;
struct __attribute__((packed)) LinkFrame {
  uint16_t magic;
  uint8_t kind,channelIndex;  // 1: İHA telemetrisi, 2: yer istasyonu uygulama cevabı
  uint32_t session,sequence,uptime,echo;
  int32_t reading;
  char text[8];
};
static_assert(sizeof(LinkFrame)==32,"LinkFrame must be 32 bytes");
AESSmall256 linkCipher;
uint8_t activeIndex=255;
inline uint8_t channelAt(uint32_t masterTime) { return (masterTime/HOP_MS)%HOP_COUNT; }
inline bool safeToSend(uint32_t masterTime) {
  uint32_t phase=masterTime%HOP_MS;
  return phase>=GUARD_MS && phase<HOP_MS-GUARD_MS;
}
void tune(uint8_t index,bool listening) {
  if(index==activeIndex) return;
  radio.stopListening(); radio.setChannel(HOP_CHANNELS[index]); activeIndex=index;
  if(listening) radio.startListening();
}
void encryptFrame(const LinkFrame& frame,uint8_t* bytes) {
  linkCipher.encryptBlock(bytes,reinterpret_cast<const uint8_t*>(&frame));
  linkCipher.encryptBlock(bytes+16,reinterpret_cast<const uint8_t*>(&frame)+16);
}
bool readFrame(LinkFrame& frame) {
  uint8_t bytes[32]; radio.read(bytes,sizeof(bytes));
  linkCipher.decryptBlock(reinterpret_cast<uint8_t*>(&frame),bytes);
  linkCipher.decryptBlock(reinterpret_cast<uint8_t*>(&frame)+16,bytes+16);
  return frame.magic==0xAE44 && frame.channelIndex<HOP_COUNT && (frame.kind==1 || frame.kind==2);
}
bool sendFrame(const LinkFrame& frame) {
  uint8_t bytes[32]; encryptFrame(frame,bytes);
  // RF24 sürümlerindeki kurtarma döngüsüne bağlı kalmadan süreyi biz sınırlarız.
  // Donanım sonucu görülene kadar bayraklar temizlenmez; yarışta sonuç kaybolmaz.
  radio.clearStatusFlags();
  radio.startWrite(bytes,sizeof(bytes),false);
  const uint32_t start=millis();
  while(millis()-start<12) {
    const uint8_t status=radio.update();
    if(status&RF24_TX_DS) { radio.clearStatusFlags(RF24_TX_DS); return true; }
    if(status&RF24_TX_DF) {
      radio.clearStatusFlags(RF24_TX_DF); radio.flush_tx(); return false;
    }
  }
  radio.flush_tx(); return false;
}
void beginLink() {
  setupRadio(HOP_CHANNELS[0]);
  linkCipher.setKey(DEMO_KEY,32);
  radio.openWritingPipe(RADIO_ADDRESS); radio.openReadingPipe(1,RADIO_ADDRESS);
}
