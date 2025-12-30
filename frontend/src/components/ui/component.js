import template from './component.html?raw';

class UIHTMLElement extends HTMLElement {

  connectedCallback() {
    this.innerHTML = template;
  };

  processUIEvent(event) {
    if (event.type === "show_value") {
        const label = event.label || "Value";
        const format = event.format || "FORMAT_SIMPLE";
        const value = event.value || 0;

        var element = this.querySelector('[data-label="' + label + '"]');
        if (element === null) {
            // Create new element
            element = document.createElement('div');
            element.className = 'ui-value-entry';
            element.setAttribute('data-label', label);
            element.innerHTML = `${value}`;

            this.appendChild(element);
        } else {
            element.innerHTML = `${value}`;
        }
    }
    console.log("UI Event: ", event);
  };

  clear() {
    this.innerHTML = template;
  }
};

customElements.define('custom-ui', UIHTMLElement);