/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/kareem/Documents/IOT/SmartPlantWateringSystem/l14_moisture-kareemcru/L14_04_PlantWater/src/L14_04_PlantWater.ino"
/*
 * Project L14_04_PlantWater
 * Description: Getting our plant water the quick way
 * Author: Kareem Crum
 * Date: 19-APR-2021
 */

//#include's
#include <Adafruit_MQTT.h>
#include <SPI.h>
#include <Wire.h>

#include "Adafruit_MQTT/Adafruit_MQTT.h" 
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h" 
#include "Adafruit_MQTT/Adafruit_MQTT.h" 
#include "Adafruit_BME280.h"
#include "Adafruit_Sensor.h"
#include "Grove_Air_quality_Sensor.h"

#include "Adafruit_SSD1306.h"
#include "credentials.h"
#include "math.h"

void setup();
void loop();
void MQTT_connect();
void hydrate();
void OLEDDisplay();
void printValues();
#line 24 "c:/Users/kareem/Documents/IOT/SmartPlantWateringSystem/l14_moisture-kareemcru/L14_04_PlantWater/src/L14_04_PlantWater.ino"
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    D4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


//Global State
TCPClient TheClient;

//Adafruit BME class
Adafruit_BME280 bme;

//Grove Air Quality Sensor class
AirQualitySensor sensor(A2);

//Adafruit display class
Adafruit_SSD1306 display(OLED_RESET);

//MQTT client class
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

//Feeds
Adafruit_MQTT_Subscribe mqttSub1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/manuelButton"); 
Adafruit_MQTT_Publish Zap = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/ZapWater");
Adafruit_MQTT_Publish Moist = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SoilMoisture");
Adafruit_MQTT_Publish Pressure = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Pressure");
Adafruit_MQTT_Publish Humid = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Humidity");
Adafruit_MQTT_Publish Temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature");
Adafruit_MQTT_Publish Air = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/AirQuality");
Adafruit_MQTT_Publish Dust = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/DustSensor");

//Variables
const int VALUE1 = 620;
const int VALUE2 = 310;
unsigned long lastMQTT;
unsigned long lastPub;
unsigned long dustTime;
unsigned long duration;
unsigned long waterTime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
int dustSensor = A0;
int soilSensor = A1;
int relay = A4;
int soilMoisturePercent; 
int soilMoistureValue; 
int current_quality =-1;
float dustSense;
float airSense;
float tempC;
float humidRH;
float pressPA;
float tempF;
float inHg;
float ratio = 0;
float concentration = 0;
bool buttonState;

SYSTEM_MODE(SEMI_AUTOMATIC);

// setup() runs once, when the device is first turned on.
void setup() 
{
  // Put initialization like pinMode and begin functions here.

  pinMode(soilSensor, INPUT);
  pinMode(dustSensor, INPUT);
  pinMode(relay, OUTPUT);

  Serial.begin(9600);
  bool status;
  status = bme.begin(0x76);
  sensor.init();
  dustTime = millis();
  delay(100); //wait for Serial Monitor to startup

  Serial.printf("Connecting to Internet \n");
  WiFi.connect();
  while(WiFi.connecting()) 
  {
    Serial.printf(".");
    delay(100);
  }
  Serial.printf("\n Connected!!!!!! \n");
  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&mqttSub1);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(2000);
  display.clearDisplay();
}

void loop()
 {

  current_quality=sensor.slope();
  if (current_quality >= 0)// if a valid data returned.
  {
      if (current_quality==0)
          Serial.println("High pollution! Force signal active");
      else if (current_quality==1)
          Serial.println("High pollution!");
      else if (current_quality==2)
          Serial.println("Low pollution!");
      else if (current_quality ==3)
          Serial.println("Fresh air");    
  }

  duration = pulseIn(dustSensor, LOW);
  lowpulseoccupancy = lowpulseoccupancy + duration;
  if ((millis()-dustTime) >= 30000){
    ratio = lowpulseoccupancy/(30000*10.0);
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62;
    if(concentration > 1){
      dustSense = concentration;
    }
    Serial.printf("Concentration = %f pcs/0.01cf",dustSense);
    lowpulseoccupancy = 0;
    dustTime = millis();
  }
 



  // this is our 'wait for incoming subscription packets' busy subloop
 
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000)))
  {
    if (subscription == &mqttSub1) 
    {
      buttonState = atoi((char *)mqttSub1.lastread);
      Serial.printf("buttonState is %i\n", buttonState);
    }   
  }

  if (buttonState == 1)
  {
      digitalWrite(relay,HIGH);
      Serial.printf("buttonState is on \n");
  }
    else
    {
      digitalWrite(relay,LOW);
      Serial.printf("buttonState is off \n");
    }
  soilMoistureValue = analogRead(soilSensor);
  Serial.printf("Soil moisture is %i\n", soilMoistureValue);
  soilMoisturePercent = map(soilMoistureValue, 1800, 3500, 100, 0);

  if((millis()-lastPub > 30000)) 
  {
    if(mqtt.Update()) 
    {
      Moist.publish(soilMoisturePercent);
      Air.publish(airSense);
      Temp.publish(tempF);
      Humid.publish(humidRH);
      Dust.publish(dustSense);
      Pressure.publish(inHg);
      Zap.publish(soilMoisturePercent);
 
    } 
    lastPub = millis();
  }


  tempC = bme.readTemperature();
  tempF = map(tempC,0.0,100.0,32.0,212.0);
  pressPA = bme.readPressure();
  inHg = pressPA/3386.389;
  humidRH = bme.readHumidity();
  airSense = sensor.getValue();
  
  MQTT_connect();
  hydrate();
  OLEDDisplay();
  printValues();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() 
{
  int8_t ret;
 
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
    // Ping MQTT Broker every 2 minutes to keep connection alive
  if ((millis()-lastMQTT)>120000) 
  {
      Serial.printf("Pinging MQTT \n");
      if(! mqtt.ping()) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      lastMQTT = millis();
  }
}
void hydrate()
{
  if(soilMoisturePercent <= 45)
  {
    if((millis()-waterTime)>10000)
    {
      digitalWrite(relay,HIGH);
      delay(500);
      digitalWrite(relay,LOW);
      waterTime = millis();
    }
  }

}
void OLEDDisplay()
{      
    display.clearDisplay();
    display.setCursor(0,0);  //oled display
    display.setRotation(2);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.printf("Temp: %0.2f\nHumidity: %0.2f\nDust %0.2f\nPressure: %0.2f\n", tempF, humidRH, dustSense, inHg);
    display.printf("Air: %0.2f\nMoist: %i %%\n", airSense, soilMoisturePercent);
    display.display();
}
void printValues() 
{
    Serial.printf("Temperature = %f\n", tempF);
    Serial.print(bme.readTemperature());
    Serial.println(" *F");
    
    Serial.printf("Pressure = %f\n", inHg);
    Serial.print(bme.readPressure());
    Serial.println(" hPa");

 
    Serial.printf("Humidity = %f\n", humidRH);
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}
