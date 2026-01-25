import './components/files/component.js';
import './components/logger/component.js';
import './components/sidebar-toggle/component.js';
import './components/ui/component.js';
import './components/portstatus/component.js';
import './components/blockly/component.js';
import './components/luapreview/component.js';
import './components/btdevicelist/component.js';

const mode = import.meta.env.VITE_MODE;

import {
	APP_EVENT_TYPE_COMMAND,
	APP_EVENT_TYPE_LOG,
	APP_EVENT_TYPE_PORTSTATUS,
	APP_REQUEST_TYPE_GET_AUTOSTART,
	APP_REQUEST_TYPE_GET_PROJECTS,
	APP_REQUEST_TYPE_GET_PROJECT_FILE,
	APP_REQUEST_TYPE_READY_FOR_EVENTS,
	APP_REQUEST_TYPE_STOP_PROGRAM,
	APP_REQUEST_TYPE_PUT_AUTOSTART,
	APP_REQUEST_TYPE_SYNTAX_CHECK,
	APP_REQUEST_TYPE_RUN_PROGRAM,
	APP_REQUEST_TYPE_PUT_PROJECT_FILE,
	APP_REQUEST_TYPE_DELETE_PROJECT,
	APP_EVENT_TYPE_BTCLASSICDEVICES,
	APP_REQUEST_TYPE_REQUEST_PAIRING,
	APP_REQUEST_TYPE_REMOVE_PAIRING,
	APP_REQUEST_TYPE_START_DISCOVERY,
	BLEClient
} from './bleclient.js'

const bleClient = new BLEClient();

var blocklyEditor = null;
var luaPreview = null;
var uiComponents = null;
var portstatus = null
var btdevicelist = null
var autoSaveEnabled = false;
var autoSaveIntervalId = null;

// ===== Notification System =====

const NotificationIcons = {
	success: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>`,
	error: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>`,
	warning: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>`,
	info: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/></svg>`
};

/**
 * Show a notification toast
 * @param {string} type - 'success' | 'error' | 'warning' | 'info'
 * @param {string} title - Notification title
 * @param {string} message - Notification message (optional)
 * @param {number} duration - Auto-dismiss duration in ms (0 = no auto-dismiss)
 */
function showNotification(type, title, message = '', duration = 4000) {
	const container = document.getElementById('notificationContainer');
	if (!container) return;

	const notification = document.createElement('div');
	notification.className = `notification ${type}`;
	notification.innerHTML = `
		<div class="notification-icon">${NotificationIcons[type] || NotificationIcons.info}</div>
		<div class="notification-content">
			<div class="notification-title">${title}</div>
			${message ? `<div class="notification-message">${message}</div>` : ''}
		</div>
		<button class="notification-close" aria-label="Dismiss">
			<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
		</button>
	`;

	const closeBtn = notification.querySelector('.notification-close');
	const dismiss = () => {
		notification.classList.add('hiding');
		setTimeout(() => notification.remove(), 200);
	};

	closeBtn.addEventListener('click', dismiss);

	container.appendChild(notification);

	if (duration > 0) {
		setTimeout(dismiss, duration);
	}

	return notification;
}

// ===== Confirmation Dialog =====

let confirmDialogResolve = null;

/**
 * Show a confirmation dialog
 * @param {string} title - Dialog title
 * @param {string} message - Dialog message
 * @param {Object} options - Optional settings
 * @param {string} options.confirmText - Confirm button text (default: 'Confirm')
 * @param {string} options.cancelText - Cancel button text (default: 'Cancel')
 * @param {boolean} options.destructive - If true, confirm button is red (default: true)
 * @returns {Promise<boolean>} - Resolves to true if confirmed, false if cancelled
 */
