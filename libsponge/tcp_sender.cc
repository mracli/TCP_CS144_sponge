#include "tcp_sender.hh"

#include "buffer.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timer(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    const auto _cur_window_size = ((_window_size == 0) ? static_cast<uint16_t>(1) : _window_size);
    // Actually we can't send any thing
    while(_cur_window_size >= bytes_in_flight()){
        const auto _available_window_size = _cur_window_size - bytes_in_flight();
        if(!_available_window_size) break;
        TCPSegment seg;
        seg.header().seqno = next_seqno();
        if(!_is_syn){
            _is_syn = seg.header().syn = true;
        }

        const auto payload_size = std::min({
            TCPConfig::MAX_PAYLOAD_SIZE, 
            _available_window_size - seg.length_in_sequence_space(),
            stream_in().buffer_size()});
        seg.payload() = Buffer(stream_in().read(payload_size));
        
        if(!_is_fin && stream_in().eof() && seg.length_in_sequence_space() < _available_window_size){
            _is_fin = seg.header().fin = true;
        }
        // try send a overwhelmed segment is forbidden
        if(seg.length_in_sequence_space() == 0) break;
        const auto size = seg.length_in_sequence_space();

        segments_out().push(seg);
        _outstanding_buf.emplace(_next_seqno, std::move(seg));
        _bytes_in_flight += size;
        _next_seqno += size;
        // start the _timer
        if(!_timer.is_running()){
            _timer.start();
        }
    }
    if(_outstanding_buf.empty())
        _timer.stop();
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    DUMMY_CODE(ackno, window_size);
    _window_size = window_size;
    auto const recved_ack_no = unwrap(ackno, _isn, _next_seqno);
    if (recved_ack_no > _next_seqno)
        return;
    
    bool success = false;

    while(!_outstanding_buf.empty()){
        const auto &[seg_no, front_seg] = _outstanding_buf.front();
        if(seg_no + front_seg.length_in_sequence_space() > recved_ack_no)
            break;
        // if success
        success = true;
        _bytes_in_flight -= front_seg.length_in_sequence_space();
        _outstanding_buf.pop();
    }

    if(success){
        _timer.reset_rto();
        _timer.start();
    }
    if(_outstanding_buf.empty())
        _timer.stop();
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    DUMMY_CODE(ms_since_last_tick);
    _timer.tick(ms_since_last_tick);
    if (_timer.is_timeout()) {
        // _timer triggered timeout event, need to resend timeout segment
        const auto [seq_no, seg] = _outstanding_buf.front();
        _segments_out.push(seg);
        if (_window_size) {
            _timer.double_rto();
        }
        _timer.start();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _timer.retranssmission_times(); }

void TCPSender::send_empty_segment() {
    TCPSegment seg{};
    seg.header().seqno = next_seqno();
    _segments_out.emplace(std::move(seg));
}
