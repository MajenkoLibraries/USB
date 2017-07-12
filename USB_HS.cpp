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

#ifdef __PIC32MZ__

#include <USB.h>

#define KVA_TO_PA(v)  ((v) & 0x1fffffff)
#define PA_TO_KVA0(pa)  ((pa) | 0x80000000)  // cachable
#define PA_TO_KVA1(pa)  ((pa) | 0xa000000

/*-------------- USB FS ---------------*/

USBHS *USBHS::_this;

#define WFB(X) (((X) + 3) / 4)

#ifdef PIN_LED_TX
static volatile uint32_t gTXLedTimeout = 0;
# define TXOn() digitalWrite(PIN_LED_TX, HIGH); gTXLedTimeout = millis();
static void TXLedSwitchOff(int id, void *tptr) {
    if (gTXLedTimeout > 0) {
        if (millis() - gTXLedTimeout >= 10) {
            digitalWrite(PIN_LED_TX, LOW);
            gTXLedTimeout = 0;
        }
    }
}
#else
# define TXOn()
#endif

#ifdef PIN_LED_RX
static volatile uint32_t gRXLedTimeout = 0;
# define RXOn() digitalWrite(PIN_LED_RX, HIGH); gRXLedTimeout = millis();
static void RXLedSwitchOff(int id, void *tptr) {
    if (gRXLedTimeout > 0) {
        if (millis() - gRXLedTimeout >= 10) {
            digitalWrite(PIN_LED_RX, LOW);
            gRXLedTimeout = 0;
        }
    }
}
#else
# define RXOn()
#endif


