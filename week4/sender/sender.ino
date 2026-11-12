#include "LinkProtocol.h"
uint32_t sessionId=0,nextSequence=0,lastSend=0,sentAt=0,expectedEcho=0;
uint32_t attempts=0,radioAcknowledged=0,applicationReplies=0,lastReport=0,lastReply=0;
bool waiting=false,haveReply=false;
void setup() {
  Serial.begin(115200); beginLink(); radio.stopListening();
  // Ders deneyinde iki yeniden başlatmayı ayırmak için oturum kimliği.
  // Bu değer kriptografik nonce veya güvenli rastgele sayı olarak kullanılmaz.
  sessionId=(uint32_t(analogRead(A0))<<20)^micros();
  if(!sessionId)sessionId=1;
  Serial.println(F("FHSS master: Uno/Mega, AES256, 15 kanal, cift yonlu uygulama verisi"));
}
void loop() {
  uint32_t now=millis();
  if(waiting) {
    while(radio.available()) {
      LinkFrame response{};
      if(readFrame(response) && response.kind==2 && response.session==sessionId &&
         response.echo==expectedEcho) {
        ++applicationReplies; lastReply=now; haveReply=true; waiting=false;
        response.text[7]='\0';
        Serial.print(F("GS="));Serial.println(response.text);
        break;
      }
    }
    if(waiting && now-sentAt>=REPLY_MS) waiting=false;
    if(!waiting)radio.stopListening();
  }
  // Master'ın zamanlaması bağlantı kopunca durmaz; aramayı yer istasyonu yapar.
  if(!waiting) {
    tune(channelAt(now),false);
    if(now-lastSend>=SEND_MS && safeToSend(now)) {
      lastSend=now;
      LinkFrame frame{};
      frame.magic=0xAE44; frame.kind=1; frame.channelIndex=activeIndex;
      frame.session=sessionId; frame.sequence=nextSequence++; frame.uptime=millis();
      frame.reading=frame.sequence; strcpy(frame.text,"UAV_OK");
      expectedEcho=frame.sequence; ++attempts;
      if(sendFrame(frame))++radioAcknowledged;
      // İkinci yöndeki veri gerçek bir uygulama cevabıdır; boş donanım ACK'si sayılmaz.
      radio.startListening(); sentAt=millis(); waiting=true;
    }
  }
  if(now-lastReport>=1000) {
    lastReport=now;
    Serial.print(F("kanal="));Serial.print(HOP_CHANNELS[activeIndex]);
    Serial.print(F(" link="));Serial.print(haveReply && now-lastReply<500?F("UP"):F("DOWN"));
    Serial.print(F(" TX="));Serial.print(attempts);
    Serial.print(F(" RF_ACK="));Serial.print(radioAcknowledged);
    Serial.print(F(" GS_cevap="));Serial.print(applicationReplies);
    Serial.print(F(" RF_kayip_%="));
    Serial.println(attempts?100.0*(attempts-radioAcknowledged)/attempts:0,2);
  }
}
