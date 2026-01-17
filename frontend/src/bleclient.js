/**
 * BLE Web Client for ESP32 communication
 * Supports Request/Response and Event-Streaming with automatic fragmentation
 */

// UUIDs (must match ESP32)
const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
const REQUEST_CHAR_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';
const RESPONSE_CHAR_UUID = '1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e';
const EVENT_CHAR_UUID = 'd8de624e-140f-4a22-8594-e2216b84a5f2';
const CONTROL_CHAR_UUID = 'f78ebbff-c8b7-4107-93de-889a6a06d408';

// Protocol Message Types (INTERNAL - for fragmentation protocol)
const ProtocolMessageType = {
	REQUEST : 0x01,
	RESPONSE : 0x02,
	EVENT : 0x03,
	CONTROL : 0x04
};

// Control Message Types (INTERNAL - for control channel)
const ControlMessageType = {
	ACK : 0x01,
	NACK : 0x02,
	RETRY : 0x03,
	BUFFER_FULL : 0x04,
	RESET : 0x05,
	MTU_INFO : 0x06,
	REQUEST_MTU_INFO : 0x07
};

// Fragment Flags (INTERNAL)
const FLAG_LAST_FRAGMENT = 0x01;
const FLAG_ERROR = 0x02;

// Constants
const FRAGMENT_HEADER_SIZE = 5;
const DEFAULT_TIMEOUT_MS = 50000;
const MAX_RETRIES = 3;

// Verbose logging (set to false for production)
const VERBOSE_LOGGING = false;

export class BLEClient {
	constructor() {
		this.device = null;
		this.server = null;
		this.service = null;

		this.requestChar = null;
		this.responseChar = null;
		this.eventChar = null;
		this.controlChar = null;

		this.mtu = 23; // Default MTU, will be updated after connection
		this.mtuReceived = false;
		this.mtuReceivedResolve = null;
		this.connected = false;

		// Fragment management
		this.fragmentBuffers = new Map();

		// Request management
		this.pendingRequests = new Map();
		this.currentMessageId = 0;

		// Event listeners
		this.eventListeners = new Map();

		// Disconnect listener
		this.onDisconnectCallback = null;
	}

	// Logging helpers
	logVerbose(...args) {
		if (VERBOSE_LOGGING) {
			console.log('[BLE VERBOSE]', ...args);
		}
	}

	logInfo(...args) {
		console.log('[BLE]', ...args);
	}

	logWarn(...args) {
		console.warn('[BLE]', ...args);
	}

	logError(...args) {
		console.error('[BLE]', ...args);
	}

	/**
	 * Establish connection to BLE device
	 * @returns {Promise<void>}
	 */
	async connect() {
		try {
			this.logInfo('Starting BLE connection...');

			// Select device
			this.device = await navigator.bluetooth.requestDevice({
				filters : [ {services : [ SERVICE_UUID ]} ],
				optionalServices : [ SERVICE_UUID ]
			});

			this.logInfo('Device selected:', this.device.name);

			// Disconnect handler
			this.device.addEventListener('gattserverdisconnected', () => {
				this.handleDisconnect();
			});

			// Connect to GATT server
			this.server = await this.device.gatt.connect();
			this.logVerbose('GATT server connected');

			// Get service
			this.service = await this.server.getPrimaryService(SERVICE_UUID);
			this.logVerbose('Service found');

			// Get characteristics
			this.requestChar = await this.service.getCharacteristic(REQUEST_CHAR_UUID);
			this.responseChar = await this.service.getCharacteristic(RESPONSE_CHAR_UUID);
			this.eventChar = await this.service.getCharacteristic(EVENT_CHAR_UUID);
			this.controlChar = await this.service.getCharacteristic(CONTROL_CHAR_UUID);

			this.logVerbose('All characteristics found');

			// Enable notifications
			await this.responseChar.startNotifications();
			await this.eventChar.startNotifications();
			await this.controlChar.startNotifications();

			// Event listeners for incoming data
			this.logVerbose('Registering event listeners for characteristics');
			this.responseChar.addEventListener('characteristicvaluechanged',
				(event) => this.handleFragment(event.target.value, 'response'));

			this.eventChar.addEventListener('characteristicvaluechanged',
				(event) => this.handleFragment(event.target.value, 'event'));

			this.controlChar.addEventListener('characteristicvaluechanged',
				(event) => this.handleControlMessage(event.target.value));

			this.connected = true;
			this.logInfo('BLE connection successfully established');

			// Wait for MTU information from server (with timeout)
			await this.waitForMTU(2000);
			this.logInfo(`MTU: ${this.mtu} bytes (Payload: ${this.mtu - 3} bytes)`);

		} catch (error) {
			this.logError('Connection error:', error);
			throw new Error(`BLE connection failed: ${error.message}`);
		}
	}

