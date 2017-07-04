#include <USB.h>

#define CDC_ACT_SET_LINE_CODING 1


uint8_t CDCACM::getDescriptorLength() {
    return 58;
}

uint8_t CDCACM::getInterfaceCount() {
    return 2;
}

uint32_t CDCACM::populateConfigurationDescriptor(uint8_t *buf) {
    uint8_t i = 0;

    buf[i++] = 9;          // length
    buf[i++] = 0x04;       // interface descriptor
    buf[i++] = _ifControl; // interface number
    buf[i++] = 0x00;       // alternate
    buf[i++] = 0x01;       // num endpoints
    buf[i++] = 0x02;       // interface class (comm)
    buf[i++] = 0x02;       // subclass (acm)
    buf[i++] = 0x01;       // protocol (at)
    buf[i++] = 0x00;       // interface (string)

    buf[i++] = 5;          // length
    buf[i++] = 0x24;       // header functional descriptor
    buf[i++] = 0x00;
    buf[i++] = 0x10; 
    buf[i++] = 0x01;

    buf[i++] = 4;          // length
    buf[i++] = 0x24;       // abstract control model descriptor
    buf[i++] = 0x02;
    buf[i++] = 0x06;

    buf[i++] = 5;          // length
    buf[i++] = 0x24;       // union functional descriptor
    buf[i++] = 0x06;
    buf[i++] = _ifControl;
    buf[i++] = _ifBulk;

    buf[i++] = 5;          // length
    buf[i++] = 0x24;       // call management functional descriptor
    buf[i++] = 0x01;
    buf[i++] = 0x00;
    buf[i++] = _ifBulk;

    buf[i++] = 7;          // length
    buf[i++] = 0x05;       // endpoint descriptor
    buf[i++] = 0x80 | _epControl;       // endpoint IN address
    buf[i++] = 0x03;       // attributes: interrupt
    buf[i++] = 0x08; 
    buf[i++] = 0x00; // packet size
    buf[i++] = 0x10;       // interval (ms)

    buf[i++] = 9;          // length
    buf[i++] = 0x04;       // interface descriptor
    buf[i++] = _ifBulk;       // interface number
    buf[i++] = 0x00;       // alternate
    buf[i++] = 0x02;       // num endpoints
    buf[i++] = 0x0a;       // interface class (data)
    buf[i++] = 0x00;       // subclass
    buf[i++] = 0x00;       // protocol
    buf[i++] = 0x00;       // interface (string)

    buf[i++] = 7;          // length
    buf[i++] = 0x05;       // endpoint descriptor
    buf[i++] = 0x80 | _epBulk;       // endpoint IN address
    buf[i++] = 0x02;       // attributes: bulk
    buf[i++] = 0x40; 
    buf[i++] = 0x00;     // packet size
    buf[i++] = 0x00;       // interval (ms)

    buf[i++] = 7;          // length
    buf[i++] = 0x05;       // endpoint descriptor
    buf[i++] = _epBulk;       // endpoint OUT address
    buf[i++] = 0x02;       // attributes: bulk
    buf[i++] = 0x40; 
    buf[i++] = 0x00; // packet size
    buf[i++] = 0x00;       // interval (ms)
    return i;
}


void CDCACM::initDevice(USBManager *manager) {
    _manager = manager;
    _ifControl = _manager->allocateInterface();
    _ifBulk = _manager->allocateInterface();
    _epControl = _manager->allocateEndpoint();
    _epBulk = _manager->allocateEndpoint();
}

bool CDCACM::getDescriptor(uint8_t ep, uint8_t target, uint8_t id, uint8_t maxlen) {
    return false;
}

void CDCACM::configureEndpoints() {
    _manager->addEndpoint(_epControl, EP_OUT, EP_CTL, 8);
    _manager->addEndpoint(_epBulk, EP_IN, EP_BLK, 64);
    _manager->addEndpoint(_epBulk, EP_OUT, EP_BLK, 64);
}


bool CDCACM::onSetupPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if (target == _ifControl) {
        uint16_t signature = (data[0] << 8) | data[1];
        switch (signature) {
            case 0x2120:
                _outAction = CDC_ACT_SET_LINE_CODING;
                return true;
            case 0x2122:
                _lineState = data[2];
                _manager->sendBuffer(0, NULL, 0);
                return true;
        }
    }
    return false;
}

bool CDCACM::onInPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if ((ep == 0) && (target == _ifControl)) {
        return true;
    }
    if (ep == _epBulk) {
        if (_txPos > 0) {
            _manager->sendBuffer(_epBulk, _txBuffer, _txPos);
            _txPos = 0;
        }
    }
    return false;
}

struct CDCLineCoding {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
} __attribute__((packed));

bool CDCACM::onOutPacket(uint8_t ep, uint8_t target, uint8_t *data, uint32_t l) {
    if (ep == 0) {
        if (target == _ifControl) {
            switch(_outAction) {
                case CDC_ACT_SET_LINE_CODING:
                    struct CDCLineCoding *coding = (struct CDCLineCoding *)data;
                    _baud = coding->dwDTERate;
                    _stopBits = coding->bCharFormat;
                    _parity = coding->bParityType;
                    _dataBits = coding->bDataBits;
                    _manager->sendBuffer(0, NULL, 0);
                    return true;
            }
        }
    }

    if (ep == _epBulk) {
        for (int i = 0; i < l; i++) {
            int bufIndex = (_rxHead + 1) % 64;
            if (bufIndex != _rxTail) {
                _rxBuffer[_rxHead] = data[i];
                _rxHead = bufIndex;
            }
        }
        return true;
    }
    return false;
}

size_t CDCACM::write(uint8_t b) {

    if (_lineState == 0) return 0;

    while (_txPos >= 64);

    _txBuffer[_txPos++] = b;
    if (_manager->sendBuffer(_epBulk, _txBuffer, _txPos)) {
        _txPos = 0;
    }
    return 1;
}

int CDCACM::available() {
    return (64 + _rxHead - _rxTail) % 64;
}

int CDCACM::read() {
    if (_rxHead == _rxTail) return -1;
    uint8_t ch = _rxBuffer[_rxTail];
    _rxTail = (_rxTail + 1) % 64;
    return ch;
}

void CDCACM::flush() {
}

CDCACM::operator int() {
    return _lineState > 0;
}

int CDCACM::peek() {
    if (_rxHead == _rxTail) return -1;
    return _rxBuffer[_rxTail];
}

