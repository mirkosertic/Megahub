import template from './component.html?raw';
import styleSheet from './style.css?raw';
import { subscribe, getState } from '../../app/state.js';
import {
    APP_EVENT_PROJECT_OPEN,
    APP_EVENT_PROJECT_CREATE,
    APP_EVENT_PROJECT_DELETE,
    APP_EVENT_AUTOSTART_SET,
} from '../../app/events.js';

class FilesHTMLElement extends HTMLElement {

	projectData = [];
	selectedProject = undefined;
	autostartProject = undefined;

	/** @type {function(): void} Unsubscribe from projects state */
	_unsubProjects = null;

	/** @type {function(): void} Unsubscribe from autostartProject state */
	_unsubAutostart = null;

	/**
	 * Validates project name and returns error message or empty string if valid.
	 * Rules: not empty, no leading/trailing spaces, only alphanumeric and spaces allowed.
	 */
	validateProjectName(name) {
		if (!name || name.length === 0) {
			return 'Project name cannot be empty';
		}
		if (name.startsWith(' ')) {
			return 'Project name cannot start with a space';
		}
		if (name.endsWith(' ')) {
			return 'Project name cannot end with a space';
		}
		if (!/^[a-zA-Z0-9 ]+$/.test(name)) {
			return 'Only letters, numbers, and spaces are allowed';
		}
		return '';
	}

	/**
	 * Updates the validation UI based on input value.
	 */
	updateValidationUI() {
		const input = this.shadowRoot.getElementById('newProjectName');
		const hint = this.shadowRoot.getElementById('validationHint');
		const createBtn = this.shadowRoot.getElementById('createbutton');
		const value = input.value;

		const error = this.validateProjectName(value);

		if (error) {
			input.classList.add('invalid');
			hint.textContent = error;
			hint.classList.add('visible');
			createBtn.setAttribute('disabled', '');
		} else {
			input.classList.remove('invalid');
			hint.textContent = '';
			hint.classList.remove('visible');
			createBtn.removeAttribute('disabled');
		}
	}

	connectedCallback() {
		const shadow = this.attachShadow({mode : 'open'})

		// Create adoptable stylesheet
		const sheet = new CSSStyleSheet()
		sheet.replaceSync(styleSheet)
		shadow.adoptedStyleSheets = [ sheet ]

		shadow.innerHTML = template

		// Subscribe to projects state — re-renders automatically when state changes
		this._unsubProjects = subscribe('projects', projects => {
			if (projects) {
				this.initProjectListWith(projects);
			}
		});

		// Subscribe to autostartProject state — updates highlight automatically
		this._unsubAutostart = subscribe('autostartProject', autostart => {
			this.setAutoStartProject(autostart);
		});
	}

	disconnectedCallback() {
		// Clean up state subscriptions to avoid memory leaks
		if (this._unsubProjects) {
			this._unsubProjects();
			this._unsubProjects = null;
		}
		if (this._unsubAutostart) {
			this._unsubAutostart();
			this._unsubAutostart = null;
		}
	}

	initProjectListWith(data) {
		this.projectData = data;
		this.render();
	}

