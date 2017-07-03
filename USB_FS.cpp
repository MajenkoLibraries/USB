#include <USB.h>

extern void dumpPacket(const uint8_t *data, uint32_t l);


#define KVA_TO_PA(v)  ((v) & 0x1fffffff)
#define PA_TO_KVA0(pa)  ((pa) | 0x80000000)  // cachable
#define PA_TO_KVA1(pa)  ((pa) | 0xa000000

/*-------------- USB FS ---------------*/

USBFS *USBFS::_this;

bool USBFS::enableUSB() {
    U1BDTP1 = (uint8_t)(KVA_TO_PA((uint32_t)&_bufferDescriptorTable[0][0]) >> 8);
    U1BDTP2 = (uint8_t)(KVA_TO_PA((uint32_t)&_bufferDescriptorTable[0][0]) >> 16);
    U1BDTP3 = (uint8_t)(KVA_TO_PA((uint32_t)&_bufferDescriptorTable[0][0]) >> 24);


    // enable usb device mode
    U1CONbits.SOFEN = 1;
    U1OTGCONbits.DPPULUP = 1;
	U1OTGCONbits.OTGEN = 1;
	U1IR = 0xFF;
	U1IEbits.URSTIE = 1;
	U1EIE = 0xFF;

	setIntVector(_USB_1_VECTOR, _usbInterrupt);
	setIntPriority(_USB_1_VECTOR, 5, 0);
	clearIntFlag(_USB_IRQ);
	setIntEnable(_USB_IRQ);

	U1SOFbits.CNT = 74;

	U1PWRCbits.USBPWR = 1;

    addEndpoint(0, EP_IN, EP_CTL, 64);
    addEndpoint(0, EP_OUT, EP_CTL, 64);
	return true;
}

bool USBFS::disableUSB() {
	U1PWRCbits.USBPWR = 0;
	return true;
}
	

bool USBFS::addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint8_t size) {
	if (id > 15) return false;
    _endpointBuffers[id].data = 0x40;
    _endpointBuffers[id].size = size;
	if (direction == EP_IN) {
		if (_endpointBuffers[id].rx[0] == NULL) {
			_endpointBuffers[id].rx[0] = (uint8_t *)malloc(size);
		}
		if (_endpointBuffers[id].rx[0] == NULL) {
			return false;
		}
		if (_endpointBuffers[id].rx[1] == NULL) {
			_endpointBuffers[id].rx[1] = (uint8_t *)malloc(size);
		}
		if (_endpointBuffers[id].rx[1] == NULL) {
			free(_endpointBuffers[id].rx[0]);
			return false;
		}
		_bufferDescriptorTable[id][0].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].rx[0]);
		_bufferDescriptorTable[id][0].flags = (_endpointBuffers[id].size << 16) | 0x80;
		_bufferDescriptorTable[id][1].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].rx[1]);
		_bufferDescriptorTable[id][1].flags = (_endpointBuffers[id].size << 16) | 0x80;
		switch (id) {
			case 0: U1EP0bits.EPRXEN = 1; break;
			case 1: U1EP1bits.EPRXEN = 1; break;
			case 2: U1EP2bits.EPRXEN = 1; break;
			case 3: U1EP3bits.EPRXEN = 1; break;
			case 4: U1EP4bits.EPRXEN = 1; break;
			case 5: U1EP5bits.EPRXEN = 1; break;
			case 6: U1EP6bits.EPRXEN = 1; break;
			case 7: U1EP7bits.EPRXEN = 1; break;
			case 8: U1EP8bits.EPRXEN = 1; break;
			case 9: U1EP9bits.EPRXEN = 1; break;
			case 10: U1EP10bits.EPRXEN = 1; break;
			case 11: U1EP11bits.EPRXEN = 1; break;
			case 12: U1EP12bits.EPRXEN = 1; break;
			case 13: U1EP13bits.EPRXEN = 1; break;
			case 14: U1EP14bits.EPRXEN = 1; break;
			case 15: U1EP15bits.EPRXEN = 1; break;
		}
	} else {
		if (_endpointBuffers[id].tx[0] == NULL) {
			_endpointBuffers[id].tx[0] = (uint8_t *)malloc(80);
		}
		if (_endpointBuffers[id].tx[0] == NULL) {
			return false;
		}
		if (_endpointBuffers[id].tx[1] == NULL) {
			_endpointBuffers[id].tx[1] = (uint8_t *)malloc(80);
		}
		if (_endpointBuffers[id].tx[1] == NULL) {
			free(_endpointBuffers[id].tx[0]);
			return false;
		}
//		_endpointBuffers[id].txAB = 0;
		_bufferDescriptorTable[id][2].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].tx[0]);
		_bufferDescriptorTable[id][2].flags = 0;
		_bufferDescriptorTable[id][3].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].tx[1]);
		_bufferDescriptorTable[id][3].flags = 0;
		switch (id) {
			case 0: U1EP0bits.EPTXEN = 1; break;
			case 1: U1EP1bits.EPTXEN = 1; break;
			case 2: U1EP2bits.EPTXEN = 1; break;
			case 3: U1EP3bits.EPTXEN = 1; break;
			case 4: U1EP4bits.EPTXEN = 1; break;
			case 5: U1EP5bits.EPTXEN = 1; break;
			case 6: U1EP6bits.EPTXEN = 1; break;
			case 7: U1EP7bits.EPTXEN = 1; break;
			case 8: U1EP8bits.EPTXEN = 1; break;
			case 9: U1EP9bits.EPTXEN = 1; break;
			case 10: U1EP10bits.EPTXEN = 1; break;
			case 11: U1EP11bits.EPTXEN = 1; break;
			case 12: U1EP12bits.EPTXEN = 1; break;
			case 13: U1EP13bits.EPTXEN = 1; break;
			case 14: U1EP14bits.EPTXEN = 1; break;
			case 15: U1EP15bits.EPTXEN = 1; break;
		}
		_enabledEndpoints |= (1 << (id + 16));
	}

	
	switch (id) {
		case 0: U1EP0bits.EPHSHK = 1; break;
		case 1: U1EP1bits.EPHSHK = 1; break;
		case 2: U1EP2bits.EPHSHK = 1; break;
		case 3: U1EP3bits.EPHSHK = 1; break;
		case 4: U1EP4bits.EPHSHK = 1; break;
		case 5: U1EP5bits.EPHSHK = 1; break;
		case 6: U1EP6bits.EPHSHK = 1; break;
		case 7: U1EP7bits.EPHSHK = 1; break;
		case 8: U1EP8bits.EPHSHK = 1; break;
		case 9: U1EP9bits.EPHSHK = 1; break;
		case 10: U1EP10bits.EPHSHK = 1; break;
		case 11: U1EP11bits.EPHSHK = 1; break;
		case 12: U1EP12bits.EPHSHK = 1; break;
		case 13: U1EP13bits.EPHSHK = 1; break;
		case 14: U1EP14bits.EPHSHK = 1; break;
		case 15: U1EP15bits.EPHSHK = 1; break;
	}
	
	return true;
}

