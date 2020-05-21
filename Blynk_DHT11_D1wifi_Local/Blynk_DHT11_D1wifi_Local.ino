/*
此程序通过私人服务器“iot.haotech.ga”,使app与硬件通讯。
*/
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "LL4c1PV_8Syqm0buOaGQBS4gkr5pjQ23";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Tenda_A0D9B8";
char pass[] = "yang1314ai";

#define DHTPIN 14          // What digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
//#define DHTTYPE DHT21   // DHT 21, AM2301

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

// This function sends Arduino's up time every second to Virtual Pin (5).
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.
void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V6, h);
  Blynk.virtualWrite(V5, t);
}

void setup()
{
  // Debug console
  Serial.begin(9600);

  //Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  Blynk.begin(auth, ssid, pass, "iot.haotech.ga", 8080);

  dht.begin();

  // Setup a function to be called every second
  timer.setInterval(5000L, sendSensor);
}

void loop()
{
  Blynk.run();
  timer.run();
}
