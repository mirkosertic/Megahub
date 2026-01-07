
import './components/header/component.js';
import './components/blockly/component.js';
import './components/luapreview/component.js';
import './components/ui/component.js';
import './components/logger/component.js';
import './components/portstatus/component.js';

var blocklyEditor = null;
var luaPreview = null;
var uiComponents = null;

// Functions

function generateCode() {
	const code = blocklyEditor.generateLUAPreview();

	luaPreview.highlightCode(code);
	return code;
};

function syntaxCheck() {
	const luaCode = generateCode();

	fetch("/syntaxcheck", {
		method : "PUT",
		body : luaCode,
		headers : {
				   "Content-type" : "text/x-lua; charset=UTF-8",
				   },
	})
		.then((response) => response.json())
		.then((response) => {
			alert("Syntax Check Result:\nSuccess: " + response.success + "\nParse Time: " + response.parseTime + "ms\nError Message: " + response.errorMessage);
		});
};

function executeCode() {
	const luaCode = generateCode();

	fetch("/execute", {
		method : "PUT",
		body : luaCode,
		headers : {
				   "Content-type" : "text/x-lua; charset=UTF-8",
				   },
	})
		.then((response) => response.json())
		.then((response) => {
			uiComponents.clear();
			alert("Syntax Check Result:\nSuccess: " + response.success + "\nParse Time: " + response.parseTime + "ms\nError Message: " + response.errorMessage);
		});
};

function stopCode() {

	fetch("/stop", {
		method : "PUT",
		body : '',
		headers : {
				   "Content-type" : "text/x-lua; charset=UTF-8",
				   },
	})
		.then((response) => response.json())
		.then((response) => {
			console.log("Stop command sent, response: " + JSON.stringify(response));
		});
}

document.getElementById("reset").addEventListener("click", () => {
	blocklyEditor.clearWorkspace();
});

document.getElementById("syntaxcheck").addEventListener("click", () => {
	syntaxCheck();
});

document.getElementById("execute").addEventListener("click", () => {
	executeCode();
});

document.getElementById("stop").addEventListener("click", () => {
	stopCode();
});

const STRORAGE_KEY = 'blockly_robot_workspace';

// Workspace als XML speichern
function saveWorkspace() {
	try {
		const xmlText = blocklyEditor.generateXML();

		if (!import.meta.env.DEV) {
			// Write model to backend
			fetch("model.xml", {
				method : "PUT",
				body : xmlText,
				headers : {
						   "Content-type" : "application/xml; charset=UTF-8",
						   },
			})
				.then((response) => {
					console.log("Got response from backend : " + JSON.stringify(response));
				});

			const luaCode = generateCode();
			// Write code to backend
			fetch("program.lua", {
				method : "PUT",
				body : luaCode,
				headers : {
						   "Content-type" : "text/x-lua; charset=UTF-8",
						   },
			})
				.then((response) => {
					console.log("Got response from backend : " + JSON.stringify(response));
				});
		} else {
			localStorage.setItem(STRORAGE_KEY, xmlText);
			console.log('Workspace saved to localStorage, sending backup to backend');
		}

		return true;
	} catch (error) {
		console.error('Error while saving project:', error);
		return false;
	}
};

function loadFromLocalStorage(key) {
	try {
		console.info("Trying to load from localStorage");
		const xmlText = localStorage.getItem(key);
		if (!xmlText) {
			console.log('No data found in localStorage');
			return false;
		}

		blocklyEditor.loadXML(xmlText);

		return true;
	} catch (error) {
		console.error('Error while loading workspace:', error);
		return false;
	}
};

document.addEventListener('DOMContentLoaded', () => {
	blocklyEditor = document.getElementById('blockly');
	blocklyEditor.addChangeListener(() => {
		generateCode();
	});
	luaPreview = document.getElementById('luapreview');
	uiComponents = document.getElementById('uicomponents');
	portstatus = document.getElementById("portstatus");

	// Enable autp save every 10 seconds
	setInterval(() => {
		saveWorkspace();
	}, 10000);

	portstatus.initialize();

	if (!import.meta.env.DEV) {
		// TODO: Move this to the blockly editor initialization
		fetch("model.xml")
			.then((response) => response.text())
			.then((text) => {
				if (!blocklyEditor.loadXML(text)) {
					console.log("Error loading workspace! Blockly failed to parse?");
				}
			})
			.catch((error) => {
				console.error('Error loading model.xml:', error);
			});

		const logger = document.getElementById('logger');
		const eventSource = new EventSource('/events');

		eventSource.onerror = (event) => {
			console.error("EventSource failed:", event);
		};
		eventSource.addEventListener("log", (event) => {
			logger.addToLog(JSON.parse(event.data).message);
		});
		eventSource.addEventListener("command", (event) => {
			uiComponents.processUIEvent(JSON.parse(event.data));
		});
		eventSource.addEventListener("portstatus", (event) => {
			portstatus.setStatus(JSON.parse(event.data));
		});
	}
});
