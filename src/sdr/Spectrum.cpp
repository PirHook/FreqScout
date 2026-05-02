#include "../../hdr/sdr/Spectrum.h"
void Spectrum::Start(uint32_t freq, uint32_t sampleRate) {
  if (_running)
    return;
  if (_sdr.DevCount() == 0) {
    std::cout << "No sdr device available\n";
    return;
  }

  if (_sdr.Open(0) != 0) {
    std::cout << "Failed to open SDR\n";
    return;
  }

  _sdr.Init(freq, sampleRate, 0, 400);
  _running = true;

  _processThread = std::thread(&Spectrum::ProcessingLoop, this);

  _sdr.Start([this](const std::vector<uint8_t> &data) {
    {
      std::lock_guard<std::mutex> lock(_queueMtx);
      _rawQueue.push(data);
    }
    _cv.notify_one();
  });
  std::cout
      << "[Spectrum] SDR Thread and Callback linked. Waiting for buffers...\n";
}

void Spectrum::Stop() {
  _running = false;
  _cv.notify_all();

  _sdr.Stop();
  _sdr.Close();

  if (_processThread.joinable()) {
    _processThread.join();
  }
}

void Spectrum::ProcessingLoop() {
  while (_running) {
    std::vector<uint8_t> workData;

    {
      std::unique_lock<std::mutex> lock(_queueMtx);
      _cv.wait(lock, [this] { return !_rawQueue.empty() || !_running; });

      if (!_running)
        break;

      workData = std::move(_rawQueue.front());
      _rawQueue.pop();
    }

    std::vector<float> magnitudes;
    for (size_t i = 0; i < workData.size(); i += 2) {
      float i_f = (workData[i] - 127.5f) / 127.5f;
      float q_f = (workData[i + 1] - 127.5f) / 127.5f;
      magnitudes.push_back(std::sqrt(i_f * i_f + q_f * q_f));
    }

    {
      std::lock_guard<std::mutex> lock(_renderMtx);
      _renderBuffer = std::move(magnitudes);
    }
  }
}

std::vector<float> Spectrum::GetCurrentData() {
  std::lock_guard<std::mutex> lock(_renderMtx);
  return _renderBuffer;
}
