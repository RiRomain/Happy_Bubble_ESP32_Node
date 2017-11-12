#include <WiFi.h>
#include <PubSubClient.h>
#include "Settings.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ArduinoJson.h>

int scanDuration = 10; //In seconds

#define base_topic "happy-bubbles/ble/" room "/raw/"

WiFiClient espClient;
PubSubClient client(espClient);

void printDeviceInfo(BLEAdvertisedDevice advertisedDevice) {
  Serial.print("---------------- Found new device  ------------------");
  Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

  Serial.print("Address          :"); Serial.println(advertisedDevice.getAddress().toString().c_str());

  if (advertisedDevice.haveRSSI()) {
    Serial.print("RSSI             : "); Serial.println(advertisedDevice.getRSSI());
  }
  if (advertisedDevice.haveTXPower()) {
    Serial.print("TX Power         : "); Serial.println(advertisedDevice.getTXPower());
  }
  if (advertisedDevice.haveName()) {
    Serial.print("Device name      : "); Serial.println(advertisedDevice.getName().c_str());
  }
  if (advertisedDevice.haveManufacturerData()) {
    Serial.print("ManufacturerData : "); Serial.println(advertisedDevice.getManufacturerData().c_str());
  }
  if (advertisedDevice.haveServiceUUID()) {
    Serial.print("ServiceUUID      : "); Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
  }
}

void sendToMqtt(BLEAdvertisedDevice advertisedDevice) {
  int rssi = advertisedDevice.getRSSI();
  BLEAddress address = advertisedDevice.getAddress();
  String macTransformed = String(address.toString().c_str());
  //cc : 40 : 80 : ba   : 9f    :  7a
  //01 2 34 5 67 8 910 11 1213 14 1516
  macTransformed.remove(14, 1);
  macTransformed.remove(11, 1);
  macTransformed.remove(8, 1);
  macTransformed.remove(5, 1);
  macTransformed.remove(2, 1);

  /*
    {
    "hostname": "living-room",
    "mac": "001060AA36F8",
    "rssi": -94,
    "is_scan_response": "0",
    "type": "3",
    "data": "0201061aff4c000215e2c56db5dffb48d2b060d0f5a71096e000680068c5"
    }
  */
  StaticJsonBuffer<500> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["hostname"] = "living-room";
  JSONencoder["mac"] = macTransformed.c_str();
  JSONencoder["rssi"] = rssi;
  JSONencoder["is_scan_response"] = "0";
  JSONencoder["type"] = "3";
  JSONencoder["data"] = "";//"0201061aff4c000215e2c56db5dffb48d2b060d0f5a71096e000680068c5";


  char JSONmessageBuffer[500];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  //happy-bubbles/ble/<the-hostname>/raw/<the-bluetooth-MAC-address
  String publishTopic = String(base_topic) + macTransformed.c_str();
  if (client.publish(publishTopic.c_str(), JSONmessageBuffer) == true) { //TODO base_topic + mac_address
    Serial.print("Success sending message to topic: "); Serial.println(publishTopic);
    Serial.print("Message: "); Serial.println(JSONmessageBuffer);
  } else {
    Serial.print("Error sending message: "); Serial.println(publishTopic);
    Serial.print("Message: "); Serial.println(JSONmessageBuffer);
  }
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      printDeviceInfo(advertisedDevice);
      sendToMqtt(advertisedDevice);
    }
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(base_topic);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {

      Serial.println("connected");

    } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  if (client.publish("esp/test", "Hello from ESP32") == true) { //TODO base_topic + mac_address
    Serial.println("Success sending message to topic");
  } else {
    Serial.println("Error sending message");
  }
  BLEDevice::init("");
  Serial.println("Finished setup");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  Serial.println("New loop");
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  BLEScanResults foundDevices = pBLEScan->start(scanDuration);
  Serial.println("");
  Serial.print("Total devices found: "); Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");

}

