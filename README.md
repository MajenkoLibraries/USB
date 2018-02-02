IMPORTANT
---------

**THIS LIBRARY IS DEPRECATED. IT IS NOW OUT OF DATE. IT HAS BEEN INTEGRATED WITH THE CHIPKIT CORE ITSELF TO BE RELEASED IN VERSION 2.0.0. BUGFIXES AND IMPROVEMENTS WILL NOT BE ADDED TO THIS LIBRARY.**




Replacement USB stack for chipKIT
--------------------------------

This is a replacement for the internal chipKIT USB stack. It supports
both the PIC32MX and the PIC32MZ (the latter in both HS and FS mode).

The design is completely modular with a central USBManager object which
manages the enumeration and communication between USB devices and a
USB hardware interface.

Usage
-----

First create objects for the USB interface and manager:

```C++
#include <USB.h>

USBFS usbDriver;
USBManager USB(usbDriver, 0xDEAD, 0xBEEF); // Provide the VID and PID here
```

Or if you want to use High Speed mode on a PIC32MZ chip change the driver to:

```C++
USBHS usbDriver;
```

You can also provide manufacturer, product and serial number (serial number is
optional and if omitted will be generated from chip information):

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
    USB.addDevice(usbSerialPort);
    USB.addDevice(Keyboard);
    USB.addDevice(Mouse);
    USB.addDevice(Joystick);
    USB.addDevice(HID);
    USB.addDevice(MIDI);

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

* HID\_Media:

Media keys are split into two types: "System" and "Consumer". System ones control the
system - things like power down, etc. Consumer are for media player control - volume,
track skip, play, pause, etc.

```C++
Media.pressSystem(key);
Media.releaseSystem(key);
Media.pressConsumer(key);
Media.releaseConsumer(key);
Media.releaseAllSystem();
Media.releaseAllConsumer();
Media.releaseAll();
```

System key definitions:

```C++
#define SYSTEM_POWERDOWN            0x0001
#define SYSTEM_SLEEP                0x0002
#define SYSTEM_WAKEUP               0x0004
#define SYSTEM_CONTEXT_MENU         0x0008
#define SYSTEM_MAIN_MENU            0x0010
#define SYSTEM_APP_MENU             0x0020
#define SYSTEM_HELP_MENU            0x0040
#define SYSTEM_MENU_EXIT            0x0080
#define SYSTEM_MENU_SELECT          0x0100
#define SYSTEM_MENU_RIGHT           0x0200
#define SYSTEM_MENU_LEFT            0x0400
#define SYSTEM_MENU_UP              0x0800
#define SYSTEM_MENU_DOWN            0x1000
```

Consumer key definitions:

```C++
#define CONSUMER_PLAY               0x000001
#define CONSUMER_PAUSE              0x000002
#define CONSUMER_RECORD             0x000004
#define CONSUMER_FASTFORWARD        0x000008
#define CONSUMER_REWIND             0x000010
#define CONSUMER_NEXTTRACK          0x000020
#define CONSUMER_PREVTRACK          0x000040
#define CONSUMER_STOP               0x000080
#define CONSUMER_EJECT              0x000100
#define CONSUMER_MUTE               0x000200
#define CONSUMER_VOLUME_UP          0x000400
#define CONSUMER_VOLUME_DOWN        0x000800
#define CONSUMER_MENU_ESCAPE        0x001000
#define CONSUMER_MENU_RIGHT         0x002000
#define CONSUMER_MENU_LEFT          0x004000
#define CONSUMER_MENU_DOWN          0x008000
#define CONSUMER_MENU_UP            0x010000
#define CONSUMER_MENU_PICK          0x020000
#define CONSUMER_MENU               0x040000
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
