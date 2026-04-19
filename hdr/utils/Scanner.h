#pragma once
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>

// struct Station {
//   int arfcn;
//   double freq;
//   int cellId, lac, mcc, mnc, pwr;
//   std::string toTableRecord() const; // Ваша реализация
// };

class Scanner {
public:
  Scanner() {}
  ~Scanner();

  void Start();
  void Stop();
  std::string GetOutput();

private:
  int fail(std::error_code ec);

private:
  reproc::process _process;
  reproc::stop_actions _stopActs;
  reproc::options _options;
  std::error_code _ec;
  std::string _output;
  std::mutex _mtx;
  std::future<std::error_code> _drainTask;
};
