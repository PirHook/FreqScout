#include "../../hdr/utils/StateManager.h"

StateManager::StateManager() : _currentState(State::eMAIN_THEME) {}
StateManager::~StateManager() {}

State StateManager::GetState() const { return _currentState; }

int StateManager::GetFps() const { return _fps; }

void StateManager::SetState(State state) { _currentState = state; }

void StateManager::Update(double dt, AppContext &ctx) {
  // -------- FPS --------
  _fpsTimer += dt;
  _fpsFrames++;

  if (_fpsTimer >= 1.0f) {
    _fps = _fpsFrames;
    _fpsFrames = 0;
    _fpsTimer = 0.0f;
  }

  switch (_currentState) {

  case State::eSCAN_STARTED: {
    if (!_gsmScanner.GetState()) {
      _gsmScanner.Start();
      ctx._stations.clear();
    }
    _currentState = State::eSCANNING;
    break;
  }

  case State::eSCANNING: {
    _scanTimer += dt;

    if (_scanTimer >= 0.1f) { // 10 Hz
      ctx._stations = _gsmScanner.GetStations();
      _scanTimer = 0.0f;
    }

    if (!ctx._stations.empty()) {
      _currentState = State::eSCANNING_DATA_ACQUIRED;
    }
    break;
  }

  case State::eSCANNING_DATA_ACQUIRED: {
    _scanTimer += dt;

    if (_scanTimer >= 0.1f) {
      ctx._stations = _gsmScanner.GetStations();
      _scanTimer = 0.0f;
    }
    break;
  }

  case State::eSCAN_DONE: {
    if (_gsmScanner.GetState())
      _gsmScanner.Stop();
    break;
  }

  case State::eSTART_SPECTRUM: {
    if (_gsmScanner.GetState()) {
      _gsmScanner.Stop();
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    std::string freqStr = ctx._selectedStation.value().freq;
    size_t pos = freqStr.find(" ");
    if (pos != std::string::npos)
      freqStr = freqStr.substr(0, pos);
    uint32_t freqHz = static_cast<uint32_t>(std::stod(freqStr) * 1e6);
    _spectrum.Start(freqHz, 2048000);

    _currentState = State::eSPECTRUM_RUNNING;
    break;
  }
  case State::eSPECTRUM_RUNNING: {
    ctx._currentSpectrumData = _spectrum.GetCurrentData();
    break;
  }
  case State::eSPECTRUM_STOP: {
    if (_spectrum.isRunning())
      _spectrum.Stop();
    _currentState = State::eSCAN_DONE;
  }

  default: {
    break;
  }
  }
}
