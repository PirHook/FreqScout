#pragma once
#include <cstdint>

enum class State : uint8_t {
  eMAIN_THEME,
  eSCAN_STARTED,
  eSCANNING,
  eSCANNING_DATA_ACQUIRED,
  eSCAN_DONE,
};

class StateManager {
public:
  StateManager();
  ~StateManager();

  State GetState() const;
  void SetState(State state);

private:
private:
  State _currentState;
};