function showConfirmDialog(title, message, options = {}) {
	const {
		confirmText = 'Confirm',
		cancelText = 'Cancel',
		destructive = true
	} = options;

	return new Promise((resolve) => {
		const overlay = document.getElementById('confirmDialog');
		const titleEl = document.getElementById('confirmDialogTitle');
		const messageEl = document.getElementById('confirmDialogMessage');
		const confirmBtn = document.getElementById('confirmDialogConfirm');
		const cancelBtn = document.getElementById('confirmDialogCancel');

		if (!overlay) {
			resolve(false);
			return;
		}

		titleEl.textContent = title;
		messageEl.textContent = message;
		confirmBtn.textContent = confirmText;
		cancelBtn.textContent = cancelText;

		// Set button style based on destructive flag
		confirmBtn.classList.toggle('primary', !destructive);

		confirmDialogResolve = resolve;
		overlay.setAttribute('aria-hidden', 'false');

		// Focus the cancel button for safety
		setTimeout(() => cancelBtn.focus(), 100);
	});
}

function closeConfirmDialog(result) {
	const overlay = document.getElementById('confirmDialog');
	if (overlay) {
		overlay.setAttribute('aria-hidden', 'true');
	}
	if (confirmDialogResolve) {
		confirmDialogResolve(result);
		confirmDialogResolve = null;
	}
}

// Set up dialog event listeners once DOM is ready
function initConfirmDialog() {
	const overlay = document.getElementById('confirmDialog');
	const confirmBtn = document.getElementById('confirmDialogConfirm');
	const cancelBtn = document.getElementById('confirmDialogCancel');

	if (!overlay || !confirmBtn || !cancelBtn) return;

	confirmBtn.addEventListener('click', () => closeConfirmDialog(true));
	cancelBtn.addEventListener('click', () => closeConfirmDialog(false));

	// Close on backdrop click
	overlay.addEventListener('click', (e) => {
		if (e.target === overlay) {
			closeConfirmDialog(false);
		}
	});

	// Close on Escape key
	document.addEventListener('keydown', (e) => {
		if (e.key === 'Escape' && overlay.getAttribute('aria-hidden') === 'false') {
			closeConfirmDialog(false);
		}
	});
}

// Make notification function globally available
window.showNotification = showNotification;
window.showConfirmDialog = showConfirmDialog;

// ===== Bluetooth Compatibility Check =====

/**
 * Show a message when Web Bluetooth API is not supported
 */
function showBluetoothUnsupportedMessage() {
	const welcomeDiv = document.querySelector('.layout-content.welcome');
	if (!welcomeDiv) return;

	welcomeDiv.innerHTML = `
		<div class="bt-unsupported">
			<div class="bt-unsupported-icon">
				<svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
					<path d="M6.5 6.5l11 11L12 23V1l5.5 5.5-11 11"/>
					<line x1="2" y1="2" x2="22" y2="22" stroke-width="2"/>
				</svg>
			</div>
			<h2 class="bt-unsupported-title">Bluetooth Not Supported</h2>
			<p class="bt-unsupported-message">
				Your browser does not support the Web Bluetooth API, which is required to connect to Megahub devices.
			</p>
			<div class="bt-unsupported-browsers">
				<h3>Supported Browsers</h3>
				<ul>
					<li><span class="browser-supported">Chrome</span> (Desktop & Android)</li>
					<li><span class="browser-supported">Edge</span> (Desktop)</li>
					<li><span class="browser-supported">Opera</span> (Desktop)</li>
					<li><span class="browser-supported">Samsung Internet</span> (Android)</li>
					<li><span class="browser-unsupported">Firefox</span> - Not supported</li>
					<li><span class="browser-unsupported">Safari</span> - Not supported</li>
				</ul>
			</div>
			<a href="https://caniuse.com/web-bluetooth" target="_blank" rel="noopener noreferrer" class="bt-unsupported-link">
				<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
					<circle cx="12" cy="12" r="10"/>
					<line x1="12" y1="16" x2="12" y2="12"/>
					<line x1="12" y1="8" x2="12.01" y2="8"/>
				</svg>
				View browser compatibility details on caniuse.com
			</a>
		</div>
	`;
}

// Functions

function generateCode() {
	const code = blocklyEditor.generateLUAPreview();

	luaPreview.highlightCode(code);
	return code;
};

