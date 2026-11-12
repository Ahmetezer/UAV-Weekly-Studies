#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Crypto.h>
#include <AES.h>
#if defined(__AVR_ATmega2560__)
constexpr uint8_t RADIO_CE=9,RADIO_CSN=53;
#else
constexpr uint8_t RADIO_CE=7,RADIO_CSN=8;
#endif
// 5 baytlık adres, kanal ve paket boyutu iki tarafta aynı seçilir.
const uint8_t RADIO_ADDRESS[6]="AEUAV";
const uint8_t DEMO_KEY[32]={0xA1,0x34,0x29,0x51,0x19,0xC2,0x72,0x06,
  0xD4,0x13,0x42,0x9E,0x17,0x68,0x25,0x83,
  0x01,0xF0,0x45,0x8A,0x13,0x2D,0x77,0x9C,0x34,0x21,0x56,0xA8,0x60,0xB5,0x98,0xE2};
RF24 radio(RADIO_CE,RADIO_CSN);
inline void setupRadio(uint8_t channel) {
  if(!radio.begin()) { Serial.println(F("HATA: nRF24 baglantisi")); while(true) delay(100); }
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS); radio.setCRCLength(RF24_CRC_16);
  radio.setPayloadSize(32); radio.setAutoAck(true); radio.setRetries(2,3);
  radio.setChannel(channel);
}
