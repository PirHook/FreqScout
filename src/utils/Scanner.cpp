#include "../../hdr/utils/Scanner.h"

#include <cstdlib>
#include <iostream>
#include <regex>
#include <signal.h>
#include <unistd.h>

Scanner::~Scanner() { Stop(); }

void Scanner::Start() {
  if (_running)
    return;

  std::cout << "START CALLED\n";

  _stopActs = {{reproc::stop::kill, reproc::milliseconds(100)}};
  _options.stop = _stopActs;

  std::vector<std::string> cmd = {
      "python3", "-u",     "/usr/local/bin/grgsm_scanner",
      "-b",      "GSM900", "-g",
      "40",      "-s",     "2e6",
      "--speed", "29",     "-v"};

  _ec = _process.start(cmd, _options);

  if (_ec) {
    std::cout << "Start error: " << _ec.message() << "\n";
    return;
  }

  _running = true;

  _drainTask = std::async(std::launch::async, [this]() {
    reproc::sink::thread_safe::string sink(_output, _mtx);
    return reproc::drain(_process, sink, sink);
  });

  std::cout << "SCANNER STARTED\n";
}

void Scanner::Stop() {
  if (!_running)
    return;

  std::cout << "STOP CALLED\n";

  _running = false;

  int status = 0;
  std::tie(status, _ec) = _process.stop(_stopActs);

  std::cout << "Soft stop status: " << status << "\n";

  std::cout << "PKILL grgsm_scanner\n";
  std::system("pkill -9 -f grgsm_scanner");

  std::cout << "PKILL python3 fallback\n";
  std::system("pkill -9 python3");

  if (_drainTask.valid()) {
    _drainTask.wait_for(std::chrono::milliseconds(0));
  }

  {
    std::lock_guard<std::mutex> lock(_mtx);

    if (!_output.empty()) {
      ParseBuffer(_output);
      _output.clear();
    }
  }

  _process = reproc::process{};

  _stations.clear();

  std::cout << "STOP COMPLETE\n";
}

bool Scanner::GetState() {
  std::lock_guard<std::mutex> lock(_mtx);
  return _running;
}

std::vector<Station> Scanner::GetStations() {
  std::lock_guard<std::mutex> lock(_mtx);
  if (!_output.empty()) {
    ParseBuffer(_output);
  }
  return {_stations.begin(), _stations.end()};
}

void Scanner::ParseBuffer(const std::string &buffer) {
  if (buffer.empty())
    return;

  static std::regex stationRegex(
      R"(ARFCN:\s*(\d+),\s*Freq:\s*([\d\.]+)M,\s*CID:\s*(\d+),\s*LAC:\s*(\d+),\s*MCC:\s*(\d+),\s*MNC:\s*(\d+),\s*Pwr:\s*(-?\d+))");

  auto begin = std::sregex_iterator(buffer.begin(), buffer.end(), stationRegex);
  auto end = std::sregex_iterator();

  for (auto i = begin; i != end; ++i) {
    Station s;

    s.arfcn = std::stoi((*i)[1]);
    s.freq = (*i)[2].str() + " MHz";
    s.cid = std::stoi((*i)[3]);
    s.lac = std::stoi((*i)[4]);
    s.mcc = (*i)[5];
    s.mnc = (*i)[6];
    s.pwr = std::stoi((*i)[7]);
    if (s.isValid())
      _stations.insert(s);
  }
}