	render() {
		const container = this.shadowRoot;
		const projectList = container.getElementById('projectList');
		const template = container.getElementById('projectItemTemplate');

		// Clear existing content
		projectList.innerHTML = '';

		if (this.projectData.length === 0) {
			projectList.innerHTML = '<div class="empty-state">No projects found</div>';
			this.shadowRoot.getElementById("openbutton").setAttribute("disabled");
			return;
		}

		// Create items from template
		this.projectData.forEach((project, index) => {
			const clone = template.content.cloneNode(true);
			const projectItem = clone.querySelector('.project-item');
			const projectNameSpan = clone.querySelector('.project-name');
			const autostartBtn = clone.querySelector('.autostart-button');
			const deleteBtn = clone.querySelector('.delete-button');

			// Set project name
			projectNameSpan.textContent = project.name;

			projectItem.addEventListener("click", (event) => {
				this.selectProject(project, projectItem);
			});

			autostartBtn.dataset.project = project.name;

			autostartBtn.addEventListener("click", (event) => {
				this._setAutoStartProjectAndNotify(project.name);
			});

			deleteBtn.addEventListener("click", (e) => {
				this.deleteProject(project.name);
			});

			// Select first item by default
			if (index === 0) {
				projectItem.classList.add('selected');
				this.selectedProject = project;

				this.shadowRoot.getElementById("openbutton").removeAttribute("disabled");
			}

			projectList.appendChild(clone);
		});

		this.shadowRoot.getElementById("openbutton").addEventListener("click", (event) => {
			this.openProject(this.selectedProject.name);
		});

		const newProjectInput = this.shadowRoot.getElementById("newProjectName");
		newProjectInput.addEventListener("input", () => {
			this.updateValidationUI();
		});

		this.shadowRoot.getElementById("createbutton").addEventListener("click", (event) => {
			var project = newProjectInput.value;
			if (!this.validateProjectName(project)) {
				this.createProject(project);
			}
		});

		this.setAutoStartProject(this.autostartProject);
	}

	deleteProject(projectName) {
		// Dispatch CustomEvent instead of calling window.Application directly
		this.dispatchEvent(new CustomEvent(APP_EVENT_PROJECT_DELETE, {
			bubbles: true,
			composed: true,
			detail: { id: projectName }
		}));
	}

	openProject(projectName) {
		// Dispatch CustomEvent instead of calling window.Application directly
		this.dispatchEvent(new CustomEvent(APP_EVENT_PROJECT_OPEN, {
			bubbles: true,
			composed: true,
			detail: { id: projectName }
		}));
	}

	createProject(projectName) {
		// Dispatch CustomEvent instead of calling window.Application directly
		this.dispatchEvent(new CustomEvent(APP_EVENT_PROJECT_CREATE, {
			bubbles: true,
			composed: true,
			detail: { id: projectName }
		}));
	}

	/**
	 * Update the autostart highlight in the UI only (no backend call).
	 * Called from the state subscription when autostartProject state changes.
	 * @param {string|null} project
	 */
	setAutoStartProject(project) {
		this.autostartProject = project;
		this._renderAutostartHighlight(project);
	}

	/**
	 * Update the autostart highlight AND notify the backend.
	 * Called only on explicit user interaction (star button click).
	 * @param {string} project
	 */
	_setAutoStartProjectAndNotify(project) {
		this.autostartProject = project;
		this._renderAutostartHighlight(project);

		if (project) {
			// Dispatch CustomEvent instead of calling window.Application directly
			this.dispatchEvent(new CustomEvent(APP_EVENT_AUTOSTART_SET, {
				bubbles: true,
				composed: true,
				detail: { project }
			}));
		}
	}

	/**
	 * Update the star button highlights to reflect the current autostart project.
	 * @param {string|null} project
	 */
	_renderAutostartHighlight(project) {
		this.shadowRoot.querySelectorAll('.autostart-button').forEach(function(item) {
			item.classList.remove('autostart-active');
			item.textContent = "☆";
			item.title = "Set as autostart";
		});

		if (project && this.projectData.length > 0) {
			const btn = this.shadowRoot.querySelector(".autostart-button[data-project='" + project + "']");
			if (btn) {
				btn.classList.add("autostart-active");
				btn.textContent = '★';
				btn.title = 'Autostart enabled';
			}
		}
	}

	selectProject(item, element) {
		this.shadowRoot.querySelectorAll('.project-item').forEach(function(item) {
			item.classList.remove('selected');
		});

		this.selectedProject = item;
		element.classList.add('selected');
	}

	initialize(projectList, autostartProject) {
		this.initProjectListWith(projectList);
		this.setAutoStartProject(autostartProject);
	}
}

customElements.define('custom-files', FilesHTMLElement);
