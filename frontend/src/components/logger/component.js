import template from './component.html?raw';
import styleSheet from './style.css?raw';

const MAX_LOG_ENTRIES = 50;

class LoggerHTMLElement extends HTMLElement {

  logCount = 0;

  connectedCallback() {
    const shadow = this.attachShadow({ mode: 'open' })
    
    // Create adoptable stylesheet
    const sheet = new CSSStyleSheet()
    sheet.replaceSync(styleSheet)
    shadow.adoptedStyleSheets = [sheet]
    
    shadow.innerHTML = template
  };

  addToLog(message) {
    const logEntry = document.createElement('div');
    logEntry.className = 'log-entry';
   
    logEntry.textContent = `${message}`;
        
    const container = this.shadowRoot;    

    // Add new entry at the top
    container.insertBefore(logEntry, container.firstChild);
    this.logCount++;
    
    // Remove oldest entry if limit exceeded
    if (this.logCount > MAX_LOG_ENTRIES) {
        container.removeChild(container.lastChild);
        this.logCount--;
    }
  };
};

customElements.define('custom-logger', LoggerHTMLElement);