// stl
#include <chrono>

// zpp_lib
#include "zpp_include/thread.hpp"
#include "zpp_include/this_thread.hpp"
#include "zpp_include/digital_out.hpp"

#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_REGISTER(blinky, LOG_LEVEL_DBG);

void blink() {
  zpp_lib::DigitalOut led(zpp_lib::DigitalOut::PinName::LED0);
  using std::literals::chrono_literals::operator""ms;
  static constexpr std::chrono::milliseconds blinkInterval = 500ms;

  while (true) {
    led = !led;
    zpp_lib::ThisThread::sleep_for(blinkInterval);
  }
}

int main(void) {
  ZPP_LOG_DBG("Running on board %s", CONFIG_BOARD_TARGET);
  ZPP_LOG_DBG("Starting thread");
  zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "Blinky");
  auto res = thread.start(blink);
  if (! res) {
    ZPP_LOG_ERR("Could not start thread (%d)", static_cast<int>(res.error()));
    return -1;
  }

  res = thread.join();
  if (! res) {
    ZPP_LOG_ERR("Could not terminate thread (%d)", static_cast<int>(res.error()));
    return -1;
  }

  return 0;
}