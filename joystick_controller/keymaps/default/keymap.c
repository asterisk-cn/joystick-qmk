#include QMK_KEYBOARD_H

// Joystick axis configuration
// JOYSTICK_AXIS_IN(pin, low, rest, high)
//   low:  ADC value when stick is at minimum position
//   rest: ADC value when stick is at center (neutral)
//   high: ADC value when stick is at maximum position
// Adjust these values based on your actual stick readings.
joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    JOYSTICK_AXIS_IN(GP26, 0, 512, 1023),  // X axis
    JOYSTICK_AXIS_IN(GP27, 0, 512, 1023),  // Y axis
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_NO),
};
