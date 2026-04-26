#pragma once

#include <atomic>
#include <future>
#include <iostream>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>

#include "CommonStructs.h"

class Scanner {
public:
  Scanner() = default;
  ~Scanner();

  void Start();
  void Stop();

  std::vector<Station> GetStations();
  bool GetState();

private:
  void ParseBuffer(const std::string &buffer);

private:
  reproc::process _process;
  reproc::options _options;
  reproc::stop_actions _stopActs;
  std::error_code _ec;

  std::future<std::error_code> _drainTask;

  std::string _output;
  std::set<Station> _stations;

  std::mutex _mtx;
  std::atomic<bool> _running{false};
};
