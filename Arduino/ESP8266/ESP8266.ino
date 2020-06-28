#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/*
   Structs
*/
struct MonitoringValues {
  char P[10];
  char T[10];
  char H[10];
  char D[5];
  char flag = '-';
};

struct ControlValues {
  char servoDirection[2];
  char LEDColor[2];
  char LEDDirection[2];
  char flag = '-';
};

/*
   Global variables
*/
MonitoringValues mv;
ControlValues cv;

WiFiClient client;
PubSubClient pubSubClient(client);

const char* ssid     = "VINICIUS";
const char* password = "25071997";
const char* mqtt_server = "192.168.0.18";//servidor interno MQTT
const char* mqtt_user = "embarcados";
const char* mqtt_pass = "embarcados";

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  /*
     Connect mqtt broker
  */
  pubSubClient.setServer(mqtt_server, 1883);
  pubSubClient.setCallback(callback);

  cv.servoDirection[0] = '0';
  cv.servoDirection[1] = '\0';
  cv.LEDColor[0] = '0';
  cv.LEDColor[1] = '\0';
  cv.LEDDirection[0] = '0';
  cv.LEDDirection[1] = '\0';
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "servo-direction") == 0)
  {
    cv.servoDirection[0] = payload[0];
    Serial.write((char*)&cv, sizeof(cv));
  }
  else if (strcmp(topic, "led-color") == 0)
  {
    cv.LEDColor[0] = payload[0];
    Serial.write((char*)&cv, sizeof(cv));
  }
  else if (strcmp(topic, "led-direction") == 0)
  {
    cv.LEDDirection[0] = payload[0];
    Serial.write((char*)&cv, sizeof(cv));
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!pubSubClient.connected()) {
    // Attempt to connect
    if (pubSubClient.connect("ESP8266", mqtt_user, mqtt_pass)) {
      pubSubClient.subscribe("servo-direction");
      pubSubClient.subscribe("led-color");
      pubSubClient.subscribe("led-direction");
    } else {
      // Wait 1 seconds before retrying
      delay(1000);
    }
  }
}

void loop() {
  delay(100);

  /*
       If got MQTT disconnected, try stablish connection again
  */
  if (!pubSubClient.connected()) {
    reconnect();
  }

  pubSubClient.loop();

  /*
     Read values from serial and send to server
  */
  if (Serial.available())
  {
    Serial.readBytesUntil('-', (char*)&mv, sizeof mv);
    pubSubClient.publish("pressure", mv.P);
    pubSubClient.publish("temperature", mv.T);
    pubSubClient.publish("humidity", mv.H);
    pubSubClient.publish("distance", mv.D);
  }
}
