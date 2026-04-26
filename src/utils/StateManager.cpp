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

  case State::eANALYZING: {
    if (_gsmScanner.GetState())
      _gsmScanner.Stop();
    break;
  }

  default: {
    break;
  }
  }
}
