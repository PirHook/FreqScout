#pragma once
#include <mutex>
#include <regex>
#include <reproc++/reproc.hpp>
#include <string>
#include <vector>

struct Station {
  int arfcn;
  double freq;
  uint32_t cellId;
  uint16_t lac;
  uint8_t mcc;
  uint8_t mnc;
  int8_t pwr;
};

class Scanner {
public:
  void Start();

  void update();

  bool is_running() const { return _running; }

  std::string getOutput() { return _output; }

private:
  void parse_buffer();

private:
  reproc::process _process;
  bool _running = false;
  std::string _output;
  std::vector<Station> _stations;
  std::mutex _mtx;
};