	/**
	 * Wait for MTU information from server
	 * @param {number} timeout - Timeout in milliseconds
	 */
	async waitForMTU(timeout = 2000) {
		this.logVerbose('Waiting for MTU information from server...');

		// Create promise for MTU reception
		const mtuPromise = new Promise((resolve) => {
			this.mtuReceivedResolve = resolve;
		});

		// Timeout promise
		const timeoutPromise = new Promise((resolve) => {
			setTimeout(() => resolve('timeout'), timeout);
		});

		// Wait for MTU or timeout
		const result = await Promise.race([ mtuPromise, timeoutPromise ]);

		// If timeout, determine MTU ourselves
		if (result === 'timeout') {
			this.logWarn('No MTU info received from server (timeout), determining MTU ourselves...');
			await this.negotiateMTU();
		} else {
			this.logInfo('MTU info from server successfully received');
		}
	}

	/**
	 * Determine MTU by testing
	 */
	async negotiateMTU() {
		const testSizes = [ 517, 251, 185, 158, 131, 104, 77, 50, 23 ];

		for (const size of testSizes) {
			try {
				const testData = new Uint8Array(size - 3);
				testData.fill(0xFF);
				await this.requestChar.writeValue(testData);
				this.mtu = size;
				this.logInfo(`MTU negotiated: ${size} bytes`);
				return;
			} catch (error) {
				continue;
			}
		}

		this.mtu = 23;
		this.logWarn('MTU negotiation failed, using minimum: 23 bytes');
	}

	/**
	 * Disconnect
	 */
	async disconnect() {
		if (this.device && this.device.gatt.connected) {
			await this.device.gatt.disconnect();
		}
		this.handleDisconnect();
	}

	/**
	 * Disconnect handler
	 */
	handleDisconnect() {
		this.logInfo('Disconnected');
		this.connected = false;

		for (const [messageId, request] of this.pendingRequests) {
			request.reject(new Error('Connection closed'));
		}
		this.pendingRequests.clear();
		this.fragmentBuffers.clear();

		if (this.onDisconnectCallback) {
			this.onDisconnectCallback();
		}
	}

	/**
	 * Register disconnect callback
	 * @param {Function} callback
	 */
	onDisconnect(callback) {
		this.onDisconnectCallback = callback;
	}

	/**
	 * Process fragment (INTERNAL)
	 * @param {DataView} dataView
	 * @param {string} type - 'response' or 'event'
	 */
	handleFragment(dataView, type) {
		if (dataView.byteLength < FRAGMENT_HEADER_SIZE) {
			this.logError('Fragment too small');
			return;
		}

		// Parse header
		const protocolMsgType = dataView.getUint8(0);
		const messageId = dataView.getUint8(1);
		const fragmentNum = dataView.getUint16(2);
		const flags = dataView.getUint8(4);

		this.logVerbose(`Fragment received: ProtocolType=${protocolMsgType}, ID=${messageId}, Num=${fragmentNum}, Flags=0x${flags.toString(16)}`);

		// Extract payload
		const payload = new Uint8Array(
			dataView.buffer,
			dataView.byteOffset + FRAGMENT_HEADER_SIZE,
			dataView.byteLength - FRAGMENT_HEADER_SIZE);

		// Buffer key
		const bufferKey = `${type}-${messageId}`;

		// Initialize or extend buffer
		if (!this.fragmentBuffers.has(bufferKey)) {
			this.fragmentBuffers.set(bufferKey, {
				fragments : [],
				protocolMsgType : protocolMsgType,
				messageId : messageId,
				timestamp : Date.now()
			});
		}

		const buffer = this.fragmentBuffers.get(bufferKey);
		buffer.fragments.push(payload);
		buffer.timestamp = Date.now();

		// Last fragment?
		if (flags & FLAG_LAST_FRAGMENT) {
			this.logVerbose(`Complete message received: ID=${messageId}, Fragments=${buffer.fragments.length}`);

			// Merge fragments
			const completeData = this.mergeFragments(buffer.fragments);

			// Release buffer
			this.fragmentBuffers.delete(bufferKey);

			// Process message
			if (type === 'response') {
				this.handleResponse(messageId, completeData);
			} else if (type === 'event') {
				this.handleEvent(messageId, completeData);
			}
		}
	}

	/**
	 * Merge fragments
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
	 * Process response
	 * @param {number} messageId - Internal message ID (protocol layer)
	 * @param {Uint8Array} data - Complete response data (incl. Application Request Type)
	 */
	handleResponse(messageId, data) {
		const pending = this.pendingRequests.get(messageId);
		if (pending) {
			clearTimeout(pending.timeout);
			this.pendingRequests.delete(messageId);
			// Return complete data (incl. request type if server sends it back)
			pending.resolve(data);
		} else {
			this.logWarn(`No pending request for message ID ${messageId}`);
		}
	}

