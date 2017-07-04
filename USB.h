#ifndef _USB_H
#define _USB_H

#if (ARDUINO >= 100)
# include <Arduino.h>
#else
# include <WProgram.h>
#endif

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
    uint8_t size;
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
#define EP_ISP 3

class USBManager;
class USBDevice;

class USBDriver {
	public:
		USBDriver() {}
		virtual bool enableUSB() = 0;
		virtual bool disableUSB() = 0;
		virtual bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint8_t size) = 0;
		virtual bool enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len) = 0;
        virtual bool canEnqueuePacket(uint8_t ep) = 0;
        virtual bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) = 0;
		virtual bool setAddress(uint8_t address) = 0;
        virtual void setManager(USBManager *mgr) {
            _manager = mgr;
        }

        USBManager *_manager;
};

class USBFS : public USBDriver {
	private:
		static __USER_ISR void _usbInterrupt() {
			_this->handleInterrupt();
		}
		static USBFS *_this;
		uint32_t _enabledEndpoints;
		struct epBuffer _endpointBuffers[16];
	public:
		USBFS() : _enabledEndpoints(0) { _this = this; }
		bool enableUSB();
		bool disableUSB();
		bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint8_t size);
		bool enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len);
		bool setAddress(uint8_t address);
        bool canEnqueuePacket(uint8_t ep);
        bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len);


        char debugLog[10][80];
        void initDebug();
        void updateDebug();
        void log(const char *);

		void handleInterrupt();

		__attribute__ ((aligned(512))) volatile struct bdt _bufferDescriptorTable[16][4];

        using USBDriver::_manager;

};

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

	public:
        void onSetupPacket(uint8_t ep, uint8_t *data, uint32_t l);
        void onInPacket(uint8_t ep, uint8_t *data, uint32_t l);
        void onOutPacket(uint8_t ep, uint8_t *data, uint32_t l);

		USBManager(USBDriver *driver, uint16_t vid, uint16_t pid);
		USBManager(USBDriver &driver, uint16_t vid, uint16_t pid);

        void begin();
        void addDevice(USBDevice *d);

        uint8_t allocateInterface();
        uint8_t allocateEndpoint();

        bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint8_t size) {
            return _driver->addEndpoint(id, direction, type, size);
        }

        bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) {
            return _driver->sendBuffer(ep, data, len);
        }

        bool canEnqueuePacket(uint8_t ep) {
            return _driver->canEnqueuePacket(ep);
        }

};

class USBDevice {
    public:
        virtual uint8_t getDescriptorLength() = 0;
        virtual uint8_t getInterfaceCount() = 0;
        virtual uint32_t populateConfigurationDescriptor(uint8_t *buf) = 0;
        virtual void initDevice(USBManager *manager) = 0;

        virtual bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) = 0;
        virtual bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) = 0;
        virtual void configureEndpoints() = 0;

        virtual bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) = 0;
        virtual bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) = 0;
        virtual bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) = 0;
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
        uint8_t _txBuffer[64];
        volatile uint8_t _txPos;
        uint8_t _rxBuffer[64];
        volatile uint8_t _rxHead;
        volatile uint8_t _rxTail;

    public:
        CDCACM() : _txPos(0), _rxHead(0), _rxTail(0) {}

        operator int();
        uint8_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) { return false; }
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);

        size_t write(uint8_t);
        int available();
        int read();
        int peek();
        void flush();
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

    public:
        uint8_t getDescriptorLength();
        uint8_t getInterfaceCount();
        uint32_t populateConfigurationDescriptor(uint8_t *buf);
        void initDevice(USBManager *manager);
        bool getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        bool getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen);
        void configureEndpoints();

        bool onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        bool onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l);
        size_t write(uint8_t);

        size_t press(uint8_t key);
        size_t release(uint8_t key);
        void releaseAll();
};
    

#endif
