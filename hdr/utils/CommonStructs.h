#pragma once

#include <cstdint>
#include <string>

struct Station {
  int arfcn;
  std::string freq;
  int cid;
  int lac;
  std::string mcc;
  std::string mnc;
  int pwr;

  bool isValid() const {
    if (cid == 0 || lac == 0 || mcc == "0" || mnc == "0" || mnc == "99")
      return false;
    return true;
  }

  bool operator<(const Station &other) const { return arfcn < other.arfcn; }
};

enum class State : uint8_t {
  eMAIN_THEME,
  eSCAN_STARTED,
  eSCANNING,
  eSCANNING_DATA_ACQUIRED,
  eSCAN_DONE,
  eSTART_SPECTRUM,
  eSPECTRUM_RUNNING,
  eSPECTRUM_STOP,
};
