#include <USB.h>


#define KVA_TO_PA(v)  ((v) & 0x1fffffff)
#define PA_TO_KVA0(pa)  ((pa) | 0x80000000)  // cachable
#define PA_TO_KVA1(pa)  ((pa) | 0xa000000

USBManager::USBManager() {
	// Constructor code here
}


void report(uint8_t v) {
	pinMode(PIN_LED2, OUTPUT);
	pinMode(PIN_LED3, OUTPUT);
	digitalWrite(PIN_LED2, LOW);	
	digitalWrite(PIN_LED3, LOW);
	while (1) {
		for (int i = 0; i < 8; i++) {
			digitalWrite(PIN_LED2, HIGH);
			digitalWrite(PIN_LED3, v & (1 << i));
			delay(500);
			digitalWrite(PIN_LED2, LOW);
			digitalWrite(PIN_LED3, LOW);
			delay(500);
		}
		delay(5000);
	}
}


/*-------------- USB FS ---------------*/

USBFS *USBFS::_this;

bool USBFS::enableUSB() {
        U1BDTP1 = (uint8_t)(KVA_TO_PA((uint32_t)&_bufferDescriptorTable[0]) >> 8);
        U1BDTP2 = (uint8_t)(KVA_TO_PA((uint32_t)&_bufferDescriptorTable[0]) >> 16);
        U1BDTP3 = (uint8_t)(KVA_TO_PA((uint32_t)&_bufferDescriptorTable[0]) >> 24);


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
	return true;
}

bool USBFS::disableUSB() {
	U1PWRCbits.USBPWR = 0;
	return true;
}
	

bool USBFS::addEndpoint(uint8_t id, uint8_t direction, uint8_t type) {
	if (id > 15) return false;
	if (direction == EP_IN) {
		if (_endpointBuffers[id].rx[0] == NULL) {
			_endpointBuffers[id].rx[0] = (uint8_t *)malloc(80);
		}
		if (_endpointBuffers[id].rx[0] == NULL) {
			return false;
		}
		if (_endpointBuffers[id].rx[1] == NULL) {
			_endpointBuffers[id].rx[1] = (uint8_t *)malloc(80);
		}
		if (_endpointBuffers[id].rx[1] == NULL) {
			free(_endpointBuffers[id].rx[0]);
			return false;
		}
		_bufferDescriptorTable[id << 4].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].rx[0]);
		_bufferDescriptorTable[id << 4].flags = (64 << 16) | 0x80;
		_bufferDescriptorTable[(id << 4) + 1].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].rx[1]);
		_bufferDescriptorTable[(id << 4) + 1].flags = (64 << 16) | 0x80;
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
		_bufferDescriptorTable[(id << 4) + 2].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].tx[0]);
		_bufferDescriptorTable[(id << 4) + 2].flags = 0;
//		_bufferDescriptorTable[(id << 4) + 2].flags = (64 << 16) | 0x80;
		_bufferDescriptorTable[(id << 4) + 3].buffer = (uint8_t *)KVA_TO_PA((uint32_t)_endpointBuffers[id].tx[1]);
		_bufferDescriptorTable[(id << 4) + 3].flags = 0;
//		_bufferDescriptorTable[(id << 4) + 3].flags = (64 << 16) | 0x80;
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

bool USBFS::enqueuePacket(uint8_t ep, uint8_t *data, uint32_t len, uint8_t d01) {
	bool sent = false;
	while (!sent) {
		if ((_bufferDescriptorTable[ep * 4 + 2].flags & 0x80) == 0) {
			if (len > 0) memcpy(_endpointBuffers[ep].tx[0], data, min(len, 64));
			_bufferDescriptorTable[ep * 4 + 2].flags = (len << 16) | 0x88 | (d01 << 6);
			sent = true;
		} else if ((_bufferDescriptorTable[ep * 4 + 3].flags & 0x80) == 0) {
			if (len > 0) memcpy(_endpointBuffers[ep].tx[1], data, min(len, 64));
			_bufferDescriptorTable[ep * 4 + 3].flags = (len << 16) | 0x88 | (d01 << 6);
			sent = true;
		}
	}
	return true;
}

void USBFS::handleInterrupt() {
	if (U1IRbits.URSTIF) {
  		addEndpoint(0, EP_IN, EP_CTL);
  		addEndpoint(0, EP_OUT, EP_CTL);
		U1IEbits.IDLEIE = 1;
		U1IEbits.TRNIE = 1;
		U1ADDR = 0;
		U1CONbits.PPBRST = 1;
		U1CONbits.PPBRST = 0;
	}
	if (U1IRbits.IDLEIF) {
		U1IEbits.IDLEIE = 0;
		U1IEbits.RESUMEIE = 1;
	}
	if (U1IRbits.RESUMEIF) {
		U1IEbits.IDLEIE = 1;
		U1IEbits.RESUMEIE = 0;
	}
	if (U1IRbits.TRNIF) {
		if (_onReceivePacket) {
			_onReceivePacket((uint8_t)U1STATbits.ENDPT, (uint8_t *)_endpointBuffers[U1STATbits.ENDPT].rx[U1STATbits.PPBI], (uint32_t)_bufferDescriptorTable[U1STATbits.ENDPT * 4 + U1STATbits.PPBI].flags >> 16);
		}
		_bufferDescriptorTable[U1STATbits.ENDPT * 4 + U1STATbits.PPBI].flags |= 0x80;
		switch(U1STATbits.ENDPT) {
			case 0: U1EP0bits.EPSTALL=0; break;
		}
		U1CONbits.TOKBUSY=0;
	}
	if (U1EIR) {
		report(U1EIR);
	}
	U1IR = 0xFF;
	clearIntFlag(_USB_IRQ);
//pinMode(PIN_LED3, OUTPUT); digitalWrite(PIN_LED3, HIGH);
}

bool USBFS::setAddress(uint8_t address) {
	U1ADDR = address;
	return true;
}
