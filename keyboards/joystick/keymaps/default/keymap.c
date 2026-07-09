#include QMK_KEYBOARD_H
#include "analog.h"

// ================= 調整パラメータ =================
// キャリブレーション手順:
//   1. JOY_DEBUG を 1 にしてビルド・書き込み
//   2. `qmk console` で生のADC値を確認（中心・上下左右の端）
//   3. 実測値を以下の LOW/REST/HIGH に反映し、JOY_DEBUG を 0 に戻す
#define JOY_DEBUG 0

#define JOY_X_PIN GP26
#define JOY_X_LOW 0     // 左いっぱいの実測値
#define JOY_X_REST 512  // 中心の実測値
#define JOY_X_HIGH 1023 // 右いっぱいの実測値

#define JOY_Y_PIN GP27
#define JOY_Y_LOW 0
#define JOY_Y_REST 512
#define JOY_Y_HIGH 1023

// デッドゾーン: 中心からこのADC値以内は 0 扱い（10bit = 0-1023 基準）
#define JOY_DEADZONE 25
// ==================================================

#define JOY_AXIS_MAX 511 // JOYSTICK_AXIS_RESOLUTION 10 -> -511..511

joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    JOYSTICK_AXIS_VIRTUAL, // X
    JOYSTICK_AXIS_VIRTUAL, // Y
};

// 生のADC値をデッドゾーン適用済みの -511..511 に変換する。
// デッドゾーンの端から可動域の端までをフルスケールに引き直すので、
// デッドゾーンを抜けた直後に値が跳ばない。
static int16_t scale_axis(int16_t raw, int16_t low, int16_t rest, int16_t high) {
    int32_t out;
    if (raw >= rest + JOY_DEADZONE) {
        int16_t span = high - rest - JOY_DEADZONE;
        if (span <= 0) return JOY_AXIS_MAX;
        out = (int32_t)(raw - rest - JOY_DEADZONE) * JOY_AXIS_MAX / span;
    } else if (raw <= rest - JOY_DEADZONE) {
        int16_t span = rest - JOY_DEADZONE - low;
        if (span <= 0) return -JOY_AXIS_MAX;
        out = (int32_t)(raw - rest + JOY_DEADZONE) * JOY_AXIS_MAX / span;
    } else {
        return 0;
    }
    if (out > JOY_AXIS_MAX) out = JOY_AXIS_MAX;
    if (out < -JOY_AXIS_MAX) out = -JOY_AXIS_MAX;
    return (int16_t)out;
}

void housekeeping_task_user(void) {
    int16_t raw_x = analogReadPin(JOY_X_PIN);
    int16_t raw_y = analogReadPin(JOY_Y_PIN);

    int16_t out_x = scale_axis(raw_x, JOY_X_LOW, JOY_X_REST, JOY_X_HIGH);
    int16_t out_y = scale_axis(raw_y, JOY_Y_LOW, JOY_Y_REST, JOY_Y_HIGH);

    joystick_set_axis(0, out_x);
    joystick_set_axis(1, out_y);

#if JOY_DEBUG
    static uint16_t last_print = 0;
    if (timer_elapsed(last_print) > 100) {
        last_print = timer_read();
        uprintf("raw x:%4d y:%4d -> out x:%4d y:%4d\n", raw_x, raw_y, out_x, out_y);
    }
#endif
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_NO),
};
