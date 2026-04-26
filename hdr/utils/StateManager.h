#pragma once
#include "../utils/Scanner.h"
#include "AppContext.h"
#include "CommonStructs.h"

#include <cstdint>

class StateManager {
public:
  StateManager();
  ~StateManager();

  State GetState() const;
  int GetFps() const;
  void SetState(State state);
  void Update(double dt, AppContext &ctx);

private:
  int _fps;
  double _fpsTimer;
  int _fpsFrames;
  double _scanTimer;

  State _currentState;
  Scanner _gsmScanner;
};
