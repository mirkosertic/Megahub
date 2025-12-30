import template from './component.html?raw';

class HeaderHTMLElement extends HTMLElement {

  connectedCallback() {
    this.innerHTML = template;
  };
};

customElements.define('custom-header', HeaderHTMLElement);