/* 
 * Project Hall effect sensor for SafeChild Car Seat Monitor
 * Author: Jon Phillips
 * Last Updated: 30 November 2023
 */


#include "Particle.h"
#include <NeoPixel.h>


SYSTEM_MODE(SEMI_AUTOMATIC);
const int PIXELCOUNT = 1;
Adafruit_NeoPixel pixel(PIXELCOUNT, D2 , WS2812B );
const int DETECT = D8;
bool magPres;
bool redFlash;
int Timer;
int isBuckled;
const int UART_TX_BUF_SIZE = 2;
const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
byte txBuf[UART_TX_BUF_SIZE];




void setup() {
    pinMode(DETECT,INPUT);
    pixel.begin();
    pixel.show();
    pixel.setBrightness(30);

    Serial.begin(9600);
    waitFor(Serial.isConnected, 15000);
    BLE.on();
    BLE.setTxPower(-30);
    BLE.addCharacteristic(txCharacteristic);
    BleAdvertisingData data;
    data.appendServiceUUID(txUuid);
    BLE.advertise(&data);

    Serial.printf("SafeChild Hall Effect Sensor BLE Address: %s\n", BLE.address().toString().c_str());


}


void loop() {

    magPres = digitalRead(DETECT);

    if(!magPres){
        pixel.setPixelColor(0,0xff0000);
        pixel.show();
        pixel.clear();
        isBuckled = 1;

        // Serial.printf("magnet present\n");

    }

    if(magPres){
        pixel.clear();
        pixel.show();
        isBuckled =0;
        // Serial.printf("magnet NOT present\n");
    }

    if(BLE.connected()){
        sprintf((char*)txBuf, "%i",isBuckled);
        txCharacteristic.setValue(txBuf , UART_TX_BUF_SIZE );
        // Serial.printf("buffer message: %s\n",(char *)txBuf);
        delay(1000);
  }

}