function syntaxCheck() {
	const luaCode = generateCode();

	window.Application.syntaxCheck(luaCode);
};

function executeCode() {
	const luaCode = generateCode();

	window.Application.executeCode(luaCode);
};

function stopCode() {
	window.Application.stop();
}

const STRORAGE_KEY = 'blockly_robot_workspace';

// Save workspace as XML
async function saveWorkspace() {
	try {
		const xmlText = blocklyEditor.generateXML();
		await window.Application.saveProjectFile("model.xml", "application/xml; charset=UTF-8", xmlText);
		const luaCode = generateCode();
		await window.Application.saveProjectFile("program.lua", "text/x-lua; charset=UTF-8", luaCode);
		console.log("Workspace saved!");
		return true;
	} catch (error) {
		console.error('Error while saving project:', error);
		return false;
	}
};

window.Application = {

	activeProject : null,

	setInitState : function() {
		Application.setMode('btconnect');
	},

	setMode : function(mode) {
		const items = document.querySelectorAll(".dynamicvisibility");
		for (const item of items) {
			item.style.display = 'none';
			item.style.visibility = 'hidden';
			if (item.classList.contains('visible-' + mode)) {
				item.style.display = 'block';
				item.style.visibility = "visible";
			}
		}
	},

	jumpToFilesView : function() {
		window.Application.setMode('management');
		window.Application.requestProjectsAndAutostartConfig((projectList, autostartProject) => {
			document.getElementById("files").initialize(projectList, autostartProject);
		});
	},

	createProject : function(projectId) {
		console.log("Creating new empty project")
		window.Application.setMode('editor');

		window.Application.activeProject = projectId;
	},

	openProject : function(projectId) {
		console.log("Opening project " + projectId);
		window.Application.setMode('editor');

		window.Application.activeProject = projectId;

		window.Application.requestProjectFile(projectId, 'model.xml', function(projectId, filename, content) {
			if (!blocklyEditor.loadXML(content)) {
				console.log("Error loading workspace! Blockly failed to parse?");
			}
		});
	},

	deleteProject : async function(project) {
		// clang-format off
		if (mode === 'bt') {
			console.log("Deleting project " + project);
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_DELETE_PROJECT, JSON.stringify({ "project": project }));
			
			window.Application.jumpToFilesView();
		} else if (mode === 'web') {
			fetch("/project/" + encodeURIComponent(project), {
				method : "DELETE"
			})
			.then((response) => {
				console.log("Got response from backend : " + JSON.stringify(response));

				window.Application.jumpToFilesView();
			})
			.catch((error) => {
				console.error('Error deleting project:', error);
			});
		}
		// clang-format on
	},

	setAutostartProject : async function(project) {
		// clang-format off
		if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_PUT_AUTOSTART, JSON.stringify({"project" : project}));
		} else if (mode === 'web') {
			fetch("/autostart", {
				method : "PUT",
				body : JSON.stringify({"project" : project}),
				headers : {
							"Content-type" : "application/json; charset=UTF-8",
						  },
			}).then((response) => {
				console.log("Got response from backend : " + JSON.stringify(response));
			})
			.catch((error) => {
				console.error('Error deleting project:', error);
			});
		}
		// clang-format off
	},

	requestProjectFile : async function(projectId, filename, callback) {
		console.log("Requesting file " + filename + " of project " + projectId);

		if (mode === 'dev') {
			if (filename === 'model.xml') {

				const xmlText = localStorage.getItem(STRORAGE_KEY);
				if (!xmlText) {
					console.log('No data found in localStorage');
					return false;
				}

				callback(projectId, filename, xmlText);
				return;
			}

			callback(projectId, filename, '');
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_PROJECT_FILE, JSON.stringify({"project" : projectId, "filename" : filename}));
			const contentResponse = new TextDecoder().decode(response);

			callback(projectId, filename, contentResponse);
		} else if (mode === 'web') {
			const response = await fetch("/project/" + encodeURIComponent(projectId) + "/" + filename);
			const responseText = await response.text();
			callback(projectId, filename, responseText);
		}
	},

	saveProjectFile: async function (filename, contentType, content) {
		var projectId = window.Application.activeProject;
		// clang-format off
		if (mode === 'dev') {
			// Dev-specific logic
			if (filename === 'model.xml') {
				localStorage.setItem(STRORAGE_KEY, content);
			}
		} else if (mode === 'bt') {
			console.log("Saving file " + filename + " to project " + projectId + " (streaming)");
			try {
				// Use streaming protocol for memory-efficient uploads
				await bleClient.uploadFileStreaming(projectId, filename, content,
					(uploaded, total) => {
						const percent = Math.round((uploaded / total) * 100);
						console.log(`Upload progress: ${percent}% (${uploaded}/${total} bytes)`);
					}
				);
				console.log("File saved successfully via streaming protocol");
			} catch (error) {
				console.warn("Streaming upload failed, falling back to legacy JSON:", error.message);
				// Fallback to legacy JSON method for compatibility
				const response = await bleClient.sendRequest(APP_REQUEST_TYPE_PUT_PROJECT_FILE, JSON.stringify({"project" : projectId, "filename" : filename, "content": content}));
				console.log("File saved via legacy JSON method");
			}
		} else if (mode === 'web') {
			fetch("/project/" + encodeURIComponent(projectId) + "/" + filename, {
				method : "PUT",
				body : content,
				headers : {
						   "Content-type" : contentType,
						   },
			})
			.then((response) => {
				console.log("Got response from backend : " + JSON.stringify(response));
			});
		}
		// clang-format on
	},

	syntaxCheck : async function(luaCode) {
		// clang-format off
		if (mode === 'dev') {
			showNotification('info', 'Syntax Check', 'Running in dev mode - no backend available');
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_SYNTAX_CHECK, JSON.stringify({ "luaScript": luaCode }));
			const contentResponse = new TextDecoder().decode(response);
			const responsejson = JSON.parse(contentResponse);
			if (responsejson.result) {
				showNotification('success', 'Syntax Check Passed', `Parse time: ${responsejson.parseTime}ms`);
			} else {
				showNotification('error', 'Syntax Check Failed', responsejson.errorMessage || 'Unknown error');
			}
		} else if (mode === 'web') {
			fetch("/syntaxcheck", {
				method : "PUT",
				body : luaCode,
				headers : {
						   "Content-type" : "text/x-lua; charset=UTF-8",
						   },
			})
			.then((response) => response.json())
			.then((response) => {
				if (response.success) {
					showNotification('success', 'Syntax Check Passed', `Parse time: ${response.parseTime}ms`);
				} else {
					showNotification('error', 'Syntax Check Failed', response.errorMessage || 'Unknown error');
				}
			});
		}
		// clang-format on
	},

	executeCode : async function(luaCode) {
		// clang-format off
		if (mode === 'dev') {
			showNotification('info', 'Execute', 'Running in dev mode - no backend available');
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_RUN_PROGRAM, JSON.stringify({ "luaScript": luaCode }));
			const contentResponse = new TextDecoder().decode(response);
			const responsejson = JSON.parse(contentResponse);
			uiComponents.clear();
			if (responsejson.result) {
				showNotification('success', 'Program Started', 'Code is now running on the device');
			} else {
				showNotification('error', 'Execution Failed', 'Could not start program');
			}
		} else if (mode === 'web') {
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
				showNotification('success', 'Program Started', 'Code is now running on the device');
			});
		}
		// clang-format on
	},

	stop : async function() {
		// clang-format off
		if (mode === 'dev') {
			showNotification('info', 'Stop', 'Running in dev mode - no backend available');
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_STOP_PROGRAM, JSON.stringify({}));
			showNotification('success', 'Program Stopped', 'Execution has been halted');
		} else if (mode === 'web') {
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
				showNotification('success', 'Program Stopped', 'Execution has been halted');
			});
		}
		blocklyEditor.removeAllProfilingOverlays();		
		// clang-format on
	},

	requestPairing : async function(mac) {
		// clang-format off
		if (mode === 'dev') {
			showNotification('info', 'Pairing', `Dev mode - simulating pairing with ${mac}`);
		} else if (mode === 'bt') {
			showNotification('info', 'Pairing', `Initiating pairing with ${mac}...`);
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_REQUEST_PAIRING, JSON.stringify({"mac" : mac}));
			showNotification('success', 'Pairing Initiated', 'Check your device for pairing confirmation');
		} else if (mode === 'web') {
			// TODO
		}
		// clang-format on
	},

	requestRemovePairing : async function(mac) {
		// clang-format off
		if (mode === 'dev') {
			showNotification('info', 'Remove Pairing', `Dev mode - simulating unpairing ${mac}`);
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_REMOVE_PAIRING, JSON.stringify({"mac" : mac}));
			showNotification('success', 'Pairing Removed', `Device ${mac} has been unpaired`);
		} else if (mode === 'web') {
			// TODO
		}
		// clang-format on
	},

	startBluetoothDiscovery : async function() {
		// clang-format off
		if (mode === 'dev') {
			showNotification('info', 'Bluetooth Scan', 'Dev mode - no actual scan');
		} else if (mode === 'bt') {
			showNotification('info', 'Scanning', 'Searching for Bluetooth devices...');
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_START_DISCOVERY, JSON.stringify({}));
		} else if (mode === 'web') {
			// TODO
		}
		// clang-format on
	},

	requestProjectsAndAutostartConfig : async function(callback) {
		var projects = [
			{name : 'dummy'},
			{name : 'testproject'},
		];

		if (mode === 'bt') {
			console.log("Requesting Projects...");
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_PROJECTS, JSON.stringify({}));
			const responseText = new TextDecoder().decode(response);
			console.log("Got Response : (" + responseText + ")");
			const responseJson = JSON.parse(responseText);
			console.log("JSON Response was " + JSON.stringify(responseJson));
			projects = responseJson.projects;
		} else if (mode === 'web') {
			const response = await fetch("/projects");
			const responseJson = await response.json();
			projects = responseJson.projects;
		}

		var autostart = "testproject";

		if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_AUTOSTART, JSON.stringify({}));
			const responseJson = JSON.parse(new TextDecoder().decode(response));
			autostart = responseJson.project;
		} else if (mode === 'web') {
			const response = await fetch("/autostart");
			const responseJson = await response.json();
			autostart = responseJson.project;
		}

		callback(projects, autostart);
	},
}

