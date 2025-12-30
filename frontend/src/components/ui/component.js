import template from './component.html?raw';
import styleSheet from './style.css?raw';

class UIHTMLElement extends HTMLElement {

  connectedCallback() {
    const shadow = this.attachShadow({ mode: 'open' })
    
    // Create adoptable stylesheet
    const sheet = new CSSStyleSheet()
    sheet.replaceSync(styleSheet)
    shadow.adoptedStyleSheets = [sheet]
    
    shadow.innerHTML = template
  };

  processUIEvent(event) {
    if (event.type === "show_value") {
        const label = event.label || "Value";
        const format = event.format || "FORMAT_SIMPLE";
        const value = event.value || 0;

        const container = this.shadowRoot;   
      
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
    console.log("UI Event: ", event);
  };

  clear() {
    this.shadowRoot.innerHTML = template;
  }
};

customElements.define('custom-ui', UIHTMLElement);