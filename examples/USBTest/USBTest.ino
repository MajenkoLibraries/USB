#include <USB.h>
#include <wchar.h>

USBFS usbDriver;
USBManager USB(usbDriver, 0x0403, 0xA662);
CDCACM uSerial;

void setup() {
	Serial.begin(1000000);
	pinMode(PIN_LED1, OUTPUT);
    USB.addDevice(&uSerial);
    USB.begin();
}

void loop() {
    if (Serial.available()) {
        uSerial.write(Serial.read());
    }
    if (uSerial.available()) {
        Serial.write(uSerial.read());
    }
}
