/**
 * BLE Web Client für ESP32 Kommunikation
 * Unterstützt Request/Response und Event-Streaming mit automatischer Fragmentierung
 */

// UUIDs (müssen mit ESP32 übereinstimmen)
const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
const REQUEST_CHAR_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';
const RESPONSE_CHAR_UUID = '1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e';
const EVENT_CHAR_UUID = 'd8de624e-140f-4a22-8594-e2216b84a5f2';
const CONTROL_CHAR_UUID = 'f78ebbff-c8b7-4107-93de-889a6a06d408';

// Protocol Message Types (INTERN - für Fragmentierungs-Protokoll)
const ProtocolMessageType = {
    REQUEST: 0x01,
    RESPONSE: 0x02,
    EVENT: 0x03,
    CONTROL: 0x04
};

// Control Message Types (INTERN - für Control-Channel)
const ControlMessageType = {
    ACK: 0x01,
    NACK: 0x02,
    RETRY: 0x03,
    BUFFER_FULL: 0x04,
    RESET: 0x05,
    MTU_INFO: 0x06,
    REQUEST_MTU_INFO: 0x07
};

// Fragment Flags (INTERN)
const FLAG_LAST_FRAGMENT = 0x01;
const FLAG_ERROR = 0x02;

// Konstanten
const FRAGMENT_HEADER_SIZE = 5;
const DEFAULT_TIMEOUT_MS = 10000;
const MAX_RETRIES = 3;

class BLEClient {
    constructor() {
        this.device = null;
        this.server = null;
        this.service = null;
        
        this.requestChar = null;
        this.responseChar = null;
        this.eventChar = null;
        this.controlChar = null;
        
        this.mtu = 23; // Default MTU, wird nach Verbindung aktualisiert
        this.mtuReceived = false;
        this.mtuReceivedResolve = null;
        this.connected = false;
        
        // Fragment-Verwaltung
        this.fragmentBuffers = new Map();
        
        // Request-Verwaltung
        this.pendingRequests = new Map();
        this.currentMessageId = 0;
        
        // Event-Listener
        this.eventListeners = new Map();
        
        // Disconnect-Listener
        this.onDisconnectCallback = null;
    }
    
    /**
     * Verbindung zum BLE-Gerät herstellen
     * @returns {Promise<void>}
     */
    async connect() {
        try {
            console.log('Starte BLE-Verbindung...');
            
            // Gerät auswählen
            this.device = await navigator.bluetooth.requestDevice({
                filters: [{ services: [SERVICE_UUID] }],
                optionalServices: [SERVICE_UUID]
            });
            
            console.log('Gerät ausgewählt:', this.device.name);
            
            // Disconnect-Handler
            this.device.addEventListener('gattserverdisconnected', () => {
                this.handleDisconnect();
            });
            
            // Mit GATT Server verbinden
            this.server = await this.device.gatt.connect();
            console.log('GATT Server verbunden');
            
            // Service abrufen
            this.service = await this.server.getPrimaryService(SERVICE_UUID);
            console.log('Service gefunden');
            
            // Characteristics abrufen
            this.requestChar = await this.service.getCharacteristic(REQUEST_CHAR_UUID);
            this.responseChar = await this.service.getCharacteristic(RESPONSE_CHAR_UUID);
            this.eventChar = await this.service.getCharacteristic(EVENT_CHAR_UUID);
            this.controlChar = await this.service.getCharacteristic(CONTROL_CHAR_UUID);
            
            console.log('Alle Characteristics gefunden');
            
            // Notifications aktivieren
            await this.responseChar.startNotifications();
            await this.eventChar.startNotifications();
            await this.controlChar.startNotifications();

            // Event-Listener für eingehende Daten
            this.responseChar.addEventListener('characteristicvaluechanged', 
                (event) => this.handleFragment(event.target.value, 'response'));
            
            this.eventChar.addEventListener('characteristicvaluechanged', 
                (event) => this.handleFragment(event.target.value, 'event'));
            
            this.controlChar.addEventListener('characteristicvaluechanged', 
                (event) => this.handleControlMessage(event.target.value));
            
            this.connected = true;
            console.log('BLE-Verbindung erfolgreich hergestellt');
            
            // Auf MTU-Information vom Server warten (mit Timeout)
            await this.waitForMTU(2000);
            console.log(`MTU: ${this.mtu} bytes (Payload: ${this.mtu - 3} bytes)`);
            
        } catch (error) {
            console.error('Verbindungsfehler:', error);
            throw new Error(`BLE-Verbindung fehlgeschlagen: ${error.message}`);
        }
    }
    
