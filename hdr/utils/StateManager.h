#pragma once
#include <cstdint>

enum class State : uint8_t {
  eMAIN_THEME,
  eSCANNING,
  eSCANNING_DATA_ACQUIRED,
  eSCAN_DONE_DATA_ACQUIRED,
  eSCAN_DONE_NO_DATA,
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
