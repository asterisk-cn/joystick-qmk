# Joystick Controller QMK Firmware

## 使用部品

- JT8P-3.2T-B10K-1-16Y
- Waveshare RP2040-Zero

## ビルド

### GitHub Actions
`main` ブランチへのpushで自動ビルド

### ローカルビルド
```bash
qmk compile -kb joystick -km default
```

## 書き込み
1. RP2040-ZeroのBOOTボタンを押しながらUSB接続
2. `RPI-RP2` ドライブにUF2をドラッグ＆ドロップ

## 配線

| アナログスティック | RP2040-Zero | 用途 |
|:---:|:---:|---|
| Pin 1 | 3V3 | X軸 VCC |
| Pin 2 | GP26 | X軸 ワイパー (ADC) |
| Pin 3 | GND | X軸 GND |
| Pin 4 (1') | 3V3 | Y軸 VCC |
| Pin 5 (2') | GP27 | Y軸 ワイパー (ADC) |
| Pin 6 (3') | GND | Y軸 GND |

## キャリブレーション
`keymap.c` の `joystick_axes` 配列で `low`, `rest`, `high` の値を実測値に合わせて調整
