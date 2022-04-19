#include <DHT.h>
#include <WiFi.h>
#include "HomeSpan.h"

DHT *globDht = NULL;

struct DEV_Identify : Service::AccessoryInformation {
  SpanCharacteristic *identify;
  
  DEV_Identify(const char *name, const char *manu, const char *sn, const char *model, const char *version) : Service::AccessoryInformation(){
    new Characteristic::Name(name);
    new Characteristic::Manufacturer(manu);
    new Characteristic::SerialNumber(sn);    
    new Characteristic::Model(model);
    new Characteristic::FirmwareRevision(version);
    identify = new Characteristic::Identify();
  }
  boolean update(){
    return true;
  }
};

struct DEV_TempSensor : Service::TemperatureSensor {
  SpanCharacteristic *temp;
  uint32_t timer = 0;
  
  DEV_TempSensor() : Service::TemperatureSensor(){
    double tempC = (double)globDht->readTemperature();
    temp = new Characteristic::CurrentTemperature(tempC);
  }

  void loop(){
    if(millis()- timer > 5000){
      timer = millis();
      double tempC = (double)globDht->readTemperature();
      if(abs(temp->getVal<double>() - tempC) > 0.1){
        temp->setVal(tempC);
      }
    }
  }
};

struct DEV_HumidSensor : Service::HumiditySensor {
  SpanCharacteristic *humid;
  uint32_t timer = 0;

  DEV_HumidSensor() : Service::HumiditySensor() { 
    double humidC = (double)globDht->readHumidity();
    humid = new Characteristic::CurrentRelativeHumidity(humidC);
  }
  
  void loop(){
    if(millis()- timer > 5000){
      timer = millis();
      double humidC = (double)globDht->readHumidity();
      if(abs(humid->getVal<double>() - humidC) > 1){
        humid->setVal(humidC);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  globDht = new DHT(15, DHT11);
  homeSpan.begin(Category::Sensors, "Environmental Sensor");
  
  new SpanAccessory();                                                          
  new DEV_Identify("Temperature Sensor", "paull", "000000001", "Sensor", "1.0");
  new DEV_TempSensor();
  new Service::HAPProtocolInformation();
  
  new SpanAccessory();
  new DEV_Identify("Humidity Sensor", "paull", "0000000002", "Sensor", "1.0");
  new DEV_HumidSensor();      
  new Service::HAPProtocolInformation();                                           
}

void loop(){
  homeSpan.poll();
}
