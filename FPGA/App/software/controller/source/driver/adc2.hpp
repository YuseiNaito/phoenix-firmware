#pragma once

#include <stdint.h>
#include <system.h>
#include <peripheral/i2c_master.hpp>

class Adc2 {
private:
    static constexpr uint32_t I2C_BASE = I2C_MASTER_0_BASE;
    static constexpr int I2C_IC_ID = I2C_MASTER_0_IRQ_INTERRUPT_CONTROLLER_ID;
    static constexpr int I2C_IRQ = I2C_MASTER_0_IRQ;
    static constexpr int I2C_ADDRESS = 0x48;
    static constexpr int NUMBER_OF_SEQUENCE = 2;

    enum {
        ADS1015_CONVERSION_DATA = 0x00,
        ADS1015_CONFIGURATION = 0x01
    };

public:
    enum MUX_t {
        MUX_AIN1_to_AIN0 = 0x0,
        MUX_AIN3_to_AIN0 = 0x1,
        MUX_AIN3_to_AIN1 = 0x2,
        MUX_AIN3_to_AIN2 = 0x3,
        MUX_GND_to_AIN0 = 0x4,
        MUX_GND_to_AIN1 = 0x5,
        MUX_GND_to_AIN2 = 0x6,
        MUX_GND_to_AIN3 = 0x7,
    };

    enum FSR_t {
        FSR_6144mV = 0x0,
        FSR_4096mV = 0x1,
        FSR_2048mV = 0x2,
        FSR_1024mV = 0x3,
        FSR_512mV = 0x4,
        FSR_256mV = 0x5,
    };

    // 初期化を行う
    static bool Initialize(void);

    // 動作を開始する
    // 割り込みが開始される
    static void Start(void);

    // 48V電源の出力電圧を取得する[V]
    static float GetDc48v(void) {
        return _Result[0];
    }

    // ドリブルモーターの電流を取得する[A]
    static float GetDribbleCurrent(void) {
        return _Result[1];
    }

    // 測定値が有効な値かどうか取得する
    // ただしこの関数がtrueを返してもGetDribbleCurrent()の値は正しくなく0を返す可能性がある
    static bool IsValid(void) {
        return _Valid;
    }

private:
    // 指定したシーケンス番号の変換を非同期的に開始する
    // シーケンス番号が最大値に達した場合、内部的に0に戻される
    static void StartConversionAsync(int sequence);

    // CONFIGURATIONレジスタの読み出しを非同期的に開始する
    static void PollStatusAsync(void) {
        I2CM_ReadRegister2Byte(I2C_BASE, ADS1015_CONFIGURATION);
    }

    // CONVERSION_DATAレジスタの読み出しを非同期的に開始する
    static void ReadResultAsync(void) {
        I2CM_ReadRegister2Byte(I2C_BASE, ADS1015_CONVERSION_DATA);
    }

    // バスリセットを非同期的に開始する
    static void StartBusResetAsync(void) {
        I2CM_BusReset(I2C_BASE);
    }

    // 非同期アクセスの完了を待つ
    static void AwaitComplete(void) {
        while (I2CM_IsBusy(I2C_BASE)) {
        }
    }

    // 非同期的にシングルショット変換を開始する
    static void ConvertAsync(MUX_t mux, FSR_t fsr) {
        uint16_t config = 0x8103 | (mux << 12) | (fsr << 9); // 128 SPS
        int txdata = (config >> 8) | (config << 8);
        I2CM_WriteRegister2Byte(I2C_BASE, ADS1015_CONFIGURATION, txdata);
    }

    // 同期的にレジスタを読み出す
    static bool ReadRegister(int address, uint16_t *value);

    // 同期的にレジスタへ書き込む
    static bool WriteRegister(int address, uint16_t value);

    // 割り込みハンドラ
    static void Handler(void *context);

    enum STATE_t {
        STATE_WriteConfig = 0,
        STATE_PollConfig,
        STATE_ReadResult,
        STATE_BusReset
    };
    static STATE_t _State;
    static int _Sequence;
    static float _Result[NUMBER_OF_SEQUENCE];
    static bool _Valid;
};
