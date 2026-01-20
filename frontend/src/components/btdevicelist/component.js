import template from './component.html?raw';
import styleSheet from './style.css?raw';

/**
 * Bluetooth Device List Custom Element
 * Displays discovered Bluetooth Classic devices with pairing/unpairing controls
 */
class BTDeviceListElement extends HTMLElement {

	devices = [];
	discoveryActive = false;
	isCollapsed = true;

	connectedCallback() {
		const shadow = this.attachShadow({ mode: 'open' });

		// Create adoptable stylesheet
		const sheet = new CSSStyleSheet();
		sheet.replaceSync(styleSheet);
		shadow.adoptedStyleSheets = [sheet];

		shadow.innerHTML = template;

		// Set up collapse toggle functionality
		this.setupCollapseToggle();

		this.shadowRoot.getElementById("startdiscovery").addEventListener("click", (e) => {
			e.stopPropagation();
			window.Application.startBluetoothDiscovery();
		});
	}

	/**
	 * Set up collapse toggle button
	 */
	setupCollapseToggle() {
		const collapseToggle = this.shadowRoot.getElementById('collapseToggle');
		const headerToggle = this.shadowRoot.getElementById('headerToggle');

		if (collapseToggle) {
			// Click on button
			collapseToggle.addEventListener('click', (e) => {
				e.stopPropagation();
				this.toggleCollapse();
			});

			// Click on entire header
			headerToggle.addEventListener('click', () => {
				this.toggleCollapse();
			});
		}
	}

	/**
	 * Toggle collapse state
	 */
	toggleCollapse() {
		this.isCollapsed = !this.isCollapsed;
		const container = this.shadowRoot.querySelector('.bt-devices-container');

		if (container) {
			if (this.isCollapsed) {
				container.classList.add('collapsed');
				this.dispatchEvent(new CustomEvent('accordion-collapse', { bubbles: true }));
			} else {
				container.classList.remove('collapsed');
				this.dispatchEvent(new CustomEvent('accordion-expand', { bubbles: true }));
			}
		}
	}

	/**
	 * Collapse the panel (called by accordion controller)
	 */
	collapse() {
		if (!this.isCollapsed) {
			this.isCollapsed = true;
			const container = this.shadowRoot.querySelector('.bt-devices-container');
			if (container) {
				container.classList.add('collapsed');
			}
		}
	}

	/**
	 * Initialize component with empty device list
	 */
	initialize() {
		this.updateDevices({
			devices: [],
			count: 0,
			discoveryActive: false
		});
	}

	/**
	 * Update device list with new data
	 * @param {Object} data - Device list data from backend
	 * @param {Array} data.devices - Array of device objects
	 * @param {number} data.count - Total device count
	 * @param {boolean} data.discoveryActive - Whether discovery is in progress
	 */
	updateDevices(data) {
		console.debug('Got Bluetooth Device List:', JSON.stringify(data));

		this.devices = data.devices || [];
		this.discoveryActive = data.discoveryActive || false;

		this.updateDiscoveryStatus();
		this.render();
	}

	/**
	 * Update discovery status indicator
	 */
	updateDiscoveryStatus() {
		const indicator = this.shadowRoot.getElementById('discoveryIndicator');
		const text = this.shadowRoot.getElementById('discoveryText');

		if (!indicator || !text) return;

		if (this.discoveryActive) {
			indicator.className = 'discovery-indicator active';
			text.textContent = 'Scanning...';
		} else {
			indicator.className = 'discovery-indicator idle';
			text.textContent = 'Idle';
		}
	}

	/**
	 * Render device list
	 */
	render() {
		const listContainer = this.shadowRoot.getElementById('devicesList');
		const noDevicesMsg = this.shadowRoot.getElementById('noDevices');

		if (!listContainer || !noDevicesMsg) return;

		// Clear existing content
		listContainer.innerHTML = '';

		// Show/hide "no devices" message
		if (this.devices.length === 0) {
			noDevicesMsg.classList.remove('hidden');
			return;
		} else {
			noDevicesMsg.classList.add('hidden');
		}

		// Render each device
		this.devices.forEach(device => {
			const card = this.createDeviceCard(device);
			listContainer.appendChild(card);
		});
	}

