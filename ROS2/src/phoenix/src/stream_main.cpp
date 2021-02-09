#include "avalon_st.hpp"
#include "uart.hpp"
#include <memory>
#include <thread>
#include <exception>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp/qos.hpp>
#include <phoenix_msgs/msg/stream_data_adc2.hpp>
#include <phoenix_msgs/msg/stream_data_status.hpp>
#include <phoenix_msgs/msg/stream_data_motion.hpp>
#include "../include/phoenix/stream_data.hpp"
#include "../include/phoenix/status_flags.hpp"

namespace phoenix {

/// UARTのデバイスパス
static constexpr char UartDeviceName[] = "/dev/ttyTHS1";

/// UARTのボーレート
static constexpr int UartBaudrate = B4000000;

/**
 * UARTで受信したデータを整形して配信するノード
 */
class StreamPublisherNode : public rclcpp::Node {
public:
    /**
     * コンストラクタ
     */
    StreamPublisherNode(const rclcpp::NodeOptions &options = rclcpp::NodeOptions()) : Node("stream_publisher"), _Uart(new Uart) {
        using namespace std::chrono_literals;
        (void)options;

        // UARTデバイスを開く
        if (_Uart->Open(UartDeviceName) == false) {
            fprintf(stderr, "Cannot open '%s'.\n", UartDeviceName);
            throw;
        }
        if (_Uart->SetBaudrate(UartBaudrate) == false) {
            fprintf(stderr, "Cannot configure desired baudrate '%d'.\n", UartBaudrate);
            throw;
        }

        // publisherを作成する
        rclcpp::QoS qos(rclcpp::QoSInitialization(RMW_QOS_POLICY_HISTORY_KEEP_LAST, QOS_DEPTH));
        _Timer = this->create_wall_timer(1ms, std::bind(&StreamPublisherNode::TimerCallback, this));
        _StatusPublisher = this->create_publisher<phoenix_msgs::msg::StreamDataStatus>("stream_data_status", qos);
        _Adc2Publisher = this->create_publisher<phoenix_msgs::msg::StreamDataAdc2>("stream_data_adc2", qos);
        _MotionPublisher = this->create_publisher<phoenix_msgs::msg::StreamDataMotion>("stream_data_motion", qos);

        // ペイロード受信バッファを確保しておく
        _Payload.reserve(MAXIMUM_PAYLOAD_LENGTH);
    }

private:
    void TimerCallback(void) {
        std::array<uint8_t, READ_BUFFER_SIZE> buffer;
        size_t read_length;
        if (_Uart->Read(buffer.data(), buffer.size(), &read_length) == false) {
            //throw std::runtime_error("Failed to read from UART device.");
            return;
        }
        /*if (0 < read_length) {
            for (size_t i = 0; i < read_length; i++)
            {
                printf((i == 0) ? "[%02X" : " %02X", buffer[i]);
            }
            printf("]\n");
        }*/
        _Converter.Parse(buffer.data(), read_length, [this](uint8_t data, int channel, bool sof, bool eof) {
            if ((_ChannelNumber != channel) || sof) {
                if (!_Payload.empty() && !_PayloadOverRun)
                    ProcessPayload(_Payload, _ChannelNumber);
                _ChannelNumber = channel;
                _Payload.clear();
                _PayloadOverRun = false;
            }
            if (_Payload.size() < MAXIMUM_PAYLOAD_LENGTH) {
                _Payload.push_back(data);
            } else {
                _PayloadOverRun = true;
            }
            if (eof) {
                if (!_Payload.empty() && !_PayloadOverRun)
                    ProcessPayload(_Payload, _ChannelNumber);
                _Payload.clear();
                _PayloadOverRun = false;
            }
            return true;
        });
    }