    /**
     * Auf MTU-Information vom Server warten
     * @param {number} timeout - Timeout in Millisekunden
     */
    async waitForMTU(timeout = 2000) {
        console.log('Warte auf MTU-Information vom Server...');
        
        // Promise für MTU-Empfang erstellen
        const mtuPromise = new Promise((resolve) => {
            this.mtuReceivedResolve = resolve;
        });
        
        // Timeout-Promise
        const timeoutPromise = new Promise((resolve) => {
            setTimeout(() => resolve('timeout'), timeout);
        });
        
        // Warten auf MTU oder Timeout
        const result = await Promise.race([mtuPromise, timeoutPromise]);
        
        // Falls Timeout, selbst ermitteln
        if (result === 'timeout') {
            console.warn('Keine MTU-Info vom Server empfangen (Timeout), ermittle selbst...');
            await this.negotiateMTU();
        } else {
            console.log('MTU-Info vom Server erfolgreich empfangen');
        }
    }
    
    /**
     * MTU durch Testen ermitteln
     */
    async negotiateMTU() {
        const testSizes = [517, 251, 185, 158, 131, 104, 77, 50, 23];
        
        for (const size of testSizes) {
            try {
                const testData = new Uint8Array(size - 3);
                testData.fill(0xFF);
                await this.requestChar.writeValue(testData);
                this.mtu = size;
                console.log(`MTU-Test erfolgreich: ${size} bytes`);
                return;
            } catch (error) {
                continue;
            }
        }
        
        this.mtu = 23;
        console.warn('MTU-Aushandlung fehlgeschlagen, verwende Minimum: 23 bytes');
    }
    
    /**
     * Verbindung trennen
     */
    async disconnect() {
        if (this.device && this.device.gatt.connected) {
            await this.device.gatt.disconnect();
        }
        this.handleDisconnect();
    }
    
    /**
     * Disconnect-Handler
     */
    handleDisconnect() {
        console.log('Verbindung getrennt');
        this.connected = false;
        
        for (const [messageId, request] of this.pendingRequests) {
            request.reject(new Error('Verbindung getrennt'));
        }
        this.pendingRequests.clear();
        this.fragmentBuffers.clear();
        
        if (this.onDisconnectCallback) {
            this.onDisconnectCallback();
        }
    }
    
    /**
     * Disconnect-Callback registrieren
     * @param {Function} callback 
     */
    onDisconnect(callback) {
        this.onDisconnectCallback = callback;
    }
    
    /**
     * Fragment verarbeiten (INTERN)
     * @param {DataView} dataView 
     * @param {string} type - 'response' oder 'event'
     */
    handleFragment(dataView, type) {
        if (dataView.byteLength < FRAGMENT_HEADER_SIZE) {
            console.error('Fragment zu klein');
            return;
        }
        
        // Header parsen
        const protocolMsgType = dataView.getUint8(0);
        const messageId = dataView.getUint8(1);
        const fragmentNum = dataView.getUint16(2);
        const flags = dataView.getUint8(4);
        
        console.log(`Fragment empfangen: ProtocolType=${protocolMsgType}, ID=${messageId}, Num=${fragmentNum}, Flags=0x${flags.toString(16)}`);
        
        // Payload extrahieren
        const payload = new Uint8Array(
            dataView.buffer,
            dataView.byteOffset + FRAGMENT_HEADER_SIZE,
            dataView.byteLength - FRAGMENT_HEADER_SIZE
        );
        
        // Buffer-Key
        const bufferKey = `${type}-${messageId}`;
        
        // Buffer initialisieren oder erweitern
        if (!this.fragmentBuffers.has(bufferKey)) {
            this.fragmentBuffers.set(bufferKey, {
                fragments: [],
                protocolMsgType: protocolMsgType,
                messageId: messageId,
                timestamp: Date.now()
            });
        }
        
        const buffer = this.fragmentBuffers.get(bufferKey);
        buffer.fragments.push(payload);
        buffer.timestamp = Date.now();
        
        // Letztes Fragment?
        if (flags & FLAG_LAST_FRAGMENT) {
            console.log(`Vollständige Nachricht empfangen: ID=${messageId}, Fragments=${buffer.fragments.length}`);
            
            // Fragmente zusammenführen
            const completeData = this.mergeFragments(buffer.fragments);
            
            // Buffer freigeben
            this.fragmentBuffers.delete(bufferKey);
            
            // Nachricht verarbeiten
            if (type === 'response') {
                this.handleResponse(messageId, completeData);
            } else if (type === 'event') {
                this.handleEvent(messageId, completeData);
            }
        }
    }
    
