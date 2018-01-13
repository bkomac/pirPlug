#include <Espiot.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <OneWire.h>

Espiot espiot;

// Vcc measurement
//ADC_MODE(ADC_VCC);
int lightTreshold = 0; // 0 - dark, >100 - light

String sensorData = "";
int sensorValue;

// CONF
String ver = "1.0.4";
long lastTime = millis();

//DS18B20
#define ONE_WIRE_BUS 15
#define TEMPERATURE_PRECISION 12 // 8 9 10 12

// DHT
#define DHTPIN 16
#define DHTTYPE DHT11 // DHT11
DHT dht(DHTPIN, DHTTYPE);

float humd = NULL;
float temp = NULL;

// PIR
#define PIRPIN 4

//RGB
#define REDPIN 13
#define GREENPIN 12
#define BLUEPIN 14

int red = 1023;
int green = 1023;
int blue = 0;

#define BUILTINLED 2
#define RELEY 50

String MODE = "AUTO";
boolean sentMsg = false;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() { //------------------------------------------------
  Serial.begin(115200);

  pinMode(PIRPIN, INPUT);
  pinMode(RELEY, OUTPUT);
  pinMode(BUILTINLED, OUTPUT);

  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);

  analogWrite(REDPIN, 0);
  analogWrite(GREENPIN, 0);
  analogWrite(BLUEPIN, 0);

  Serial.println("PirPlug ...v" + String(ver));
  delay(300);

  Serial.print("\nPWMRANGE: ");
  Serial.println(PWMRANGE);

  espiot.init(ver);
  //espiot.enableVccMeasure();
  espiot.SENSOR = "PIR,DHT21,DS18B20";

  dht.begin();

  Serial.println("sensors.begin() ...");
  delay(300);
  sensors.begin();

  int statusCode;

  espiot.server.on("/info", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["DHTPIN"] = DHTPIN;
    root["DHTTYPE"] = DHTTYPE;
    root["PIRPIN"] = PIRPIN;
    root["REDPIN"] = REDPIN;
    root["BLUEPIN"] = BLUEPIN;
    root["GREENPIN"] = GREENPIN;

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/state", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    delay(300);
    float humd1 = dht.readHumidity();
    delay(100);
    float temp1 = dht.readTemperature();

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;

    root["temp"] = temp1;
    root["hum"] = humd1;
    root["light"] = sensorValue;

    JsonObject &rgb = root.createNestedObject("rgb");
    rgb["red"] = red;
    rgb["green"] = green;
    rgb["blue"] = blue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/rgb", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    delay(300);
    float humd1 = dht.readHumidity();
    delay(100);
    float temp1 = dht.readTemperature();

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;

    root["red"] = red;
    root["green"] = green;
    root["blue"] = blue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/rgb", HTTP_OPTIONS, []() {
    espiot.blink();
    espiot.server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    espiot.server.sendHeader("Access-Control-Allow-Headers",
                      "Origin, X-Requested-With, Content-Type, Accept");
    espiot.server.send(200, "text/plain", "");
  });

  espiot.server.on("/rgb", HTTP_PUT, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(espiot.server.arg("plain"));

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;

    red = root["red"];
    green = root["green"];
    blue = root["blue"];

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

} //--

// -----------------------------------------------------------------------------
// loop ------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void loop() {
  yield();

  espiot.loop();

  sensorValue = analogRead(A0); // read analog input pin 0

  sensors.requestTemperatures();

  float humd1 = NULL;
  float temp1 = NULL;

  // DHT
  delay(300);
  humd1 = dht.readHumidity();
  delay(100);
  temp1 = dht.readTemperature();

  float temp = sensors.getTempC(0);

  if (String(humd1) != "nan" && String(temp1) != "nan" && temp1 != NULL) {
    humd = humd1;
    temp = temp1;

    Serial.print(F("\nTemperature: "));
    Serial.print(temp);
    Serial.print(F("\nHumidity: "));
    Serial.println(humd);

  } else {
    temp = NULL;
    humd = NULL;
  }

  yield();

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["id"] = espiot.getDeviceId();
  root["name"] = espiot.deviceName;
  root["temp"] = temp;
  //root["hum"] = humd;
  root["light"] = sensorValue;
  root["motion"] = NULL;


  yield();

  int inputState = digitalRead(PIRPIN);
  if (MODE == "AUTO") {
    if (inputState == HIGH) {
      Serial.println(F("Sensor high..."));
      digitalWrite(BUILTINLED, LOW);
      digitalWrite(RELEY, HIGH);

      analogWrite(BLUEPIN, blue);
      analogWrite(GREENPIN, green);
      analogWrite(REDPIN, red);

      lastTime = millis();
      if(!sentMsg){
        String content;
        root["motion"] = true;
        root.printTo(content);
        espiot.mqPublishSubTopic(content, "motion");
        sentMsg = true;
      }
    } else{
        root["motion"] = false;
    }
  }

  if (millis() > lastTime + espiot.timeOut && inputState != HIGH) {
    digitalWrite(RELEY, LOW);
    digitalWrite(BUILTINLED, HIGH);
    analogWrite(BLUEPIN, 1023);
    analogWrite(GREENPIN, 1023);
    analogWrite(REDPIN, 1023);

    sentMsg = false;
    Serial.println("timeout... " + String(lastTime));
    Serial.print("Light: ");
    Serial.println(sensorValue, DEC);

    String content;
    root.printTo(content);
    espiot.mqPublish(content);

    lastTime = millis();
  }

  yield();

} //---------------------------------------------------------------
