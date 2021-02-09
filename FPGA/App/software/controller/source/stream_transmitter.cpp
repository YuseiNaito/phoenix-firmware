#include "stream_transmitter.hpp"
#include "driver/adc2.hpp"
#include "centralized_monitor.hpp"
#include <stream_data.hpp>
#include <peripheral/msgdma.hpp>
#include <peripheral/imu_spim.hpp>
#include <peripheral/vector_controller.hpp>
#include <peripheral/motor_controller.hpp>

static StreamDataStatus_t StreamDataStatus;
static constexpr MsgdmaTransmitDescriptor StreamDataDesciptorStatus(StreamDataStatus, StreamIdStatus);

static StreamDataAdc2_t StreamDataAdc2;
static constexpr MsgdmaTransmitDescriptor StreamDataDesciptorAdc2(StreamDataAdc2, StreamIdAdc2);

static StreamDataMotion_t StreamDataMotion;
static constexpr MsgdmaTransmitDescriptor StreamDataDesciptorMotion(StreamDataMotion, StreamIdMotion);

void StreamTransmitter::TransmitStatus(void) {
    __builtin_stwio(&StreamDataStatus.error_flags, CentralizedMonitor::GetErrorFlags());
    __builtin_stwio(&StreamDataStatus.fault_flags, CentralizedMonitor::GetFaultFlags());
    StreamDataDesciptorStatus.TransmitAsync(_Device);
}

void StreamTransmitter::TransmitAdc2(void) {
    __builtin_sthio(&StreamDataAdc2.dc48v_voltage, Adc2::GetDc48v());
    __builtin_sthio(&StreamDataAdc2.dribble_current, Adc2::GetDribbleCurrent());
    StreamDataDesciptorAdc2.TransmitAsync(_Device);
}

void StreamTransmitter::TransmitMotion(int performance_counter) {
    __builtin_sthio(&StreamDataMotion.performance_counter, static_cast<uint16_t>(performance_counter));
    __builtin_sthio(&StreamDataMotion.accelerometer[0], IMU_SPIM_GetAccelDataX(IMU_SPIM_BASE));
    __builtin_sthio(&StreamDataMotion.accelerometer[1], IMU_SPIM_GetAccelDataY(IMU_SPIM_BASE));
    __builtin_sthio(&StreamDataMotion.accelerometer[2], IMU_SPIM_GetAccelDataZ(IMU_SPIM_BASE));
    __builtin_sthio(&StreamDataMotion.gyroscope[0], IMU_SPIM_GetGyroDataX(IMU_SPIM_BASE));
    __builtin_sthio(&StreamDataMotion.gyroscope[1], IMU_SPIM_GetGyroDataY(IMU_SPIM_BASE));
    __builtin_sthio(&StreamDataMotion.gyroscope[2], IMU_SPIM_GetGyroDataZ(IMU_SPIM_BASE));
    __builtin_sthio(&StreamDataMotion.encoder_pulse_count[0], VectorController::GetEncoderValue(1));
    __builtin_sthio(&StreamDataMotion.encoder_pulse_count[1], VectorController::GetEncoderValue(2));
    __builtin_sthio(&StreamDataMotion.encoder_pulse_count[2], VectorController::GetEncoderValue(3));
    __builtin_sthio(&StreamDataMotion.encoder_pulse_count[3], VectorController::GetEncoderValue(4));
    __builtin_sthio(&StreamDataMotion.motor_current_d[0], VectorController::GetCurrentMeasurementD(1));
    __builtin_sthio(&StreamDataMotion.motor_current_d[1], VectorController::GetCurrentMeasurementD(2));
    __builtin_sthio(&StreamDataMotion.motor_current_d[2], VectorController::GetCurrentMeasurementD(3));
    __builtin_sthio(&StreamDataMotion.motor_current_d[3], VectorController::GetCurrentMeasurementD(4));
    __builtin_sthio(&StreamDataMotion.motor_current_q[0], VectorController::GetCurrentMeasurementQ(1));
    __builtin_sthio(&StreamDataMotion.motor_current_q[1], VectorController::GetCurrentMeasurementQ(2));
    __builtin_sthio(&StreamDataMotion.motor_current_q[2], VectorController::GetCurrentMeasurementQ(3));
    __builtin_sthio(&StreamDataMotion.motor_current_q[3], VectorController::GetCurrentMeasurementQ(4));
    __builtin_sthio(&StreamDataMotion.motor_current_ref_q[0], VectorController::GetCurrentReferenceQ(1));
    __builtin_sthio(&StreamDataMotion.motor_current_ref_q[1], VectorController::GetCurrentReferenceQ(2));
    __builtin_sthio(&StreamDataMotion.motor_current_ref_q[2], VectorController::GetCurrentReferenceQ(3));
    __builtin_sthio(&StreamDataMotion.motor_current_ref_q[3], VectorController::GetCurrentReferenceQ(4));
    __builtin_sthio(&StreamDataMotion.motor_power_5, MotorController::GetPower());
    StreamDataDesciptorMotion.TransmitAsync(_Device);
}

alt_msgdma_dev *StreamTransmitter::_Device;