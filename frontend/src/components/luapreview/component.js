import template from './component.html?raw';

import Prism from 'prismjs'
import 'prismjs/components/prism-lua'
import 'prismjs/plugins/line-numbers/prism-line-numbers'
import 'prismjs/themes/prism-tomorrow.css'
import 'prismjs/plugins/line-numbers/prism-line-numbers.css'


class LuaPreviewHTMLElement extends HTMLElement {

  connectedCallback() {
      this.innerHTML = template;
  };

  highlightCode(code) {
    this.innerHTML = `<pre class="line-numbers"><code class="language-lua">${Prism.highlight(code, Prism.languages.lua, 'lua')}</code></pre>`;

    // Highlight all code blocks
    Prism.highlightAll();
  };
};

customElements.define('custom-lua-preview', LuaPreviewHTMLElement);