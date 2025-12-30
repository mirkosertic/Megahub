import './components/header/component.js';
import './components/files/component.js';
import './components/logger/component.js';

document.addEventListener('DOMContentLoaded', () => {
    if (!import.meta.env.DEV) {
        const logger = document.getElementById('logger');

        const eventSource = new EventSource('/events');
        eventSource.onerror = (event) => {
            console.error("EventSource failed:", event);
        };
        eventSource.addEventListener("log", (event) => {
            logger.addToLog(JSON.parse(event.data).message);
        });
    }
});

