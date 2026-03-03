import template from './component.html?raw';
import styleSheet from './style.css?raw';

const MAX_TRAIL_POINTS = 1000;
const MIN_EXTENT = 0.5; // meters — minimum world span before auto-zoom kicks in
const PADDING = 0.1; // 10% margin around bounds
const GRID_MINOR = 0.1; // meters between minor grid lines
const GRID_MAJOR = 1.0; // meters between major grid lines
const DROPOUT_MS = 400; // ms before marker dims (2× 200ms send interval)

class MapHTMLElement extends HTMLElement {
    isCollapsed = true;

    // Trail and bounds
    trail = [];
    minX = 0;
    maxX = 0;
    minY = 0;
    maxY = 0;
    boundsSet = false;
    lastPointMs = 0;

    // RAF deduplication
    rafPending = false;

    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        const sheet = new CSSStyleSheet();
        sheet.replaceSync(styleSheet);
        shadow.adoptedStyleSheets = [sheet];

        shadow.innerHTML = template;

        this._canvas = shadow.getElementById('mapCanvas');
        this._ctx = this._canvas.getContext('2d');

        this._setupCollapseToggle();
        this._setupResizeObserver();
    }

    _setupCollapseToggle() {
        const collapseToggle = this.shadowRoot.getElementById('collapseToggle');
        const headerToggle = this.shadowRoot.getElementById('headerToggle');

        collapseToggle.addEventListener('click', (e) => {
            e.stopPropagation();
            this._toggleCollapse();
        });
        headerToggle.addEventListener('click', () => {
            this._toggleCollapse();
        });
    }

    _toggleCollapse() {
        this.isCollapsed = !this.isCollapsed;
        const container = this.shadowRoot.querySelector('.map-container');
        if (this.isCollapsed) {
            container.classList.add('collapsed');
            this.dispatchEvent(new CustomEvent('accordion-collapse', { bubbles: true }));
        } else {
            container.classList.remove('collapsed');
            this.dispatchEvent(new CustomEvent('accordion-expand', { bubbles: true }));
            this._resizeCanvas();
            this.scheduleDraw();
        }
    }

    collapse() {
        if (!this.isCollapsed) {
            this.isCollapsed = true;
            const container = this.shadowRoot.querySelector('.map-container');
            if (container) container.classList.add('collapsed');
        }
    }

    _setupResizeObserver() {
        this._resizeObserver = new ResizeObserver(() => {
            this._resizeCanvas();
            this.scheduleDraw();
        });
        this._resizeObserver.observe(this);
    }

    _resizeCanvas() {
        const content = this.shadowRoot.querySelector('.collapsible-content');
        if (!content || this.isCollapsed) return;
        const rect = content.getBoundingClientRect();
        const w = Math.floor(rect.width);
        const h = Math.floor(rect.height);
        if (w > 0 && h > 0 && (this._canvas.width !== w || this._canvas.height !== h)) {
            this._canvas.width = w;
            this._canvas.height = h;
        }
    }

    expandBounds(x, y) {
        if (!this.boundsSet) {
            this.minX = this.maxX = x;
            this.minY = this.maxY = y;
            this.boundsSet = true;
        } else {
            if (x < this.minX) this.minX = x;
            if (x > this.maxX) this.maxX = x;
            if (y < this.minY) this.minY = y;
            if (y > this.maxY) this.maxY = y;
        }
    }

    resetBounds() {
        this.boundsSet = false;
        this.minX = this.maxX = this.minY = this.maxY = 0;
    }

    processUIEvent(event) {
        if (event.type === 'map_points') {
            // Insert a trail break if data arrives after a BLE dropout gap
            if (this.trail.length > 0 && this.lastPointMs > 0) {
                if (Date.now() - this.lastPointMs > DROPOUT_MS) {
                    this.trail.push(null); // null = visual break in polyline
                }
            }
            for (const pt of event.points) {
                this.trail.push({ x: pt.x, y: pt.y, heading: pt.h });
                this.expandBounds(pt.x, pt.y);
            }
            while (this.trail.length > MAX_TRAIL_POINTS) {
                this.trail.shift();
            }
            this.lastPointMs = Date.now();
            this.scheduleDraw();
        } else if (event.type === 'map_clear') {
            this.trail = [];
            this.resetBounds();
            this.scheduleDraw();
        }
    }

    scheduleDraw() {
        if (!this.rafPending) {
            this.rafPending = true;
            requestAnimationFrame(() => {
                this.rafPending = false;
                this.draw();
            });
        }
    }

    draw() {
        const canvas = this._canvas;
        const ctx = this._ctx;
        if (!ctx || canvas.width === 0 || canvas.height === 0) return;

        const W = canvas.width;
        const H = canvas.height;

        // ── 1. Clear ──
        ctx.fillStyle = '#1e1e1e';
        ctx.fillRect(0, 0, W, H);

        // ── 2. Compute world-to-canvas transform ──
        const boundsW = Math.max(this.maxX - this.minX, MIN_EXTENT);
        const boundsH = Math.max(this.maxY - this.minY, MIN_EXTENT);
        const centerX = this.boundsSet ? (this.minX + this.maxX) / 2 : 0;
        const centerY = this.boundsSet ? (this.minY + this.maxY) / 2 : 0;

        const scale = Math.min(W, H) / (Math.max(boundsW, boundsH) * (1 + 2 * PADDING));

        // World → canvas: x right, y up (flip Y for canvas where y grows down)
        const toCanvas = (wx, wy) => ({
            x: W / 2 + (wx - centerX) * scale,
            y: H / 2 - (wy - centerY) * scale,
        });

        // ── 3. Draw grid ──
        const gridExtent = Math.max(boundsW, boundsH) * (0.5 + PADDING) + GRID_MAJOR;
        const gx0 = Math.floor((centerX - gridExtent) / GRID_MINOR) * GRID_MINOR;
        const gy0 = Math.floor((centerY - gridExtent) / GRID_MINOR) * GRID_MINOR;
        const gx1 = centerX + gridExtent;
        const gy1 = centerY + gridExtent;

        ctx.save();
        for (let gx = gx0; gx <= gx1; gx += GRID_MINOR) {
            const isMajor = Math.abs(Math.round(gx / GRID_MAJOR) * GRID_MAJOR - gx) < 0.001;
            ctx.strokeStyle = isMajor ? 'rgba(106,106,106,0.4)' : 'rgba(106,106,106,0.15)';
            ctx.lineWidth = isMajor ? 1 : 0.5;
            const cx0 = toCanvas(gx, gy0);
            const cx1 = toCanvas(gx, gy1);
            ctx.beginPath();
            ctx.moveTo(cx0.x, cx0.y);
            ctx.lineTo(cx1.x, cx1.y);
            ctx.stroke();
        }
        for (let gy = gy0; gy <= gy1; gy += GRID_MINOR) {
            const isMajor = Math.abs(Math.round(gy / GRID_MAJOR) * GRID_MAJOR - gy) < 0.001;
            ctx.strokeStyle = isMajor ? 'rgba(106,106,106,0.4)' : 'rgba(106,106,106,0.15)';
            ctx.lineWidth = isMajor ? 1 : 0.5;
            const cy0 = toCanvas(gx0, gy);
            const cy1 = toCanvas(gx1, gy);
            ctx.beginPath();
            ctx.moveTo(cy0.x, cy0.y);
            ctx.lineTo(cy1.x, cy1.y);
            ctx.stroke();
        }
        ctx.restore();

        // ── 3b. Grid labels in meters (at major lines) ──
        ctx.save();
        ctx.font = '10px monospace';
        ctx.fillStyle = 'rgba(140,140,140,0.75)';
        const fmtM = (v) => (v % 1 === 0 ? `${v}m` : `${v.toFixed(1)}m`);

        // X labels along the bottom edge (one per major vertical grid line)
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        for (let gx = Math.ceil(gx0 / GRID_MAJOR) * GRID_MAJOR; gx <= gx1; gx += GRID_MAJOR) {
            const cx = toCanvas(gx, 0).x;
            if (cx < 4 || cx > W - 4) continue;
            ctx.fillText(fmtM(Math.round(gx * 100) / 100), cx, H - 13);
        }

        // Y labels along the left edge (one per major horizontal grid line)
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        for (let gy = Math.ceil(gy0 / GRID_MAJOR) * GRID_MAJOR; gy <= gy1; gy += GRID_MAJOR) {
            const cy = toCanvas(0, gy).y;
            if (cy < 4 || cy > H - 4) continue;
            ctx.fillText(fmtM(Math.round(gy * 100) / 100), 2, cy);
        }
        ctx.restore();

        // ── 4. Draw origin marker ──
        const origin = toCanvas(0, 0);
        const crossSize = 6;
        ctx.strokeStyle = '#858585';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(origin.x - crossSize, origin.y);
        ctx.lineTo(origin.x + crossSize, origin.y);
        ctx.moveTo(origin.x, origin.y - crossSize);
        ctx.lineTo(origin.x, origin.y + crossSize);
        ctx.stroke();

        if (this.trail.length === 0) return;

        // ── 5. Draw trail (null entries = visual breaks from BLE dropout) ──
        const trailLen = this.trail.length;
        const fadeStart = Math.floor(trailLen * 0.8); // last 20% at full opacity

        let prev = null;
        for (let i = 0; i < trailLen; i++) {
            const pt = this.trail[i];
            if (pt === null) {
                prev = null; // lift pen — break caused by dropout
                continue;
            }
            const c = toCanvas(pt.x, pt.y);
            if (prev !== null) {
                const alpha = i < fadeStart ? 0.3 : 0.8;
                ctx.strokeStyle = `rgba(78,201,176,${alpha})`; // --vscode-accent-teal
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                ctx.moveTo(prev.x, prev.y);
                ctx.lineTo(c.x, c.y);
                ctx.stroke();
            }
            prev = c;
        }

        // ── 6. Draw robot marker (filled triangle pointing in heading direction) ──
        const last = [...this.trail].reverse().find((p) => p !== null);
        if (!last) return;
        const robotPos = toCanvas(last.x, last.y);
        // heading: 0° = +X (right on canvas, but canvas Y is flipped)
        // In world: heading 0° = +X direction, heading 90° = +Y (left in ROS = up in canvas)
        const headingRad = last.heading * (Math.PI / 180);
        // Canvas angle: right = 0, up = -π/2. World +X = canvas right, world +Y = canvas up (flipped Y)
        const canvasAngle = -headingRad; // negate because canvas Y is inverted

        const isDropout = Date.now() - this.lastPointMs > DROPOUT_MS;
        const markerColor = isDropout ? 'rgba(0,122,204,0.4)' : '#007acc';

        const size = 8;
        ctx.save();
        ctx.translate(robotPos.x, robotPos.y);
        ctx.rotate(canvasAngle);
        ctx.fillStyle = markerColor;
        ctx.beginPath();
        ctx.moveTo(size, 0); // tip (forward)
        ctx.lineTo(-size * 0.6, -size * 0.5); // back-left
        ctx.lineTo(-size * 0.6, size * 0.5); // back-right
        ctx.closePath();
        ctx.fill();
        ctx.restore();
    }
}

customElements.define('custom-map', MapHTMLElement);
