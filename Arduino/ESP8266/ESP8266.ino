#include <ESP8266WiFi.h>

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

const char* ssid     = "VINICIUS";
const char* password = "25071997";
const char* host = "192.168.0.10";
const int httpPort = 30000;

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
     Connect to server
  */
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
}

void loop() {
  delay(100);

  /*
     If got disconnected, try stablish connection again
  */
  if (!client.connected())
  {
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
  }

  /*
     Read values from serial and send to server
  */
  if (Serial.available())
  {
    Serial.readBytesUntil('-', (char*)&mv, sizeof mv);
    client.write((char*)&mv, sizeof mv);
  }

  /*
     Read values from client and send to serial
  */
  if (client.available())
  {
    client.readBytesUntil('-', (char*)&cv, sizeof cv);
    Serial.write((char*)&cv, sizeof cv);
  }    
}