	/**
	 * Create device card element
	 * @param {Object} device - Device data
	 * @returns {HTMLElement} Device card element
	 */
	createDeviceCard(device) {
		const card = document.createElement('div');
		card.className = 'device-card' + (device.paired ? ' paired' : '');

		// Device header with name and MAC
		const header = document.createElement('div');
		header.className = 'device-header';

		const info = document.createElement('div');
		info.className = 'device-info';

		const name = document.createElement('div');
		name.className = 'device-name';
		name.textContent = device.name || 'Unknown Device';
		name.title = device.name || 'Unknown Device';

		const mac = document.createElement('div');
		mac.className = 'device-mac';
		mac.textContent = device.mac;

		info.appendChild(name);
		info.appendChild(mac);

		// Device status (type badge and paired indicator)
		const status = document.createElement('div');
		status.className = 'device-status';

		const typeBadge = document.createElement('span');
		typeBadge.className = 'device-type-badge';
		typeBadge.textContent = this.getDeviceTypeLabel(device.type);
		status.appendChild(typeBadge);

		if (device.paired) {
			const pairedBadge = document.createElement('span');
			pairedBadge.className = 'paired-indicator';
			pairedBadge.textContent = 'Paired';
			status.appendChild(pairedBadge);
		}

		header.appendChild(info);
		header.appendChild(status);

		// Device details (RSSI, CoD, last seen)
		const details = document.createElement('div');
		details.className = 'device-details';

		// RSSI indicator
		const rssiItem = document.createElement('div');
		rssiItem.className = 'device-detail-item';
		rssiItem.innerHTML = `
			<span class="device-detail-label">Signal:</span>
			${this.createRSSIIndicator(device.rssi)}
			<span class="device-detail-value">${device.rssi} dBm</span>
		`;
		details.appendChild(rssiItem);

		// CoD (Class of Device)
		const codItem = document.createElement('div');
		codItem.className = 'device-detail-item';
		codItem.innerHTML = `
			<span class="device-detail-label">CoD:</span>
			<span class="device-detail-value">0x${device.cod.toString(16).padStart(6, '0').toUpperCase()}</span>
		`;
		details.appendChild(codItem);

		// Device actions (pair/unpair buttons)
		const actions = document.createElement('div');
		actions.className = 'device-actions';

		if (device.paired) {
			// Unpair button
			const unpairBtn = document.createElement('button');
			unpairBtn.className = 'device-action-btn danger';
			unpairBtn.textContent = 'Remove Pairing';
			unpairBtn.addEventListener('click', () => this.handleUnpair(device.mac));
			actions.appendChild(unpairBtn);
		} else {
			// Pair button
			const pairBtn = document.createElement('button');
			pairBtn.className = 'device-action-btn primary';
			pairBtn.textContent = 'Pair Device';
			pairBtn.addEventListener('click', () => this.handlePair(device.mac));
			actions.appendChild(pairBtn);
		}

		// Assemble card
		card.appendChild(header);
		card.appendChild(details);
		card.appendChild(actions);

		return card;
	}

	/**
	 * Get device type label from type enum
	 * @param {number} type - Device type enum value
	 * @returns {string} Human-readable device type
	 */
	getDeviceTypeLabel(type) {
		const types = {
			0x00: 'Unknown',
			0x01: 'Computer',
			0x02: 'Phone',
			0x03: 'Audio',
			0x04: 'Peripheral',
			0x05: 'Imaging',
			0x06: 'Wearable',
			0x07: 'Toy',
			0x08: 'Health',
			0x09: 'Gamepad',
			0x0A: 'Keyboard',
			0x0B: 'Mouse',
			0x0C: 'Joystick'
		};
		return types[type] || 'Unknown';
	}

	/**
	 * Create RSSI signal strength indicator
	 * @param {number} rssi - RSSI value in dBm
	 * @returns {string} HTML string for RSSI indicator
	 */
	createRSSIIndicator(rssi) {
		// RSSI thresholds: excellent (-50), good (-60), fair (-70), poor (-80+)
		const bars = [];
		const bar1Active = rssi >= -80;
		const bar2Active = rssi >= -70;
		const bar3Active = rssi >= -60;

		bars.push(`<span class="rssi-bar bar-1${bar1Active ? ' active' : ''}"></span>`);
		bars.push(`<span class="rssi-bar bar-2${bar2Active ? ' active' : ''}"></span>`);
		bars.push(`<span class="rssi-bar bar-3${bar3Active ? ' active' : ''}"></span>`);

		return `<span class="rssi-indicator">${bars.join('')}</span>`;
	}

	/**
	 * Handle pair button click
	 * @param {string} macAddress - Device MAC address
	 */
	handlePair(macAddress) {
		console.log('Pair requested for device:', macAddress);

		window.Application.requestPairing(macAddress);
	}

	/**
	 * Handle unpair button click
	 * @param {string} macAddress - Device MAC address
	 */
	handleUnpair(macAddress) {
		console.log('Unpair requested for device:', macAddress);

		window.Application.requestRemovePairing(macAddress);
	}
};

customElements.define('custom-btdevicelist', BTDeviceListElement);
