import template from './component.html?raw';
import styleSheet from './style.css?raw';

class UIHTMLElement extends HTMLElement {

	isCollapsed = true;

	connectedCallback() {
		const shadow = this.attachShadow({mode : 'open'})

		// Create adoptable stylesheet
		const sheet = new CSSStyleSheet()
		sheet.replaceSync(styleSheet)
		shadow.adoptedStyleSheets = [ sheet ]

		shadow.innerHTML = template

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
		const container = this.shadowRoot.querySelector('.ui-container');

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
			const container = this.shadowRoot.querySelector('.ui-container');
			if (container) {
				container.classList.add('collapsed');
			}
		}
	}

	processUIEvent(event) {
		if (event.type === "show_value") {
			const label = event.label || "Value";
			const format = event.format || "FORMAT_SIMPLE";
			const value = event.value || 0;

			const container = this.shadowRoot.getElementById('uiValuesList');
			const noValuesMsg = this.shadowRoot.querySelector('.no-values-message');

			if (!container) return;

			// Hide the "no values" message
			if (noValuesMsg) {
				noValuesMsg.classList.add('hidden');
			}

			var element = container.querySelector('[data-label="' + label + '"]');
			if (element === null) {
				// Create new element
				element = document.createElement('div');
				element.className = 'ui-value-entry';
				element.setAttribute('data-label', label);
				element.innerHTML = `${value}`;

				container.appendChild(element);
			} else {
				element.innerHTML = `${value}`;
			}
		}
	};

	clear() {
		const container = this.shadowRoot.getElementById('uiValuesList');
		const noValuesMsg = this.shadowRoot.querySelector('.no-values-message');

		if (container) {
			// Remove all ui-value-entry elements
			const entries = container.querySelectorAll('.ui-value-entry');
			entries.forEach(entry => entry.remove());
		}

		// Show the "no values" message again
		if (noValuesMsg) {
			noValuesMsg.classList.remove('hidden');
		}
	}
};

customElements.define('custom-ui', UIHTMLElement);