	/**
	 * Process event
	 * @param {number} messageId - Internal message ID (protocol layer)
	 * @param {Uint8Array} data - Complete event data
	 */
	handleEvent(messageId, data) {
		if (data.length === 0) {
			this.logWarn('Event received without data');
			return;
		}

		// First byte is the Application Event Type
		const appEventType = data[0];
		const eventData = data.slice(1);

		this.logVerbose(`Event received: AppEventType=${appEventType}, Size=${eventData.length}`);

		// Call event listeners
		const listeners = this.eventListeners.get(appEventType);
		if (listeners) {
			for (const listener of listeners) {
				try {
					listener(eventData);
				} catch (error) {
					this.logError('Error in event listener:', error);
				}
			}
		}

		// Wildcard listeners (for all events)
		const wildcardListeners = this.eventListeners.get('*');
		if (wildcardListeners) {
			for (const listener of wildcardListeners) {
				try {
					listener(appEventType, eventData);
				} catch (error) {
					this.logError('Error in wildcard event listener:', error);
				}
			}
		}
	}

	/**
	 * Process control message (INTERNAL)
	 * @param {DataView} dataView
	 */
	handleControlMessage(dataView) {
		if (dataView.byteLength < 1)
			return;

		const ctrlType = dataView.getUint8(0);

		this.logVerbose(`Control message received: Type=${ctrlType}, Length=${dataView.byteLength}`);

		switch (ctrlType) {
			case ControlMessageType.MTU_INFO:
				if (dataView.byteLength >= 4) {
					const mtu = (dataView.getUint8(2) << 8) | dataView.getUint8(3);
					this.logInfo(`MTU received from server: ${mtu} bytes (payload: ${mtu - 3} bytes)`);
					this.mtu = mtu;
					this.mtuReceived = true;

					if (this.mtuReceivedResolve) {
						this.mtuReceivedResolve('received');
						this.mtuReceivedResolve = null;
					}
				} else {
					this.logError('MTU_INFO too short:', dataView.byteLength);
				}
				break;

			case ControlMessageType.ACK:
				if (dataView.byteLength >= 2) {
					const messageId = dataView.getUint8(1);
					this.logVerbose(`ACK for message ${messageId}`);
				}
				break;

			case ControlMessageType.NACK:
				if (dataView.byteLength >= 2) {
					const messageId = dataView.getUint8(1);
					this.logWarn(`NACK received for message ${messageId}`);

					const pending = this.pendingRequests.get(messageId);
					if (pending) {
						clearTimeout(pending.timeout);
						this.pendingRequests.delete(messageId);
						pending.reject(new Error(`Server rejected request ${messageId} (NACK)`));
					}
				}
				break;

			case ControlMessageType.BUFFER_FULL:
				if (dataView.byteLength >= 2) {
					const messageId = dataView.getUint8(1);
					this.logWarn(`Buffer full for message ${messageId}`);

					const pending = this.pendingRequests.get(messageId);
					if (pending) {
						clearTimeout(pending.timeout);
						this.pendingRequests.delete(messageId);
						pending.reject(new Error(`Server buffer full for message ${messageId}`));
					}
				}
				break;

			case ControlMessageType.RETRY:
				if (dataView.byteLength >= 2) {
					const messageId = dataView.getUint8(1);
					this.logVerbose(`Server requests retry for message ${messageId}`);
				}
				break;

			case ControlMessageType.RESET:
				this.logInfo('Server requests reset');
				this.fragmentBuffers.clear();
				break;

			default:
				this.logWarn(`Unknown control message type: ${ctrlType}`);
		}
	}

