#pragma once

#include <system.h>
#include <stdint.h>

/// 半精度浮動小数点数をuint16_tに格納するために型宣言する
using __fp16 = uint16_t;

/// ADC1の出力値と電流との換算係数 [A/LSB]
static constexpr float ADC1_CURRENT_SCALE = 1.0f / 1977.5390625f;

/// IMUの加速度の換算係数 [m/s^2/LSB]
static constexpr float IMU_ACCELEROMETER_SCALE = 4.78515625e-3f;

/// IMUの角速度の換算係数 [rad/s/LSB]
static constexpr float IMU_GYROSCOPE_SCALE =  1.06526444e-3f;

/// IMUの割り込み周期 [Hz]
static constexpr float IMU_OUTPUT_RATE = 1000;

/// 車輪の実効的な円周 [m]
static constexpr float WHEEL_CIRCUMFERENCE = 0.157f;

/// 車輪が一回転したときのエンコーダのパルス数
static constexpr float ENCODER_PPR = 4096;







/**
 * 単精度浮動小数点数を半精度浮動小数点数に変換する
 * カスタム命令により高速に変換できる
 * @param src 単精度浮動小数点数
 * @return 半精度浮動小数点数 (上位16bitは0)
 */
static inline int Fp32ToFp16(float src) {
    return __builtin_custom_inf(ALT_CI_FLOAT32TO16_0_N, src);
}