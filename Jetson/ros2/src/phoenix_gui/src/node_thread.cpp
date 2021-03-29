#include "node_thread.hpp"
#include <chrono>

constexpr int NodeThread::QUIT_TIMEOUT;

void NodeThread::run(void) {
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(_Node);
    while (!isInterruptionRequested()) {
        executor.spin_once(std::chrono::milliseconds(1));
    }
}
