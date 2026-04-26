#include "../../hdr/utils/Scanner.h"

Scanner::~Scanner() {
  Stop(); // ок, но теперь Stop безопасный
}

void Scanner::Start() {
  std::cout << "START CALLED\n";

  // FIX: убрали terminate, только жесткий kill
  _stopActs = {{reproc::stop::kill, reproc::milliseconds(100)}};

  // FIX: не полагаемся на _options.stop
  _options.stop = _stopActs;

  // FIX: убрали stdbuf — он ломает kill
  std::vector<std::string> cmd = {
      "python3", "-u",     "/usr/local/bin/grgsm_scanner",
      "-b",      "GSM900", "-g",
      "40",      "-s",     "2e6",
      "--speed", "29",     "-v"};

  _ec = _process.start(cmd, _options);

  if (_ec == std::errc::no_such_file_or_directory) {
    std::cout << "Program not found. PATH issue. Code: " << _ec.value()
              << std::endl;
    return;
  } else if (_ec) {
    std::cout << "Start error: " << fail(_ec) << std::endl;
    return;
  }

  _running = true;

  // FIX: async оставили, но он теперь НЕ блокирует Stop
  _drainTask = std::async(std::launch::async, [this]() {
    reproc::sink::thread_safe::string sink(_output, _mtx);
    return reproc::drain(_process, sink, sink);
  });
}

void Scanner::Stop() {
  if (!_running)
    return;

  std::cout << "STOP CALLED\n";

  int status = 0;
  std::tie(status, _ec) = _process.stop(_stopActs);

  std::cout << "Force killing process group\n";
  kill(-_process.pid().first, SIGKILL);

  if (_drainTask.valid()) {
    if (_drainTask.wait_for(std::chrono::milliseconds(200)) ==
        std::future_status::ready) {
      _ec = _drainTask.get();
    } else {
      std::cout << "Drain still running, skipping wait\n";
    }
  }

  if (_ec) {
    std::cout << "Stop error: " << fail(_ec) << std::endl;
  }

  std::cout << "Exit status: " << status << std::endl;

  // FIX: обязательно сброс процесса
  _process = reproc::process{};
  _running = false;
}

std::vector<Station> Scanner::GetStations() {
  // FIX: не блокируемся надолго
  if (_drainTask.valid() && _drainTask.wait_for(std::chrono::milliseconds(
                                100)) != std::future_status::ready) {

    std::lock_guard<std::mutex> lock(_mtx);
    ParseBuffer(_output);
    return {_stations.begin(), _stations.end()};
  }

  // FIX: аккуратно забираем результат один раз
  if (_drainTask.valid()) {
    _ec = _drainTask.get();
  }

  std::lock_guard<std::mutex> lock(_mtx);

  if (_output.empty() && _ec) {
    std::cout << "Drain error: " << _ec.message() << "\n";
  }

  return {_stations.begin(), _stations.end()};
}

bool Scanner::GetState() {
  std::lock_guard<std::mutex> lock(_mtx);
  return _running;
}

int Scanner::fail(std::error_code ec) {
  std::cerr << ec.message();
  return ec.value();
}

void Scanner::ParseBuffer(const std::string &buffer) {
  // FIX: используй buffer, а не _output напрямую
  if (buffer.empty())
    return;

  static std::regex stationRegex(
      R"(ARFCN:\s*(\d+),\s*Freq:\s*([\d\.]+)M,\s*CID:\s*(\d+),\s*LAC:\s*(\d+),\s*MCC:\s*(\d+),\s*MNC:\s*(\d+),\s*Pwr:\s*(-?\d+))");

  auto begin = std::sregex_iterator(buffer.begin(), buffer.end(), stationRegex);
  auto end = std::sregex_iterator();

  for (auto i = begin; i != end; ++i) {
    const std::smatch &match = *i;

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
