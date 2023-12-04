/* 
 * Project: Key fob for SafeChild Care Seat Monitor
 * Author: Jon Phillips
 * Last Updated: 30 November 2023
 */


#include "Particle.h"
#include <NeoPixel.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT/Adafruit_MQTT_SPARK.h>
#include <Adafruit_MQTT/Adafruit_MQTT.h>
#include <credentials.h>
#include <math.h>
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

void bleConnect();
void onDataReceived(const uint8_t *data, size_t len, const BlePeerDevice &peer, void *context);
void MQTT_connect();
bool MQTT_ping();
void publish();
void alert();
const int PIXELCOUNT = 1;
bool alertOn;


Adafruit_NeoPixel pixel ( PIXELCOUNT, SPI1 , WS2812B );


const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
const size_t SCAN_RESULT_MAX = 20;


BleScanResult scanResults[SCAN_RESULT_MAX];
BleCharacteristic peerRxCharacteristic;
BleCharacteristic peerTxCharacteristic;
BlePeerDevice peer;
BleCharacteristic rxCharacteristic ("rx",BleCharacteristicProperty::WRITE_WO_RSP,rxUuid,serviceUuid,onDataReceived,NULL);
uint8_t i;

TCPClient TheClient; 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 
Adafruit_MQTT_Publish pubFeedLt = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SafeChildLight");
Adafruit_MQTT_Publish pubFeedTxt = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SafeChildText");
Adafruit_MQTT_Publish pubFeedFR = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SafeChildFobRange");
const int BUZZER = D16;
const int VIBRATE1 = D1;
const int VIBRATE2 = D6;
int count;
int RSSI;
int BuckleCheck;
bool lastBuckleCheck;
int timer;
int sinWave;
int lastTime;
float t;
Thread thread("ALERT",alert);

void setup() {
    Serial.begin(9600);
    waitFor(Serial.isConnected, 15000);
    BLE.on();
    peerTxCharacteristic.onDataReceived(onDataReceived, &peerTxCharacteristic);

    pixel.begin();
    pixel.show();
    pixel.setBrightness(100);
    pinMode (BUZZER,OUTPUT);

    WiFi.on();
    WiFi.connect();
    while(WiFi.connecting()) {
    Serial.printf(".");
    pinMode(VIBRATE1,OUTPUT);
    pinMode(VIBRATE2,OUTPUT);
    }

}


void loop() {   
    MQTT_connect();
    MQTT_ping();
    bleConnect();

       
                while(peer.connected()){
                
                    pixel.setBrightness(40);
                    pixel.setPixelColor(0,0x0000ff);
                    pixel.show();
                    peer.getCharacteristicByUUID(peerRxCharacteristic, rxUuid);
                    peer.getCharacteristicByUUID(peerTxCharacteristic, txUuid); 
                    publish();    
                    alertOn=false;               
                }

                while(!peer.connected()){
                   
                    if(lastBuckleCheck){
                        pixel.setBrightness(100);
                        pixel.setPixelColor(0,0xff0000);
                        pixel.show();                      
                        alertOn=true;
                        // alert();                      
                        pixel.clear();
                        pixel.show();
                        publish();
                        
                    }
                    if(!lastBuckleCheck){
                        pixel.setBrightness(40);
                        pixel.setPixelColor(0,0x00ff00);
                        pixel.show();
                        publish();
                    }
                   
                    bleConnect();
               }
   
   
 
         

   Serial.printf("lastBuckleCheck------ %i\n",lastBuckleCheck);
 
}

void onDataReceived(const uint8_t *data, size_t len, const BlePeerDevice &peer, void *context){
 




    if(peer.address()[0]==0x05){

    // Serial.printf("Received data from :%02X :%02X :%02X :%02X :%02X :%02X\n", peer.address()[0], peer.address()[1], peer.address()[2], peer.address()[3], peer.address()[4], peer.address()[5]);
    // Serial.printf("incoming %i\n",atoi((char*)data));
    Serial.printf("RSSI ---- %i\n",RSSI);
    BuckleCheck = atoi((char*)data);


       
        if(BuckleCheck==1){
            pixel.setPixelColor(0,0x0000ff);
            lastBuckleCheck=true;
            }

        if(BuckleCheck==0){
            pixel.setPixelColor(0,0x0000ff);
            pixel.show();
            lastBuckleCheck=false;
        }

        Serial.printf("BuckleCheck ----  %i\n",BuckleCheck);
    }
}

void bleConnect(){
 count = BLE.scan(scanResults, SCAN_RESULT_MAX);
    if(count > 0){
        for( i = 0; i < count; i++){
        BleUuid foundServiceUuid;
        size_t svcCount = scanResults[i].advertisingData().serviceUUID(&foundServiceUuid,1);
            if(svcCount > 0 && foundServiceUuid == txUuid){
                RSSI = scanResults[i].rssi();
              
                peer = BLE.connect(scanResults[i].address());        
            }
        }
    }
}

void MQTT_connect() {
  int8_t ret;
 
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}

void publish(){
    static int counter;


     if((millis()-lastTime) > 8000) {
        lastTime = millis();
            if(mqtt.Update()) {
           
            // Serial.printf("Publishing ---- %i \n",lastBuckleCheck); 
           
            
            if(lastBuckleCheck && peer.connected()){
                pubFeedTxt.publish("Car seat in range, Seat Belt Locked");                
                pubFeedFR.publish(0);
                pubFeedLt.publish(0);
            }
            if(!lastBuckleCheck && peer.connected()){
                pubFeedTxt.publish("Car seat in range,Seat Belt not locked");                
                pubFeedFR.publish(0);
                pubFeedLt.publish(0);
            }   
            if(!peer.connected() && !lastBuckleCheck){
                pubFeedTxt.publish("Car seat out of range, Seat belt not locked");
                pubFeedFR.publish(1);
                pubFeedLt.publish(0);
            }
            if(!peer.connected() && lastBuckleCheck){
                pubFeedTxt.publish("DANGER!!! IS YOUR CHILD SAFE???");                
                pubFeedFR.publish(0);
                pubFeedLt.publish(1);
            }

            
        }
     }


}

// void alert(){
//     static int i;
    
//     for(i=0;i<20;i++){
//         t = millis()/1000.0;
//         sinWave = 500*sin(2*M_PI*(2)*t)+3000;
//         tone(BUZZER,sinWave,500);
//         delay(90);
//         noTone(BUZZER);
//         digitalWrite(VIBRATE1,HIGH);
//         digitalWrite(VIBRATE2,HIGH);          
//     }
//     digitalWrite(VIBRATE1,LOW);
//     digitalWrite(VIBRATE2,LOW);
// }

void alert(){
    static int i;
    
    while(true){
    if(alertOn){
        t = millis()/1000.0;
        sinWave = 500*sin(2*M_PI*(2)*t)+3000;
        tone(BUZZER,sinWave,500);
        delay(90);
        noTone(BUZZER);
        digitalWrite(VIBRATE1,HIGH);
        digitalWrite(VIBRATE2,HIGH);          
    }
    else{
    digitalWrite(VIBRATE1,LOW);
    digitalWrite(VIBRATE2,LOW);
    }
    }
    
}
