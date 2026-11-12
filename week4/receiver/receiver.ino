#include "LinkProtocol.h"
enum class FollowState:uint8_t { SEARCH,FOLLOW,COAST };
FollowState state=FollowState::SEARCH;
uint32_t timeOffset=0,lastContact=0,lastScan=0,session=0,lastSequence=0,replySequence=0;
uint32_t received=0,lost=0,repliesAcknowledged=0,lastReport=0;
bool haveSequence=false;
void setup() {
  Serial.begin(115200); beginLink(); tune(0,true);
  Serial.println(F("FHSS yer istasyonu: SEARCH -> FOLLOW -> COAST"));
}
void loop() {
  uint32_t now=millis();
  while(radio.available()) {
    LinkFrame data{};
    if(!readFrame(data) || data.kind!=1)continue;
    const uint32_t observed=millis();
    // Uptime ofseti yalnız geçerli veriyle güncellenir. RF/AES gecikmesi birkaç ms
    // belirsizlik yaratır; bu yüzden slot kenarlarında 40 ms koruma aralığı vardır.
    timeOffset=data.uptime-observed;
    if(!haveSequence || session!=data.session) {
      session=data.session; lastSequence=data.sequence; haveSequence=true;
    } else {
      const int32_t advance=int32_t(data.sequence-lastSequence);
      if(advance<=0)continue;
      lost+=uint32_t(advance)-1; lastSequence=data.sequence;
    }
    ++received; lastContact=observed; state=FollowState::FOLLOW;
    // Master önce RX moduna dönebilsin. Yön değiştirme aynı donanım radyoyla yapılır.
    delayMicroseconds(1500);
    LinkFrame answer{};
    answer.magic=0xAE44;answer.kind=2;answer.channelIndex=activeIndex;
    answer.session=session;answer.sequence=replySequence++;answer.uptime=millis();
    answer.echo=data.sequence;answer.reading=received;strcpy(answer.text,"GS_OK");
    radio.stopListening();
    if(sendFrame(answer))++repliesAcknowledged;
    radio.startListening();
  }
  now=millis();
  if(state!=FollowState::SEARCH) {
    const uint32_t silence=now-lastContact;
    if(silence>=SEARCH_AFTER_MS) { state=FollowState::SEARCH;lastScan=now; }
    else {
      state=silence>=HOLD_AFTER_MS?FollowState::COAST:FollowState::FOLLOW;
      // Paket kesilse de son saat ofsetiyle doğru frekansların takibi sürer.
      tune(channelAt(now+timeOffset),true);
    }
  }
  if(state==FollowState::SEARCH && now-lastScan>=SEARCH_DWELL_MS) {
    lastScan=now;tune((activeIndex+1)%HOP_COUNT,true);
  }
  if(now-lastReport>=1000) {
    lastReport=now;
    Serial.print(F("durum="));
    Serial.print(state==FollowState::SEARCH?F("SEARCH"):
                state==FollowState::FOLLOW?F("FOLLOW"):F("COAST"));
    Serial.print(F(" kanal="));Serial.print(HOP_CHANNELS[activeIndex]);
    Serial.print(F(" RX="));Serial.print(received);
    Serial.print(F(" kayip="));Serial.print(lost);
    Serial.print(F(" kayip_%="));Serial.print(received+lost?100.0*lost/(received+lost):0,2);
    Serial.print(F(" cevap_ACK="));Serial.println(repliesAcknowledged);
  }
}
