#include "tcp_connection.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "tcp_state.hh"

#include <cstdint>
#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment_received = 0;
    
    const auto &header = seg.header();

    if(header.rst){
        _set_rst_state();
        return;
    }

    bool need_empty_seg = seg.length_in_sequence_space() > 0;

    _receiver.segment_received(seg);

    if(header.ack){
        _sender.ack_received(header.ackno, header.win);    
        if(need_empty_seg && !_sender.segments_out().empty()){
            need_empty_seg = false;
        }
    }

    // _receiver LISTEN -> SYN_RECV state
    if( TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED &&
        TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV){
        connect();
        return;
    }

    // _receiver FIN_RECV -> PASSIVE close (CLOSE WAIT)
    if( TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_ACKED &&
        TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV){
        _linger_after_streams_finish = false;
    }

    // PASSIVE close
    if( !_linger_after_streams_finish &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED &&
        TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV){
        _active = false;
        return;
    }

    // keep-alive
    if( _receiver.ackno().has_value() && 
        seg.length_in_sequence_space() == 0 &&
        _receiver.ackno().value() - 1 == seg.header().seqno){
        need_empty_seg = true;
    }

    if(need_empty_seg)
        _sender.send_empty_segment();

    _send_with_ack_if_possible();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    auto write_size = _sender.stream_in().write(data);
    _sender.fill_window();

    _send_with_ack_if_possible();

    return write_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);

    // maximum retry time limit
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        while(segments_out().size()) segments_out().pop();
        _set_rst_state();
        TCPSegment seg;
        seg.header().rst = true;
        seg.header().seqno = _sender.next_seqno();
        segments_out().emplace(std::move(seg));
        return;
    }

    _send_with_ack_if_possible();

    // ACTIVE CLOSE
    if( _linger_after_streams_finish &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED &&
        TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        _time_since_last_segment_received >= 10 * _cfg.rt_timeout){
        _active = false;
        _linger_after_streams_finish = false;
    }
}

void TCPConnection::end_input_stream() {
    // _sender convert to FIN_SENT state
    _sender.stream_in().end_input();
    _sender.fill_window();
    _send_with_ack_if_possible();
}

void TCPConnection::connect() {
    if (TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED) {
        _sender.fill_window();
        _send_with_ack_if_possible();
    }
}

void TCPConnection::_send_with_ack_if_possible() {
    while (_sender.segments_out().size()) {
        auto &&seg = _sender.segments_out().front();
        // If seg's ACK part is blank, we can try add it
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = std::min(_receiver.window_size(), static_cast<size_t>(UINT16_MAX));
        }
        segments_out().emplace(std::move(seg));
        _sender.segments_out().pop();
    }
}

void TCPConnection::_set_rst_state() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _linger_after_streams_finish = false;
    _active = false;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _set_rst_state();
            TCPSegment seg;
            seg.header().rst = true;
            seg.header().seqno = _sender.next_seqno();
            _segments_out.emplace(std::move(seg));
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