	/**
	 * Send fragmented message (INTERNAL)
	 * @param {BLECharacteristic} characteristic
	 * @param {number} protocolMsgType - Protocol Message Type (REQUEST/RESPONSE/EVENT)
	 * @param {number} messageId - Internal message ID
	 * @param {Uint8Array} data - Complete data (incl. Application Type)
	 */
	async sendFragmented(characteristic, protocolMsgType, messageId, data) {
		const payloadSize = this.mtu - FRAGMENT_HEADER_SIZE;
		const totalFragments = Math.ceil(data.length / payloadSize) || 1;

		this.logVerbose(`Sending fragmented: ProtocolType=${protocolMsgType}, ID=${messageId}, Size=${data.length}, Fragments=${totalFragments}`);

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

			// Send fragment with ATT Write Response (BLE flow control)
			// This waits for ESP32 confirmation before sending next fragment
			await characteristic.writeValueWithResponse(fragment.slice(0, FRAGMENT_HEADER_SIZE + payload.length));
		}
	}

	/**
	 * Send request and wait for response (promise-based)
	 * @param {number} appRequestType - Application Request Type (e.g. 1=Echo, 2=GetStatus, 3=SetConfig)
	 * @param {Uint8Array|Array|string} data - Request data
	 * @param {number} timeout - Timeout in milliseconds
	 * @returns {Promise<Uint8Array>} - Response data
	 */
	async sendRequest(appRequestType, data = [], timeout = DEFAULT_TIMEOUT_MS) {
		if (!this.connected) {
			throw new Error('Not connected');
		}

		// Convert data to Uint8Array
		let payload;
		if (typeof data === 'string') {
			payload = new TextEncoder().encode(data);
		} else if (data instanceof Uint8Array) {
			payload = data;
		} else if (Array.isArray(data)) {
			payload = new Uint8Array(data);
		} else {
			throw new Error('Invalid data format');
		}

		// Request data: [Application Request Type][Payload...]
		const requestData = new Uint8Array(1 + payload.length);
		requestData[0] = appRequestType;
		requestData.set(payload, 1);

		const messageId = this.currentMessageId++;
		if (this.currentMessageId > 255)
			this.currentMessageId = 0;

		return new Promise((resolve, reject) => {
			// Set timeout
			const timeoutId = setTimeout(() => {
				this.pendingRequests.delete(messageId);
				reject(new Error(`Request timeout (${timeout}ms)`));
			}, timeout);

			// Register request
			this.pendingRequests.set(messageId, {
				resolve,
				reject,
				timeout : timeoutId,
				timestamp : Date.now()
			});

			// Send request (Protocol Type = REQUEST)
			this.sendFragmented(this.requestChar, ProtocolMessageType.REQUEST, messageId, requestData)
				.catch(error => {
					clearTimeout(timeoutId);
					this.pendingRequests.delete(messageId);
					reject(error);
				});
		});
	}

	/**
	 * Send request with automatic retry
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
				this.logInfo(`Request attempt ${attempt + 1}/${maxRetries}`);
				return await this.sendRequest(appRequestType, data, timeout);
			} catch (error) {
				lastError = error;
				this.logWarn(`Request failed (attempt ${attempt + 1}):`, error.message);

				if (attempt < maxRetries - 1) {
					const delay = 1000 * Math.pow(2, attempt);
					this.logWarn(`Waiting ${delay}ms before retry...`);
					await this.sleep(delay);
				}
			}
		}

		throw lastError;
	}

	/**
	 * Register event listener
	 * @param {number|string} appEventType - Application Event Type or '*' for all events
	 * @param {Function} callback - Callback function (data: Uint8Array)
	 */
	addEventListener(appEventType, callback) {
		if (!this.eventListeners.has(appEventType)) {
			this.eventListeners.set(appEventType, []);
		}
		this.eventListeners.get(appEventType).push(callback);

		this.logVerbose(`Event listener registered for AppEventType: ${appEventType}`);
	}

	/**
	 * Remove event listener
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
	 * Send control message (INTERNAL)
	 * @param {number} ctrlType
	 * @param {number} messageId
	 */
	async sendControlMessage(ctrlType, messageId) {
		const data = new Uint8Array([ ctrlType, messageId ]);
		await this.controlChar.writeValue(data);
	}

	/**
	 * Helper function: Sleep
	 * @param {number} ms
	 * @returns {Promise<void>}
	 */
	sleep(ms) {
		return new Promise(resolve => setTimeout(resolve, ms));
	}

	/**
	 * Connection status
	 * @returns {boolean}
	 */
	isConnected() {
		return this.connected && this.device && this.device.gatt.connected;
	}
}

export const APP_REQUEST_TYPE_STOP_PROGRAM = 0x01;
export const APP_REQUEST_TYPE_GET_PROJECT_FILE = 0x02;
export const APP_REQUEST_TYPE_PUT_PROJECT_FILE = 0x03;
export const APP_REQUEST_TYPE_DELETE_PROJECT = 0x04;
export const APP_REQUEST_TYPE_SYNTAX_CHECK = 0x05;
export const APP_REQUEST_TYPE_RUN_PROGRAM = 0x06;
export const APP_REQUEST_TYPE_GET_PROJECTS = 0x07;
export const APP_REQUEST_TYPE_GET_AUTOSTART = 0x08;
export const APP_REQUEST_TYPE_PUT_AUTOSTART = 0x09;
export const APP_REQUEST_TYPE_READY_FOR_EVENTS = 0x0A;

export const APP_EVENT_TYPE_LOG = 0x01;
export const APP_EVENT_TYPE_PORTSTATUS = 0x02;
export const APP_EVENT_TYPE_COMMAND = 0x03;
export const APP_EVENT_TYPE_BTCLASSICDEVICES = 0x04;
