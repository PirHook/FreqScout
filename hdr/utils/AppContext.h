#pragma once

#include "CommonStructs.h"
#include <optional>
#include <vector>

struct AppContext {
  std::vector<Station> _stations;
  std::optional<Station> _selectedStation;
  std::vector<float> _currentSpectrumData;
  std::vector<float> _currentWaterfall;
};
