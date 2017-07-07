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

The order you add the devices is the order they appear in the configuration descriptor and thus the interface number(s) that get
assigned to them.

All devices are "begun" automatically (no xxx.begin(...) functions).

Most devices adhere to the Arduino interface (Mouse.click(), Keyboard.press(), etc) where such an interface exists.

Windows
-------

In Windows 10 no drivers are needed. The Class Compliant drivers handle all the devices. In earlier versions of Windows you will need a
driver (.inf file) that describes all the interfaces. At the moment it's up to you to craft one of those files (modify an existing
one from elsewhere).