async function initBLEConnection() {
	try {
		await bleClient.connect();
		console.log('Connected!');

		const logger = document.getElementById('logger');

		portstatus.initialize();
		btdevicelist.initialize();

		bleClient.addEventListener(APP_EVENT_TYPE_LOG, (data) => {
			const message = new TextDecoder().decode(data);
			logger.addToLog(message);
		});

		bleClient.addEventListener(APP_EVENT_TYPE_PORTSTATUS, (data) => {
			const status = JSON.parse(new TextDecoder().decode(data));
			portstatus.updateStatus(status);
		});

		bleClient.addEventListener(APP_EVENT_TYPE_COMMAND, (data) => {
			const command = JSON.parse(new TextDecoder().decode(data));
			if (command.type === "thread_statistics") {
				blocklyEditor.addProfilingOverlay(command.blockid, command.min, command.avg, command.max);
			} else {
				uicomponents.processUIEvent(command);
			}
		});

		bleClient.addEventListener(APP_EVENT_TYPE_BTCLASSICDEVICES, (data) => {
			const devices = JSON.parse(new TextDecoder().decode(data));
			btdevicelist.updateDevices(devices);
		});

		// Wildcard listener
		// bleClient.addEventListener('*', (appEventType, data) => {
		//	console.log(`Event Type ${appEventType} received:`, new TextDecoder().decode(data));
		//});

		// Activate Eventing
		console.log("Notifying, I am ready!");
		await bleClient.sendRequest(APP_REQUEST_TYPE_READY_FOR_EVENTS, JSON.stringify({}));

		console.log("Now I should get log messages");

		bleClient.onDisconnect(() => {
			console.log('Connection terminated!');
			window.Application.setInitState();
		});

		// We are ready to do something, we start with the files view
		window.Application.jumpToFilesView();

	} catch (error) {
		console.error('Error:', error);
	}
};

