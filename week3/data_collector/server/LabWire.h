#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// İki kart kendi kapalı deney bağlantısını kurar. Ev/ofis Wi-Fi bilgisi gerekmez.
constexpr char LAB_SSID[] = "AE-UAV-LAB";
constexpr char LAB_PASSWORD[] = "UavLab-2026";
const IPAddress LAB_AP(192,168,4,1), LAB_SERVER(192,168,4,2), LAB_MASK(255,255,255,0);
constexpr uint16_t CONTROL_PORT=4210, DATA_PORT=4211;
constexpr uint32_t LAB_MAGIC=0x41454C42;
constexpr uint16_t SLOT_COUNT=10, SLOT_MS=18, WINDOW_MS=250;
constexpr uint16_t TRAIN_COUNT=1000, TEST_COUNT=200;
constexpr uint8_t TOKEN_GO='G', TOKEN_LABELS='L', TOKEN_DONE='D';

// Sabit genişlikli alanlar iki kartta aynı 12/64 baytlık taşıma biçimini korur.
struct RunHeader { uint32_t magic, run, count; };
struct Datagram { uint32_t run; uint16_t trial, slot; uint8_t payload[56]; };
static_assert(sizeof(RunHeader)==12 && sizeof(Datagram)==64,"Wire format mismatch");
