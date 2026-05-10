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
  int nfft = 1024;
  fftwf_complex *in =
      (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * nfft);
  fftwf_complex *out =
      (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * nfft);
  fftwf_plan plan =
      fftwf_plan_dft_1d(nfft, in, out, FFTW_FORWARD, FFTW_MEASURE);

  GenerateHannWindow(nfft);

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

    for (int i = 0; i < nfft; i++) {
      float i_raw = (workData[2 * i] - 127.5f) / 127.5f;
      float q_raw = (workData[2 * i + 1] - 127.5f) / 127.5f;

      in[i][0] = i_raw * _window[i];
      in[i][1] = q_raw * _window[i];
    }

    fftwf_execute(plan);

    std::vector<float> magnitudes(nfft);
    for (int i = 0; i < nfft; i++) {
      int targetIdx = (i < nfft / 2) ? (i + nfft / 2) : (i - nfft / 2);

      float re = out[i][0];
      float im = out[i][1];
      float mag = sqrtf(re * re + im * im) / nfft;

      magnitudes[targetIdx] = 20.0f * log10f(mag + 1e-6f);
    }

    {
      float centerGain = 20.0f;
      int centerBin = nfft / 2;
      int spread = 50;

      for (int i = 0; i < nfft; i++) {
        float dist = std::abs(i - centerBin);
        if (dist < spread) {
          float factor = 1.0f - (dist / spread);
          magnitudes[i] += centerGain * factor;
        }
      }
    }

    {
      std::lock_guard<std::mutex> lock(_renderMtx);
      std::copy_backward(_wfData.begin(),
                         _wfData.begin() + (WF_HEIGHT - 1) * WF_WIDTH,
                         _wfData.end());
      std::copy(magnitudes.begin(), magnitudes.end(), _wfData.begin());
      _renderBuffer = magnitudes;
    }
  }

  fftwf_destroy_plan(plan);
  fftwf_free(in);
  fftwf_free(out);
}

std::vector<float> Spectrum::GetCurrentData() {
  std::lock_guard<std::mutex> lock(_renderMtx);
  return _renderBuffer;
}

std::vector<float> Spectrum::GetWfData() {
  std::lock_guard<std::mutex> lock(_renderMtx);
  return _wfData;
}

void Spectrum::GenerateHannWindow(int n) {
  _window.resize(n);
  for (int i = 0; i < n; ++i) {
    _window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (n - 1)));
  }
}
