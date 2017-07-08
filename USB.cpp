#include <USB.h>

#define D2H(X) ((X & 0xF) < 10 ? '0' + (X & 0xF) : 'A' - 10 + (X & 0xF))

USBManager::USBManager(USBDriver *driver, uint16_t vid, uint16_t pid, const char *mfg, const char *prod, const char *ser) {
    _driver = driver;
    _driver->setManager(this);
    _devices = NULL;
    _vid = vid;
    _pid = pid;
    _ifCount = 0;
    _epCount = 1;
    _manufacturer = mfg;
    _product = prod;
    _serial = ser;
}

USBManager::USBManager(USBDriver &driver, uint16_t vid, uint16_t pid, const char *mfg, const char *prod, const char *ser) {
    _driver = &driver;
    _driver->setManager(this);
    _devices = NULL;
    _vid = vid;
    _pid = pid;
    _ifCount = 0;
    _epCount = 1;
    _manufacturer = mfg;
    _product = prod;
    _serial = ser;
}

USBManager::USBManager(USBDriver *driver, uint16_t vid, uint16_t pid) {
    _driver = driver;
    _driver->setManager(this);
    _devices = NULL;
    _vid = vid;
    _pid = pid;
    _ifCount = 0;
    _epCount = 1;
    _manufacturer = "chipKIT";
    _product = _BOARD_NAME_;
    _serial = _defSerial;
    _defSerial[0] = 'C';
    _defSerial[1] = 'K';
    _defSerial[2] = D2H(DEVID >> 28);
    _defSerial[3] = D2H(DEVID >> 24);
    _defSerial[4] = D2H(DEVID >> 20);
    _defSerial[5] = D2H(DEVID >> 16);
    _defSerial[6] = D2H(DEVID >> 12);
    _defSerial[7] = D2H(DEVID >> 8);
    _defSerial[8] = D2H(DEVID >> 4);
    _defSerial[9] = D2H(DEVID);
    _defSerial[10] = D2H(DEVCFG3 >> 12);
    _defSerial[11] = D2H(DEVCFG3 >> 8);
    _defSerial[12] = D2H(DEVCFG3 >> 4);
    _defSerial[13] = D2H(DEVCFG3);
}

USBManager::USBManager(USBDriver &driver, uint16_t vid, uint16_t pid) {
    _driver = &driver;
    _driver->setManager(this);
    _devices = NULL;
    _vid = vid;
    _pid = pid;
    _ifCount = 0;
    _epCount = 1;
    _manufacturer = "chipKIT";
    _product = _BOARD_NAME_;
    _serial = _defSerial;
    _defSerial[0] = 'C';
    _defSerial[1] = 'K';
    _defSerial[2] = D2H(DEVID >> 28);
    _defSerial[3] = D2H(DEVID >> 24);
    _defSerial[4] = D2H(DEVID >> 20);
    _defSerial[5] = D2H(DEVID >> 16);
    _defSerial[6] = D2H(DEVID >> 12);
    _defSerial[7] = D2H(DEVID >> 8);
    _defSerial[8] = D2H(DEVID >> 4);
    _defSerial[9] = D2H(DEVID);
    _defSerial[10] = D2H(DEVCFG3 >> 12);
    _defSerial[11] = D2H(DEVCFG3 >> 8);
    _defSerial[12] = D2H(DEVCFG3 >> 4);
    _defSerial[13] = D2H(DEVCFG3);
}

uint8_t USBManager::allocateInterface() {
    uint8_t i = _ifCount;
    _ifCount++;
    return i;
}

uint8_t USBManager::allocateEndpoint() {
    uint8_t i = _epCount;
    _epCount++;
    return i;
}

void USBManager::begin() {
    _wantedAddress = 0;
    for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
        scan->device->configureEndpoints();
    }
    _driver->enableUSB();
}

