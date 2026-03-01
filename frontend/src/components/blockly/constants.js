// Pin group definitions — single source of truth for all Blockly GPIO dropdowns.
// Each entry is a Blockly dropdown option: [display label, value emitted as Lua code].
// These must exactly match the globals registered in lib/megahub/src/megahub.cpp newLuaState().

/** Output-capable ESP32 GPIO pins: support input, output, and FastLED. */
export const GPIO_OUTPUT_PINS = [
    ['GPIO13', 'GPIO13'],
    ['GPIO16', 'GPIO16'],
    ['GPIO17', 'GPIO17'],
    ['GPIO25', 'GPIO25'],
    ['GPIO26', 'GPIO26'],
    ['GPIO27', 'GPIO27'],
    ['GPIO32', 'GPIO32'],
    ['GPIO33', 'GPIO33'],
];

/** Input-only ESP32 GPIO pins: hardware output buffer absent, read-only. */
export const GPIO_INPUT_ONLY_PINS = [
    ['GPIO34', 'GPIO34'],
    ['GPIO35', 'GPIO35'],
    ['GPIO36', 'GPIO36'],
    ['GPIO39', 'GPIO39'],
];

/** All ESP32 GPIO pins (output-capable + input-only). */
export const GPIO_ALL_PINS = [...GPIO_OUTPUT_PINS, ...GPIO_INPUT_ONLY_PINS];

/** GPIO expansion pins on the WeDo device connected to LEGO port 1. */
export const UART1_PINS = [
    ['UART1_GP4', 'UART1_GP4'],
    ['UART1_GP5', 'UART1_GP5'],
    ['UART1_GP6', 'UART1_GP6'],
    ['UART1_GP7', 'UART1_GP7'],
];

/** GPIO expansion pins on the WeDo device connected to LEGO port 3. */
export const UART2_PINS = [
    ['UART2_GP4', 'UART2_GP4'],
    ['UART2_GP5', 'UART2_GP5'],
    ['UART2_GP6', 'UART2_GP6'],
    ['UART2_GP7', 'UART2_GP7'],
];

/** All UART expansion pins (port 1 and port 3). */
export const UART_ALL_PINS = [...UART1_PINS, ...UART2_PINS];

/** Pins valid for hub.pinMode() and hub.digitalWrite(): output-capable GPIO + UART expansion. */
export const PINS_READWRITE = [...GPIO_OUTPUT_PINS, ...UART_ALL_PINS];

/** Pins valid for hub.digitalRead(): all GPIO + UART expansion. */
export const PINS_READ = [...GPIO_ALL_PINS, ...UART_ALL_PINS];

/** Pins valid for fastled.addleds(): output-capable GPIO only. */
export const PINS_FASTLED = GPIO_OUTPUT_PINS;
