#pragma once
#include "LabWire.h"
WiFiServer control(CONTROL_PORT);
WiFiUDP samples;
struct Observation { int16_t arrival[SLOT_COUNT], rssi[3]; };

bool capture(WiFiClient& link,uint32_t run,uint16_t trial,Observation& obs) {
  for (auto& value:obs.arrival) value=-1;
  uint8_t nextRssi=0;
  constexpr uint16_t sampleAt[]={0,90,180};
  const uint32_t start=millis();
  if (link.write(&TOKEN_GO,1)!=1) return false;
  while (millis()-start<WINDOW_MS) {
    const uint32_t elapsed=millis()-start;
    if (nextRssi<3 && elapsed>=sampleAt[nextRssi]) {
      const int32_t rssi=WiFi.RSSI();
      if (WiFi.status()!=WL_CONNECTED || rssi>=0 || rssi<-127) return false;
      obs.rssi[nextRssi++]=rssi;
    }
    const int size=samples.parsePacket();
    if (size>0) {
      Datagram packet{};
      if (size==sizeof(packet) && samples.remoteIP()==LAB_AP &&
          samples.read(reinterpret_cast<uint8_t*>(&packet),sizeof(packet))==sizeof(packet) &&
          packet.run==run && packet.trial==trial && packet.slot<SLOT_COUNT &&
          obs.arrival[packet.slot]<0) {
        const uint32_t arrival=millis()-start;
        if(arrival<WINDOW_MS) obs.arrival[packet.slot]=arrival;
      }
      while(samples.available()) samples.read();
    }
    delay(1);
  }
  return nextRssi==3 && link.connected();
}

int emitObservation(uint32_t run,uint16_t trial,const Observation& obs) {
  int16_t missing=0;
  for(uint8_t slot=1;slot<SLOT_COUNT-1;++slot) missing+=obs.arrival[slot]<0;
  const int16_t refs=(obs.arrival[0]<0)+(obs.arrival[9]<0);
  const int16_t span=refs?-1:obs.arrival[9]-obs.arrival[0];
  const int16_t low=min(obs.rssi[0],min(obs.rssi[1],obs.rssi[2]));
  const int16_t high=max(obs.rssi[0],max(obs.rssi[1],obs.rssi[2]));
  // Bu sıra lib/tree_model.py içindeki FEATURES ile birebir aynı olmalıdır.
  const int16_t features[]={missing,refs,span,
      int16_t((obs.rssi[0]+obs.rssi[1]+obs.rssi[2])/3),int16_t(high-low)};
  const uint32_t started=micros();
  const int answer=predict(features);
  const uint32_t latency=micros()-started;
  Serial.printf("S,%lu,%u",(unsigned long)run,trial);
  for(auto value:obs.rssi) Serial.printf(",%d",value);
  for(auto value:obs.arrival) Serial.printf(",%d",value);
  for(auto value:features) Serial.printf(",%d",value);
  Serial.printf(",%d,%lu\n",answer,(unsigned long)latency);
  return answer;
}

bool checkFreshTest(WiFiClient& link,uint32_t run,const int8_t* predictions) {
  uint8_t labels[TEST_COUNT];
  if(link.write(&TOKEN_LABELS,1)!=1 ||
     link.readBytes(reinterpret_cast<char*>(labels),sizeof(labels))!=sizeof(labels)) return false;
  uint16_t correct=0, attacks=0;
  for(uint16_t i=0;i<TEST_COUNT;++i) {
    if(labels[i]>1) return false;
    attacks+=labels[i]; correct+=predictions[i]==labels[i];
  }
  if(attacks!=TEST_COUNT/2) return false;
  if(!MODEL_READY) Serial.printf("RESULT,S,%lu,NA\n",(unsigned long)run);
  else {
    // Kayan noktalı printf/aritmetik yerine yüzdeyi yüzde-bir biriminde hesapla.
    const uint32_t scaled=uint32_t(correct)*10000UL/TEST_COUNT;
    Serial.printf("RESULT,S,%lu,%u,%u,%lu.%02lu%%\n",(unsigned long)run,correct,TEST_COUNT,
                  (unsigned long)(scaled/100),(unsigned long)(scaled%100));
  }
  return true;
}

void serve(WiFiClient& link) {
  link.setNoDelay(true); link.setTimeout(2000);
  RunHeader header{};
  if(link.readBytes(reinterpret_cast<char*>(&header),sizeof(header))!=sizeof(header) ||
     header.magic!=LAB_MAGIC || !header.run ||
     (header.count!=TRAIN_COUNT && header.count!=TEST_COUNT)) {
    Serial.println("ERROR,S,invalid_header"); return;
  }
  Serial.printf("META,S,%lu,%ld,%s,%s\n",(unsigned long)header.run,(long)WiFi.channel(),
                WiFi.BSSIDstr().c_str(),MODEL_TAG);
  int8_t predictions[TEST_COUNT];
  for(uint16_t trial=0;trial<header.count;++trial) {
    Observation observation{};
    if(!capture(link,header.run,trial,observation)) {
      Serial.println("ERROR,S,window_capture"); return;
    }
    const int answer=emitObservation(header.run,trial,observation);
    if(header.count==TEST_COUNT) predictions[trial]=answer;
  }
  if(header.count==TEST_COUNT && !checkFreshTest(link,header.run,predictions)) {
    Serial.println("ERROR,S,test_evaluation"); return;
  }
  if(link.write(&TOKEN_DONE,1)!=1) { Serial.println("ERROR,S,finish"); return; }
  Serial.printf("DONE,S,%lu\n",(unsigned long)header.run);
}

void setup() {
  Serial.begin(115200); Serial.setTimeout(100);
  WiFi.persistent(false); WiFi.mode(WIFI_STA); WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  if(!WiFi.config(LAB_SERVER,LAB_AP,LAB_MASK)) Serial.println("ERROR,S,static_ip");
  WiFi.begin(LAB_SSID,LAB_PASSWORD);
  if(!samples.begin(DATA_PORT)) {
    Serial.println("ERROR,S,udp_bind"); while(true) delay(100);
  }
  control.begin();
}
void loop() {
  if(Serial.available()) {
    String command=Serial.readStringUntil('\n'); command.trim();
    if(command=="?" && WiFi.status()==WL_CONNECTED) Serial.printf("READY,S,%s\n",MODEL_TAG);
  }
  WiFiClient link=control.accept();
  if(link && link.remoteIP()==LAB_AP) { serve(link); link.stop(); }
  delay(1);
}