void USBManager::onSetupPacket(uint8_t ep, uint8_t *data, uint32_t l) {
    uint16_t signature = (data[0] << 8) | data[1];
    uint16_t outLength = (data[7] << 8) | data[6];

    _target = (data[5] << 8) | data[4];

    switch (signature) {
        case 0x8006: // Get Descriptor
            switch (data[3]) {
                case 1: { // Device Descriptor
                        struct DeviceDescriptor o;
                        o.bLength = sizeof(struct DeviceDescriptor);
                        o.bDescriptorType = 0x01;
                        o.bcdUSB = 0x0101;
                        o.bDeviceClass = 0xEF; //0x00;
                        o.bDeviceSubClass = 0x02; //0x00;
                        o.bDeviceProtocol = 0x01; //0x00;
                        o.bMaxPacketSize = 0x40;
                        o.idVendor = _vid;
                        o.idProduct = _pid;
                        o.bcdDevice = 0x0200;
                        o.iManufacturer = 0x01;
                        o.iProduct = 0x02;
                        o.iSerialNumber = 0x03;
                        o.bNumConfigurations = 0x01;
                        _driver->sendBuffer(0, (const uint8_t *)&o, min(outLength, sizeof(struct DeviceDescriptor)));
                    }
                    break;

                case 2: { // Configuration Descriptor
                        uint32_t len = sizeof(struct ConfigurationDescriptor);
                        uint8_t faces = 0;

                        for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
                            len += scan->device->getDescriptorLength();
                            faces += scan->device->getInterfaceCount();
                        }

                        uint8_t *buf = (uint8_t *)alloca(len);
                        if (!buf) {
                            break;
                        }
                        uint8_t *ptr = buf;
                        struct ConfigurationDescriptor *desc = (struct ConfigurationDescriptor *)buf;

                        desc->bLength = sizeof(struct ConfigurationDescriptor);
                        desc->bDescriptorType = 0x02;
                        desc->wTotalLength = len;
                        desc->bNumInterfaces = faces;
                        desc->bConfigurationValue = 1;
                        desc->iConfiguration = 0;
                        desc->bmAttributes = 0x80;
                        desc->bMaxPower = 250;

                        ptr += sizeof(struct ConfigurationDescriptor);

                        for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
                            ptr += scan->device->populateConfigurationDescriptor(ptr);
                        }
                        _driver->sendBuffer(0, buf, min(outLength, len));
//                        free(buf);
                    }
                    break;

                case 3: { // String Descriptor
                        switch (data[2]) {
                            case 0x00: { // Header
                                    struct StringDescriptorHeader o;
                                    o.bLength = sizeof(struct StringDescriptorHeader);
                                    o.bDescriptorType = 0x03;
                                    o.wLANGID = 0x0409;
                                    _driver->sendBuffer(0, (const uint8_t *)&o, min(outLength, sizeof(struct StringDescriptorHeader)));
                                }
                                break;

                            case 0x01: { // Manufacturer
                                    uint8_t mlen = strlen(_manufacturer);
                                    uint8_t o[mlen * 2 + 2];
                                    o[0] = mlen * 2 + 2;
                                    o[1] = 0x03;
                                    for (int i = 0; i < mlen; i++) {
                                        o[2 + (i * 2)] = _manufacturer[i];
                                        o[3 + (i * 2)] = 0;
                                    }
                                    _driver->sendBuffer(0, (const uint8_t *)&o, min(outLength, mlen * 2 + 2));
                                }
                                break;

                            case 0x02: { // Product
                                    uint8_t mlen = strlen(_product);
                                    uint8_t o[mlen * 2 + 2];
                                    o[0] = mlen * 2 + 2;
                                    o[1] = 0x03;
                                    for (int i = 0; i < mlen; i++) {
                                        o[2 + (i * 2)] = _product[i];
                                        o[3 + (i * 2)] = 0;
                                    }
                                    _driver->sendBuffer(0, (const uint8_t *)&o, min(outLength, mlen * 2 + 2));
                                }
                                break;

                            case 0x03: { // Serial
                                    uint8_t mlen = strlen(_serial);
                                    uint8_t o[mlen * 2 + 2];
                                    o[0] = mlen * 2 + 2;
                                    o[1] = 0x03;
                                    for (int i = 0; i < mlen; i++) {
                                        o[2 + (i * 2)] = _serial[i];
                                        o[3 + (i * 2)] = 0;
                                    }
                                    _driver->sendBuffer(0, (const uint8_t *)&o, min(outLength, mlen * 2 + 2));
                                }
                                break;

                            default:
                                _driver->sendBuffer(0, NULL, 0);
                                break;
                        }

                        break;
                    }
                    break;

                default:
                    for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
                        if (scan->device->getDescriptor(ep, 0, data[3], outLength)) {
                            return;
                        }
                    }
                    _driver->sendBuffer(0, NULL, 0);
                    break;
            }

            break;

        case 0x8106: // Get report descriptor
            for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
                if (scan->device->getReportDescriptor(ep, _target, data[2], outLength)) {
                    return;
                }
            }
            _driver->sendBuffer(0, NULL, 0);
            break;

        case 0x0005: // Set Address
            _driver->sendBuffer(0, NULL, 0);
            _wantedAddress = data[2];
            break;

        default:
            for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
                if (scan->device->onSetupPacket(ep, _target, data, l)) {
                    return;
                }
            }
            _driver->sendBuffer(0, NULL, 0);
            break;
    }
}

void USBManager::onInPacket(uint8_t ep, uint8_t *data, uint32_t l) {
    if (_wantedAddress != 0) {
        _driver->setAddress(_wantedAddress);
        _wantedAddress = 0;
    }
    for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
        if (scan->device->onInPacket(ep, _target, data, l)) {
            return;
        }
    }
}

void USBManager::onOutPacket(uint8_t ep, uint8_t *data, uint32_t l) {
    for (struct USBDeviceList *scan = _devices; scan; scan = scan->next) {
        if (scan->device->onOutPacket(ep, _target, data, l)) {
            return;
        }
    }
}

void USBManager::addDevice(USBDevice *d) {
    struct USBDeviceList *newDevice = (struct USBDeviceList *)malloc(sizeof(struct USBDeviceList));
    if (!newDevice) {
        return;
    }
    struct USBDeviceList *scan;
    d->initDevice(this);
    newDevice->device = d;
    newDevice->next = NULL;
    if (_devices == NULL) {
        _devices = newDevice;
        return;
    }
    
    scan = _devices;
    while (scan->next != NULL) {
        scan = scan->next;
    }
    scan->next = newDevice;
}

static void fatalError() {
    pinMode(PIN_LED1, OUTPUT);
    while(1) {
        digitalWrite(PIN_LED1, HIGH);
        delay(100);
        digitalWrite(PIN_LED1, LOW);
        delay(900);
    }
}
