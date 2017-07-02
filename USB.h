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
    uint32_t length;
    const uint8_t *buffer;
};

#define EP_IN 0
#define EP_OUT 1

#define EP_CTL 0
#define EP_INT 1
#define EP_BLK 2
#define EP_ISP 3

class USBDriver {
	public:
		USBDriver() {}
		virtual bool enableUSB() = 0;
		virtual bool disableUSB() = 0;
		virtual bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type) = 0;
		virtual bool enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len) = 0;
        virtual bool canEnqueuePacket(uint8_t ep) = 0;
        virtual bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) = 0;
		virtual bool setAddress(uint8_t address) = 0;
		virtual bool onInPacket(void (*func)(uint8_t ep, uint8_t *data, uint32_t len)) {
			_onInPacket = func;
		}
		virtual bool onOutPacket(void (*func)(uint8_t ep, uint8_t *data, uint32_t len)) {
			_onOutPacket = func;
		}
		virtual bool onSetupPacket(void (*func)(uint8_t ep, uint8_t *data, uint32_t len)) {
			_onSetupPacket = func;
		}

		void (*_onInPacket)(uint8_t, uint8_t *, uint32_t);
		void (*_onOutPacket)(uint8_t, uint8_t *, uint32_t);
		void (*_onSetupPacket)(uint8_t, uint8_t *, uint32_t);
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
		virtual bool enableUSB();
		virtual bool disableUSB();
		virtual bool addEndpoint(uint8_t id, uint8_t direction, uint8_t type);
		virtual bool enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len);
		virtual bool setAddress(uint8_t address);
        virtual bool canEnqueuePacket(uint8_t ep);
        virtual bool sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len);


        char debugLog[10][80];
        void initDebug();
        void updateDebug();
        void log(const char *);

		void handleInterrupt();

		__attribute__ ((aligned(512))) volatile struct bdt _bufferDescriptorTable[16][4];

};

class USBManager {
	private:
		// Private functions and variables here
	public:
		USBManager() {}
};

#endif
