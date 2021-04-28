#ifndef UDP_UDP_H
#define UDP_UDP_H

#include <cstddef>
#include <string>

#include <netinet/in.h>

class udp {
private:
  // ソケットハンドル
  int sock;
  // sockaddr_in構造体
  sockaddr_in receive_addr;

  ip_mreq mreq;

public:
  // 自身のポートを指定
  udp(const std::string &address, unsigned int port);

  ~udp();

  // 送信先のIPとポートを指定
  int send(std::string ip, unsigned int port, char *buffer, std::size_t length);

  // 受信したら受信データの長さを返す
  // 受信していない場合1未満の値を返す
  int receive(char *buffer, std::size_t length);
};

#endif
