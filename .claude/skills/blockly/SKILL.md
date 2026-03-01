---
name: blockly
description: Blockly visual programming context for the Megahub project. Use when adding, modifying, or debugging custom Blockly blocks, the Lua code generator, the toolbox, block colors, or the BLOCKS.md documentation generation script.
---

# Blockly Integration — Megahub IDE

## Overview

- **Blockly 12.3.1** with the built-in **Lua code generator** (`blockly/lua`)
- Custom Web Component `<custom-blockly>` in `frontend/src/components/blockly/component.js`
- Each custom block is a **self-contained JS file** with definition + Lua generator + color
- All 25 custom blocks auto-register; `BLOCKS.md` is auto-generated via Puppeteer

---

## File Structure

```
frontend/src/components/blockly/
  component.js          Main component — init, toolbox, workspace, Lua generation
  component.html        Minimal web component template
  colors.js             Color hue constants for all categories (0–360)
  mh_*.js               20 Megahub core blocks (motors, GPIO, threads, debug)
  lego_*.js             2 LEGO Powered Up blocks
  ui_*.js               1 UI display block
```

---

## Block Definition Pattern

Every block is one file. All fields are required unless noted:

```javascript
export const definition = {
    category: 'Category Name',      // Groups this block in the toolbox
    colour: colorConstant,          // Import from colors.js — never hardcode
    blockdefinition: {              // Standard Blockly JSON definition
        type: 'mh_block_name',      // Must match filename (minus .js)
        message0: 'label %1 %2',    // %N = input/field placeholders
        args0: [ /* inputs */ ],
        previousStatement: null,    // Include for statement blocks
        nextStatement: null,        // Include for statement blocks
        // OR: output: null         // For value blocks that return a value
        tooltip: 'Description for BLOCKS.md and hover UI',
        helpUrl: ''
    },
    generator: (block, generator) => {
        // Return string + "\n" for statement blocks
        // Return [ "code", precedence ] for value blocks
    },
    inputsForToolbox: { /* optional shadow defaults */ }
};
```

### Statement block generator pattern
```javascript
generator: (block, generator) => {
    const val = generator.valueToCode(block, 'VALUE', 0);
    const stmt = generator.statementToCode(block, 'DO');
    const field = block.getFieldValue('FIELD_NAME');
    return `myFunction(${val}, ${field})\n`;
}
```

### Value block generator pattern
```javascript
generator: (block, generator) => {
    const val = block.getFieldValue('VALUE');
    return [ val, 0 ];   // [ code, precedence ]
}
```

---

## Naming Conventions

| Prefix | Category | Example |
|--------|----------|---------|
| `mh_` | Megahub core | `mh_set_motor_speed`, `mh_wait`, `mh_startthread` |
| `lego_` | LEGO Powered Up | `lego_get_mode_dataset`, `lego_select_mode` |
| `ui_` | User interface | `ui_show_value` |

- All lowercase, underscores only
- Block type ID **must match** the JS filename (e.g., `mh_wait.js` → `type: 'mh_wait'`)

---

## Color System (`colors.js`)

Colors are Blockly hue values (0–360). Always import — never hardcode a number.

| Constant | Hue | Category |
|----------|-----|----------|
| `colorControlFlow` | 20 | Control flow |
| `colorLogic` | 40 | Logic |
| `colorMath` | 60 | Math |
| `colorText` | 80 | Text |
| `colorLists` | 100 | Lists |
| `colorIO` | 120 | I/O |
| `colorLego` | 140 | LEGO© |
| `colorGamepad` | 160 | Gamepad |
| `colorFastLED` | 180 | FastLED |
| `colorIMU` | 200 | IMU |
| `colorUI` | 220 | UI |
| `colorAlgorithms` | 230 | Algorithms |
| `colorDebug` | 240 | Debug |

---

## Adding a New Block

1. **Create** `frontend/src/components/blockly/mh_myblock.js` using the pattern above
2. **Import and register** in `component.js`:
   - Add `import * as mh_myblock from './mh_myblock.js'` with the other imports
   - Add `mh_myblock` to the `customBlocks` object
3. **Toolbox**: automatic — `generateToolbox()` groups by `category` field
4. **Test** in dev mode: `npm run dev`
5. **Regenerate docs**: `npm run generate-docs` (see below)

The `component.js` registration loop handles everything:
```javascript
// Blockly block definition
Blockly.Blocks[type].init = function() { this.jsonInit(def.blockdefinition); }
// Lua generator
luaGenerator.forBlock[type] = (block) => def.generator(block, luaGenerator);
```

---

## Lua Code Generation

The workspace generates Lua code on every change:

```javascript
// In component.js — called on workspace change events
generateLUAPreview() {
    const code = luaGenerator.workspaceToCode(this.workspace);
    // code is the complete Lua script
}
```

Generated Lua script structure (from `mh_init` and `mh_startthread` blocks):
```lua
hub.init(function()
    -- initialization block contents
end)

hub.startthread('Thread Name', blockId, stackSize, false, function()
    -- thread loop contents
end)
```

---

## BLOCKS.md Auto-Generation

`BLOCKS.md` is **automatically generated** — never edit it manually.

**Command:**
```bash
cd frontend
npm run generate-docs
```

**How it works** (`frontend/scripts/generate-block-docs-simple.js`):
1. Runs `vite build` to produce a renderer bundle
2. Launches **Puppeteer** (headless Chrome)
3. Loads `block-renderer.html` which runs Blockly in the browser
4. For each block: renders to SVG → screenshot to PNG at 2× DPI
5. Saves PNGs to `docs/blocks/{category}/{blocktype}.png`
6. Generates Markdown with: block image, tooltip description, type, message template, inputs table, fields table
7. Appends a statistics summary (block counts by type and category)

**Output structure:**
```
docs/blocks/
  control flow/mh_init.png
  lego©/lego_get_mode_dataset.png
  ...
BLOCKS.md   ← regenerated in full
```

**When to regenerate**: after adding, removing, or changing any block definition, tooltip, inputs, or fields. The `tooltip` field in `blockdefinition` becomes the description in BLOCKS.md — keep it informative.

---

## Toolbox Toolbox Shadow Defaults (`inputsForToolbox`)

To pre-populate inputs in the toolbox with useful defaults, add `inputsForToolbox`:

```javascript
inputsForToolbox: {
    VALUE: {
        shadow: {
            type: 'math_number',
            fields: { NUM: 500 }
        }
    }
}
```

The `generateToolbox()` function reads this and adds `<shadow>` elements automatically.
