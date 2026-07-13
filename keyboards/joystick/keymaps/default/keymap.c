#include QMK_KEYBOARD_H
#include "analog.h"
#include "raw_hid.h"

#define JOY_X_PIN GP26
#define JOY_Y_PIN GP27

#define DEFAULT_X_LOW 0
#define DEFAULT_X_REST 512
#define DEFAULT_X_HIGH 1023
#define DEFAULT_Y_LOW 0
#define DEFAULT_Y_REST 512
#define DEFAULT_Y_HIGH 1023
#define DEFAULT_DEADZONE 25

#define JOY_AXIS_MAX 511

enum hid_cmd {
    CMD_GET_RAW  = 0x01,
    CMD_GET_CAL  = 0x02,
    CMD_SET_CAL  = 0x03,
    CMD_SAVE_CAL = 0x04,
    CMD_STREAM   = 0x05,
};

typedef struct {
    int16_t x_low;
    int16_t x_rest;
    int16_t x_high;
    int16_t y_low;
    int16_t y_rest;
    int16_t y_high;
    int16_t deadzone;
} calibration_t;

static calibration_t cal;
static bool streaming;

joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    JOYSTICK_AXIS_VIRTUAL,
    JOYSTICK_AXIS_VIRTUAL,
};

static int16_t scale_axis(int16_t raw, int16_t low, int16_t rest, int16_t high) {
    int32_t out;
    int16_t dz = cal.deadzone;
    if (raw >= rest + dz) {
        int16_t span = high - rest - dz;
        if (span <= 0) return JOY_AXIS_MAX;
        out = (int32_t)(raw - rest - dz) * JOY_AXIS_MAX / span;
    } else if (raw <= rest - dz) {
        int16_t span = rest - dz - low;
        if (span <= 0) return -JOY_AXIS_MAX;
        out = (int32_t)(raw - rest + dz) * JOY_AXIS_MAX / span;
    } else {
        return 0;
    }
    if (out > JOY_AXIS_MAX) out = JOY_AXIS_MAX;
    if (out < -JOY_AXIS_MAX) out = -JOY_AXIS_MAX;
    return (int16_t)out;
}

static void send_raw_report(int16_t raw_x, int16_t raw_y, int16_t out_x, int16_t out_y) {
    uint8_t buf[32] = {0};
    buf[0] = CMD_GET_RAW;
    buf[1] = raw_x >> 8;  buf[2] = raw_x & 0xFF;
    buf[3] = raw_y >> 8;  buf[4] = raw_y & 0xFF;
    buf[5] = out_x >> 8;  buf[6] = out_x & 0xFF;
    buf[7] = out_y >> 8;  buf[8] = out_y & 0xFF;
    raw_hid_send(buf, sizeof(buf));
}

static void send_cal_report(void) {
    uint8_t buf[32] = {0};
    buf[0]  = CMD_GET_CAL;
    buf[1]  = cal.x_low >> 8;    buf[2]  = cal.x_low & 0xFF;
    buf[3]  = cal.x_rest >> 8;   buf[4]  = cal.x_rest & 0xFF;
    buf[5]  = cal.x_high >> 8;   buf[6]  = cal.x_high & 0xFF;
    buf[7]  = cal.y_low >> 8;    buf[8]  = cal.y_low & 0xFF;
    buf[9]  = cal.y_rest >> 8;   buf[10] = cal.y_rest & 0xFF;
    buf[11] = cal.y_high >> 8;   buf[12] = cal.y_high & 0xFF;
    buf[13] = cal.deadzone >> 8;  buf[14] = cal.deadzone & 0xFF;
    raw_hid_send(buf, sizeof(buf));
}

void eeconfig_init_user(void) {
    cal = (calibration_t){
        DEFAULT_X_LOW, DEFAULT_X_REST, DEFAULT_X_HIGH,
        DEFAULT_Y_LOW, DEFAULT_Y_REST, DEFAULT_Y_HIGH,
        DEFAULT_DEADZONE,
    };
    eeconfig_update_user_datablock(&cal, 0, sizeof(cal));
}

void keyboard_post_init_user(void) {
    eeconfig_read_user_datablock(&cal, 0, sizeof(cal));
    if (cal.x_high <= cal.x_rest || cal.x_rest <= cal.x_low ||
        cal.y_high <= cal.y_rest || cal.y_rest <= cal.y_low ||
        cal.deadzone < 0 || cal.deadzone > 500) {
        eeconfig_init_user();
    }
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    switch (data[0]) {
        case CMD_GET_RAW: {
            int16_t rx = analogReadPin(JOY_X_PIN);
            int16_t ry = analogReadPin(JOY_Y_PIN);
            send_raw_report(rx, ry,
                scale_axis(rx, cal.x_low, cal.x_rest, cal.x_high),
                scale_axis(ry, cal.y_low, cal.y_rest, cal.y_high));
            break;
        }
        case CMD_GET_CAL:
            send_cal_report();
            break;
        case CMD_SET_CAL:
            cal.x_low    = (data[1] << 8)  | data[2];
            cal.x_rest   = (data[3] << 8)  | data[4];
            cal.x_high   = (data[5] << 8)  | data[6];
            cal.y_low    = (data[7] << 8)  | data[8];
            cal.y_rest   = (data[9] << 8)  | data[10];
            cal.y_high   = (data[11] << 8) | data[12];
            cal.deadzone = (data[13] << 8) | data[14];
            send_cal_report();
            break;
        case CMD_SAVE_CAL: {
            eeconfig_update_user_datablock(&cal, 0, sizeof(cal));
            uint8_t buf[32] = {0};
            buf[0] = CMD_SAVE_CAL;
            buf[1] = 0x01;
            raw_hid_send(buf, sizeof(buf));
            break;
        }
        case CMD_STREAM:
            streaming = data[1] != 0;
            break;
    }
}

void housekeeping_task_user(void) {
    int16_t raw_x = analogReadPin(JOY_X_PIN);
    int16_t raw_y = analogReadPin(JOY_Y_PIN);
    int16_t out_x = scale_axis(raw_x, cal.x_low, cal.x_rest, cal.x_high);
    int16_t out_y = scale_axis(raw_y, cal.y_low, cal.y_rest, cal.y_high);

    joystick_set_axis(0, out_x);
    joystick_set_axis(1, -out_y);

    if (streaming) {
        static uint16_t last_stream = 0;
        if (timer_elapsed(last_stream) > 16) {
            last_stream = timer_read();
            send_raw_report(raw_x, raw_y, out_x, out_y);
        }
    }
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_NO),
};
