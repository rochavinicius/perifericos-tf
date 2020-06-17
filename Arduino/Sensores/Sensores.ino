#include <SFE_BMP180.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <Servo.h>
#include <FastLED.h>

/*
   Defines
*/
#define NUM_LEDS 8
#define LED_PIN 6

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
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int numRows = 4;
const int numCols = 20;

MonitoringValues mv;
ControlValues cv;

SFE_BMP180 bmp;

DHT dht(A1, DHT22);

Servo servo;

CRGBArray<NUM_LEDS> leds;

void setup() {
  /*
     LCD
  */
  lcd.begin(numCols, numRows);
  lcd.setCursor(0, 0);

  /*
     BMP180
  */
  if (!bmp.begin()) {
    lcd.print("BMP Failed.");
    while (1);
  }

  /*
     DHT22
  */
  dht.begin();

  /*
     Servo motor
  */
  servo.attach(9);

  /*
     LED Stripe
  */
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);

  /*
     Serial
  */
  Serial.begin(115200);
}

void loop() {
  /*
     Monitoring
  */
  readPressure();
  readTemperatureAndHumidity();
  readDistance();

  /*
     Control
  */
  sendToSerial();
  if (readFromSerial())
  {
    moveServo();
  }
  fillLeds();
}

void sendToSerial() {
  Serial.write((char*)&mv, sizeof(mv));
}

int readFromSerial() {
  if (!Serial.available())
    return 0;

  Serial.readBytesUntil('-', (char*)&cv, sizeof(cv));
  return 1;
}

void fillLeds() {
  int r, g, b, color, dir;

  if (strcmp(cv.LEDColor, "1") == 0)
    color = 1;
  else if (strcmp(cv.LEDColor, "2") == 0)
    color = 2;
  else
    color = 0;

  if (strcmp(cv.LEDDirection, "1") == 0)
    dir = 1;
  else
    dir = 0;

  switch (color) {
    case 0: // red
      r = 255;
      g = 0;
      b = 0;
      break;
    case 1: // green
      r = 0;
      g = 255;
      b = 0;
      break;
    case 2: // blue
      r = 0;
      g = 0;
      b = 255;
      break;
    default: // default white
      r = 255;
      g = 255;
      b = 255;
      break;
  }

  if (dir == 0) { // left 2 right
    for (int i = 0; i < NUM_LEDS; i++) {
      leds.fadeToBlackBy(30);
      leds[i] = CRGB(r, g, b);
      FastLED.delay(30);
    }
  } else { // right 2 left
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
      leds.fadeToBlackBy(30);
      leds[i] = CRGB(r, g, b);
      FastLED.delay(30);
    }
  }
}

void moveServo() {
  int pos;
  if (strcmp(cv.servoDirection, "1") == 0)
    pos = 1;
  else if (strcmp(cv.servoDirection, "2") == 0)
    pos = 2;
  else if (strcmp(cv.servoDirection, "3") == 0)
    pos = 3;
  else
    pos = 0;

  switch (pos) {
    case 0: // -90
      servo.write(0);
      break;
    case 1: // -45
      servo.write(68);
      break;
    case 2: // 45
      servo.write(117);
      break;
    case 3: // 90
      servo.write(180);
      break;
    default:
      servo.write(0);
      break;
  }
}

void readDistance() {
  int x = analogRead(A0);
  int d = (6787 / (x - 3)) - 4;
  sprintf(mv.D, "%d", d);

  lcd.setCursor(0, 0);
  lcd.print("Distance: ");
  lcd.print(mv.D);
  lcd.print(" cm");
}

void readTemperatureAndHumidity() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(t) || isnan(h)) {
    lcd.print("Error read DHT22");
    while (1);
  }

  dtostrf(h, 4, 2, mv.H);
  dtostrf(t, 4, 2, mv.T);

  lcd.setCursor(0, 2);
  lcd.print("Temperature: ");
  lcd.print(mv.T);
  lcd.print(" C");
  lcd.setCursor(0, 3);
  lcd.print("Humidity: ");
  lcd.print(mv.H);
  lcd.print(" %");
}

void readPressure() {
  char status;
  double t;
  double p;

  if ((status = bmp.startTemperature()) != 0) {
    delay(status);

    if (bmp.getTemperature(t) != 0) {
      if ((status = bmp.startPressure(3)) != 0) {
        delay(status);

        if (bmp.getPressure(p, t) != 0) {
          dtostrf(p, 4, 2, mv.P);
          lcd.setCursor(0, 1);
          lcd.print("Pressure: ");
          lcd.print(mv.P);
          lcd.print("hPa");
        } else {
          // error get pressure
          lcd.print("Error Get P");
        }
      } else {
        // error start pressure
        lcd.print("Error Start P");
      }
    } else {
      // error get temperature
      lcd.print("Error Get T BMP");
    }
  } else {
    // erro start temperature
    lcd.print("Error Start T BMP");
  }
}
