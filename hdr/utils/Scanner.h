#pragma once
#include <future>
#include <iostream>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>

struct Station {
  int arfcn;
  std::string freq;
  int cid;
  int lac;
  std::string mcc;
  std::string mnc;
  int pwr;

  bool operator<(const Station &other) const { return arfcn < other.arfcn; }
};

class Scanner {
public:
  Scanner() {}
  ~Scanner();

  void Start();
  void Stop();
  std::vector<Station> GetStations();

private:
  int fail(std::error_code ec);
  void ParseBuffer(const std::string &buffer);

private:
  reproc::process _process;
  reproc::stop_actions _stopActs;
  reproc::options _options;
  std::error_code _ec;
  std::string _output;
  std::mutex _mtx;
  std::future<std::error_code> _drainTask;
  std::set<Station> _stations;
};
