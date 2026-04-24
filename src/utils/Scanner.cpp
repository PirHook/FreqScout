#include "../../hdr/utils/Scanner.h"

Scanner::~Scanner() { Stop(); }

void Scanner::Start() {
  std::cout << "START CALLED\n";
  _stopActs = {{reproc::stop::terminate, reproc::milliseconds(1000)},
               {reproc::stop::kill, reproc::milliseconds(1000)},
               {}};
  _options.stop = _stopActs;
  // stdbuf -oL -eL python3 -u $(which grgsm_scanner) -b GSM900 -g 40 -s 2e6
  // --speed 29 -v
  std::vector<std::string> cmd = {
      "stdbuf",  "-oL",    "-eL",
      "python3", "-u",     "/usr/local/bin/grgsm_scanner",
      "-b",      "GSM900", "-g",
      "40",      "-s",     "2e6",
      "--speed", "29",     "-v"};
  _ec = _process.start(cmd, _options);
  if (_ec == std::errc::no_such_file_or_directory) {
    std::cout << "Program not found. Make sure it's available from the PATH.";
    std::cout << _ec.value() << std::endl;
    return;
  } else if (_ec) {
    std::cout << fail(_ec) << std::endl;
    return;
  }

  _drainTask = std::async(std::launch::async, [this]() {
    reproc::sink::thread_safe::string sink(_output, _mtx);
    return reproc::drain(_process, sink, sink);
  });
}

void Scanner::Stop() {
  int status = 0;
  std::tie(status, _ec) = _process.stop(_options.stop);
  if (_ec) {
    std::cout << fail(_ec) << std::endl;
  }
  std::cout << status << std::endl;
}

std::vector<Station> Scanner::GetStations() {
  if (_drainTask.valid() && _drainTask.wait_for(std::chrono::milliseconds(
                                500)) != std::future_status::ready) {
    std::lock_guard<std::mutex> lock(_mtx);
    ParseBuffer(_output);
    return {_stations.begin(), _stations.end()};
  }

  if (_drainTask.valid()) {
    _ec = _drainTask.get();
  }

  std::lock_guard<std::mutex> lock(_mtx);
  if (_output.empty() && _ec) {
    std::cout << "Error: " + _ec.message() << "\n";
    return {_stations.begin(), _stations.end()};
  }

  return {_stations.begin(), _stations.end()};
}

int Scanner::fail(std::error_code ec) {
  std::cerr << ec.message();
  return ec.value();
}

void Scanner::ParseBuffer(const std::string &buffer) {
  if (_output.empty())
    return;

  if (_output.find("Allocating 15 zero-copy buffers") != std::string::npos) {
    std::cout
        << "[Scanner] Critical hardware string detected. Killing process...\n";
    Stop();
    return;
  }

  std::regex stationRegex(
      R"(ARFCN:\s*(\d+),\s*Freq:\s*([\d\.]+)M,\s*CID:\s*(\d+),\s*LAC:\s*(\d+),\s*MCC:\s*(\d+),\s*MNC:\s*(\d+),\s*Pwr:\s*(-?\d+))");

  auto words_begin =
      std::sregex_iterator(_output.begin(), _output.end(), stationRegex);
  auto words_end = std::sregex_iterator();

  for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
    std::smatch match = *i;

    Station s;
    s.arfcn = std::stoi(match[1].str());
    s.freq = match[2].str() + " MHz";
    s.cid = std::stoi(match[3].str());
    s.lac = std::stoi(match[4].str());
    s.mcc = match[5].str();
    s.mnc = match[6].str();
    s.pwr = std::stoi(match[7].str());

    _stations.insert(s);
  }
}
