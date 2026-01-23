import template from './component.html?raw';
import styleSheet from './style.css?raw';

import Prism from 'prismjs'
import 'prismjs/components/prism-lua'
import 'prismjs/plugins/line-numbers/prism-line-numbers'
import 'prismjs/themes/prism-tomorrow.css'
import 'prismjs/plugins/line-numbers/prism-line-numbers.css'

class LuaPreviewHTMLElement extends HTMLElement {

	isCollapsed = true;

	connectedCallback() {
		const shadow = this.attachShadow({ mode: 'open' });

		// Create adoptable stylesheet
		const sheet = new CSSStyleSheet();
		sheet.replaceSync(styleSheet);
		shadow.adoptedStyleSheets = [sheet];

		// Add Prism styles to shadow DOM
		const prismStyles = document.createElement('style');
		prismStyles.textContent = `
			/* Prism Tomorrow theme - minimal subset for Lua */
			code[class*="language-"],
			pre[class*="language-"] {
				color: #ccc;
				background: none;
				font-family: Consolas, Monaco, 'Andale Mono', 'Ubuntu Mono', monospace;
				text-align: left;
				white-space: pre;
				word-spacing: normal;
				word-break: normal;
				word-wrap: normal;
				line-height: 1.5;
				tab-size: 4;
				hyphens: none;
			}
			pre[class*="language-"] {
				padding: 0;
				margin: 0;
				overflow: auto;
			}
			.token.comment,
			.token.block-comment,
			.token.prolog,
			.token.doctype,
			.token.cdata {
				color: #999;
			}
			.token.punctuation {
				color: #ccc;
			}
			.token.tag,
			.token.attr-name,
			.token.namespace,
			.token.deleted {
				color: #e2777a;
			}
			.token.function-name {
				color: #6196cc;
			}
			.token.boolean,
			.token.number,
			.token.function {
				color: #f08d49;
			}
			.token.property,
			.token.class-name,
			.token.constant,
			.token.symbol {
				color: #f8c555;
			}
			.token.selector,
			.token.important,
			.token.atrule,
			.token.keyword,
			.token.builtin {
				color: #cc99cd;
			}
			.token.string,
			.token.char,
			.token.attr-value,
			.token.regex,
			.token.variable {
				color: #7ec699;
			}
			.token.operator,
			.token.entity,
			.token.url {
				color: #67cdcc;
			}
			.token.important,
			.token.bold {
				font-weight: bold;
			}
			.token.italic {
				font-style: italic;
			}
			.token.entity {
				cursor: help;
			}
			.token.inserted {
				color: green;
			}
			/* Line numbers */
			pre[class*="language-"].line-numbers {
				position: relative;
				padding-left: 3.8em;
				counter-reset: linenumber;
			}
			pre[class*="language-"].line-numbers > code {
				position: relative;
				white-space: inherit;
			}
			.line-numbers .line-numbers-rows {
				position: absolute;
				pointer-events: none;
				top: 0;
				font-size: 100%;
				left: -3.8em;
				width: 3em;
				letter-spacing: -1px;
				border-right: 1px solid #999;
				user-select: none;
			}
			.line-numbers-rows > span {
				display: block;
				counter-increment: linenumber;
			}
			.line-numbers-rows > span:before {
				content: counter(linenumber);
				color: #999;
				display: block;
				padding-right: 0.8em;
				text-align: right;
			}
		`;
		shadow.appendChild(prismStyles);

		shadow.innerHTML += template;

		// Set up collapse toggle functionality
		this.setupCollapseToggle();
	};

	/**
	 * Set up collapse toggle button
	 */
	setupCollapseToggle() {
		const collapseToggle = this.shadowRoot.getElementById('collapseToggle');
		const headerToggle = this.shadowRoot.getElementById('headerToggle');

		if (collapseToggle) {
			// Click on button
			collapseToggle.addEventListener('click', (e) => {
				e.stopPropagation();
				this.toggleCollapse();
			});

			// Click on entire header
			headerToggle.addEventListener('click', () => {
				this.toggleCollapse();
			});
		}

		// Set up copy button
		this.setupCopyButton();
	}

	/**
	 * Set up copy code button
	 */
	setupCopyButton() {
		const copyBtn = this.shadowRoot.getElementById('copyCodeBtn');
		if (copyBtn) {
			copyBtn.addEventListener('click', async (e) => {
				e.stopPropagation();
				await this.copyCodeToClipboard();
			});
		}
	}

	/**
	 * Copy the current Lua code to clipboard
	 */
	async copyCodeToClipboard() {
		const copyBtn = this.shadowRoot.getElementById('copyCodeBtn');
		const codePreview = this.shadowRoot.getElementById('codePreview');

		if (!codePreview) return;

		// Get the code text content
		const codeElement = codePreview.querySelector('code');
		if (!codeElement) {
			// No code to copy
			if (window.showNotification) {
				window.showNotification('warning', 'No Code', 'No Lua code to copy');
			}
			return;
		}

		const codeText = codeElement.textContent;

		try {
			await navigator.clipboard.writeText(codeText);

			// Show copied feedback
			if (copyBtn) {
				copyBtn.classList.add('copied');
				const textSpan = copyBtn.querySelector('.copy-btn-text');
				if (textSpan) {
					textSpan.textContent = 'Copied!';
				}

				// Reset after 2 seconds
				setTimeout(() => {
					copyBtn.classList.remove('copied');
					if (textSpan) {
						textSpan.textContent = 'Copy';
					}
				}, 2000);
			}

			if (window.showNotification) {
				window.showNotification('success', 'Copied', 'Lua code copied to clipboard');
			}
		} catch (error) {
			console.error('Failed to copy code:', error);
			if (window.showNotification) {
				window.showNotification('error', 'Copy Failed', 'Could not copy code to clipboard');
			}
		}
	}

	/**
	 * Toggle collapse state
	 */
	toggleCollapse() {
		this.isCollapsed = !this.isCollapsed;
		const container = this.shadowRoot.querySelector('.lua-preview-container');

		if (container) {
			if (this.isCollapsed) {
				container.classList.add('collapsed');
				this.dispatchEvent(new CustomEvent('accordion-collapse', { bubbles: true }));
			} else {
				container.classList.remove('collapsed');
				this.dispatchEvent(new CustomEvent('accordion-expand', { bubbles: true }));
			}
		}
	}

	/**
	 * Collapse the panel (called by accordion controller)
	 */
	collapse() {
		if (!this.isCollapsed) {
			this.isCollapsed = true;
			const container = this.shadowRoot.querySelector('.lua-preview-container');
			if (container) {
				container.classList.add('collapsed');
			}
		}
	}

	highlightCode(code) {
		const codePreview = this.shadowRoot.getElementById('codePreview');
		if (codePreview) {
			codePreview.innerHTML = `<pre class="line-numbers"><code class="language-lua">${Prism.highlight(code, Prism.languages.lua, 'lua')}</code></pre>`;

			// Manually add line numbers for shadow DOM
			const pre = codePreview.querySelector('pre');
			if (pre) {
				const lines = code.split('\n').length;
				const lineNumbersWrapper = document.createElement('span');
				lineNumbersWrapper.className = 'line-numbers-rows';
				lineNumbersWrapper.innerHTML = '<span></span>'.repeat(lines);
				pre.appendChild(lineNumbersWrapper);
			}
		}
	};

	clear() {
		const codePreview = this.shadowRoot.getElementById('codePreview');
		if (codePreview) {
			codePreview.innerHTML = '<div class="no-code-message">No code generated yet</div>';
		}
	}
};

customElements.define('custom-lua-preview', LuaPreviewHTMLElement);
