#include <Wire.h>
#include <DFRobot_ADS1115.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>                 //to manage files on SD Card
#include "QuickMedianLib.h"

#define WATER_TEMP_SENSOR 6
#define SD_CHIP_SELECT    4       //Communication PIN SD Card
#define LOG_FILE "TURBCAL.csv"

File myFile;        
DFRobot_ADS1115 ads(&Wire);
OneWire oneWireWater(WATER_TEMP_SENSOR);
DallasTemperature sensorInWater(&oneWireWater);

float tOffsetA;
float tOffsetB;
float tOffsetC;
float wTemp;
unsigned long counter;
float voltageMeasurementArray[10];
float turbidity;
float voltageMedian;
void setup() {
  Serial.begin(9600);
  isCardMounted();
  counter = readFile("counter").toInt();
  tOffsetA = readFile("tOffsetA").toFloat();
  tOffsetB = readFile("tOffsetB").toFloat();
  tOffsetC = readFile("tOffsetC").toFloat();
  sensorInWater.begin();
  ads.setAddr_ADS1115(ADS1115_IIC_ADDRESS0);   // 0x48
  ads.setGain(eGAIN_TWOTHIRDS);   // 2/3x gain
  ads.setMode(eMODE_SINGLE);       
  ads.setRate(eRATE_860);          // 860 SPS for less noise
  ads.setOSMode(eOSMODE_SINGLE);   // Set to start a single-conversion
  ads.init();
}

void loop() {
  sensorInWater.requestTemperatures();
  wTemp = sensorInWater.getTempCByIndex(0);
      if (ads.checkADS1115())
    {
      for (int i = 0; i< 10; i++){
      voltageMeasurementArray[i] = ads.readVoltage(0);
      delay(1000);}
      voltageMedian = QuickMedian<float>::GetMedian(voltageMeasurementArray, sizeof(voltageMeasurementArray) / sizeof(float));
      turbidity = (tOffsetA*pow(voltageMedian, -3))+(tOffsetB*wTemp)+ tOffsetC;
      Serial.println("COUNT: "+
      String(counter)+" | WATER_TEMP: "+String(wTemp) + " | MEDIAN_VOLTAGE: " + String(voltageMedian) +" | TURBIDITY [NTU]: "+ String(turbidity));
      SaveData();
      counter += 1;
      ChangeParameter("counter", String(counter));
    }
    else
    {
      Serial.println("ADS1115 Disconnected!");
    }
}

void SaveData() { //save all data on the SD card
  String csvObject = "";
  if (!SD.exists(LOG_FILE)) {
    csvObject = "Measure_number[-],Water_temp[c],Sensor0_median_voltage[mV],Turbidity[NTU]\n";
  }
  myFile = SD.open(LOG_FILE, FILE_WRITE);
  csvObject = csvObject + String(counter) + "," + String(wTemp) + "," + String(voltageMedian) +","+ String(turbidity);
  if (myFile) {
    myFile.println(csvObject);
    myFile.close();
  }
}
void isCardMounted() {
  if (SD.begin(SD_CHIP_SELECT)) {
    return;
  }
  Serial.println("Checking SD card..");
  while (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    delay(1000);
  }
  Serial.println("SD Card Mounted");
}

String readFile(String nameFile) { //read the content of a file from the SD card
  String result;
  nameFile = nameFile + ".txt";
  if (SD.exists(nameFile)) {
    //    msg=msg+"file exists! ";
    myFile = SD.open(nameFile);
    result = "";
    while (myFile.available()) {
      char c = myFile.read();
      if ((c != '\n') && (c != '\r')) {
        result += c;
        delay(5);
      }
    }
  }
  else {
    result = "0";
    myFile = SD.open(nameFile, FILE_WRITE); {
      myFile.println(result);
      myFile.close();
    }
  }
  return result;
}
void ChangeParameter (String nameparameter , String value) { //all values should be given as string
  String name = nameparameter + ".txt";
  if (SD.exists(name)) {
    SD.remove(name);
  }
  myFile = SD.open(name, FILE_WRITE);
  if (myFile) {
    myFile.println(value);
    myFile.close();
  }
}