    /**
     * Fragmente zusammenführen
     * @param {Array<Uint8Array>} fragments 
     * @returns {Uint8Array}
     */
    mergeFragments(fragments) {
        const totalLength = fragments.reduce((sum, f) => sum + f.length, 0);
        const result = new Uint8Array(totalLength);
        let offset = 0;
        
        for (const fragment of fragments) {
            result.set(fragment, offset);
            offset += fragment.length;
        }
        
        return result;
    }
    
    /**
     * Response verarbeiten
     * @param {number} messageId - Interne Message-ID (Protokoll-Ebene)
     * @param {Uint8Array} data - Vollständige Response-Daten (inkl. Application Request Type)
     */
    handleResponse(messageId, data) {
        const pending = this.pendingRequests.get(messageId);
        if (pending) {
            clearTimeout(pending.timeout);
            this.pendingRequests.delete(messageId);
            // Komplette Daten zurückgeben (inkl. Request Type falls Server ihn zurücksendet)
            pending.resolve(data);
        } else {
            console.warn(`Keine ausstehende Anfrage für Message ID ${messageId}`);
        }
    }
    
    /**
     * Event verarbeiten
     * @param {number} messageId - Interne Message-ID (Protokoll-Ebene)
     * @param {Uint8Array} data - Vollständige Event-Daten
     */
    handleEvent(messageId, data) {
        if (data.length === 0) {
            console.warn('Event ohne Daten empfangen');
            return;
        }
        
        // Erstes Byte ist der Application Event Type
        const appEventType = data[0];
        const eventData = data.slice(1);
        
        console.log(`Event empfangen: AppEventType=${appEventType}, Size=${eventData.length}`);
        
        // Event-Listener aufrufen
        const listeners = this.eventListeners.get(appEventType);
        if (listeners) {
            for (const listener of listeners) {
                try {
                    listener(eventData);
                } catch (error) {
                    console.error('Fehler im Event-Listener:', error);
                }
            }
        }
        
        // Wildcard-Listener (für alle Events)
        const wildcardListeners = this.eventListeners.get('*');
        if (wildcardListeners) {
            for (const listener of wildcardListeners) {
                try {
                    listener(appEventType, eventData);
                } catch (error) {
                    console.error('Fehler im Wildcard-Event-Listener:', error);
                }
            }
        }
    }
    
    /**
     * Control Message verarbeiten (INTERN)
     * @param {DataView} dataView 
     */
    handleControlMessage(dataView) {
        if (dataView.byteLength < 1) return;
        
        const ctrlType = dataView.getUint8(0);
        
        console.log(`Control Message empfangen: Type=${ctrlType}, Length=${dataView.byteLength}`);
        
        switch (ctrlType) {
            case ControlMessageType.MTU_INFO:
                if (dataView.byteLength >= 4) {
                    const mtu = (dataView.getUint8(2) << 8) | dataView.getUint8(3);
                    console.log(`✓ MTU-Info vom Server: ${mtu} bytes (Nutzlast: ${mtu - 3} bytes)`);
                    this.mtu = mtu;
                    this.mtuReceived = true;
                    
                    if (this.mtuReceivedResolve) {
                        this.mtuReceivedResolve('received');
                        this.mtuReceivedResolve = null;
                    }
                } else {
                    console.error('MTU_INFO zu kurz:', dataView.byteLength);
                }
                break;
                
            case ControlMessageType.ACK:
                if (dataView.byteLength >= 2) {
                    const messageId = dataView.getUint8(1);
                    console.log(`✓ ACK für Message ${messageId}`);
                }
                break;
                
            case ControlMessageType.NACK:
                if (dataView.byteLength >= 2) {
                    const messageId = dataView.getUint8(1);
                    console.warn(`✗ NACK für Message ${messageId}`);
                    
                    const pending = this.pendingRequests.get(messageId);
                    if (pending) {
                        clearTimeout(pending.timeout);
                        this.pendingRequests.delete(messageId);
                        pending.reject(new Error(`Server hat Request ${messageId} abgelehnt (NACK)`));
                    }
                }
                break;
                
            case ControlMessageType.BUFFER_FULL:
                if (dataView.byteLength >= 2) {
                    const messageId = dataView.getUint8(1);
                    console.error(`✗ Buffer voll für Message ${messageId}`);
                    
                    const pending = this.pendingRequests.get(messageId);
                    if (pending) {
                        clearTimeout(pending.timeout);
                        this.pendingRequests.delete(messageId);
                        pending.reject(new Error(`Server-Buffer voll für Message ${messageId}`));
                    }
                }
                break;
                
            case ControlMessageType.RETRY:
                if (dataView.byteLength >= 2) {
                    const messageId = dataView.getUint8(1);
                    console.log(`↻ Server fordert Retry für Message ${messageId} an`);
                }
                break;
                
            case ControlMessageType.RESET:
                console.log('⟲ Server fordert Reset an');
                this.fragmentBuffers.clear();
                break;
                
            default:
                console.warn(`⚠ Unbekannter Control Type: ${ctrlType}`);
        }
    }
    
