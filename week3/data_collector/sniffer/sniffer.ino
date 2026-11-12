/*
  Hafta 3 ek ölçüm modu: ESP8266'nın promiscuous callback'inden 802.11 başlığı.
  Kaynak düzeni: ESP8266 Non-OS SDK paket yakalama arayüzü, 12 bayt RxControl.
  RSSI gerçek alım metadata'sıdır. Noise floor/SNR donanımdan alınmadığı için üretilmez.
  Bu kod, client/server deneyinden AYRI olarak bir karta yüklenir.
*/
#include <ESP8266WiFi.h>
extern "C" {
#include <user_interface.h>
}
constexpr uint8_t OBSERVE_CHANNEL=6, QUEUE_SIZE=16;
struct PacketInfo {
  uint32_t timeUs;
  int8_t rssi;
  uint8_t type,subtype,retry,transmitter[6],receiver[6];
  uint16_t sequence,callbackBytes;
};
PacketInfo records[QUEUE_SIZE];
volatile uint8_t head=0,tail=0;
volatile uint32_t discarded=0;

void receiveFrame(uint8_t* buffer,uint16_t length) {
  // SDK'daki 60/128 baytlık callback biçimlerinde ilk 12 bayt RF metadata'sıdır.
  // 12 baytlık yalnız kontrol bilgisi dönüşleri burada MAC paketi sayılmaz.
  if(length!=128 && !(length>=60 && (length-50)%10==0))return;
  const uint8_t* frame=buffer+12;
  const uint8_t type=(frame[0]>>2)&3;
  if(type!=0 && type!=2)return;  // Yönetim/veri: en az 24 baytlık MAC başlığı.
  const uint8_t next=(head+1)%QUEUE_SIZE;
  if(next==tail) { ++discarded;return; }
  PacketInfo& item=records[head];
  item.timeUs=micros();item.rssi=int8_t(buffer[0]);
  item.type=type;item.subtype=(frame[0]>>4)&15;item.retry=(frame[1]>>3)&1;
  memcpy(item.receiver,frame+4,6);memcpy(item.transmitter,frame+10,6);
  item.sequence=(uint16_t(frame[22])|(uint16_t(frame[23])<<8))>>4;
  item.callbackBytes=length;
  head=next;
  // Callback içinde Serial/dosya/ağ işlemi yapılmaz. Wi-Fi görevini bloke etmez.
}

void printMac(const uint8_t* mac) {
  for(uint8_t i=0;i<6;++i) { if(i)Serial.print(':');if(mac[i]<16)Serial.print('0');Serial.print(mac[i],HEX); }
}

void setup() {
  Serial.begin(115200);
  WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.disconnect();
  wifi_promiscuous_enable(0);
  wifi_set_channel(OBSERVE_CHANNEL);
  wifi_set_promiscuous_rx_cb(receiveFrame);
  Serial.println("time_us,channel,rssi,type,subtype,retry,sequence,callback_bytes,transmitter,receiver");
  wifi_promiscuous_enable(1);
}

void loop() {
  if(tail!=head) {
    // Tek çekirdekli ESP8266'da kısa kopya sırasında kuyruk girdisi sabit tutulur.
    noInterrupts();PacketInfo item=records[tail];tail=(tail+1)%QUEUE_SIZE;interrupts();
    Serial.printf("%lu,%u,%d,%u,%u,%u,%u,%u,",(unsigned long)item.timeUs,OBSERVE_CHANNEL,
                  item.rssi,item.type,item.subtype,item.retry,item.sequence,item.callbackBytes);
    printMac(item.transmitter);Serial.print(',');printMac(item.receiver);Serial.println();
  }
  static uint32_t lastReport=0;
  if(millis()-lastReport>=1000) {
    lastReport=millis();
    Serial.printf("# discarded_callback_records=%lu\n",(unsigned long)discarded);
  }
  delay(0);
}
