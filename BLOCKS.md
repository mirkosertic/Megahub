# Blockly Blocks Documentation

**Generated:** 2026-01-26

This documentation covers all 73 blocks used in the Megahub project, including 25 custom blocks and 48 standard Blockly blocks with custom colors.

Block images are rendered as high-quality PNG screenshots to accurately show all block features including text inputs, checkboxes, and statement blocks.

## Table of Contents

- [Control flow](#control-flow) (4 blocks)
- [Logic](#logic) (7 blocks)
- [Loop](#loop) (3 blocks)
- [Math](#math) (13 blocks)
- [Text](#text) (13 blocks)
- [Lists](#lists) (12 blocks)
- [I/O](#i-o) (5 blocks)
- [LEGO©](#lego-) (2 blocks)
- [Gamepad](#gamepad) (5 blocks)
- [FastLED](#fastled) (4 blocks)
- [IMU](#imu) (1 blocks)
- [UI](#ui) (1 blocks)
- [Algorithms](#algorithms) (1 blocks)
- [Debug](#debug) (2 blocks)

## Control flow

### mh_init

![mh_init](docs/blocks/control-flow/mh_init.png)

**Description:** Initialization

**Message:** `Initialization do %1`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| DO | input_statement | Any |

---

### mh_startthread

![mh_startthread](docs/blocks/control-flow/mh_startthread.png)

**Description:** Starts a thread

**Type:** Custom Statement Block

**Message:** `Start thread %1 with stacksize %2 and profiling %3 and do %4`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| STACKSIZE | input_value | Number |
| DO | input_statement | Any |

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| NAME | field_input | Default: `Name` |
| PROFILING | field_checkbox | - |

---

### mh_wait

![mh_wait](docs/blocks/control-flow/mh_wait.png)

**Description:** Wait

**Type:** Custom Statement Block

**Message:** `Wait %1ms`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| VALUE | input_value | Number |

---

### mh_stopthread

![mh_stopthread](docs/blocks/control-flow/mh_stopthread.png)

**Description:** Stops a thread

**Type:** Custom Statement Block

**Message:** `Stop thread %1`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| HANDLE | input_value | Any |

---

## Logic

### controls_if

![controls_if](docs/blocks/logic/controls_if.png)

**Description:** Conditional block that executes code based on a condition

**Type:** Standard Blockly Block

---

### logic_compare

![logic_compare](docs/blocks/logic/logic_compare.png)

**Description:** Compare two values (equal, not equal, less than, greater than, etc.)

**Type:** Standard Blockly Block

---

### logic_operation

![logic_operation](docs/blocks/logic/logic_operation.png)

**Description:** Logical operations (AND, OR)

**Type:** Standard Blockly Block

---

### logic_negate

![logic_negate](docs/blocks/logic/logic_negate.png)

**Description:** Negate a boolean value (NOT)

**Type:** Standard Blockly Block

---

### logic_boolean

![logic_boolean](docs/blocks/logic/logic_boolean.png)

**Description:** Boolean value (true or false)

**Type:** Standard Blockly Block

---

### logic_null

![logic_null](docs/blocks/logic/logic_null.png)

**Description:** Null value

**Type:** Standard Blockly Block

---

### logic_ternary

![logic_ternary](docs/blocks/logic/logic_ternary.png)

**Description:** Ternary conditional (if-then-else expression)

**Type:** Standard Blockly Block

---

## Loop

### controls_repeat_ext

![controls_repeat_ext](docs/blocks/loop/controls_repeat_ext.png)

**Description:** Repeat a set of statements a specified number of times

**Type:** Standard Blockly Block

---

### controls_for

![controls_for](docs/blocks/loop/controls_for.png)

**Description:** Count from a start number to an end number by a given increment

**Type:** Standard Blockly Block

---

### controls_flow_statements

![controls_flow_statements](docs/blocks/loop/controls_flow_statements.png)

**Description:** Break out of or continue a loop

**Type:** Standard Blockly Block

---

## Math

### math_number

![math_number](docs/blocks/math/math_number.png)

**Description:** A number value

**Type:** Standard Blockly Block

---

### math_arithmetic

![math_arithmetic](docs/blocks/math/math_arithmetic.png)

**Description:** Arithmetic operations (add, subtract, multiply, divide, power)

**Type:** Standard Blockly Block

---

### math_single

![math_single](docs/blocks/math/math_single.png)

**Description:** Single number operations (square root, absolute, negate, etc.)

**Type:** Standard Blockly Block

---

### math_trig

![math_trig](docs/blocks/math/math_trig.png)

**Description:** Trigonometric functions (sin, cos, tan, asin, acos, atan)

**Type:** Standard Blockly Block

---

### math_constant

![math_constant](docs/blocks/math/math_constant.png)

**Description:** Mathematical constants (pi, e, phi, sqrt(2), etc.)

**Type:** Standard Blockly Block

---

### math_number_property

![math_number_property](docs/blocks/math/math_number_property.png)

**Description:** Check if a number has a property (even, odd, prime, etc.)

**Type:** Standard Blockly Block

---

### math_round

![math_round](docs/blocks/math/math_round.png)

**Description:** Round a number (round, round up, round down)

**Type:** Standard Blockly Block

---

### math_on_list

![math_on_list](docs/blocks/math/math_on_list.png)

**Description:** Perform operation on a list (sum, min, max, average, etc.)

**Type:** Standard Blockly Block

---

### math_modulo

![math_modulo](docs/blocks/math/math_modulo.png)

**Description:** Remainder of division

**Type:** Standard Blockly Block

---

### math_constrain

![math_constrain](docs/blocks/math/math_constrain.png)

**Description:** Constrain a number to be within specified limits

**Type:** Standard Blockly Block

---

### math_random_int

![math_random_int](docs/blocks/math/math_random_int.png)

**Description:** Random integer between two numbers

**Type:** Standard Blockly Block

---

### math_random_float

![math_random_float](docs/blocks/math/math_random_float.png)

**Description:** Random fraction between 0 and 1

**Type:** Standard Blockly Block

---

### math_atan2

![math_atan2](docs/blocks/math/math_atan2.png)

**Description:** Arctangent of the quotient of two numbers

**Type:** Standard Blockly Block

---

## Text

### text

![text](docs/blocks/text/text.png)

**Description:** A text string

**Type:** Standard Blockly Block

---

### text_join

![text_join](docs/blocks/text/text_join.png)

**Description:** Join multiple text strings together

**Type:** Standard Blockly Block

---

### text_append

![text_append](docs/blocks/text/text_append.png)

**Description:** Append text to a variable

**Type:** Standard Blockly Block

---

### text_length

![text_length](docs/blocks/text/text_length.png)

**Description:** Get the length of a text string

**Type:** Standard Blockly Block

---

### text_isEmpty

![text_isEmpty](docs/blocks/text/text_isEmpty.png)

**Description:** Check if a text string is empty

**Type:** Standard Blockly Block

---

### text_indexOf

![text_indexOf](docs/blocks/text/text_indexOf.png)

**Description:** Find the position of text within text

**Type:** Standard Blockly Block

---

### text_charAt

![text_charAt](docs/blocks/text/text_charAt.png)

**Description:** Get a character at a specific position

**Type:** Standard Blockly Block

---

### text_getSubstring

![text_getSubstring](docs/blocks/text/text_getSubstring.png)

**Description:** Get a substring from text

**Type:** Standard Blockly Block

---

### text_changeCase

![text_changeCase](docs/blocks/text/text_changeCase.png)

**Description:** Change the case of text (UPPERCASE, lowercase, Title Case)

**Type:** Standard Blockly Block

---

### text_trim

![text_trim](docs/blocks/text/text_trim.png)

**Description:** Trim spaces from the start and/or end of text

**Type:** Standard Blockly Block

---

### text_count

![text_count](docs/blocks/text/text_count.png)

**Description:** Count occurrences of text within text

**Type:** Standard Blockly Block

---

### text_replace

![text_replace](docs/blocks/text/text_replace.png)

**Description:** Replace text within text

**Type:** Standard Blockly Block

---

### text_print

![text_print](docs/blocks/text/text_print.png)

**Description:** Print text to the console

**Type:** Standard Blockly Block

---

## Lists

### lists_create_empty

![lists_create_empty](docs/blocks/lists/lists_create_empty.png)

**Description:** Create an empty list

**Type:** Standard Blockly Block

---

### lists_create_with

![lists_create_with](docs/blocks/lists/lists_create_with.png)

**Description:** Create a list with specified values

**Type:** Standard Blockly Block

---

### lists_repeat

![lists_repeat](docs/blocks/lists/lists_repeat.png)

**Description:** Create a list with a value repeated a number of times

**Type:** Standard Blockly Block

---

### lists_length

![lists_length](docs/blocks/lists/lists_length.png)

**Description:** Get the length of a list

**Type:** Standard Blockly Block

---

### lists_isEmpty

![lists_isEmpty](docs/blocks/lists/lists_isEmpty.png)

**Description:** Check if a list is empty

**Type:** Standard Blockly Block

---

### lists_indexOf

![lists_indexOf](docs/blocks/lists/lists_indexOf.png)

**Description:** Find the position of an item in a list

**Type:** Standard Blockly Block

---

### lists_getIndex

![lists_getIndex](docs/blocks/lists/lists_getIndex.png)

**Description:** Get an item from a list at a specific position

**Type:** Standard Blockly Block

---

### lists_setIndex

![lists_setIndex](docs/blocks/lists/lists_setIndex.png)

**Description:** Set an item in a list at a specific position

**Type:** Standard Blockly Block

---

### lists_getSublist

![lists_getSublist](docs/blocks/lists/lists_getSublist.png)

**Description:** Get a sublist from a list

**Type:** Standard Blockly Block

---

### lists_split

![lists_split](docs/blocks/lists/lists_split.png)

**Description:** Split text into a list, or join a list into text

**Type:** Standard Blockly Block

---

### lists_sort

![lists_sort](docs/blocks/lists/lists_sort.png)

**Description:** Sort a list

**Type:** Standard Blockly Block

---

### lists_reverse

![lists_reverse](docs/blocks/lists/lists_reverse.png)

**Description:** Reverse a list

**Type:** Standard Blockly Block

---

## I/O

### mh_digitalwrite

![mh_digitalwrite](docs/blocks/i-o/mh_digitalwrite.png)

**Description:** Set the state of a GPIO pin

**Type:** Custom Statement Block

**Message:** `Digital write %2 to %1`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| VALUE | input_value | Any |

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| PIN | field_dropdown | `GPIO13`, `GPIO16`, `GPIO17`, `GPIO25`, `GPIO26`, `GPIO27`, `GPIO32`, `GPIO33`, `UART1_GP4`, `UART1_GP5`, `UART1_GP6`, `UART1_GP7`, `UART2_GP4`, `UART2_GP5`, `UART2_GP6`, `UART2_GP7` |

---

### mh_pinmode

![mh_pinmode](docs/blocks/i-o/mh_pinmode.png)

**Description:** Set the mode of a GPIO pin

**Type:** Custom Statement Block

**Message:** `Set pin mode of %1 to %2`

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| PIN | field_dropdown | `GPIO13`, `GPIO16`, `GPIO17`, `GPIO25`, `GPIO26`, `GPIO27`, `GPIO32`, `GPIO33`, `UART1_GP4`, `UART1_GP5`, `UART1_GP6`, `UART1_GP7`, `UART2_GP4`, `UART2_GP5`, `UART2_GP6`, `UART2_GP7` |
| MODE | field_dropdown | `PINMODE_INPUT`, `PINMODE_INPUT_PULLUP`, `PINMODE_INPUT_PULLDOWN`, `PINMODE_OUTPUT` |

---

### mh_digitalread

![mh_digitalread](docs/blocks/i-o/mh_digitalread.png)

**Description:** Liest den Zustand eines GPIO-Pins

**Message:** `Digital Read %1`

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| PIN | field_dropdown | `GPIO13`, `GPIO16`, `GPIO17`, `GPIO25`, `GPIO26`, `GPIO27`, `GPIO32`, `GPIO33`, `GPIO34`, `GPIO35`, `GPIO36`, `GPIO39`, `UART1_GP4`, `UART1_GP5`, `UART1_GP6`, `UART1_GP7`, `UART2_GP4`, `UART2_GP5`, `UART2_GP6`, `UART2_GP7` |

---

### mh_port

![mh_port](docs/blocks/i-o/mh_port.png)

**Description:** A LEGO© intergface port

**Message:** `%1`

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| PORT | field_dropdown | `PORT1`, `PORT2`, `PORT3`, `PORT4` |

---

### mh_set_motor_speed

![mh_set_motor_speed](docs/blocks/i-o/mh_set_motor_speed.png)

**Description:** Set the speed of a connected motor

**Type:** Custom Statement Block

**Message:** `Set Motor speed of %1 to %2`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| PORT | input_value | Any |
| VALUE | input_value | Any |

---

## LEGO©

### lego_get_mode_dataset

![lego_get_mode_dataset](docs/blocks/lego-/lego_get_mode_dataset.png)

**Description:** Get a dataset value from the selected mode of a port

**Message:** `Get dataset %2 from selected mode of %1`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| PORT | input_value | Any |
| DATASET | input_value | Any |

---

### lego_select_mode

![lego_select_mode](docs/blocks/lego-/lego_select_mode.png)

**Description:** Sets the mode of a port

**Type:** Custom Statement Block

**Message:** `Set the mode of %1 to %2`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| PORT | input_value | Any |
| MODE | input_value | Any |

---

## Gamepad

### mh_gamepad_gamepad

![mh_gamepad_gamepad](docs/blocks/gamepad/mh_gamepad_gamepad.png)

**Description:** A Gamepad connection

**Message:** `%1`

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| GAMEPAD | field_dropdown | `GAMEPAD1` |

---

### mh_gamepad_buttonpressed

![mh_gamepad_buttonpressed](docs/blocks/gamepad/mh_gamepad_buttonpressed.png)

**Description:** Checks if a specific button is pressed on a connected gamepad

**Message:** `Checks if %2 is pressed on %1`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| GAMEPAD | input_value | Any |

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| BUTTON | field_dropdown | `GAMEPAD_BUTTON_1`, `GAMEPAD_BUTTON_1`, `GAMEPAD_BUTTON_2`, `GAMEPAD_BUTTON_4`, `GAMEPAD_BUTTON_5`, `GAMEPAD_BUTTON_6`, `GAMEPAD_BUTTON_7`, `GAMEPAD_BUTTON_8`, `GAMEPAD_BUTTON_9`, `GAMEPAD_BUTTON_10`, `GAMEPAD_BUTTON_11`, `GAMEPAD_BUTTON_12`, `GAMEPAD_BUTTON_13`, `GAMEPAD_BUTTON_14`, `GAMEPAD_BUTTON_15`, `GAMEPAD_BUTTON_16` |

---

### mh_gamepad_buttonsraw

![mh_gamepad_buttonsraw](docs/blocks/gamepad/mh_gamepad_buttonsraw.png)

**Description:** Gets the raw button values of a Gamepad in as a 32bit integer

**Message:** `Gets the raw button values of %1`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| GAMEPAD | input_value | Any |

---

### mh_gamepad_value

![mh_gamepad_value](docs/blocks/gamepad/mh_gamepad_value.png)

**Description:** Gets a value from a connected Gamepad

**Message:** `Get %2 from %1`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| GAMEPAD | input_value | Any |

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| VALUE | field_dropdown | `GAMEPAD_LEFT_X`, `GAMEPAD_LEFT_Y`, `GAMEPAD_RIGHT_X`, `GAMEPAD_RIGHT_Y`, `GAMEPAD_DPAD` |

---

### mh_gamepad_connected

![mh_gamepad_connected](docs/blocks/gamepad/mh_gamepad_connected.png)

**Description:** Tests if the Gamepad is connected

**Message:** `Test if %1 is connected`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| GAMEPAD | input_value | Any |

---

## FastLED

### mh_fastled_addleds

![mh_fastled_addleds](docs/blocks/fastled/mh_fastled_addleds.png)

**Description:** Initialize FastLED

**Type:** Custom Statement Block

**Message:** `Initialize FastLED of type %1 on pin %2 with %3 LEDs`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| NUM_LEDS | input_value | Any |

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| TYPE | field_dropdown | `NEOPIXEL` |
| PIN | field_dropdown | `GPIO13`, `GPIO16`, `GPIO17`, `GPIO25`, `GPIO26`, `GPIO27`, `GPIO32`, `GPIO33` |

---

### mh_fastled_show

![mh_fastled_show](docs/blocks/fastled/mh_fastled_show.png)

**Description:** Shop current FastLED values to the LED strip

**Type:** Custom Statement Block

**Message:** `FastLED show`

---

### mh_fastled_clear

![mh_fastled_clear](docs/blocks/fastled/mh_fastled_clear.png)

**Description:** Clear current FastLED values and sent them to the LED strip

**Type:** Custom Statement Block

**Message:** `FastLED clear`

---

### mh_fastled_set

![mh_fastled_set](docs/blocks/fastled/mh_fastled_set.png)

**Description:** Set LED color

**Type:** Custom Statement Block

**Message:** `Set LED #%1 to color %2`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| INDEX | input_value | Any |

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| COLOR | field_colour | - |

---

## IMU

### mh_imu_value

![mh_imu_value](docs/blocks/imu/mh_imu_value.png)

**Description:** Gets a value from the IMU

**Message:** `Get %1 from IMU`

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| VALUE | field_dropdown | `YAW`, `PITCH`, `ROLL`, `ACCELERATION_X`, `ACCELERATION_Y`, `ACCELERATION_Z` |

---

## UI

### ui_show_value

![ui_show_value](docs/blocks/ui/ui_show_value.png)

**Description:** Show value on the UI

**Type:** Custom Statement Block

**Message:** `Show value %1: %2 with style %3`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| VALUE | input_value | Any |

**Fields:**

| Name | Type | Options/Default |
|------|------|----------------|
| LABEL | field_input | Default: `Label` |
| STYLE | field_dropdown | `FORMAT_SIMPLE` |

---

## Algorithms

### mh_alg_pid

![mh_alg_pid](docs/blocks/algorithms/mh_alg_pid.png)

**Description:** Proportional-Integral-Derivative controller. Returns control output based on setpoint and process variable (PV).

**Type:** Custom Value Block

**Message:** `PID controller %1 setpoint: %2 PV: %3 
%4 Kp: %5 Ki: %6 Kd: %7 %8 
output: %9 to %10`

**Inputs:**

| Name | Type | Check |
|------|------|-------|
| undefined | input_dummy | Any |
| SETPOINT | input_value | Number |
| PV | input_value | Number |
| undefined | input_dummy | Any |
| KP | input_value | Number |
| KI | input_value | Number |
| KD | input_value | Number |
| undefined | input_dummy | Any |
| OUT_MIN | input_value | Number |
| OUT_MAX | input_value | Number |

---

## Debug

### mh_debug_free_heap

![mh_debug_free_heap](docs/blocks/debug/mh_debug_free_heap.png)

**Description:** Get free HEAP memory

**Message:** `Get free HEAP memory`

---

### mh_debug_millis

![mh_debug_millis](docs/blocks/debug/mh_debug_millis.png)

**Description:** Get the number of milliseconds since system startup

**Message:** `Get milliseconds since system startup`

---

## Generation Statistics

- Total blocks: 73
- Custom Megahub blocks: 25
- Standard Blockly blocks: 48
- PNG images generated: 73

---

*Documentation generated automatically from block definitions with high-quality PNG screenshots.*