    /**
     * Fragmentierte Nachricht senden (INTERN)
     * @param {BLECharacteristic} characteristic 
     * @param {number} protocolMsgType - Protocol Message Type (REQUEST/RESPONSE/EVENT)
     * @param {number} messageId - Interne Message-ID
     * @param {Uint8Array} data - Vollständige Daten (inkl. Application Type)
     */
    async sendFragmented(characteristic, protocolMsgType, messageId, data) {
        const payloadSize = this.mtu - FRAGMENT_HEADER_SIZE;
        const totalFragments = Math.ceil(data.length / payloadSize) || 1;
        
        console.log(`Sende fragmentiert: ProtocolType=${protocolMsgType}, ID=${messageId}, Size=${data.length}, Fragments=${totalFragments}`);
        
        for (let i = 0; i < totalFragments; i++) {
            const fragment = new Uint8Array(Math.min(this.mtu, FRAGMENT_HEADER_SIZE + data.length - i * payloadSize));
            
            // Header
            fragment[0] = protocolMsgType;
            fragment[1] = messageId;
            fragment[2] = (i >> 8) & 0xFF;
            fragment[3] = i & 0xFF;
            
            // Flags
            const flags = (i === totalFragments - 1) ? FLAG_LAST_FRAGMENT : 0x00;
            fragment[4] = flags;
            
            // Payload
            const start = i * payloadSize;
            const end = Math.min(start + payloadSize, data.length);
            const payload = data.slice(start, end);
            fragment.set(payload, FRAGMENT_HEADER_SIZE);
            
            // Fragment senden
            await characteristic.writeValue(fragment.slice(0, FRAGMENT_HEADER_SIZE + payload.length));
            
            // Kleine Pause zwischen Fragmenten
            if (i < totalFragments - 1) {
                await this.sleep(10);
            }
        }
    }
    
    /**
     * Request senden und auf Response warten (Promise-basiert)
     * @param {number} appRequestType - Application Request Type (z.B. 1=Echo, 2=GetStatus, 3=SetConfig)
     * @param {Uint8Array|Array|string} data - Request-Daten
     * @param {number} timeout - Timeout in Millisekunden
     * @returns {Promise<Uint8Array>} - Response-Daten
     */
    async sendRequest(appRequestType, data = [], timeout = DEFAULT_TIMEOUT_MS) {
        if (!this.connected) {
            throw new Error('Nicht verbunden');
        }
        
        // Daten in Uint8Array konvertieren
        let payload;
        if (typeof data === 'string') {
            payload = new TextEncoder().encode(data);
        } else if (data instanceof Uint8Array) {
            payload = data;
        } else if (Array.isArray(data)) {
            payload = new Uint8Array(data);
        } else {
            throw new Error('Ungültiges Datenformat');
        }
        
        // Request-Daten: [Application Request Type][Payload...]
        const requestData = new Uint8Array(1 + payload.length);
        requestData[0] = appRequestType;
        requestData.set(payload, 1);
        
        const messageId = this.currentMessageId++;
        if (this.currentMessageId > 255) this.currentMessageId = 0;
        
        return new Promise((resolve, reject) => {
            // Timeout setzen
            const timeoutId = setTimeout(() => {
                this.pendingRequests.delete(messageId);
                reject(new Error(`Request Timeout (${timeout}ms)`));
            }, timeout);
            
            // Request registrieren
            this.pendingRequests.set(messageId, {
                resolve,
                reject,
                timeout: timeoutId,
                timestamp: Date.now()
            });
            
            // Request senden (Protocol Type = REQUEST)
            this.sendFragmented(this.requestChar, ProtocolMessageType.REQUEST, messageId, requestData)
                .catch(error => {
                    clearTimeout(timeoutId);
                    this.pendingRequests.delete(messageId);
                    reject(error);
                });
        });
    }
    
