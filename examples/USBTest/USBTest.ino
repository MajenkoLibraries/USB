#include <USB.h>
#include <wchar.h>

USBFS usbDriver;


extern void report(uint8_t v);

uint32_t wantedAddress = 0;

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

static const byte cdcacm_configuration_descriptor[] = {
	9,          // length
	0x02,       // configuration descriptor
	67, 0,      // total length
	0x02,       // num interfaces
	0x01,       // configuration value
	0x00,       // configuration (string)
	0x80,       // attributes
	250,        // 500 mA

	9,          // length
	0x04,       // interface descriptor
	0x00,       // interface number
	0x00,       // alternate
	0x01,       // num endpoints
	0x02,       // interface class (comm)
	0x02,       // subclass (acm)
	0x01,       // protocol (at)
	0x00,       // interface (string)

	5,          // length
	0x24,       // header functional descriptor
	0x00,
	0x10, 0x01,

	4,          // length
	0x24,       // abstract control model descriptor
	0x02,
	0x06,

	5,          // length
	0x24,       // union functional descriptor
	0x06,
	0x00,       // comm
	0x01,       // data

	5,          // length
	0x24,       // call management functional descriptor
	0x01,
	0x00,
	0x01,

	7,          // length
	0x05,       // endpoint descriptor
	0x81,       // endpoint IN address
	0x03,       // attributes: interrupt
	0x08, 0x00, // packet size
	0x10,       // interval (ms)

	9,          // length
	0x04,       // interface descriptor
	0x01,       // interface number
	0x00,       // alternate
	0x02,       // num endpoints
	0x0a,       // interface class (data)
	0x00,       // subclass
	0x00,       // protocol
	0x00,       // interface (string)

	7,          // length
	0x05,       // endpoint descriptor
	0x82,       // endpoint IN address
	0x02,       // attributes: bulk
	0x40, 0x00,     // packet size
	0x00,       // interval (ms)

	7,          // length
	0x05,       // endpoint descriptor
	0x03,       // endpoint OUT address
	0x02,       // attributes: bulk
	0x40, 0x00, // packet size
	0x00,       // interval (ms)
};

static const byte string_descriptor[] = {
	4,          // length
	0x03,       // string descriptor
	0x09, 0x04, // english (usa)

	34,         // length
	0x03,       // string descriptor
	'w', 0, 'w', 0, 'w', 0, '.', 0, 'c', 0, 'p', 0, 'u', 0, 's', 0, 't', 0, 'i', 0, 'c', 0, 'k', 0, '.', 0, 'c', 0, 'o', 0, 'm', 0,

	18,         // length
	0x03,       // string descriptor
	'S', 0, 't', 0, 'k', 0, '5', 0, '0', 0, '0', 0, 'v', 0, '2', 0,
};



