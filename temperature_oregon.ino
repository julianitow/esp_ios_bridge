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
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DiOremote.h>

#define ESP_RECV_PIN D2
#define ESP_EMIT_PIN D6

#define wifi_ssid "uifeedu75"
#define wifi_password "mandalorianBGdu75"

#define mqtt_server "share.alesia-julianitow.ovh"
#define temperature_topic "sensor/temperature"
#define humidity_topic "sensor/humidity"
#define relay_set_on_topic "relay/set/on"
#define relay_get_on_topic "relay/get/on"

long lastMsg = 0;
long lastRecu = 0;

int temp = 0;

char message_buff[100];

const unsigned long ON_CODE  = 1278825104;
const unsigned long OFF_CODE = 1278825088;

bool relay_status;

WiFiClient espClient;
PubSubClient client(espClient);
DiOremote myRemote = DiOremote(ESP_EMIT_PIN);
 
void setup() {
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(ESP_RECV_PIN), ext_int_1, CHANGE);

  setup_wifi();           //On se connecte au réseau wifi
  client.setServer(mqtt_server, 1883);    //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);
}
 
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  routine();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion a ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connexion WiFi etablie ");
  Serial.print("=> Addresse IP : ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  //Boucle jusqu'à obtenur une reconnexion
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect("ESP8266Client")) {
      Serial.println("OK");
      client.subscribe(relay_set_on_topic);
      client.subscribe(relay_get_on_topic);
      Serial.println("Subscribed to:" + String(relay_set_on_topic));
      Serial.println("Subscribed to:" + String(relay_get_on_topic));
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println(" On attend 5 secondes avant de recommencer");
      delay(5000);
    }
  }
}

void routine() {
  long now = millis();
  cli();
  word p = pulse;
  pulse = 0;
  sei();
   
  if(p != 0) {
    if(orscV2.nextPulse(p)) {
      const byte* DataDecoded = DataToDecoder(orscV2); 
      byte source = channel(DataDecoded);
      int src = int(source);
      temp = temperature(DataDecoded);
      int batt = battery(DataDecoded);
      int hum = humidity(DataDecoded);
      if(source == 1 && temp < 30 && temp > 0 && batt > 50){
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
        if (now - lastMsg > 1000 * 60) {
          lastMsg = now;
        }
        client.publish(temperature_topic, String(temp).c_str(), true);   //Publie la température sur le topic temperature_topic
        client.publish(humidity_topic, String(hum).c_str(), true);
      }
    }
  }
}

void sendOnOff(bool val) {
  if(val){
     Serial.println("Sending: " + String(ON_CODE));
     myRemote.send(ON_CODE);
  } else {
    Serial.println("Sending: " + String(OFF_CODE));
    myRemote.send(OFF_CODE);
  }
  relay_status = val;
}

void getOnOff() {
  client.publish(relay_get_on_topic, String(relay_status).c_str(), true);
}

void callback(char* tpc, byte* payload, unsigned int length) {

  int i = 0;
  String topic = String(tpc);
  Serial.println("Message recu =>  topic: " + topic);
  Serial.print(" | longueur: " + String(length,DEC));
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  
  String msgString = String(message_buff);
  Serial.println("Payload: " + msgString);

  if(topic == relay_get_on_topic) {
    getOnOff();
  } else if (topic == relay_set_on_topic) {
    if(msgString == "false") {
      sendOnOff(false);
    } else {
      sendOnOff(true);
    }
  }
}
