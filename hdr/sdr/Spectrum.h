#pragma once

#include "SDRDevice.h"
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>

class Spectrum {
public:
  Spectrum() {}
  ~Spectrum() { Stop(); }

  void Start(uint32_t freq, uint32_t sampleRate);
  void Stop();
  bool isRunning() { return _running; }

  std::vector<float> GetCurrentData();

private:
  void ProcessingLoop();

private:
  SDRDevice _sdr;
  std::thread _processThread;
  std::atomic<bool> _running{false};

  std::queue<std::vector<uint8_t>> _rawQueue;
  std::mutex _queueMtx;
  std::condition_variable _cv;

  std::vector<float> _renderBuffer;
  std::mutex _renderMtx;

  const size_t FFT_SIZE = 2048;
};
