#include "../../hdr/utils/StateManager.h"

StateManager::StateManager() : _currentState(State::eMAIN_THEME) {}
StateManager::~StateManager() {}

State StateManager::GetState() const { return _currentState; }

void StateManager::SetState(State state) { _currentState = state; }