    /**
     * Request mit automatischem Retry senden
     * @param {number} appRequestType - Application Request Type
     * @param {Uint8Array|Array|string} data 
     * @param {number} maxRetries 
     * @param {number} timeout 
     * @returns {Promise<Uint8Array>}
     */
    async sendRequestWithRetry(appRequestType, data = [], maxRetries = MAX_RETRIES, timeout = DEFAULT_TIMEOUT_MS) {
        let lastError;
        
        for (let attempt = 0; attempt < maxRetries; attempt++) {
            try {
                console.log(`Request-Versuch ${attempt + 1}/${maxRetries}`);
                return await this.sendRequest(appRequestType, data, timeout);
            } catch (error) {
                lastError = error;
                console.warn(`Request fehlgeschlagen (Versuch ${attempt + 1}):`, error.message);
                
                if (attempt < maxRetries - 1) {
                    const delay = 1000 * Math.pow(2, attempt);
                    console.log(`Warte ${delay}ms vor erneutem Versuch...`);
                    await this.sleep(delay);
                }
            }
        }
        
        throw lastError;
    }
    
    /**
     * Event-Listener registrieren
     * @param {number|string} appEventType - Application Event Type oder '*' für alle Events
     * @param {Function} callback - Callback-Funktion (data: Uint8Array)
     */
    addEventListener(appEventType, callback) {
        if (!this.eventListeners.has(appEventType)) {
            this.eventListeners.set(appEventType, []);
        }
        this.eventListeners.get(appEventType).push(callback);
        
        console.log(`Event-Listener registriert für AppEventType: ${appEventType}`);
    }
    
    /**
     * Event-Listener entfernen
     * @param {number|string} appEventType 
     * @param {Function} callback 
     */
    removeEventListener(appEventType, callback) {
        const listeners = this.eventListeners.get(appEventType);
        if (listeners) {
            const index = listeners.indexOf(callback);
            if (index !== -1) {
                listeners.splice(index, 1);
            }
            
            if (listeners.length === 0) {
                this.eventListeners.delete(appEventType);
            }
        }
    }
    
    /**
     * Control Message senden (INTERN)
     * @param {number} ctrlType 
     * @param {number} messageId 
     */
    async sendControlMessage(ctrlType, messageId) {
        const data = new Uint8Array([ctrlType, messageId]);
        await this.controlChar.writeValue(data);
    }
    
    /**
     * Hilfsfunktion: Sleep
     * @param {number} ms 
     * @returns {Promise<void>}
     */
    sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
    
    /**
     * Verbindungsstatus
     * @returns {boolean}
     */
    isConnected() {
        return this.connected && this.device && this.device.gatt.connected;
    }
}

// Beispiel-Verwendung
async function connect() {
    const client = new BLEClient();
    
    try {
        await client.connect();
        console.log('Verbunden!');
        
        // Event-Listener registrieren (Application Event Type 1 = z.B. Sensor-Event)
        client.addEventListener(1, (data) => {
            console.log('Sensor-Event empfangen:', data);
        });
        
        // Wildcard-Listener
        client.addEventListener('*', (appEventType, data) => {
            console.log(`Event Type ${appEventType} empfangen:`, data);
        });
        
        client.onDisconnect(() => {
            console.log('Verbindung wurde getrennt!');
        });
        
        // Request senden (Application Request Type 1 = z.B. Echo)
        const response = await client.sendRequest(1, JSON.stringify({"name": "value"}));
        console.log('Response:', JSON.stringify(JSON.parse(new TextDecoder().decode(response))));
       
    } catch (error) {
        console.error('Fehler:', error);
    }
}

document.getElementById("connect").addEventListener("click", (ev) => {
    connect();
});
