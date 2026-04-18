#include "../../hdr/utils/Scanner.h"

void Scanner::Start() {
  if (_running)
    return;

  reproc::options options;
  options.redirect.parent = false;

  std::vector<std::string> cmd = {
      "grgsm_scanner", "-b",      "GSM900", "-g", "40", "-s",
      "2e6",           "--speed", "29",     "-v"};

  auto error = _process.start(cmd, options);
  if (!error)
    _running = true;
}

void Scanner::update() {
  if (!_running)
    return;

  char buffer[1024];
  auto [bytes, err] =
      _process.read(reproc::stream::out, reinterpret_cast<uint8_t *>(buffer),
                    sizeof(buffer) - 1);

  if (!err) {
    buffer[bytes] = '\0';
    _output += buffer;
    parse_buffer();
  } else if (err) {
    _running = false;
  }
}

void Scanner::parse_buffer() {
  std::regex pattern(
      R"(ARFCN:\s*(\d+),\s*Freq:\s*([\d\.]+)M,\s*CID:\s*(\d+),\s*LAC:\s*(\d+),\s*MCC:\s*(\d+),\s*MNC:\s*(\d+),\s*Pwr:\s*(-?\d+))");

  auto it = std::sregex_iterator(_output.begin(), _output.end(), pattern);
  auto end = std::sregex_iterator();

  for (; it != end; ++it) {
    std::smatch match = *it;

    int arfcn = std::stoi(match[1]);
    double freq = std::stod(match[2]);
    int cid = std::stoi(match[3]);
    int lac = std::stoi(match[4]);
    int mcc = std::stoi(match[5]);
    int mnc = std::stoi(match[6]);
    int pwr = std::stoi(match[7]);

    std::lock_guard<std::mutex> lock(_mtx);

    auto found =
        std::find_if(_stations.begin(), _stations.end(),
                     [arfcn](const Station &s) { return s.arfcn == arfcn; });

    if (found == _stations.end()) {
      Station gsmStation;
      gsmStation.arfcn = arfcn;
      gsmStation.freq = freq;
      gsmStation.cellId = static_cast<uint8_t>(cid);
      gsmStation.lac = static_cast<uint8_t>(lac);
      gsmStation.mcc = static_cast<uint8_t>(mcc);
      gsmStation.mnc = static_cast<uint8_t>(mnc);
      gsmStation.pwr = static_cast<int8_t>(pwr);
      _stations.push_back(gsmStation);
    } else {
      found->pwr = pwr;
      found->cellId = cid;
      found->lac = lac;
    }
  }

  if (_output.size() > 10000) {
    _output.clear();
  }
}
