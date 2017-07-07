Replacement USB stack for chipKIT
--------------------------------

* This is ***WORK IN PROGRESS***

Usage
-----

First create objects for the USB interface and manager:

    #include <USB.h>

    USBFS usbDriver;
    USBManager USB(usbDriver, 0xDEAD, 0xBEEF); // Provide the VID and PID here

Then create objects for each device you want:

    CDCACM usbSerialPort;
    HID_Keyboard Keyboard;
    HID_Mouse Mouse;
    HID_Joystick Joystick;
    HID_Raw HID;

You can have as many of each device as you like, endpoints and heap permitting.

Next add the devices to the manager and start the USB system:

    void setup() {
        USB.addDevice(&usbSerialPort);
        USB.addDevice(&Keyboard);
        USB.addDevice(&Mouse);
        USB.addDevice(&Joystick);
        USB.addDevice(&HID);

        USB.begin();
    }

All devices are "begun" automatically (no xxx.begin(...) functions).

Most devices adhere to the Arduino interface (Mouse.click(), Keyboard.press(), etc) where such an interface exists.
