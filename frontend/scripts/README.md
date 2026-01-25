# Blockly Documentation Generator

This directory contains scripts for automatically generating documentation for Megahub's Blockly blocks.

## Scripts

### generate-block-docs-simple.js

Generates comprehensive markdown documentation (`BLOCKS.md`) for all Blockly blocks used in the Megahub project.

**Features:**
- Documents all 72 blocks (24 custom Megahub blocks + 48 standard Blockly blocks)
- Includes block descriptions, inputs, fields, and categories
- Organizes blocks by category (Control Flow, Logic, Math, I/O, LEGO, etc.)
- Color-coded block information

**Usage:**
```bash
npm run generate-docs
```

**Configuration:**
Set the output path via environment variable:
```bash
BLOCKS_MD_PATH=/path/to/output.md npm run generate-docs
```

Default: `/Users/mirko/Nextcloud/Schaltungen/LegoESP32/BLOCKS.md`

### generate-block-docs.js (Advanced - SVG rendering)

Full-featured version that attempts to render blocks to SVG images. Requires additional dependencies (canvas, native libraries) which may be difficult to install on some systems.

Currently not used due to dependency complexity. The simple version (above) is recommended for most use cases.

## Build Integration

The documentation generator is automatically run before each build:
- `npm run build:bt` - Runs `prebuild:bt` which generates docs
- `npm run build:web` - Runs `prebuild:web` which generates docs
- `npm run build:all` - Generates docs before building both versions

You can also run it manually with:
```bash
npm run generate-docs
```

## Output

The generated documentation includes:
- Table of contents organized by category
- For each block:
  - Block name and type
  - Color code (HSV value)
  - Description/tooltip
  - Message template
  - Inputs (with types and validation)
  - Fields (with options/defaults)
- Generation statistics

## Configuration File

Optional configuration can be set in `docs-config.json`:
```json
{
  "mdPath": "../../BLOCKS.md",
  "categories": {
    "order": [
      "Control flow",
      "Logic",
      "Loop",
      "Math",
      "Text",
      "Lists",
      "I/O",
      "LEGO©",
      "Gamepad",
      "FastLED",
      "IMU",
      "UI",
      "Debug"
    ]
  }
}
```