bool USBFS::canEnqueuePacket(uint8_t ep) {
    uint8_t buffer = _endpointBuffers[ep].txAB;
    uint8_t bdt_entry = buffer ? 3 : 2;
    if ((_bufferDescriptorTable[ep][bdt_entry].flags & 0x80) == 0) return true;
    return false;
}

bool USBFS::enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len) {
	bool sent = false;

    uint8_t buffer = _endpointBuffers[ep].txAB;
    uint8_t bdt_entry = buffer ? 3 : 2;

    while (!sent) {
        if ((_bufferDescriptorTable[ep][bdt_entry].flags & 0x80) == 0) {
            if (len > 0) memcpy(_endpointBuffers[ep].tx[buffer], data, min(len, _endpointBuffers[ep].size));
            _bufferDescriptorTable[ep][bdt_entry].flags = (len << 16) | 0x00 | _endpointBuffers[ep].data; 
            sent = true;
            _bufferDescriptorTable[ep][bdt_entry].flags |= 0x80;
        }
	}

    _endpointBuffers[ep].txAB = 1 - _endpointBuffers[ep].txAB;
	_endpointBuffers[ep].data = _endpointBuffers[ep].data ? 0 : 0x40;

    switch(ep) {
        case 0: U1EP0bits.EPSTALL=0; break;
        case 1: U1EP1bits.EPSTALL=0; break;
        case 2: U1EP2bits.EPSTALL=0; break;
        case 3: U1EP3bits.EPSTALL=0; break;
        case 4: U1EP4bits.EPSTALL=0; break;
        case 5: U1EP5bits.EPSTALL=0; break;
        case 6: U1EP6bits.EPSTALL=0; break;
        case 7: U1EP7bits.EPSTALL=0; break;
        case 8: U1EP8bits.EPSTALL=0; break;
        case 9: U1EP9bits.EPSTALL=0; break;
        case 10: U1EP10bits.EPSTALL=0; break;
        case 11: U1EP11bits.EPSTALL=0; break;
        case 12: U1EP12bits.EPSTALL=0; break;
        case 13: U1EP13bits.EPSTALL=0; break;
        case 14: U1EP14bits.EPSTALL=0; break;
        case 15: U1EP15bits.EPSTALL=0; break;
    }

	return true;
}

