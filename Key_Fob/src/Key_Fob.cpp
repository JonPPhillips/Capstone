/* 
 * Project: Key fob for SafeChild Care Seat Monitor
 * Author: Jon Phillips
 * Last Updated: 30 November 2023
 */


#include "Particle.h"
#include <NeoPixel.h>
SYSTEM_MODE(SEMI_AUTOMATIC);
const int PIXELCOUNT = 1;
Adafruit_NeoPixel pixel ( PIXELCOUNT, SPI1 , WS2812B );
int BUZZER = D16;
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

const size_t SCAN_RESULT_MAX = 20;
BleScanResult scanResults[SCAN_RESULT_MAX];

BleCharacteristic peerRxCharacteristic;
BleCharacteristic peerTxCharacteristic;
BlePeerDevice peer;
void bleConnect();
void onDataReceived(const uint8_t *data, size_t len, const BlePeerDevice &peer, void *context);
BleCharacteristic rxCharacteristic ("rx",BleCharacteristicProperty::WRITE_WO_RSP,rxUuid,serviceUuid,onDataReceived,NULL);
uint8_t i;
int count;
int RSSI;
int BuckleCheck;
bool lastBuckleCheck;
int timer;




void setup() {
    Serial.begin(9600);
    waitFor(Serial.isConnected, 15000);
    BLE.on();
    peerTxCharacteristic.onDataReceived(onDataReceived, &peerTxCharacteristic);

    pixel.begin();
    pixel.show();
    pixel.setBrightness(100);
    pinMode (BUZZER,OUTPUT);
   
 


}


void loop() {   

    bleConnect();

       
                while(peer.connected()){
                    pixel.setBrightness(40);
                    pixel.setPixelColor(0,0x0000ff);
                    pixel.show();
                    peer.getCharacteristicByUUID(peerRxCharacteristic, rxUuid);
                    peer.getCharacteristicByUUID(peerTxCharacteristic, txUuid);
                   
                    
                }

                while(!peer.connected()){
                    if(lastBuckleCheck){
                        pixel.setBrightness(100);
                        pixel.setPixelColor(0,0xff0000);
                        pixel.show();
                        tone(BUZZER,3300,500);
                        delay(250);
                        pixel.clear();
                        pixel.show();
                        delay(250);

                    }
                    if(!lastBuckleCheck){
                        pixel.setBrightness(40);
                        pixel.setPixelColor(0,0x00ff00);
                        pixel.show();
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
