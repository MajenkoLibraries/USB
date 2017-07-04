#include <USB.h>


uint8_t HID_Mouse::getDescriptorLength() {
    return 32 - 7;
}

uint8_t HID_Mouse::getInterfaceCount() {
    return 1;
}

static const uint8_t mouseHidReport[] = {
    //  Mouse
    0x05, 0x01, // USAGE_PAGE (Generic Desktop) // 54
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM (Button 1)
    0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x25, 0x01, //     LOGICAL_MAXIMUM (1)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x75, 0x01, //     REPORT_SIZE (1)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
    0x95, 0x01, //     REPORT_COUNT (1)
    0x75, 0x05, //     REPORT_SIZE (5)
    0x81, 0x03, //     INPUT (Cnst,Var,Abs)
    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x09, 0x38, //     USAGE (Wheel)
    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (8)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x81, 0x06, //     INPUT (Data,Var,Rel)
    0xc0, //   END_COLLECTION
    0xc0, // END_COLLECTION
};

uint32_t HID_Mouse::populateConfigurationDescriptor(uint8_t *buf) {
    uint8_t i = 0;
    buf[i++] =                      0x09;
    buf[i++] =                      0x04;
    buf[i++] =                      _ifInt;
    buf[i++] =                      0;
    buf[i++] =                      1;
    buf[i++] =                      0x03;
    buf[i++] =                      0x00;
    buf[i++] =                      2;
    buf[i++] =                      0;

    /* HID Class-Specific Descriptor */

    buf[i++] =                      0x09;
    buf[i++] =                      0x21;
    buf[i++] =                      0x11;
    buf[i++] =                      0x01;
    buf[i++] =                      0x00;
    buf[i++] =                      1;
    buf[i++] =                      0x22;
    buf[i++] =                      sizeof(mouseHidReport) & 0xFF;
    buf[i++] =                      sizeof(mouseHidReport) >> 8;

    /* Endpoint Descriptor */

    buf[i++] =                      0x07;
    buf[i++] =                      0x05;
    buf[i++] =                      0x80 | _epInt;
    buf[i++] =                      0x03;
    buf[i++] =                      0x40; // Size = 64
    buf[i++] =                      0x00;
    buf[i++] =                      1;

    /* Endpoint Descriptor */

//    buf[i++] =                      0x07;
//    buf[i++] =                      0x05;
//    buf[i++] =                      _epInt;
//    buf[i++] =                      0x03;
//    buf[i++] =                      0x40; // Size = 64
//    buf[i++] =                      0x00;
//    buf[i++] =                      1;

    return i;
}


void HID_Mouse::initDevice(USBManager *manager) {
    _manager = manager;
    _ifInt = _manager->allocateInterface();
    _epInt = _manager->allocateEndpoint();
}

bool HID_Mouse::getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) {
    return false;
}

bool HID_Mouse::getReportDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) {
    if (target == _ifInt) {
        _manager->sendBuffer(0, mouseHidReport, min(sizeof(mouseHidReport), maxlen));
        return true;
    }
    return false;
}

void HID_Mouse::configureEndpoints() {
//    _manager->addEndpoint(_epInt, EP_IN, EP_INT, 64);
    _manager->addEndpoint(_epInt, EP_OUT, EP_INT, 64);
}


bool HID_Mouse::onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {

    if (data[4] != _ifInt) return false;

    uint16_t signature = (data[0] << 8) | data[1];
    switch (signature) {
        case 0xA101: {
                uint8_t m[4];
                m[0] = 0;
                m[1] = 0;
                m[2] = 0;
                m[3] = 0; 
                _manager->sendBuffer(0, m, 4);
                return true;
            }
            break;
    }
    return false;
}

bool HID_Mouse::onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    return false;
}

bool HID_Mouse::onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if (ep == 0) {
        if (target == _ifInt) {
            return true;
        }
    }

    if (ep == _epInt) {
        return true;
    }
    return false;
}

void HID_Mouse::sendReport(const uint8_t *b, uint8_t l) {
    uint32_t ts = millis();
    while (!_manager->sendBuffer(_epInt, b, l)) {
        if (millis() - ts > 10) return;
    }
}

void HID_Mouse::click(uint8_t b) {
    press(b);
    release(b);
}

void HID_Mouse::move(int8_t x, int8_t y, int8_t wheel)
{
    uint8_t m[4];
    m[0] = _buttons;
    m[1] = x;
    m[2] = y;
    m[3] = wheel;
    sendReport(m, 4);
}

void HID_Mouse::buttons(uint8_t b) {
    if (b != _buttons) {
        _buttons = b;
        move(0,0,0);
    }
}

void HID_Mouse::press(uint8_t b) {
    buttons(_buttons | b);
}

void HID_Mouse::release(uint8_t b) {
    buttons(_buttons & ~b);
}

bool HID_Mouse::isPressed(uint8_t b) {
    if ((b & _buttons) > 0)
        return true;
    return false;
}


