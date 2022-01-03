/**
 * Récupère toutes les trames sur le récepteur 433 Mhz,
 * essaie de les décoder avec la bibliothèque Arduino Oregon Library
 * https://github.com/Mickaelh51/Arduino-Oregon-Library et les
 * dumpe sur le port série en JSON. 
 */

ICACHE_RAM_ATTR void ext_int_1();

#include <SPI.h>
#include <EEPROM.h>
#include <Oregon.h>
 
#define ARDUINO_PIN D2
 
void setup() {
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(ARDUINO_PIN), ext_int_1, CHANGE);
}
 
void loop() {
  routine();
}

void routine() {
  cli();
  word p = pulse;
  pulse = 0;
  sei();
   
  if(p != 0) {
    if(orscV2.nextPulse(p)) {
      const byte* DataDecoded = DataToDecoder(orscV2); 
      byte source = channel(DataDecoded);
      int src = int(source);
      int temp = temperature(DataDecoded);
      Serial.println(src);
      if(source == 1 && temp < 30 && temp > 0){
        String json = "{\"source\":";
        json += String(source);
        json += ",\"temperature\":";
        json += temperature(DataDecoded);
        json += ",\"humidity\":";
        json += humidity(DataDecoded);
        json += ",\"battery\":\"";
        json += battery(DataDecoded) > 50 ? "ok" : "low";
        json += "\"}";
        Serial.println(json);
        }
    }
  }
}
