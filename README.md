Replacement USB stack for chipKIT
--------------------------------

* This is ***WORK IN PROGRESS***

Usage
-----

First create objects for the USB interface and manager:

```C++
#include <USB.h>

USBFS usbDriver;
USBManager USB(usbDriver, 0xDEAD, 0xBEEF); // Provide the VID and PID here
```

You can also provide manufacturer, product and serial number (must be all three
at the moment):

```C++
USBManager USB(usbDriver, 0xDEAD, 0xBEEF, "My Company", "My Board", "12345ABCXYZ"); 
```

Then create objects for each device you want:

```C++
CDCACM usbSerialPort;
HID_Keyboard Keyboard;
HID_Mouse Mouse;
HID_Joystick Joystick;
HID_Raw HID;
Audio_MIDI MIDI;
```

You can have as many of each device as you like, endpoints and heap permitting.

Next add the devices to the manager and start the USB system:

```C++
void setup() {
    USB.addDevice(&usbSerialPort);
    USB.addDevice(&Keyboard);
    USB.addDevice(&Mouse);
    USB.addDevice(&Joystick);
    USB.addDevice(&HID);
    USB.addDevice(&MIDI);

    USB.begin();
}
```

The order you add the devices is the order they appear in the configuration descriptor and thus the interface number(s) that get
assigned to them.

All devices are "begun" automatically (no xxx.begin(...) functions).

Most devices adhere to the Arduino interface (Mouse.click(), Keyboard.press(), etc) where such an interface exists.

Windows
-------

In Windows 10 no drivers are needed. The Class Compliant drivers handle all the devices. In earlier versions of Windows you will need a
driver (.inf file) that describes all the interfaces. At the moment it's up to you to craft one of those files (modify an existing
one from elsewhere).

API
---

* CDCACM

Inherits the Arduino `Stream` class, so uses the standard `print`, `write`, `read` etc.

* HID\_Keyboard:

Adheres to the Arduino Keyboard API:


```C++
Keyboard.print(...); etc,
Keyboard.press(key);
Keyboard.release(key);
Keyboard.releaseAll();
```

* HID\_Mouse:

Adheres to the Arduino Mouse API:

```C++
Mouse.click(button);
Mouse.press(button);
Mouse.release(button);
Mouse.move(dx, dy, dz);
```

* HID\_Joystick:

There is no standard for this API yet. If a standard emerges this class will
be adapted to match it.

The joystick has 6 axes and 16 buttons as well as a hat direction switch with
full 8-bit resolution.

```C++
Joystick.setX(x);
Joystick.setY(y);
Joystick.setZ(z);
Joystick.rotateX(x);
Joystick.rotateY(y);
Joystick.rotateZ(z);
Joystick.setHat(angle);
Joystick.press(button);
Joystick.release(button);
```

* HID\_Raw:

The API for this device hasn't yet been decided upon. It is still a work in progress.

* Audio\_MIDI:

There is no standard API for this device.

```C++
MIDI.noteOn(channel, note, velocity);
MIDI.noteOff(channel, note);
MIDI.onMidiMessage(callback):
    Callback: void callback(uint8_t status, uint8_t d0, uint8_t d1) { ... }
```