bool USBFS::enableUSB() {
#ifdef PIN_LED_TX
    pinMode(PIN_LED_TX, OUTPUT);
    digitalWrite(PIN_LED_TX, LOW);
    createTask(TXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

#ifdef PIN_LED_RX
    pinMode(PIN_LED_RX, OUTPUT);
    digitalWrite(PIN_LED_RX, LOW);
    createTask(RXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

    clearIntFlag(_USB_VECTOR);
    setIntVector(_USB_VECTOR, _usbInterrupt);
    setIntPriority(_USB_VECTOR, 6, 0);
    setIntEnable(_USB_VECTOR);

    USBCSR0bits.SOFTCONN = 1;        // D+/D- active
    USBCSR0bits.HSEN = 0;           // Full speed negotiation
    USBCSR0bits.FUNC = 0;           // Address 0

    USBCSR2bits.RESETIE = 1;

#if defined(USBCRCON)
    USBCRCONbits.USBIE = 1;
#endif

    addEndpoint(0, EP_IN, EP_CTL, 64, _ctlRxA, _ctlRxB);
    addEndpoint(0, EP_OUT, EP_CTL, 64, _ctlTxA, _ctlTxB);

	return true;
}

bool USBHS::enableUSB() {
#ifdef PIN_LED_TX
    pinMode(PIN_LED_TX, OUTPUT);
    digitalWrite(PIN_LED_TX, LOW);
    createTask(TXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

#ifdef PIN_LED_RX
    pinMode(PIN_LED_RX, OUTPUT);
    digitalWrite(PIN_LED_RX, LOW);
    createTask(RXLedSwitchOff, 10, TASK_ENABLE, NULL);
#endif

    setIntVector(_USB_VECTOR, _usbInterrupt);
    setIntPriority(_USB_VECTOR, 6, 0);
    clearIntFlag(_USB_VECTOR);
    setIntEnable(_USB_VECTOR);

    IFS4bits.USBIF = 0;
    IEC4bits.USBIE = 1;


    USBCSR0bits.SOFTCONN = 1;        // D+/D- active
    USBCSR0bits.HSEN = 1;           // High speed negotiation
    USBCSR0bits.FUNC = 0;           // Address 0

    USBCSR2bits.RESETIE = 1;

#if defined(USBCRCON)
    USBCRCONbits.USBIE = 1;
#endif

    addEndpoint(0, EP_IN, EP_CTL, 64, _ctlRxA, _ctlRxB);
    addEndpoint(0, EP_OUT, EP_CTL, 64, _ctlTxA, _ctlTxB);

	return true;
}

bool USBHS::disableUSB() {
	return true;
}
	

bool USBHS::addEndpoint(uint8_t id, uint8_t direction, uint8_t type, uint32_t size, uint8_t *a, uint8_t *b) {
	if (id > 7) return false;

    uint8_t sz = 0;
    if (size <= 8192)   sz = 0b1101;
    if (size <= 4096)   sz = 0b1100;
    if (size <= 2048)   sz = 0b1011;
    if (size <= 1024)   sz = 0b1010;
    if (size <= 512)    sz = 0b1001;
    if (size <= 256)    sz = 0b1000;
    if (size <= 128)    sz = 0b0111;
    if (size <= 64)     sz = 0b0110;
    if (size <= 32)     sz = 0b0101;
    if (size <= 16)     sz = 0b0100;
    if (size <= 8)      sz = 0b0011;

    if (id == 0) {
        USBCSR1bits.EP0IE = 1;
        USBE0CSR0bits.TXMAXP = size;
        if (direction == EP_IN) {
            _endpointBuffers[0].rx[0] = a;
            _endpointBuffers[0].rx[1] = b;
        } else {
            _endpointBuffers[0].tx[0] = a;
            _endpointBuffers[0].tx[1] = b;
        }
        _endpointBuffers[0].size = 64;

    } else {

        uint8_t ep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = id;


        USBIENCSR0bits.TXMAXP = size;

        switch (type) {
            case EP_CTL: USBIENCSR3bits.PROTOCOL = 0b00; break;
            case EP_ISO: USBIENCSR3bits.PROTOCOL = 0b01; break;
            case EP_BLK: USBIENCSR3bits.PROTOCOL = 0b10; break;
            case EP_INT: USBIENCSR3bits.PROTOCOL = 0b11; break;
        }

        _endpointBuffers[id].size = size;

        if (direction == EP_IN) {
            _endpointBuffers[id].rx[0] = a;
            _endpointBuffers[id].rx[1] = b;
            USBFIFOAbits.RXFIFOAD = _fifoOffset;
            USBIENCSR0bits.CLRDT = 1;
            USBOTGbits.RXFIFOSZ = sz;

            _fifoOffset += size / 8;
            switch (id) {
                case 1: USBCSR2bits.EP1RXIE = 1;
                case 2: USBCSR2bits.EP2RXIE = 1;
                case 3: USBCSR2bits.EP3RXIE = 1;
                case 4: USBCSR2bits.EP4RXIE = 1;
                case 5: USBCSR2bits.EP5RXIE = 1;
                case 6: USBCSR2bits.EP6RXIE = 1;
                case 7: USBCSR2bits.EP7RXIE = 1;
            }

        } else if (direction == EP_OUT) {
            _endpointBuffers[id].tx[0] = a;
            _endpointBuffers[id].tx[1] = b;
            USBFIFOAbits.TXFIFOAD = _fifoOffset;
            USBIENCSR0bits.CLRDT = 1;
            USBOTGbits.RXFIFOSZ = sz;

            _fifoOffset += size / 8;
            switch (id) {
                case 1: USBCSR1bits.EP1TXIE = 1;
                case 2: USBCSR1bits.EP2TXIE = 1;
                case 3: USBCSR1bits.EP3TXIE = 1;
                case 4: USBCSR1bits.EP4TXIE = 1;
                case 5: USBCSR1bits.EP5TXIE = 1;
                case 6: USBCSR1bits.EP6TXIE = 1;
                case 7: USBCSR1bits.EP7TXIE = 1;
            }
        }

        if (type == EP_ISO) {
            USBIENCSR1bits.ISO = 1;
        } else {
            USBIENCSR1bits.ISO = 0;
        }


        USBIENCSR3bits.SPEED = 0b01; // High speed


        USBCSR3bits.ENDPOINT = ep;
    }
	return true;
}

bool USBHS::canEnqueuePacket(uint8_t ep) {
    if (ep == 0) {
        return (USBE0CSR0bits.TXRDY == 0);
    }

    bool rdy = false;
    uint8_t oep = USBCSR3bits.ENDPOINT;
    USBCSR3bits.ENDPOINT = ep;
    rdy = (USBIENCSR0bits.TXPKTRDY == 0);
    USBCSR3bits.ENDPOINT = oep;
    
    return rdy;
}

bool USBHS::enqueuePacket(uint8_t ep, const uint8_t *data, uint32_t len) {
    if (!canEnqueuePacket(ep)) return false;

    volatile uint8_t *fifo = NULL;
    switch (ep) {
        case 0: fifo = (uint8_t *)&USBFIFO0; break;
        case 1: fifo = (uint8_t *)&USBFIFO1; break;
        case 2: fifo = (uint8_t *)&USBFIFO2; break;
        case 3: fifo = (uint8_t *)&USBFIFO3; break;
        case 4: fifo = (uint8_t *)&USBFIFO4; break;
        case 5: fifo = (uint8_t *)&USBFIFO5; break;
        case 6: fifo = (uint8_t *)&USBFIFO6; break;
        case 7: fifo = (uint8_t *)&USBFIFO7; break;
    }
    if (fifo == NULL) return false;

    for (uint32_t i = 0; i < len; i++) {
        *fifo = data[i];
    }

    if (ep == 0) {
        USBE0CSR0bits.TXRDY = 1; 
    } else {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = ep;
        USBIENCSR0bits.MODE = 1;
        USBIENCSR0bits.TXPKTRDY = 1; 
        USBCSR3bits.ENDPOINT = oep;
    }
	return true;
}

bool USBHS::sendBuffer(uint8_t ep, const uint8_t *data, uint32_t len) {
    uint32_t remain = len;
    uint32_t pos = 0;

    uint32_t psize = _endpointBuffers[ep].size;

    if (len == 0) {
        while (1) {
            if (canEnqueuePacket(ep)) {
                enqueuePacket(ep, NULL, 0);
                return true;
            }
        }
    }

    while (remain > 0) {
        if (canEnqueuePacket(ep)) {
            uint32_t toSend = min(remain, psize);
            enqueuePacket(ep, &data[pos], toSend);
            pos += toSend;
            remain -= toSend;
        }
    }

    return true;
}

void USBHS::handleInterrupt() {
    uint32_t csr0 = USBCSR0;
    bool isEP0IF = (csr0 & (1<<16)) ? true : false;
    bool isEP1TXIF = (csr0 & (1<<17)) ? true : false;
    bool isEP2TXIF = (csr0 & (1<<18)) ? true : false;
    bool isEP3TXIF = (csr0 & (1<<19)) ? true : false;
    bool isEP4TXIF = (csr0 & (1<<20)) ? true : false;
    bool isEP5TXIF = (csr0 & (1<<21)) ? true : false;
    bool isEP6TXIF = (csr0 & (1<<22)) ? true : false;
    bool isEP7TXIF = (csr0 & (1<<23)) ? true : false;
    uint32_t csr1 = USBCSR1;
    bool isEP1RXIF = (csr1 & (1 << 1)) ? true : false;
    bool isEP2RXIF = (csr1 & (1 << 2)) ? true : false;
    bool isEP3RXIF = (csr1 & (1 << 3)) ? true : false;
    bool isEP4RXIF = (csr1 & (1 << 4)) ? true : false;
    bool isEP5RXIF = (csr1 & (1 << 5)) ? true : false;
    bool isEP6RXIF = (csr1 & (1 << 6)) ? true : false;
    bool isEP7RXIF = (csr1 & (1 << 7)) ? true : false;
    uint32_t csr2 = USBCSR2;
    bool __attribute__((unused)) isRESUMEIF = (csr2 & (1 << 17)) ? true : false;
    bool isRESETIF = (csr2 & (1 << 18)) ? true : false;
    bool __attribute__((unused)) isSOFIF = (csr2 & (1 << 19)) ? true : false;
    bool __attribute__((unused)) isCONNIF = (csr2 & (1 << 20)) ? true : false;
    bool __attribute__((unused)) isDISCONIF = (csr2 & (1 << 21)) ? true : false;
    bool __attribute__((unused)) isSESSRQIF = (csr2 & (1 << 22)) ? true : false;
    bool __attribute__((unused)) isVBUSERRIF = (csr2 & (1 << 23)) ? true : false;
#ifdef DEBUG
    if (isEP0IF) Serial.println("EP0IF");
    if (isEP1TXIF) Serial.println("EP1TXIF");
    if (isEP2TXIF) Serial.println("EP2TXIF");
    if (isEP3TXIF) Serial.println("EP3TXIF");
    if (isEP4TXIF) Serial.println("EP4TXIF");
    if (isEP5TXIF) Serial.println("EP5TXIF");
    if (isEP6TXIF) Serial.println("EP6TXIF");
    if (isEP7TXIF) Serial.println("EP7TXIF");
    if (isEP1RXIF) Serial.println("EP1RXIF");
    if (isEP2RXIF) Serial.println("EP2RXIF");
    if (isEP3RXIF) Serial.println("EP3RXIF");
    if (isEP4RXIF) Serial.println("EP4RXIF");
    if (isEP5RXIF) Serial.println("EP5RXIF");
    if (isEP6RXIF) Serial.println("EP6RXIF");
    if (isEP7RXIF) Serial.println("EP7RXIF");
    if (isRESUMEIF) Serial.println("RESUMEIF");
    if (isRESETIF) Serial.println("RESETIF");
    if (isSOFIF) Serial.println("SOFIF");
    if (isDISCONIF) Serial.println("DISCONIF");
    if (isSESSRQIF) Serial.println("SESSRQIF");
    if (isVBUSERRIF) Serial.println("VBUSERRIF");
#endif
    if (isRESETIF) {
        addEndpoint(0, EP_IN, EP_CTL, 64, _ctlRxA, _ctlRxB);
        addEndpoint(0, EP_OUT, EP_CTL, 64, _ctlTxA, _ctlTxB);
    }

    volatile uint8_t *fifo;
    if (isEP0IF) {
        if (USBE0CSR0bits.RXRDY) {

            uint32_t pktlen = USBE0CSR2bits.RXCNT;

            fifo = (uint8_t *)&USBFIFO0;
            for (uint32_t i = 0; i < pktlen; i++) {
                _endpointBuffers[0].rx[0][i] = *(fifo + (i & 3));
            }

            USBE0CSR0bits.RXRDYC = 1;

            if (_manager) _manager->onSetupPacket(0, _endpointBuffers[0].rx[0], pktlen);
            USBE0CSR0bits.SETENDC = 1;
        } else {
            if (_manager) _manager->onInPacket(0, _endpointBuffers[0].tx[0], _endpointBuffers[0].size);
        }
    }

    if (isEP1RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 1;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;

        fifo = (uint8_t *)&USBFIFO1;
        for (uint32_t i = 0; i < pktlen; i++) {
            _endpointBuffers[1].rx[0][i] = *(fifo + (i & 3));
        }

        USBIENCSR1bits.RXPKTRDY = 0;
        if (_manager) _manager->onOutPacket(1, _endpointBuffers[1].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP2RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 2;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;

        fifo = (uint8_t *)&USBFIFO2;
        for (uint32_t i = 0; i < pktlen; i++) {
            _endpointBuffers[2].rx[0][i] = *(fifo + (i & 3));
        }

        USBIENCSR1bits.RXPKTRDY = 0;
        if (_manager) _manager->onOutPacket(2, _endpointBuffers[2].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP3RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 3;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;

        fifo = (uint8_t *)&USBFIFO3;
        for (uint32_t i = 0; i < pktlen; i++) {
            _endpointBuffers[3].rx[0][i] = *(fifo + (i & 3));
        }
        
        USBIENCSR1bits.RXPKTRDY = 0;
        if (_manager) _manager->onOutPacket(3, _endpointBuffers[3].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP4RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 4;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;

        fifo = (uint8_t *)&USBFIFO4;
        for (uint32_t i = 0; i < pktlen; i++) {
            _endpointBuffers[4].rx[0][i] = *(fifo + (i & 3));
        }
        
        USBIENCSR1bits.RXPKTRDY = 0;
        if (_manager) _manager->onOutPacket(4, _endpointBuffers[4].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP5RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 5;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;

        fifo = (uint8_t *)&USBFIFO5;
        for (uint32_t i = 0; i < pktlen; i++) {
            _endpointBuffers[5].rx[0][i] = *(fifo + (i & 3));
        }
        
        USBIENCSR1bits.RXPKTRDY = 0;
        if (_manager) _manager->onOutPacket(5, _endpointBuffers[5].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP6RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 6;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;

        fifo = (uint8_t *)&USBFIFO6;
        for (uint32_t i = 0; i < pktlen; i++) {
            _endpointBuffers[6].rx[0][i] = *(fifo + (i & 3));
        }
        
        USBIENCSR1bits.RXPKTRDY = 0;
        if (_manager) _manager->onOutPacket(6, _endpointBuffers[6].rx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP7RXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 7;

        uint32_t pktlen = USBIENCSR2bits.RXCNT;

        fifo = (uint8_t *)&USBFIFO7;
        for (uint32_t i = 0; i < pktlen; i++) {
            _endpointBuffers[7].rx[0][i] = *(fifo + (i & 3));
        }
        
        USBIENCSR1bits.RXPKTRDY = 0;
        if (_manager) _manager->onOutPacket(7, _endpointBuffers[7].tx[0], pktlen);

        USBCSR3bits.ENDPOINT = oep;
    }

    if (isEP1TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 1;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(1, _endpointBuffers[1].tx[0], _endpointBuffers[3].size);
    }
        
    if (isEP2TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 2;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(2, _endpointBuffers[2].tx[0], _endpointBuffers[3].size);
    }
        
    if (isEP3TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 3;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(3, _endpointBuffers[3].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP4TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 4;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(4, _endpointBuffers[4].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP5TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 5;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(5, _endpointBuffers[5].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP6TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 6;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(6, _endpointBuffers[6].rx[0], _endpointBuffers[3].size);
    }
        
    if (isEP7TXIF) {
        uint8_t oep = USBCSR3bits.ENDPOINT;
        USBCSR3bits.ENDPOINT = 7;
        USBIENCSR0bits.MODE = 0;
        USBCSR3bits.ENDPOINT = oep;
        if (_manager) _manager->onInPacket(7, _endpointBuffers[7].rx[0], _endpointBuffers[3].size);
    }
        


    clearIntFlag(_USB_VECTOR);
}

bool USBHS::setAddress(uint8_t address) {
    USBCSR0bits.FUNC = address & 0x7F;
	return true;
}

void USBHS::haltEndpoint(uint8_t ep) {
    uint8_t oep = USBCSR3bits.ENDPOINT;
    USBCSR3bits.ENDPOINT = ep;
    if (!USBIENCSR0bits.SENDSTALL) USBIENCSR0bits.SENDSTALL = 1;
    USBCSR3bits.ENDPOINT = oep;
}

void USBHS::resumeEndpoint(uint8_t ep) {
    uint8_t oep = USBCSR3bits.ENDPOINT;
    USBCSR3bits.ENDPOINT = ep;
    if (USBIENCSR0bits.SENDSTALL) USBIENCSR0bits.SENDSTALL = 0;
    USBCSR3bits.ENDPOINT = oep;
}


#endif