void gotSetupPacket(uint8_t ep, uint8_t *data, uint32_t l) {
	for (int i = 0; i < l; i++) {
		Serial.print(data[i], HEX);
		Serial.write(' ');
	}

	Serial.println();
	uint16_t signature = (data[0] << 8) | data[1];
	uint8_t outLength = data[6];

	switch (signature) {
		case 0x8006: // Get Descriptor
			switch (data[3]) {
				case 1: { // Device Descriptor
						struct DeviceDescriptor o;
						o.bLength = sizeof(struct DeviceDescriptor);
						o.bDescriptorType = 0x01;
						o.bcdUSB = 0x0101;
						o.bDeviceClass = 0x02;
						o.bDeviceSubClass = 0x00;
						o.bDeviceProtocol = 0x00;
						o.bMaxPacketSize = 0x40;
						o.idVendor = 0x0403;
						o.idProduct = 0xA662;
						o.bcdDevice = 0x0180;
						o.iManufacturer = 0x01;
						o.iProduct = 0x02;
						o.iSerialNumber = 0x03;
						o.bNumConfigurations = 0x01;
						usbDriver.sendBuffer(0, (const uint8_t *)&o, min(outLength, sizeof(struct DeviceDescriptor)));
					}
					break;

				case 2: { // Configuration Descriptor
						struct ConfigurationDescriptor o;
						o.bLength = sizeof(struct ConfigurationDescriptor);
						o.bDescriptorType = 0x02;
						o.wTotalLength = o.bLength;
						o.bNumInterfaces = 0;
						o.bConfigurationValue = 1;
						o.iConfiguration = 1;
						o.bmAttributes = 0x80;
						o.bMaxPower = 250;
						usbDriver.sendBuffer(0, (const uint8_t *)cdcacm_configuration_descriptor, outLength);
					}
					break;

				case 3: { // String Descriptor
						switch (data[2]) {
							case 0x00: { // Header
									struct StringDescriptorHeader o;
									o.bLength = sizeof(struct StringDescriptorHeader);
									o.bDescriptorType = 0x03;
									o.wLANGID = 0x0409;
									usbDriver.sendBuffer(0, (const uint8_t *)&o, min(outLength, sizeof(struct StringDescriptorHeader)));
								}
								break;

							case 0x01: { // Manufacturer
									const char *man = "chipKIT";
                                    uint8_t mlen = strlen(man);
									uint8_t o[mlen * 2 + 2];
									o[0] = mlen * 2 + 2;
									o[1] = 0x03;
                                    for (int i = 0; i < mlen; i++) {
                                        o[2 + (i * 2)] = man[i];
                                        o[3 + (i * 2)] = 0;
                                    }
									usbDriver.sendBuffer(0, (const uint8_t *)&o, min(outLength, mlen * 2 + 2));
								}
								break;

                            case 0x02: { // Product
                                    const char *prod = "MAX32";
                                    uint8_t mlen = strlen(prod);
                                    uint8_t o[mlen * 2 + 2];
                                    o[0] = mlen * 2 + 2;
                                    o[1] = 0x03;
                                    for (int i = 0; i < mlen; i++) {
                                        o[2 + (i * 2)] = prod[i];
                                        o[3 + (i * 2)] = 0;
                                    }
                                    usbDriver.sendBuffer(0, (const uint8_t *)&o, min(outLength, mlen * 2 + 2));
                                }
                                break;

                            case 0x03: { // Serial
                                    const char *ser = "1234567890";
                                    uint8_t mlen = strlen(ser);
                                    uint8_t o[mlen * 2 + 2];
                                    o[0] = mlen * 2 + 2;
                                    o[1] = 0x03;
                                    for (int i = 0; i < mlen; i++) {
                                        o[2 + (i * 2)] = ser[i];
                                        o[3 + (i * 2)] = 0;
                                    }
                                    usbDriver.sendBuffer(0, (const uint8_t *)&o, min(outLength, mlen * 2 + 2));
                                }
                                break;

							default:
								usbDriver.sendBuffer(0, NULL, 0);
								break;
						}

						break;
					}
					break;

				default:
					usbDriver.sendBuffer(0, NULL, 0);
					break;
			}

			break;

		case 0x0005: // Set Address
			usbDriver.sendBuffer(0, NULL, 0);
			wantedAddress = data[2];
			break;

		default:
			usbDriver.sendBuffer(0, NULL, 0);
			break;
	}
}

void gotInPacket(uint8_t ep, uint8_t *data, uint32_t l) {
	if (wantedAddress != 0) {
		usbDriver.setAddress(wantedAddress);
		Serial.printf("Address set to %d\r\n", wantedAddress);
		wantedAddress = 0;
	}
}

void gotOutPacket(uint8_t ep, uint8_t *data, uint32_t l) {
	for (int i = 0; i < l; i++) {
		Serial.print(data[i], HEX);
		Serial.write(' ');
	}

	Serial.println();
}


void setup() {
	Serial.begin(1000000);
	pinMode(PIN_LED1, OUTPUT);
	usbDriver.onSetupPacket(gotSetupPacket);
	usbDriver.onOutPacket(gotOutPacket);
	usbDriver.onInPacket(gotInPacket);
	usbDriver.enableUSB();
}

void loop() {
	digitalWrite(PIN_LED1, HIGH);
	delay(500);
	digitalWrite(PIN_LED1, LOW);
	delay(500);
}
