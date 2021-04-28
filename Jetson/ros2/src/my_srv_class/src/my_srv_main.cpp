#include <rclcpp/rclcpp.hpp>
#include <bits/stdc++.h>
#include <bitset>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include "udp.hpp"

#include <memory>
#include "phoenix_msgs/srv/set_speed.hpp"


static constexpr char listen_address[] = "224.5.23.2";
static constexpr short listen_port = 10004;

// 受信バッファサイズ
static const std::size_t recv_buf_size = 11;

// 受信データからロボットIDを取得するインライン関数
static inline int get_id(char *data) { return (data[0] & 0x0F) - 0x01; }

using namespace std::literals::chrono_literals;
using SetSpeed_ = phoenix_msgs::srv::SetSpeed;


// タイムアウト時間
static const auto time_limit = 1000ms;

// メインループのスリープ時間
static const auto time_sleep = 1ms;

// タイムアウトチェック用インライン関数
static inline bool is_timeout(std::chrono::system_clock::time_point last) {
  return std::chrono::system_clock::now() - last > time_limit;
}


int main(int argc, char * argv[]){
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("udp_receiver");
//サービスクライアントの作成をする
  auto client = node -> create_client<SetSpeed_>("/jetson_desktop/set_speed");

//サービスをまつ
  while (!client->wait_for_service(1s)) {
    //シャットダウンされたかの確認
    if(!rclcpp::ok()){
      RCLCPP_ERROR(node->get_logger(),"待ってる間に切れました");
      rclcpp::shutdown();
      return 1;
    }
    RCLCPP_INFO(node->get_logger(),"サービス待ち");
  }
  
RCLCPP_INFO(node->get_logger(),"成功");
  auto request = std::make_shared<SetSpeed_::Request>();
  

/*if (argc < 2) {
    std::cout << "ロボットIDを指定してください" << std::endl;
    return -1;
  }*/

  const int my_id = 1;//atoi(argv[1]);

  // 初期設定およびオブジェクトの生成
  // TODO: Kicker 向けの初期設定およびオブジェクトの生成
  udp udp0(listen_address, listen_port);

  // タイムアウト検知用変数
  auto time_last_recv = std::chrono::system_clock::now();

  int speed_x = 0;
  int speed_y = 0;
  int speed_w = 0;
  int dribble = 3;
  int frame_number = 1;

  while(true){
    // TODO: ループを脱してプログラムを終了する条件を追加する
    // 受信処理
    char recv_buf[recv_buf_size];
    int recv_result = udp0.receive(recv_buf, recv_buf_size);

    // 受信した場合、受信データのロボットIDと自身のIDを比較し、
    // 同一だったときFPGAおよびKickerへの送信データの更新を行う
    if (recv_result > 0 && get_id(recv_buf) == my_id) {

      // タイムアウト検知用変数の更新
      time_last_recv = std::chrono::system_clock::now();

      // FPGAおよびKickerへの送信データの更新
      const auto velocity = recv_buf[1] << 8 | recv_buf[2];
      const auto direction = recv_buf[3] << 8 | recv_buf[4];
      speed_x =
          static_cast<int>(velocity * std::cos(2.0 * M_PI * direction / 65535));
      speed_y =
          static_cast<int>(velocity * std::sin(2.0 * M_PI * direction / 65535));
      speed_w = ((recv_buf[0] & 0x80) == 0 ? 1 : -1) *
                (recv_buf[5] << 8 | recv_buf[6]) * 8 / 1000;
      dribble = recv_buf[7] & 0x0f;
    /*
    std::cout<<"x="<<speed_x<<std::endl;
    std::cout<<"y="<<speed_y<<std::endl;
    std::cout<<"w="<<speed_w<<std::endl;
    */
    }

    //リクエストに値をいれる
    request->speed_x = speed_x;
    
    request->speed_y = speed_y;
    request->speed_omega = speed_w;
    request->dribble_power = dribble;

    //サーバーにリクエストを送る非同期で
    auto result = client->async_send_request(request);
  
    //結果待ち
    if (rclcpp::spin_until_future_complete(node, result) ==
    rclcpp::executor::FutureReturnCode::SUCCESS)
    {
      RCLCPP_INFO(node->get_logger(), "Receive : '%s'", result.get()->succeeded);
    } else {
      RCLCPP_ERROR(node->get_logger(), "Problem while waiting for response.");
    }

    // 一定時間スリープ
    std::this_thread::sleep_for(time_sleep);
  
  }
  

  return 0;
}