bool USBFS::sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) {

    if (_endpointBuffers[ep].buffer != NULL) {
        // Already sending something ... ?
        return false;
    }

    _endpointBuffers[ep].length = len;
    _endpointBuffers[ep].buffer = (uint8_t *)malloc(len);
    memcpy(_endpointBuffers[ep].buffer, data, len);
    _endpointBuffers[ep].bufferPtr = _endpointBuffers[ep].buffer;

    while (!canEnqueuePacket(ep));

    uint32_t toSend = min(_endpointBuffers[ep].size, _endpointBuffers[ep].length);
    enqueuePacket(ep, _endpointBuffers[ep].bufferPtr, toSend);
    _endpointBuffers[ep].length -= toSend;
    _endpointBuffers[ep].bufferPtr += toSend;

    return true;
}

void USBFS::handleInterrupt() {
    uint32_t toSend;

	if (U1IRbits.TRNIF) {

		uint8_t ep = U1STATbits.ENDPT;
		uint8_t bdt_slot = U1STATbits.PPBI | (U1STATbits.DIR << 1);

		uint8_t pid = (_bufferDescriptorTable[ep][bdt_slot].flags >> 2) & 0x0F;

		switch (pid) {
			case 0x01: // OUT
                if (_manager) _manager->onOutPacket(ep, _endpointBuffers[ep].rx[U1STATbits.PPBI], _bufferDescriptorTable[ep][bdt_slot].flags >> 16);
     //           _endpointBuffers[ep].data = _endpointBuffers[ep].data ? 0 : 0x40;
				_bufferDescriptorTable[ep][bdt_slot].flags = (_endpointBuffers[ep].size << 16) | 0x80 | _endpointBuffers[ep].data; 
				break;
			case 0x09: // IN
                if (_endpointBuffers[ep].length > 0) {
                    toSend = min(_endpointBuffers[ep].size, _endpointBuffers[ep].length);
                    enqueuePacket(ep, _endpointBuffers[ep].bufferPtr, toSend);
                    _endpointBuffers[ep].length -= toSend;
                    _endpointBuffers[ep].bufferPtr += toSend;
                } else {
                    if (_endpointBuffers[ep].buffer != NULL) {
                        free(_endpointBuffers[ep].buffer);
                        _endpointBuffers[ep].buffer = NULL;
                    }
                }

                if (_manager) _manager->onInPacket(ep, _endpointBuffers[ep].rx[U1STATbits.PPBI], _bufferDescriptorTable[ep][bdt_slot].flags >> 16);
				break;
			case 0x0d: // SETUP
				_endpointBuffers[ep].data = 0x40;
                if (_manager) _manager->onSetupPacket(ep, _endpointBuffers[ep].rx[U1STATbits.PPBI], _bufferDescriptorTable[ep][bdt_slot].flags >> 16);
				_bufferDescriptorTable[ep][bdt_slot].flags = (_endpointBuffers[ep].size << 16) | 0x80;
				break;
			default:
				break;
		}


		switch(ep) {
			case 0: U1EP0bits.EPSTALL=0; break;
			case 1: U1EP1bits.EPSTALL=0; break;
			case 2: U1EP2bits.EPSTALL=0; break;
			case 3: U1EP3bits.EPSTALL=0; break;
			case 4: U1EP4bits.EPSTALL=0; break;
			case 5: U1EP5bits.EPSTALL=0; break;
			case 6: U1EP6bits.EPSTALL=0; break;
			case 7: U1EP7bits.EPSTALL=0; break;
			case 8: U1EP8bits.EPSTALL=0; break;
			case 9: U1EP9bits.EPSTALL=0; break;
			case 10: U1EP10bits.EPSTALL=0; break;
			case 11: U1EP11bits.EPSTALL=0; break;
			case 12: U1EP12bits.EPSTALL=0; break;
			case 13: U1EP13bits.EPSTALL=0; break;
			case 14: U1EP14bits.EPSTALL=0; break;
			case 15: U1EP15bits.EPSTALL=0; break;
		}
		U1CONbits.TOKBUSY=0;
	}
	if (U1IRbits.URSTIF) {
		U1IEbits.IDLEIE = 1;
		U1IEbits.TRNIE = 1;
		U1ADDR = 0;
		U1CONbits.PPBRST = 1;
		U1CONbits.PPBRST = 0;
		for (int i  = 0; i < 16; i++) {
			_endpointBuffers[i].txAB = 0;
            _endpointBuffers[i].data = 0x00;
		}
	}
	if (U1IRbits.IDLEIF) {
		U1IEbits.IDLEIE = 0;
		U1IEbits.RESUMEIE = 1;
	}
	if (U1IRbits.RESUMEIF) {
		U1IEbits.IDLEIE = 1;
		U1IEbits.RESUMEIE = 0;
	}
	if (U1EIR) {
    
	}
	U1EIR = 0xFF;
	U1IR = 0xFF;
	clearIntFlag(_USB_IRQ);
}

bool USBFS::setAddress(uint8_t address) {
	U1ADDR = address;
	return true;
}

