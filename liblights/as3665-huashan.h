/*
 * Copyright (C) 2012-2013 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ===================================================================== */
/* === LibLights AS3665 Sequencer === */
char const*const LEDS_SEQUENCER_LOAD_PROGRAM      = "000e0e9d009c0e9c8f9d80%02xff9dc0%02xff9d80%02xff9dc0%02xff9d80a004c00000000%03x\n";
char const*const LEDS_SEQUENCER_MODE_DISABLED     = "disabled";
char const*const LEDS_SEQUENCER_MODE_ACTIVATED    = "reload";
char const*const LEDS_SEQUENCER_RUN_DISABLED      = "hold";
char const*const LEDS_SEQUENCER_RUN_ACTIVATED     = "run";
const int LEDS_SEQUENCER_BLINK_CMD_ALL            = 511;
const int LEDS_SEQUENCER_BLINK_CMD_SIDES          = 438; 
const int LEDS_SEQUENCER_BLINK_CMD_MIDDLE         = 73;
const int LEDS_SEQUENCER_BLINK_RAMPUP_SMOOTH      = 2;
const int LEDS_SEQUENCER_BLINK_RAMPDOWN_SMOOTH    = 3;
const int LEDS_SEQUENCER_BLINK_FULLLIGHTS         = 511;
const float LEDS_SEQUENCER_SECOND_TIME            = 13.0;

/* ===================================================================== */
/* === Module AS3665 Sequence details === */
/*
  ==[ Common structure ]==

    00 0e0e 9d00 9c0e 9c8f 9d80 00ff 9dc0 00ff 9d80 %%ff 9dc0 %%ff 9d80 a004 c000 0000 0%%%

  ==[ Detailed structure ]==

      00 : Initialization.
    0e0e : Start the PWM with 16 clock cycles per step (0.49ms at 32768Hz), 7 steps, ramp up, 14 ramp cycles.
    9d00 : Clear the MUX table.
    9c0e : Set MUX start address to 14.
    9c8f : Set MUX end address to 15.
    9d80 : Increase the MUX pointer by one (or cycle).
    %%ff (02ff) : Flash turning on delay (0x00 = instant, 0x02 = smooth).
    9dc0 : Decrease the MUX pointer by one (or cycle).
    %%ff (0dff) : Flash turnied on delay (0x00 = 0ms, 0x0d = 1000ms).
    9d80 : Increase the MUX pointer by one (or cycle).
    %%ff (03ff) : Flash turning off delay (0x01 = instant, 0x03 = smooth).
    9dc0 : Decrease the MUX pointer by one (or cycle).
    %%ff (0dff) : Flash turned off delay (0x00 = 0ms, 0x0d = 1000ms).
    9d80 : Increase the MUX pointer by one (or cycle).
    a004 : Create an infinite loop, with 4 steps.
    c000 : End command, no interrupt, increment program counter.
    0000 : Goto sequencer program start.
    0%%% (0fff) : Trigger the concerned RGB LEDs.

*/
