#pragma once
#include <functional>
#include <rtl-sdr.h>
#include <thread>
#include <vector>

enum class ErrCodes : int {
  DEV_ALREADY_OPENED = 100,
  DEV_ALREADY_CLOSED = 101
};

using DataHandler = std::function<void(const std::vector<uint8_t> &)>;

class SDRDevice {
public:
  SDRDevice() = default;
  ~SDRDevice() = default;

  int DevCount();
  int Open(int index);
  int Close();
  int Init(const uint32_t freq, const uint32_t sampRate, const int ppm,
           const int gain);
  int Start(DataHandler handler);
  int Stop();

private:
  rtlsdr_dev_t *_rtl = nullptr;
  std::thread _workerThread;
  DataHandler _dataHandler;

  static void rtlsdrCallback(uint8_t *buf, uint32_t len, void *ctx);
};
