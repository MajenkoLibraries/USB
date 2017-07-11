#include <USB.h>

USBFS usbDriver;
// Change to this instead for High Speed mode on MZ chips:
// USBHS usbDriver;

USBManager USB(usbDriver, 0x0403, 0xA662);
CDCACM uSerial1;
CDCACM uSerial2;
HID_Keyboard Keyboard;
HID_Mouse Mouse;

void setup() {
    USB.addDevice(uSerial1);
    USB.addDevice(uSerial2);
    USB.addDevice(Keyboard);
    USB.addDevice(Mouse);
    USB.begin();
}

void loop() {
    static uint32_t ts = millis();

    // Print a message periodically on each serial port
    if (millis() - ts >= 1000) {
        ts = millis();
        uSerial1.println("dude");
        uSerial2.println("sweet");
    }

    // Type anything sent through uSerial1 on the keyboard
    // CAUTION: This can cause feedback!
    if (uSerial1.available()) {
        Keyboard.print(uSerial1.read());
    }

    // Control the mouse on uSerial2 - WASD to move and Q/E to click
    if (uSerial2.available()) {
        char c = uSerial2.read();
        switch (c) {
            case 'w': Mouse.move(0, -4, 0); break;  // Up
            case 's': Mouse.move(0, 4, 0); break;   // Down
            case 'a': Mouse.move(-4, 0, 0); break;  // Left
            case 'd': Mouse.move(4, 0, 0); break;   // Right
            case 'q': Mouse.click(MOUSE_LEFT); break;   // Left click
            case 'e': Mouse.click(MOUSE_RIGHT); break;  // Right click
        }
    }
}
