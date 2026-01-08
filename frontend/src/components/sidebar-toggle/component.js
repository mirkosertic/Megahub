/**
 * Sidebar Toggle Component
 * Provides a button to toggle the sidebar visibility on mobile/tablet devices
 * Automatically hidden on desktop (≥1512px) where sidebar is always visible
 */

import styleSheet from './style.css?raw'

class CustomSidebarToggle extends HTMLElement {
  constructor() {
    super()
    this.attachShadow({ mode: 'open' })
  }

  connectedCallback() {
    // Apply styles
    const sheet = new CSSStyleSheet()
    sheet.replaceSync(styleSheet)
    this.shadowRoot.adoptedStyleSheets = [sheet]

    // Create toggle button
    const button = document.createElement('button')
    button.className = 'toggle-btn'
    button.setAttribute('aria-label', 'Toggle sidebar')
    button.setAttribute('title', 'Toggle sidebar')

    // Use hamburger icon (☰) or menu icon
    button.innerHTML = `
      <svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M3 12H21M3 6H21M3 18H21" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
      </svg>
    `

    this.shadowRoot.appendChild(button)

    // Toggle sidebar when clicked
    button.addEventListener('click', () => {
      this.toggleSidebar()
    })

    // Close sidebar when backdrop is clicked
    this.setupBackdropListener()

    // Close sidebar on Escape key
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        this.closeSidebar()
      }
    })
  }

  toggleSidebar() {
    const layout = document.querySelector('.global-layout-sidebar')
    if (layout) {
      layout.classList.toggle('sidebar-visible')

      // Update aria-expanded attribute
      const isVisible = layout.classList.contains('sidebar-visible')
      const button = this.shadowRoot.querySelector('button')
      button.setAttribute('aria-expanded', isVisible.toString())

      // Add or remove backdrop
      if (isVisible) {
        this.addBackdrop()
      } else {
        this.removeBackdrop()
      }
    }
  }

  closeSidebar() {
    const layout = document.querySelector('.global-layout-sidebar')
    if (layout && layout.classList.contains('sidebar-visible')) {
      layout.classList.remove('sidebar-visible')
      const button = this.shadowRoot.querySelector('button')
      button.setAttribute('aria-expanded', 'false')
      this.removeBackdrop()
    }
  }

  addBackdrop() {
    // Check if backdrop already exists
    if (document.querySelector('.sidebar-backdrop')) {
      return
    }

    const backdrop = document.createElement('div')
    backdrop.className = 'sidebar-backdrop'
    backdrop.setAttribute('aria-hidden', 'true')
    document.body.appendChild(backdrop)

    // Close sidebar when backdrop is clicked
    backdrop.addEventListener('click', () => {
      this.closeSidebar()
    })
  }

  removeBackdrop() {
    const backdrop = document.querySelector('.sidebar-backdrop')
    if (backdrop) {
      backdrop.remove()
    }
  }

  setupBackdropListener() {
    // Listen for clicks on the backdrop (added dynamically)
    document.addEventListener('click', (e) => {
      if (e.target.classList.contains('sidebar-backdrop')) {
        this.closeSidebar()
      }
    })
  }
}

customElements.define('custom-sidebar-toggle', CustomSidebarToggle)

export default CustomSidebarToggle
