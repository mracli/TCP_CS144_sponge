#pragma once

#include <cstdint>

using namespace std;

struct Timer {
  private:
    uint64_t _time{};
    unsigned int _initial_rto;
    unsigned int _rto;
    unsigned int _retranssmission_times{};
    bool _running{};

  public:
    Timer(unsigned int initial_rto) : _initial_rto(initial_rto), _rto(initial_rto) {}

    void start() {
        _running = true;
        _time = 0;
    }

    void stop() { _running = false; }

    void tick(const size_t ms_since_last_tick) { _time += ms_since_last_tick; }

    void double_rto() {
        _rto <<= 1;
        _retranssmission_times++;
    }

    void reset_rto() {
        _rto = _initial_rto;
        _retranssmission_times = 0;
    }

    bool is_running() const { return _running; }

    bool is_timeout() const { return _running && _time >= _rto; }

    unsigned int retranssmission_times() const { return _retranssmission_times; }
};