import template from './component.html?raw';
import styleSheet from './style.css?raw';

class FilesHTMLElement extends HTMLElement {

	projectData = [];
	selectedProject = undefined;
	autostartProject = undefined;

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
	};

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
				this.setAutoStartProject(project.name);
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
	};

	deleteProject(projectName) {
		window.Application.deleteProject(projectName);
	}

	openProject(projectName) {
		window.Application.openProject(projectName);
	}

	createProject(projectName) {
		window.Application.createProject(projectName);
	}

	setAutoStartProject(project) {
		this.autostartProject = project;

		this.shadowRoot.querySelectorAll('.autostart-button').forEach(function(item) {
			item.classList.remove('autostart-active');
			item.textContent = "☆";
			item.title = "Set as autostart";
		});

		if (this.autostartProject && this.projectData.length > 0) {
			const btn = this.shadowRoot.querySelector(".autostart-button[data-project='" + project + "']");
			if (btn) {
				btn.classList.add("autostart-active");
				btn.textContent = '★';
				btn.title = 'Autostart enabled';
			}

			window.Application.setAutostartProject(project);
		}
	}

	selectProject(item, element) {
		this.shadowRoot.querySelectorAll('.project-item').forEach(function(item) {
			item.classList.remove('selected');
		});

		this.selectedProject = item;
		element.classList.add('selected');
	};

	initialize(projectList, autostartProject) {
		this.initProjectListWith(projectList);
		this.setAutoStartProject(autostartProject);
	};
};

customElements.define('custom-files', FilesHTMLElement);
