# Block Documentation Generator

## Overview

The Blockly block documentation generator creates comprehensive documentation for all blocks used in the Megahub project, including visual SVG renderings of each block as it appears in the Blockly toolbar.

## Features

1. **Metadata Extraction**: Analyzes block definitions to extract detailed metadata
2. **Visual SVG Rendering**: Uses Puppeteer to render each block as an SVG image
3. **Categorized Organization**: Groups blocks by category for easy navigation
4. **Automatic Generation**: Runs automatically as part of the build process

## Generated Files

### Documentation
- **Location**: `/BLOCKS.md`
- **Content**: Complete documentation with block descriptions, parameters, and visual representations
- **Format**: Markdown with embedded SVG images

### SVG Images
- **Location**: `/docs/blocks/`
- **Organization**: Subdirectories by category (e.g., `control-flow/`, `i-o/`, `gamepad/`)
- **Count**: 72 blocks (24 custom + 48 standard Blockly blocks)
- **Total Size**: ~320KB

## Architecture

### Components

1. **Main Script**: `/frontend/scripts/generate-block-docs-simple.js`
   - Loads block definitions from individual block files
   - Extracts metadata from block definitions
   - Orchestrates SVG rendering via Puppeteer
   - Generates markdown documentation

2. **HTML Renderer**: `/frontend/scripts/block-renderer.html`
   - Loads Blockly from CDN
   - Provides functions to register and render blocks
   - Exports blocks as SVG
   - Runs in headless browser via Puppeteer

3. **Block Definitions**: `/frontend/src/components/blockly/*.js`
   - Individual block definition files
   - Export `definition` object with block metadata and generator code

### Workflow

```
npm run generate-docs
  └─> node generate-block-docs-simple.js
       ├─> Load block definitions from ES6 modules
       ├─> Extract metadata from each block
       ├─> Launch Puppeteer browser
       │    ├─> Load block-renderer.html
       │    ├─> For each block:
       │    │    ├─> Register block definition (if custom)
       │    │    ├─> Render block to SVG
       │    │    └─> Save SVG to docs/blocks/{category}/{blockname}.svg
       │    └─> Close browser
       ├─> Generate markdown with embedded image references
       └─> Write to BLOCKS.md
```

## Usage

### Manual Generation

```bash
cd frontend
npm run generate-docs
```

### Automatic Generation

The documentation is automatically regenerated during the build process:

```bash
npm run prebuild:bt  # Before building for Bluetooth
npm run prebuild:web # Before building for web
```

This is configured in `package.json`:

```json
{
  "scripts": {
    "prebuild:bt": "npm run generate-docs",
    "prebuild:web": "npm run generate-docs"
  }
}
```

## Dependencies

### NPM Packages
- **puppeteer**: ^24.15.0 - Headless browser for SVG rendering
- **blockly**: ^12.3.1 - Loaded in HTML renderer via CDN

### System Requirements
- Node.js (ES6 module support)
- Chrome/Chromium (installed by Puppeteer)

## Block Categories

The documentation organizes blocks into the following categories:

1. **Control flow** (4 blocks) - Initialization, threads, wait
2. **Logic** (7 blocks) - Conditionals, comparisons, boolean operations
3. **Loop** (3 blocks) - Repeat, for, flow control
4. **Math** (13 blocks) - Numbers, arithmetic, trigonometry
5. **Text** (13 blocks) - String manipulation
6. **Lists** (12 blocks) - List operations
7. **I/O** (5 blocks) - Digital I/O, pins, motors
8. **LEGO©** (2 blocks) - LEGO sensor integration
9. **Gamepad** (5 blocks) - Gamepad input handling
10. **FastLED** (4 blocks) - RGB LED control
11. **IMU** (1 block) - Inertial measurement unit
12. **UI** (1 block) - User interface
13. **Debug** (2 blocks) - Debugging utilities

## SVG Rendering Details

### Process
1. Puppeteer launches a headless Chrome instance
2. Loads the HTML renderer with Blockly
3. For each block:
   - Registers the block definition (custom blocks only)
   - Creates a block instance in a temporary workspace
   - Extracts the SVG from the rendered block
   - Calculates bounding box with padding
   - Saves as standalone SVG file

### SVG Characteristics
- **Format**: Standalone SVG with embedded styles
- **Sizing**: Auto-calculated based on block content
- **Padding**: 5px around each block
- **Attributes**: Proper viewBox for scaling
- **Quality**: Clean, vector-based images

## Customization

### Adding New Blocks

1. Create a new block definition file in `/frontend/src/components/blockly/`
2. Export a `definition` object with:
   - `category`: Block category name
   - `colour`: HSV color value
   - `blockdefinition`: Blockly JSON definition
   - `generator`: Code generator function

3. Import the block in `generate-block-docs-simple.js`
4. Add to the `blockModules` object
5. Run `npm run generate-docs`

### Customizing Output

Edit `generate-block-docs-simple.js` to:
- Change markdown formatting
- Adjust SVG padding/sizing
- Modify category order
- Add additional metadata

### Styling SVG Output

Edit `block-renderer.html` to:
- Change Blockly theme
- Adjust SVG rendering parameters
- Modify block appearance

## Troubleshooting

### Common Issues

**Issue**: Puppeteer timeout errors
- **Solution**: Increase timeout in `page.goto()` and `page.waitForFunction()`
- **Location**: `generate-block-docs-simple.js`, line ~317

**Issue**: SVG files not generated for certain blocks
- **Solution**: Check browser console logs, verify block definition syntax
- **Debug**: Set Puppeteer to `headless: false` to see rendering

**Issue**: Missing dependencies
- **Solution**: Run `npm install` in the frontend directory

**Issue**: Incorrect SVG sizing
- **Solution**: Adjust padding values in `block-renderer.html`, `renderBlockToSVG()` function

## Performance

- **Generation Time**: ~30-60 seconds for all 72 blocks
- **Browser Memory**: ~200MB during rendering
- **Output Size**: ~320KB total for all SVGs
- **Cache**: No caching - regenerates all files each time

## Future Enhancements

Potential improvements:
- [ ] Add caching to skip unchanged blocks
- [ ] Generate PNG/WebP versions for broader compatibility
- [ ] Add interactive preview in documentation
- [ ] Export block catalog as JSON
- [ ] Add block search functionality
- [ ] Generate code examples for each block
- [ ] Add block usage statistics from example projects

## Maintenance

The documentation generator is stable and requires minimal maintenance. Update the Puppeteer version periodically to ensure compatibility with the latest Chrome version.

## References

- [Blockly Developer Tools](https://developers.google.com/blockly)
- [Puppeteer Documentation](https://pptr.dev/)
- [SVG Specification](https://www.w3.org/TR/SVG2/)
