#include "../../hdr/utils/Scanner.h"

Scanner::~Scanner() { Stop(); }

void Scanner::Start() {
  std::cout << "START CALLED\n";
  _stopActs = {{reproc::stop::terminate, reproc::milliseconds(5000)},
               {reproc::stop::kill, reproc::milliseconds(2000)},
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

std::string Scanner::GetOutput() {
  if (_drainTask.valid() && _drainTask.wait_for(std::chrono::milliseconds(
                                500)) != std::future_status::ready) {
    std::lock_guard<std::mutex> lock(_mtx);
    std::string str = _output;
    // _output.clear();
    return str;
  }

  if (_drainTask.valid()) {
    _ec = _drainTask.get();
  }

  std::lock_guard<std::mutex> lock(_mtx);
  if (_output.empty() && _ec) {
    return "Error: " + _ec.message();
  }

  std::string str = _output;
  // _output.clear();
  return str;
}

int Scanner::fail(std::error_code ec) {
  std::cerr << ec.message();
  return ec.value();
}
