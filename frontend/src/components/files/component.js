import template from './component.html?raw';

class FilesHTMLElement extends HTMLElement {

  connectedCallback() {
    this.innerHTML = template;
  };
};

customElements.define('custom-files', FilesHTMLElement);