/*
 * Copyright (c) 2017, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Majenko Technologies nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _USB_H
#define _USB_H

#if (ARDUINO >= 100)
# include <Arduino.h>
#else
# include <WProgram.h>
#endif

#define USB_TX_TIMEOUT 75

struct bdt
{
        uint32_t flags;
        uint8_t *buffer;
} __attribute__((packed));  // 512 byte aligned in buffer

struct epBuffer {
	uint8_t *rx[2];
	uint8_t *tx[2];
	uint8_t data;
	uint8_t txAB;
    uint32_t size;
    uint32_t length;
    uint8_t *buffer;
    uint8_t *bufferPtr;
};

struct DeviceDescriptor {
    uint8_t     bLength;
    uint8_t     bDescriptorType;
    uint16_t    bcdUSB;
    uint8_t     bDeviceClass;
    uint8_t     bDeviceSubClass;
    uint8_t     bDeviceProtocol;
    uint8_t     bMaxPacketSize;
    uint16_t    idVendor;
    uint16_t    idProduct;
    uint16_t    bcdDevice;
    uint8_t     iManufacturer;
    uint8_t     iProduct;
    uint8_t     iSerialNumber;
    uint8_t     bNumConfigurations;
} __attribute__((packed));

struct ConfigurationDescriptor {
    uint8_t     bLength;
    uint8_t     bDescriptorType;
    uint16_t    wTotalLength;
    uint8_t     bNumInterfaces;
    uint8_t     bConfigurationValue;
    uint8_t     iConfiguration;
    uint8_t     bmAttributes;
    uint8_t     bMaxPower;
} __attribute__((packed));

struct InterfaceDescriptor {
    uint8_t     bLength;
    uint8_t     bDescriptorType;
    uint8_t     bInterfaceNumber;
    uint8_t     bAlternateSetting;
    uint8_t     bNumEndpoints;
    uint8_t     bInterfaceClass;
    uint8_t     bInterfaceSubClass;
    uint8_t     bInterfaceProtocol;
    uint8_t     iInterface;
} __attribute__((packed));

struct StringDescriptorHeader {
    uint8_t     bLength;
    uint8_t     bDescriptorType;
    uint16_t    wLANGID;
} __attribute__((packed));

#define EP_IN 0
#define EP_OUT 1

#define EP_CTL 0
#define EP_INT 1
#define EP_BLK 2
#define EP_ISO 3

class USBManager;
class USBDevice;

class USBDriver {
	public:
		USBDriver() {}
		virtual bool enableUSB() = 0;       // Turn on and configure USB
		virtual bool disableUSB() = 0;      // Turn off USB
		virtual bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint32_t size, uint8_t *a, uint8_t *b) = 0;    // Add an endpoint
		virtual bool enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len) = 0;  // Queue a single packet on an endpoint
        virtual bool canEnqueuePacket(uint8_t ep) = 0;  // True if a packet can be enqueued
        virtual bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) = 0; // Send an entire buffer through and endpoint
		virtual bool setAddress(uint8_t address) = 0;   // Set the device address
        virtual void setManager(USBManager *mgr) {  // Add the manager object to this driver.
            _manager = mgr;
        }

        virtual bool isHighSpeed() = 0;

        USBManager *_manager;
};
#ifdef __PIC32MX__
class USBFS : public USBDriver {
	private:
		static __USER_ISR void _usbInterrupt() {
			_this->handleInterrupt();
		}
		static USBFS *_this;
		uint32_t _enabledEndpoints;
		struct epBuffer _endpointBuffers[16];

        uint8_t _ctlRxA[64];
        uint8_t _ctlRxB[64];
        uint8_t _ctlTxA[64];
        uint8_t _ctlTxB[64];

        volatile bool _inIsr;

	public:
		USBFS() : _enabledEndpoints(0), _inIsr(false) { _this = this; }
		bool enableUSB();
		bool disableUSB();
		bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint32_t size, uint8_t *a, uint8_t *b);
		bool enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len);
		bool setAddress(uint8_t address);
        bool canEnqueuePacket(uint8_t ep);
        bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len);

        bool isHighSpeed() { return false; }


		void handleInterrupt();

		__attribute__ ((aligned(512))) volatile struct bdt _bufferDescriptorTable[16][4];

        using USBDriver::_manager;

};
#endif

#ifdef __PIC32MZ__
class USBFS;
class USBHS : public USBDriver {
	protected:
		static __USER_ISR void _usbInterrupt() {
			_this->handleInterrupt();
		}
		static USBHS *_this;
        uint32_t _fifoOffset;
		uint32_t _enabledEndpoints;
		struct epBuffer _endpointBuffers[16];

        uint8_t _ctlRxA[64];
        uint8_t _ctlRxB[64];
        uint8_t _ctlTxA[64];
        uint8_t _ctlTxB[64];

        volatile bool _inIsr;

	public:
		USBHS() : _fifoOffset(8), _enabledEndpoints(0), _inIsr(false) { _this = this; }
		virtual bool enableUSB();
        virtual bool isHighSpeed() { return true; }
		bool disableUSB();
		bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint32_t size, uint8_t *a, uint8_t *b);
		bool enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len);
		bool setAddress(uint8_t address);
        bool canEnqueuePacket(uint8_t ep);
        bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len);


		void handleInterrupt();

        using USBDriver::_manager;

        friend USBFS;

};

class USBFS : public USBHS {
    public:
        bool enableUSB();
        bool isHighSpeed() { return false; }
};
#endif

struct USBDeviceList {
    USBDevice *device;
    struct USBDeviceList *next;
};

class USBManager {
	protected:
		// Private functions and variables here
        USBDriver *_driver;
        uint8_t _wantedAddress;
        USBDeviceList *_devices;
        uint16_t _vid;
        uint16_t _pid;
        uint8_t _ifCount;
        uint8_t _epCount;
        uint8_t _target;

        const char *_manufacturer;
        const char *_product;
        const char *_serial;
        char _defSerial[14];

        void populateDefaultSerial();

	public:
        void onSetupPacket(uint8_t ep, uint8_t *data, uint32_t l);
        void onInPacket(uint8_t ep, uint8_t *data, uint32_t l);
        void onOutPacket(uint8_t ep, uint8_t *data, uint32_t l);

        bool isHighSpeed() { return _driver->isHighSpeed(); }

		USBManager(USBDriver *driver, uint16_t vid, uint16_t pid, const char *mfg, const char *prod, const char *ser = NULL);
		USBManager(USBDriver &driver, uint16_t vid, uint16_t pid, const char *mfg, const char *prod, const char *ser = NULL);
		USBManager(USBDriver *driver, uint16_t vid, uint16_t pid);
		USBManager(USBDriver &driver, uint16_t vid, uint16_t pid);

        void begin();
        void addDevice(USBDevice *d);
        void addDevice(USBDevice &d) { addDevice(&d); }

        uint8_t allocateInterface();
        uint8_t allocateEndpoint();

        bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint32_t size, uint8_t *a, uint8_t *b) {
            return _driver->addEndpoint(id, direction, type, size, a, b);
        }

        bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) {
            return _driver->sendBuffer(ep, data, len);
        }

        bool canEnqueuePacket(uint8_t ep) {
            return _driver->canEnqueuePacket(ep);
        }

        void end() {}

};

class USBDevice {
    public:
        virtual uint16_t getDescriptorLength() = 0; // Returns the length of the config descriptor block
        virtual uint8_t getInterfaceCount() = 0;    // Returns the number of interfaces for this device
        virtual bool getStringDescriptor(uint8_t idx, uint16_t maxlen) = 0; // Send a string descriptor matching this index
        virtual uint32_t populateConfigurationDescriptor(uint8_t *buf) = 0; // Fills *buf with the config descriptor data. Returns the number of bytes used
        virtual void initDevice(USBManager *manager) = 0;   // Initialize a device and set the manager object
        
        virtual bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) = 0; // Called when GET_DESCRIPTOR arrives
        virtual bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) = 0;   // called when GET_REPORT_DESCRIPTOR arrives
        virtual void configureEndpoints() = 0;  // Configures all the endpoints for the device

        virtual bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) = 0;  // Called when a SETUP packet arrives
        virtual bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) = 0; // Called when an IN packet is requested
        virtual bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) = 0;    // Called when an OUT packet arrives
};

class CDCACM : public USBDevice, public Stream {
    private:
        USBManager *_manager;
        uint8_t _ifControl;
        uint8_t _ifBulk;
        uint8_t _epControl;
        uint8_t _epBulk;
        uint8_t _outAction;

        uint8_t _lineState;
        uint32_t _baud;
        uint8_t _stopBits;
        uint8_t _dataBits;
        uint8_t _parity;
#if defined (__PIC32MX__)
        uint8_t _txBuffer[64];
        uint8_t _rxBuffer[64];
        uint8_t _bulkRxA[64];
        uint8_t _bulkRxB[64];
        uint8_t _bulkTxA[64];
        uint8_t _bulkTxB[64];
#define CDCACM_BUFFER_SIZE 64
#elif defined(__PIC32MZ__)
        uint8_t _txBuffer[512];
        uint8_t _rxBuffer[512];
        uint8_t _bulkRxA[512];
        uint8_t _bulkRxB[512];
        uint8_t _bulkTxA[512];
        uint8_t _bulkTxB[512];
#define CDCACM_BUFFER_SIZE 512
#endif
        volatile uint32_t _txPos;
        volatile uint32_t _rxHead;
        volatile uint32_t _rxTail;

        uint8_t _ctlA[8];
        uint8_t _ctlB[8];

    public:
        CDCACM() : _txPos(0), _rxHead(0), _rxTail(0) {}

        operator int();
        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) { return false; }
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);

        size_t write(uint8_t);
        size_t write(const uint8_t *b, size_t len);

        int available();
        int read();
        int peek();
        void flush();
        void begin() {}
        void begin(uint32_t baud) {}
        void end() {}
};

struct KeyReport {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} __attribute__((packed));

class HID_Keyboard : public USBDevice, public Print {
    private:
        USBManager *_manager;
        uint8_t _ifInt;
        uint8_t _epInt;
        struct KeyReport _keyReport;
        void sendReport(struct KeyReport *keys);
        uint8_t _intA[8];
        uint8_t _intB[8];

    public:
        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        size_t write(uint8_t);

        size_t press(uint8_t key);
        size_t release(uint8_t key);
        void releaseAll();

        void begin(void) {};
        void end(void) {};
};

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

class HID_Media : public USBDevice {
    private:
        USBManager *_manager;
        uint8_t _ifInt;
        uint8_t _epInt;
        void sendReport(uint8_t id, uint16_t data);
        void sendReport(uint8_t id, uint32_t data);
        uint8_t _intA[8];
        uint8_t _intB[8];

        uint16_t _systemKeys;
        uint32_t _consumerKeys;

    public:
        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        size_t write(uint8_t);

        size_t pressSystem(uint16_t key);
        size_t releaseSystem(uint16_t key);
        size_t pressConsumer(uint32_t key);
        size_t releaseConsumer(uint32_t key);
        void releaseAllSystem();
        void releaseAllConsumer();
        void releaseAll();

        void begin(void) {};
        void end(void) {};
};

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_ALL (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)


class HID_Mouse : public USBDevice {
    private:
        USBManager *_manager;
        uint8_t _ifInt;
        uint8_t _epInt;
        void sendReport(const uint8_t *b, uint8_t l);
        uint8_t _buttons;
        void buttons(uint8_t b);
        uint8_t _intA[8];
        uint8_t _intB[8];

    public:
        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);

        HID_Mouse() : _buttons(0) {}
    
        void begin(void) {};
        void end(void) {};
        void click(uint8_t b = MOUSE_LEFT);
        void move(signed char x, signed char y, signed char wheel = 0);
        void press(uint8_t b = MOUSE_LEFT);     // press LEFT by default
        void release(uint8_t b = MOUSE_LEFT);   // release LEFT by default
        bool isPressed(uint8_t b = MOUSE_LEFT); // check LEFT by default
};

struct JoystickReport {
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t z;
    } position;
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t z;
    } rotation;
    uint8_t hat;
    uint16_t buttons;
} __attribute__((packed));

class HID_Joystick : public USBDevice {
    private:
        USBManager *_manager;
        uint8_t _ifInt;
        uint8_t _epInt;
        void sendReport(const uint8_t *b, uint8_t l);
        struct JoystickReport _rep;
        uint8_t _intA[16];
        uint8_t _intB[16];

    public:
        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);

        HID_Joystick(void);
        void begin(void) {};
        void end(void) {};
        void setX(uint8_t x);
        void setY(uint8_t y);
        void setZ(uint8_t z);
        void rotateX(uint8_t x);
        void rotateY(uint8_t y);
        void rotateZ(uint8_t z);
        void setPosition(uint8_t x, uint8_t y, uint8_t z);
        void setRotation(uint8_t x, uint8_t y, uint8_t z);
        void press(uint8_t b);
        void release(uint8_t b);
        void setHat(uint8_t d);
};

class HID_Raw : public USBDevice {
    private:
        USBManager *_manager;
        uint8_t _ifInt;
        uint8_t _epInt;
        uint8_t _intRxA[64];
        uint8_t _intRxB[64];
        uint8_t _intTxA[64];
        uint8_t _intTxB[64];
        bool _nextPacketIsMine;
        uint8_t _features[256];

    public:
        void sendReport(uint8_t *b, uint8_t l);
        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);

        void begin(void) {};
        void end(void) {};

        uint8_t getFeature(uint8_t f) { return _features[f]; }
        void setFeature(uint8_t f, uint8_t v) { _features[f] = v; }
};

class Audio_MIDI : public USBDevice {
    private:
        USBManager *_manager;
        uint8_t _ifBulk;
        uint8_t _epBulk;

#if defined (__PIC32MX__)
        uint8_t _bulkRxA[64];
        uint8_t _bulkRxB[64];
        uint8_t _bulkTxA[64];
        uint8_t _bulkTxB[64];
#elif defined(__PIC32MZ__)
        uint8_t _bulkRxA[512];
        uint8_t _bulkRxB[512];
        uint8_t _bulkTxA[512];
        uint8_t _bulkTxB[512];
#endif

        void (*_onMidiMessage)(uint8_t status, uint8_t d0, uint8_t d1);

    public:
        uint16_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getStringDescriptor(uint8_t idx, uint16_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);

        void begin(void) {};
        void end(void) {};

        bool sendMessage(uint8_t cable, uint8_t code, uint8_t b0, uint8_t b1, uint8_t b2);

        bool noteOn(uint8_t channel, uint8_t note, uint8_t velocity);
        bool noteOff(uint8_t channel, uint8_t note);
        void onMidiMessage(void (*func)(uint8_t status, uint8_t d0, uint8_t d1));

};

#endif
