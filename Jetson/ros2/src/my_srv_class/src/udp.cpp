#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "udp.hpp"

udp::udp(const std::string &address, unsigned int port) {
  // ソケットの作成
  sock = socket(AF_INET, SOCK_DGRAM, 0);

  const auto yes = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  // 構造体の設定
  receive_addr.sin_family = AF_INET;
  receive_addr.sin_port = htons(port);
  receive_addr.sin_addr.s_addr = inet_addr(address.c_str());

  // ソケットとポートの結合
  bind(sock, reinterpret_cast<sockaddr *>(&receive_addr), sizeof(receive_addr));

  mreq.imr_interface.s_addr = INADDR_ANY;
  mreq.imr_multiaddr.s_addr = inet_addr(address.c_str());

  if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
                 sizeof(mreq)) != 0) {
    perror("setsockopt");
  }

  // ノンブロッキングモードに設定
  // mode=0でブロッキング、mode=1でノンブロッキング
  const int mode = 1;
  ioctl(sock, FIONBIO, &mode);
}

udp::~udp() {
  // ソケットの終了
  close(sock);
}

int udp::send(std::string ip, unsigned int port, char *buffer,
              std::size_t length) {
  // 構造体の設定
  sockaddr_in send_addr;
  send_addr.sin_family = AF_INET;
  send_addr.sin_port = htons(port);
  send_addr.sin_addr.s_addr = inet_addr(ip.c_str());

  return sendto(sock, buffer, length, 0,
                reinterpret_cast<sockaddr *>(&send_addr), sizeof(send_addr));
}

int udp::receive(char *buffer, std::size_t length) {
  return recv(sock, buffer, length, 0);
}