/**
 * Initialize sidebar accordion behavior
 * Only one panel can be expanded at a time
 */
function initSidebarAccordion() {
	const accordion = document.getElementById('sidebarAccordion');
	if (!accordion)
		return;

	const panels = accordion.querySelectorAll('[data-accordion-panel]');

	// Listen for panel expand events
	panels.forEach(panel => {
		panel.addEventListener('accordion-expand', (e) => {
			// Collapse all other panels
			panels.forEach(otherPanel => {
				if (otherPanel !== panel && otherPanel.collapse) {
					otherPanel.collapse();
				}
				otherPanel.classList.remove('accordion-expanded');
			});

			// Mark this panel as expanded
			panel.classList.add('accordion-expanded');
		});

		panel.addEventListener('accordion-collapse', (e) => {
			panel.classList.remove('accordion-expanded');
		});
	});
}

document.addEventListener('DOMContentLoaded', () => {
	blocklyEditor = document.getElementById('blockly');
	blocklyEditor.addChangeListener(() => {
		generateCode();
	});
	luaPreview = document.getElementById('luapreview');
	uiComponents = document.getElementById('uicomponents');
	portstatus = document.getElementById("portstatus");
	btdevicelist = document.getElementById("btdevicelist");

	// Initialize accordion behavior
	initSidebarAccordion();

	if (mode === 'bt') {
		window.Application.setInitState();

		// Check for Web Bluetooth API support
		if (!navigator.bluetooth) {
			// Hide the connect button and show unsupported message
			const btconnectBtn = document.getElementById("btconnect");
			btconnectBtn.style.display = 'none';

			// Show the Bluetooth unsupported view
			showBluetoothUnsupportedMessage();
		} else {
			document.getElementById("btconnect").addEventListener("click", (ev) => {
				initBLEConnection();
			});
		}

	} else {
		window.Application.jumpToFilesView();
	}

	if (mode == "web") {
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
			portstatus.updateStatus(JSON.parse(event.data));
		});
	}

	if (mode == "dev") {
		// Initialize BT Device List with dummy data
		btdevicelist.updateDevices({
			"devices" : [
				{
					"mac" : "AA:BB:CC:11:22:33",
					"name" : "8BitDo M30 gamepad",
					"type" : 0x09, // GAMEPAD
					"paired" : true,
					"rssi" : -45,
					"cod" : 0x002508
				},
				{
					"mac" : "DD:EE:FF:44:55:66",
					"name" : "Logitech MX Master 3",
					"type" : 0x0B, // MOUSE
					"paired" : false,
					"rssi" : -67,
					"cod" : 0x002580
				},
				{
					"mac" : "11:22:33:AA:BB:CC",
					"name" : "Apple Magic Keyboard",
					"type" : 0x0A, // KEYBOARD
					"paired" : true,
					"rssi" : -52,
					"cod" : 0x002540
				},
				{
					"mac" : "77:88:99:DD:EE:FF",
					"name" : "Unknown Device",
					"type" : 0x00, // UNKNOWN
					"paired" : false,
					"rssi" : -78,
					"cod" : 0x000000
				},
				{
					"mac" : "44:55:66:11:22:33",
					"name" : "Sony WH-1000XM4",
					"type" : 0x03, // AUDIO
					"paired" : true,
					"rssi" : -58,
					"cod" : 0x240404
				}
			],
			"count" : 5,
			"discoveryActive" : true
		});

		portstatus.updateStatus({
			"ports" : [
				{
					 "id" : 1,
					 "connected" : false
				},
				{
					 "id" : 2,
					 "connected" : false
				},
				{
					 "id" : 3,
					 "connected" : true,
					 "device" : {
						"type" : "BOOST Color and Distance Sensor",
						"icon" : "⚙️",
						"modes" : [
							{
								"id" : "0",
								"name" : "COLOR"
							}
						]
					}
				},
				{
					 "id" : 4,
					 "connected" : false
				}
			]
		});
	}

	// Initialize confirmation dialog
	initConfirmDialog();

	document.getElementById("reset").addEventListener("click", async () => {
		const confirmed = await showConfirmDialog(
			'Reset Workspace',
			'Are you sure you want to reset the workspace? All unsaved changes to your Blockly program will be lost.',
			{ confirmText: 'Reset', cancelText: 'Cancel', destructive: true }
		);
		if (confirmed) {
			blocklyEditor.clearWorkspace();
			showNotification('success', 'Workspace Reset', 'All blocks have been cleared');
		}
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

	// Manual save button
	document.getElementById("save").addEventListener("click", async () => {
		if (window.Application.activeProject) {
			const success = await saveWorkspace();
			if (success) {
				showNotification('success', 'Saved', 'Workspace saved successfully');
			} else {
				showNotification('error', 'Save Failed', 'Could not save workspace');
			}
		} else {
			showNotification('warning', 'No Project', 'Open or create a project first');
		}
	});

	// Copy project to clipboard button
	document.getElementById("copyProject").addEventListener("click", async () => {
		try {
			const xmlText = blocklyEditor.generateXML();
			await navigator.clipboard.writeText(xmlText);
			showNotification('success', 'Copied to Clipboard', 'Blockly project has been copied to clipboard');
		} catch (error) {
			console.error('Failed to copy to clipboard:', error);
			showNotification('error', 'Copy Failed', 'Could not copy project to clipboard');
		}
	});

	// Paste project from clipboard button
	document.getElementById("pasteProject").addEventListener("click", async () => {
		try {
			const clipboardText = await navigator.clipboard.readText();

			// Check if it looks like valid Blockly XML
			if (!clipboardText.trim().startsWith('<xml') && !clipboardText.trim().startsWith('<?xml')) {
				showNotification('error', 'Invalid Data', 'Clipboard does not contain valid Blockly XML');
				return;
			}

			const confirmed = await showConfirmDialog(
				'Paste Project',
				'This will replace your current workspace with the project from clipboard. Continue?',
				{ confirmText: 'Paste', cancelText: 'Cancel', destructive: true }
			);

			if (confirmed) {
				const success = blocklyEditor.loadXML(clipboardText);
				if (success) {
					showNotification('success', 'Project Loaded', 'Workspace has been loaded from clipboard');
				} else {
					showNotification('error', 'Load Failed', 'Could not parse the Blockly XML from clipboard');
				}
			}
		} catch (error) {
			console.error('Failed to read from clipboard:', error);
			showNotification('error', 'Paste Failed', 'Could not read from clipboard. Make sure you have granted clipboard permissions.');
		}
	});

	// Auto-save toggle button
	const autosaveBtn = document.getElementById("autosave");
	autosaveBtn.addEventListener("click", () => {
		autoSaveEnabled = !autoSaveEnabled;
		autosaveBtn.setAttribute("aria-pressed", autoSaveEnabled ? "true" : "false");
		autosaveBtn.setAttribute("title", autoSaveEnabled ? "Auto-save (on)" : "Auto-save (off)");

		if (autoSaveEnabled) {
			// Start auto-save interval
			autoSaveIntervalId = setInterval(() => {
				if (window.Application.activeProject) {
					saveWorkspace();
				}
			}, 10000);
			showNotification('success', 'Auto-save Enabled', 'Workspace will be saved every 10 seconds');
		} else {
			// Stop auto-save interval
			if (autoSaveIntervalId) {
				clearInterval(autoSaveIntervalId);
				autoSaveIntervalId = null;
			}
			showNotification('info', 'Auto-save Disabled', 'Automatic saving has been turned off');
		}
	});
});
