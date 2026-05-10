#pragma once

#include "SDRDevice.h"
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <fftw3.h>
#include <iostream>
#include <math.h>
#include <mutex>
#include <queue>
#include <vector>

class Spectrum {
public:
  Spectrum() { _wfData.assign(WF_WIDTH * WF_HEIGHT, -100.0f); }
  ~Spectrum() { Stop(); }

  void Start(uint32_t freq, uint32_t sampleRate);
  void Stop();
  bool isRunning() { return _running; }

  std::vector<float> GetCurrentData();
  std::vector<float> GetWfData();

private:
  void GenerateHannWindow(int n);
  void ProcessingLoop();

private:
  SDRDevice _sdr;
  std::thread _processThread;
  std::atomic<bool> _running{false};

  std::queue<std::vector<uint8_t>> _rawQueue;
  std::mutex _queueMtx;
  std::condition_variable _cv;

  std::vector<float> _renderBuffer;
  std::vector<float> _window;
  std::vector<float> _wfData;
  std::mutex _renderMtx;

  const size_t FFT_SIZE = 2048;
  const int WF_WIDTH = 1024;
  const int WF_HEIGHT = 100;
};
