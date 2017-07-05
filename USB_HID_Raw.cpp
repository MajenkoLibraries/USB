#include <USB.h>

uint8_t HID_Raw::getDescriptorLength() {
    return (9 + 9 + 7 + 7);
}

uint8_t HID_Raw::getInterfaceCount() {
    return 1;
}

static const uint8_t rawHidReport[] = {
  0x06, 0x00, 0xFF,            // (GLOBAL) USAGE_PAGE         0xFF00 Vendor-defined 
  0xA1, 0x01,                  // (MAIN)   COLLECTION         0x01 Application (Usage=0x0: Page=, Usage=, Type=) <-- Warning: USAGE type should be CA (Application)
  0x15, 0x00,                  //   (GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0
  0x26, 0xFF, 0x00,            //   (GLOBAL) LOGICAL_MAXIMUM    0x00FF (255) 
  0x75, 0x08,                  //   (GLOBAL) REPORT_SIZE        0x08 (8) Number of bits per field 
  0x95, 0x40,                  //   (GLOBAL) REPORT_COUNT       0x40 (64) Number of fields 
  0x09, 0x01,                  //   (LOCAL)  USAGE              0xFF000001  
  0x81, 0x02,                  //   (MAIN)   INPUT              0x00000002 (64 fields x 8 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap 
  0x09, 0x01,                  //   (LOCAL)  USAGE              0xFF000001  
  0x91, 0x02,                  //   (MAIN)   OUTPUT             0x00000002 (64 fields x 8 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap 
  0xC0,      
};

uint32_t HID_Raw::populateConfigurationDescriptor(uint8_t *buf) {
    uint8_t i = 0;
    buf[i++] =                      0x09;
    buf[i++] =                      0x04;
    buf[i++] =                      _ifInt;
    buf[i++] =                      0;
    buf[i++] =                      2;
    buf[i++] =                      0x03;
    buf[i++] =                      0x00;
    buf[i++] =                      0;
    buf[i++] =                      0;

    /* HID Class-Specific Descriptor */

    buf[i++] =                      0x09;
    buf[i++] =                      0x21;
    buf[i++] =                      0x11;
    buf[i++] =                      0x01;
    buf[i++] =                      0x00;
    buf[i++] =                      1;
    buf[i++] =                      0x22;
    buf[i++] =                      sizeof(rawHidReport) & 0xFF;
    buf[i++] =                      sizeof(rawHidReport) >> 8;

    /* Endpoint Descriptor */

    buf[i++] =                      0x07;
    buf[i++] =                      0x05;
    buf[i++] =                      0x80 | _epInt;
    buf[i++] =                      0x03;
    buf[i++] =                      0x40; // Size = 64
    buf[i++] =                      0x00;
    buf[i++] =                      1;

    buf[i++] =                      0x07;
    buf[i++] =                      0x05;
    buf[i++] =                      0x00 | _epInt;
    buf[i++] =                      0x03;
    buf[i++] =                      0x40; // Size = 64
    buf[i++] =                      0x00;
    buf[i++] =                      1;

    return i;
}


void HID_Raw::initDevice(USBManager *manager) {
    _manager = manager;
    _ifInt = _manager->allocateInterface();
    _epInt = _manager->allocateEndpoint();
}

bool HID_Raw::getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) {
    return false;
}

bool HID_Raw::getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) {
    if (target == _ifInt) {
        _manager->sendBuffer(0, rawHidReport, min(sizeof(rawHidReport), maxlen));
        return true;
    }
    return false;
}

void HID_Raw::configureEndpoints() {
    _manager->addEndpoint(_epInt, EP_IN, EP_INT, 64, _intRxA, _intRxB);
    _manager->addEndpoint(_epInt, EP_OUT, EP_INT, 64, _intTxA, _intTxB);
}

bool HID_Raw::onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {

    if (data[4] != _ifInt) return false;
    uint16_t signature = (data[0] << 8) | data[1];
    switch (signature) {
        case 0xA101: {
                if (data[3] == 1) {
                    uint8_t nothing[64];
                    memset(nothing, 0, 64);
                    _manager->sendBuffer(0, nothing, 64);
                } else {
                    _manager->sendBuffer(0, &_features[data[2]], 1);
                }
                return true;
            }
            break;
        case 0x2109: {
                Serial.print("F: ");
                for (int i = 0; i < l; i++) {
                    Serial.print(data[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
                _nextPacketIsMine = true;
                return true;
            }
            break;

    }
    return false;

}

bool HID_Raw::onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
//    if ((ep == 0) && (target == _ifInt)) {
//        return true;
//    }
//    if (ep == _epInt) {
//        sendReport(&_keyReport);
//    }
    return false;
}

bool HID_Raw::onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if (ep == 0) {
        if (_nextPacketIsMine == true) {
            _features[data[0]] = data[1];
            Serial.print("D: ");
            for (int i = 0; i < l; i++) {
                Serial.print(data[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            _manager->sendBuffer(0, NULL, 0);
            _nextPacketIsMine = false;
            return true;
        }
    }

    if (ep == _epInt) {
        Serial.print("O: ");
        for (int i = 0; i < l; i++) {
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        return true;
    }
    return false;
}

void HID_Raw::sendReport(uint8_t *b, uint8_t l) {
    uint8_t data[64];
    memset(data, 0, 64);
    memcpy(data, b, l);
    uint32_t ts = millis();
    while(!_manager->sendBuffer(_epInt, data, 64)) {
        if (millis() - ts > 5) return;
    }
}
