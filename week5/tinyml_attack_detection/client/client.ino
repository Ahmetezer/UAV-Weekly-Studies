#include "LabWire.h"
WiFiUDP samples;

// Kontrol TCP'si yalnızca pencere başlangıcını bildirir. Ölçülen veri UDP'dir;
// böylece TCP yeniden iletimi, bilinçli olarak eksiltilen paketi geri getirmez.
bool waitToken(WiFiClient& link, uint8_t expected) {
  const uint32_t start=millis();
  while (millis()-start<5000 && link.connected()) {
    if (link.available()) return link.read()==expected;
    delay(1);  // ESP8266 Wi-Fi yığınına ve watchdog'a işlem zamanı verilir.
  }
  return false;
}

void performRun(uint32_t run, uint16_t count) {
  WiFiClient link;
  if (!link.connect(LAB_SERVER,CONTROL_PORT)) {
    Serial.println("ERROR,C,server_unreachable"); return;
  }
  link.setNoDelay(true);
  const RunHeader header{LAB_MAGIC,run,count};
  if (link.write(reinterpret_cast<const uint8_t*>(&header),sizeof(header))!=sizeof(header)) {
    Serial.println("ERROR,C,header"); return;
  }
  randomSeed(run);
  bool attack=false;
  uint8_t testLabels[TEST_COUNT];
  for (uint16_t trial=0;trial<count;++trial) {
    if (!waitToken(link,TOKEN_GO)) { Serial.println("ERROR,C,window_sync"); return; }
    // Her iki denemede bir normal ve bir atak vardır; sıraları rastgele seçilir.
    // 1000 deneme sonunda tam 500/500 denge, ayrıca yeni testte 100/100 oluşur.
    attack=(trial%2==0)?random(2)==1:!attack;
    if (count==TEST_COUNT) testLabels[trial]=attack;
    const int dropped=attack?random(1,9):-1;  // İlk/son referans paketleri düşürülmez.
    const uint32_t start=millis();
    for (uint16_t slot=0;slot<SLOT_COUNT;++slot) {
      while (millis()-start<slot*SLOT_MS) delay(1);
      if (slot==dropped) continue;
      const Datagram packet{run,trial,slot,{}};
      if (!samples.beginPacket(LAB_SERVER,DATA_PORT) ||
          samples.write(reinterpret_cast<const uint8_t*>(&packet),sizeof(packet))!=sizeof(packet) ||
          !samples.endPacket()) { Serial.println("ERROR,C,udp_send"); return; }
    }
    // Etiket RF veri paketine eklenmez. Bilgisayar, etiket ve ölçümü bu kimlikle birleştirir.
    Serial.printf("C,%lu,%u,%u,%d\n",(unsigned long)run,trial,attack,dropped);
  }
  // Test etiketleri ancak TÜM tahminler tamamlandıktan sonra server'a gönderilir.
  if (count==TEST_COUNT &&
      (!waitToken(link,TOKEN_LABELS) || link.write(testLabels,sizeof(testLabels))!=sizeof(testLabels))) {
    Serial.println("ERROR,C,test_labels"); return;
  }
  if (!waitToken(link,TOKEN_DONE)) { Serial.println("ERROR,C,finish"); return; }
  Serial.printf("DONE,C,%lu\n",(unsigned long)run);
}

void setup() {
  Serial.begin(115200); Serial.setTimeout(100);
  WiFi.persistent(false); WiFi.mode(WIFI_AP);
  if (!WiFi.softAPConfig(LAB_AP,LAB_AP,LAB_MASK) ||
      !WiFi.softAP(LAB_SSID,LAB_PASSWORD,6,false,1) || !samples.begin(DATA_PORT)) {
    Serial.println("ERROR,C,setup");
    while(true) delay(100);
  }
}
void loop() {
  if (!Serial.available()) { delay(2); return; }
  String command=Serial.readStringUntil('\n'); command.trim();
  if (command=="?") { Serial.println("READY,C"); return; }
  unsigned long run=0,count=0; char extra=0;
  if (sscanf(command.c_str(),"R %lu %lu %c",&run,&count,&extra)==2 &&
      run!=0 && (count==TRAIN_COUNT || count==TEST_COUNT)) performRun(run,count);
}
