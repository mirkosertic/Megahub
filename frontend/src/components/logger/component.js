import template from './component.html?raw';

const MAX_LOG_ENTRIES = 50;

class LoggerHTMLElement extends HTMLElement {

  logCount = 0;

  connectedCallback() {
    this.innerHTML = template;
  };

  addToLog(message) {
    const logEntry = document.createElement('div');
    logEntry.className = 'log-entry';
   
    logEntry.textContent = `${message}`;
    
    // Add new entry at the top
    this.insertBefore(logEntry, this.firstChild);
    this.logCount++;
    
    // Remove oldest entry if limit exceeded
    if (this.logCount > MAX_LOG_ENTRIES) {
        this.removeChild(this.lastChild);
        logCount--;
    }
  };
};

customElements.define('custom-logger', LoggerHTMLElement);