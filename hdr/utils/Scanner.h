#pragma once

#include <future>
#include <iostream>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <vector>
#include <atomic>

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

    bool isValid() const {
        if (cid == 0 || lac == 0 || mcc == "0" || mnc == "0" || mnc == "99")
      return false;
    return true;
    }

    bool operator<(const Station &other) const {
        return arfcn < other.arfcn;
    }
};

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
    // ---- reproc ----
    reproc::process _process;
    reproc::options _options;
    reproc::stop_actions _stopActs;
    std::error_code _ec;

            // ---- async drain ----
    std::future<std::error_code> _drainTask;

            // ---- data ----
    std::string _output;
    std::set<Station> _stations;

            // ---- sync ----
    std::mutex _mtx;

            // ---- state ----
    std::atomic<bool> _running{false};
};
