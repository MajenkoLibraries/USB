#include <USB.h>
#include <wchar.h>

USBFS usbDriver;
USBManager USB(usbDriver, 0x0403, 0xA662);
CDCACM uSerial;
HID_Keyboard Keyboard;

void setup() {
	Serial.begin(1000000);
	pinMode(PIN_LED1, OUTPUT);
    USB.addDevice(&uSerial);
    USB.addDevice(&Keyboard);
    USB.begin();
    pinMode(PIN_BTN1, INPUT);
    pinMode(PIN_BTN2, INPUT);
}

void loop() {
    static uint8_t p1 = digitalRead(PIN_BTN1);
    if (digitalRead(PIN_BTN1) != p1) {
        p1 = digitalRead(PIN_BTN1);
        if (p1 == HIGH) {
            Keyboard.print("Hello");
        }
    }

    static uint8_t p2 = digitalRead(PIN_BTN2);
    if (digitalRead(PIN_BTN2) != p2) {
        p2 = digitalRead(PIN_BTN2);
        if (p2 == HIGH) {
            Keyboard.releaseAll();
        }
    }

    
    if (Serial.available()) {
        uSerial.write(Serial.read());
    }
    if (uSerial.available()) {
        Serial.write(uSerial.read());
    }
    
}
