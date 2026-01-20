import template from './component.html?raw';
import styleSheet from './style.css?raw';

class PortstatusElement extends HTMLElement {

	status = undefined;
	isCollapsed = true;

	connectedCallback() {
		const shadow = this.attachShadow({mode : 'open'})

		// Create adoptable stylesheet
		const sheet = new CSSStyleSheet()
		sheet.replaceSync(styleSheet)
		shadow.adoptedStyleSheets = [ sheet ]

		shadow.innerHTML = template;

		// Set up collapse toggle functionality
		this.setupCollapseToggle();
	};

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
		const container = this.shadowRoot.querySelector('.portstatus-container');

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
			const container = this.shadowRoot.querySelector('.portstatus-container');
			if (container) {
				container.classList.add('collapsed');
			}
		}
	}

	initialize() {
		this.updateStatus({
			ports : [
			]
		});
	};

	updateStatus(data) {
		console.debug("Got Portstatus: " + JSON.stringify(data));
		this.status = data;
		this.render();
	};

	render() {
		if (this.status && this.status.ports) {

			const grid = this.shadowRoot.getElementById('portsGrid');

			grid.innerHTML = '';

			this.status.ports.forEach(port => {
				const card = document.createElement('div');
				card.className = 'port-card';

				if (port.connected && port.device) {
					card.innerHTML = `
							<div class="port-header">
								<span class="port-label">Port ${port.id}</span>
								<div class="port-header-right">
									<div class="device-type-inline">
										<span class="device-icon-inline">${port.device.icon}</span>
										<span>${port.device.type}</span>
									</div>
									<div class="status-indicator status-connected"></div>
								</div>
							</div>
							<div class="modes-section">
								<div class="modes-label">Modes</div>
								<div class="modes-list">
									${port.device.modes.map(mode => `<span class="mode-badge">(${mode.id}) ${mode.name}</span>`).join('')}
								</div>
							</div>
						`;
				} else {
					card.innerHTML = `
							<div class="port-header">
								<span class="port-label">Port ${port.id}</span>
								<div class="status-indicator status-disconnected"></div>
							</div>
							<div class="no-device">No device connected</div>
						`;
				}

				grid.appendChild(card);
			});
		}
	};
};

customElements.define('custom-portstatus', PortstatusElement);
