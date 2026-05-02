#include "../../hdr/sdr/SDRDevice.h"
#include <stdio.h>

int SDRDevice::DevCount() { return rtlsdr_get_device_count(); }

int SDRDevice::Open(int index) {
  if (!_rtl) {
    int res = rtlsdr_open(&_rtl, index);
    if (res == 0) {
      rtlsdr_reset_buffer(_rtl);
    }
    return res;
  }
  return static_cast<int>(ErrCodes::DEV_ALREADY_OPENED);
}

int SDRDevice::Close() {
  if (_rtl) {
    int res = rtlsdr_close(_rtl);
    _rtl = nullptr;
    return res;
  }
  return static_cast<int>(ErrCodes::DEV_ALREADY_CLOSED);
}

int SDRDevice::Init(const uint32_t freq, const uint32_t sampRate, const int ppm,
                    const int gain) {
  int result = 0;
  result = rtlsdr_set_center_freq(_rtl, freq);
  if (result < 0) {
    return result;
  }
  result = rtlsdr_set_sample_rate(_rtl, sampRate);
  if (result < 0) {
    return result;
  }
  result = rtlsdr_set_freq_correction(_rtl, ppm);
  if (result < 0) {
    return result;
  }
  result = rtlsdr_set_tuner_gain_mode(_rtl, 1); // 1 - manual gain handle
  if (result < 0) {
    return result;
  }
  result = rtlsdr_set_tuner_gain(_rtl, gain);
  if (result < 0) {
    return result;
  }
  rtlsdr_reset_buffer(_rtl);

  return 0;
}

int SDRDevice::Start(DataHandler handler) {
  if (!_rtl)
    return static_cast<int>(ErrCodes::DEV_ALREADY_CLOSED);
  _dataHandler = handler;

  _workerThread = std::thread(
      [this]() { rtlsdr_read_async(_rtl, rtlsdrCallback, this, 0, 0); });
  return 0;
}

int SDRDevice::Stop() {
  if (!_rtl)
    return static_cast<int>(ErrCodes::DEV_ALREADY_CLOSED);
  int res = rtlsdr_cancel_async(_rtl);

  if (_workerThread.joinable()) {
    _workerThread.join();
  }

  rtlsdr_reset_buffer(_rtl);

  return res;
}

void SDRDevice::rtlsdrCallback(uint8_t *buf, uint32_t len, void *ctx) {
  auto *self = static_cast<SDRDevice *>(ctx);
  if (self && self->_dataHandler) {
    std::vector<uint8_t> data(buf, buf + len);
    self->_dataHandler(data);
  }
}
