import template from './component.html?raw';
import styleSheet from './style.css?raw';

class FilesHTMLElement extends HTMLElement {

	projectData = [];
	selectedProject = undefined;
	autostartProject = undefined;

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
			this.createOrOpenProject(this.selectedProject.name);
		});

		this.shadowRoot.getElementById("createbutton").addEventListener("click", (event) => {
			var project = this.shadowRoot.getElementById("newProjectName").value;
			this.createOrOpenProject(project);
		});

		this.setAutoStartProject(this.autostartProject);
	};

	deleteProject(projectName) {

		fetch("/project/" + encodeURIComponent(projectName), {
			method : "DELETE"
		})
			.then((response) => {
				console.log("Got response from backend : " + JSON.stringify(response));

				this.initialize();
			})
			.catch((error) => {
				console.error('Error deleting project:', error);
			});
	}

	createOrOpenProject(projectName) {
		if (!import.meta.env.DEV) {
			window.location.href = "/project/" + encodeURIComponent(projectName) + "/";
		} else {
			window.location.href = "/project.html";
		}
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
			btn.classList.add("autostart-active");
			btn.textContent = '★';
			btn.title = 'Autostart enabled';

			fetch("/autostart", {
				method : "PUT",
				body : JSON.stringify(
					{"project" : project}
						 ),
				headers : {
									   "Content-type" : "application/json; charset=UTF-8",
									   },
			})
				.then((response) => {
					console.log("Got response from backend : " + JSON.stringify(response));
				})
				.catch((error) => {
					console.error('Error deleting project:', error);
				});
		}
	}

	selectProject(item, element) {
		this.shadowRoot.querySelectorAll('.project-item').forEach(function(item) {
			item.classList.remove('selected');
		});

		this.selectedProject = item;
		element.classList.add('selected');
	};

	initialize() {
		if (!import.meta.env.DEV) {
			fetch("/projects")
				.then((response) => response.json())
				.then((json) => {
					this.initProjectListWith(json.projects);
				})
				.catch((error) => {
					console.error('Error loading /project', error);
				});

			fetch("/autostart")
				.then((response) => response.json())
				.then((json) => {
					this.setAutoStartProject(json.project);
				})
				.catch((error) => {
					console.error('Error loading /autostart', error);
					this.setAutoStartProject(undefined);
				});

		} else {
			let projectArray = [
				{name : 'dummy'},
				{name : 'testproject'},
			];

			this.initProjectListWith(projectArray);
			this.setAutoStartProject("testproject");
		}
	}
};

customElements.define('custom-files', FilesHTMLElement);