    void ProcessPayload(const std::vector<uint8_t> &payload, int channel) {
        /*printf("(%02X)[", channel);
        for (auto it = payload.begin(); it != payload.end(); ++it)
        {
            printf((it == payload.begin()) ? "%02X" : " %02X", *it);
        }
        printf("]\n");*/
        switch (channel) {
        case StreamIdStatus:
            if (payload.size() == sizeof(StreamDataStatus_t)) {
                PublishStatus(reinterpret_cast<const StreamDataStatus_t *>(payload.data()));
            }
            break;

        case StreamIdAdc2:
            if (payload.size() == sizeof(StreamDataAdc2_t)) {
                PublishAdc2(reinterpret_cast<const StreamDataAdc2_t *>(payload.data()));
            }
            break;

        case StreamIdMotion:
            if (payload.size() == sizeof(StreamDataMotion_t)) {
                PublishMotion(reinterpret_cast<const StreamDataMotion_t *>(payload.data()));
            }
            break;

        default:
            break;
        }

        /*struct payload_t {
            uint16_t frame_number;
            uint16_t pos_theta;
            int32_t error;
            float current;
            int16_t adc1_u_data_1;
            int16_t adc1_v_data_1;
            int16_t current_d;
            int16_t current_q;
            int16_t speed;
        };
        static int cnt = 0;
        static int index = 0;
        if ((channel == 0x72) && (payload.size() == sizeof(payload_t))) {
            const payload_t *payload_data = reinterpret_cast<const payload_t *>(payload.data());
            fprintf(fp, "%d,%d,%f,%d,%d,%d,%d,%d,%d\n", index++, payload_data->error, payload_data->current, payload_data->adc1_u_data_1, payload_data->adc1_v_data_1, payload_data->pos_theta, payload_data->current_d, payload_data->current_q, payload_data->speed);
            if (100 <= ++cnt) {
                cnt = 0;
                printf("f=%d, u=%d, v=%d, p=%d, d=%d, q=%d, s=%d\n", payload_data->frame_number, payload_data->adc1_u_data_1, payload_data->adc1_v_data_1, payload_data->pos_theta, payload_data->current_d, payload_data->current_q, payload_data->speed);
            }
        }*/
    }

    /**
     * StreamDataStatusを配信する
     */
    void PublishStatus(const StreamDataStatus_t *data) {
        uint32_t error_flags = data->error_flags;
        uint32_t fault_flags = data->fault_flags;
        phoenix_msgs::msg::StreamDataStatus msg;
        if (error_flags != 0) {
            msg.error_module_sleep = (error_flags & ErrorCauseModuleSleep);
            msg.error_fpga_stop = (error_flags & ErrorCauseFpgaStop);
            msg.error_dc48v_uv = (error_flags & ErrorCauseDc48vUnderVoltage);
            msg.error_dc48v_ov = (error_flags & ErrorCauseDc48vOverVoltage);
            msg.error_motor_oc[0] = (error_flags & ErrorCauseMotor1OverCurrent);
            msg.error_motor_oc[1] = (error_flags & ErrorCauseMotor2OverCurrent);
            msg.error_motor_oc[2] = (error_flags & ErrorCauseMotor3OverCurrent);
            msg.error_motor_oc[3] = (error_flags & ErrorCauseMotor4OverCurrent);
            msg.error_motor_oc[4] = (error_flags & ErrorCauseMotor5OverCurrent);
            msg.error_motor_hall_sensor[0] = (error_flags & ErrorCauseMotor1HallSensor);
            msg.error_motor_hall_sensor[1] = (error_flags & ErrorCauseMotor2HallSensor);
            msg.error_motor_hall_sensor[2] = (error_flags & ErrorCauseMotor3HallSensor);
            msg.error_motor_hall_sensor[3] = (error_flags & ErrorCauseMotor4HallSensor);
            msg.error_motor_hall_sensor[4] = (error_flags & ErrorCauseMotor5HallSensor);
        }
        if (fault_flags != 0) {
            msg.fault_adc2_timeout = (fault_flags & FaultCauseAdc2Timeout);
            msg.fault_imu_timeout = (fault_flags & FaultCauseImuTimeout);
            msg.fault_motor_ot[0] = (fault_flags & FaultCauseMotor1OverTemperature);
            msg.fault_motor_ot[1] = (fault_flags & FaultCauseMotor2OverTemperature);
            msg.fault_motor_ot[2] = (fault_flags & FaultCauseMotor3OverTemperature);
            msg.fault_motor_ot[3] = (fault_flags & FaultCauseMotor4OverTemperature);
            msg.fault_motor_ot[4] = (fault_flags & FaultCauseMotor5OverTemperature);
            msg.fault_motor_oc[0] = (fault_flags & FaultCauseMotor1OverCurrent);
            msg.fault_motor_oc[1] = (fault_flags & FaultCauseMotor2OverCurrent);
            msg.fault_motor_oc[2] = (fault_flags & FaultCauseMotor3OverCurrent);
            msg.fault_motor_oc[3] = (fault_flags & FaultCauseMotor4OverCurrent);
            msg.fault_motor_oc[4] = (fault_flags & FaultCauseMotor5OverCurrent);
            msg.fault_motor_load_switch[0] = (fault_flags & FaultCauseMotor1LoadSwitch);
            msg.fault_motor_load_switch[1] = (fault_flags & FaultCauseMotor2LoadSwitch);
            msg.fault_motor_load_switch[2] = (fault_flags & FaultCauseMotor3LoadSwitch);
            msg.fault_motor_load_switch[3] = (fault_flags & FaultCauseMotor4LoadSwitch);
            msg.fault_motor_load_switch[4] = (fault_flags & FaultCauseMotor5LoadSwitch);
        }
        _StatusPublisher->publish(msg);
    }

    /**
     * StreamDataAdc2を配信する
     */
    void PublishAdc2(const StreamDataAdc2_t *data) {
        phoenix_msgs::msg::StreamDataAdc2 msg;
        msg.dc48v_voltage = data->dc48v_voltage;
        msg.dribble_current = data->dribble_current;
        _Adc2Publisher->publish(msg);
    }

    /**
     * StreamDataMotionを配信する
     */
    void PublishMotion(const StreamDataMotion_t *data) {
        phoenix_msgs::msg::StreamDataMotion msg;
        msg.performance_counter = data->performance_counter;
        for (int axis = 0; axis < 3; axis++) {
            msg.accelerometer[axis] = data->accelerometer[axis];
            msg.gyroscope[axis] = data->gyroscope[axis];
        }
        for (int index = 0; index < 4; index++) {
            msg.encoder_pulse_count[index] = data->encoder_pulse_count[index];
            msg.motor_current_d[index] = data->motor_current_d[index];
            msg.motor_current_q[index] = data->motor_current_q[index];
            msg.motor_current_ref_q[index] = data->motor_current_ref_q[index];
        }
        msg.motor_power_5 = data->motor_power_5;
        _MotionPublisher->publish(msg);
    }

    /// UARTデバイス
    std::shared_ptr<Uart> _Uart;

    /// Avalon-STのパーサー
    AvalonStBytesToPacketsConverter _Converter;

    /// 受信中のパケットのチャンネル番号
    int _ChannelNumber = -1;

    /// 受信中のパケットのペイロードを格納するバッファ
    std::vector<uint8_t> _Payload;

    /// 受信中のパケットがMAXIMUM_PAYLOAD_LENGTHを超えたことを示すフラグ
    bool _PayloadOverRun = false;

    /// 受信処理を行うトリガーとなるタイマー
    rclcpp::TimerBase::SharedPtr _Timer;

    /// StreamDataStatus(FPGA内のNios IIのCentralizedMonitorのステータスフラグ)を配信するpublisher
    rclcpp::Publisher<phoenix_msgs::msg::StreamDataStatus>::SharedPtr _StatusPublisher;

    /// StreamDataAdc2(FPGAに繋がったADC2の測定値)を配信するpublisher
    rclcpp::Publisher<phoenix_msgs::msg::StreamDataAdc2>::SharedPtr _Adc2Publisher;

    /// StreamDataMotion(IMUやモーター制御の情報)を配信するpublisher
    rclcpp::Publisher<phoenix_msgs::msg::StreamDataMotion>::SharedPtr _MotionPublisher;

    /// QoSのパラメータ
    static constexpr int QOS_DEPTH = 10;

    /// 受信バッファの大きさ
    static constexpr size_t READ_BUFFER_SIZE = 4096;

    /// 許容できるペイロードの長さ
    static constexpr size_t MAXIMUM_PAYLOAD_LENGTH = 1024;
};

}  // namespace phoenix

#ifdef PHOENIX_BUILD_AS_LIBRARY
#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(phoenix::StreamPublisherNode)
#else
int main(int argc, char *argv[]) {
    // ROS2ノードを起動する
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<phoenix::StreamPublisherNode>());
    rclcpp::shutdown();
    return 0;
}
#endif