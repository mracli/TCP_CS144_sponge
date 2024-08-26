#include "tcp_receiver.hh"

#include "wrapping_integers.hh"
#include <cstdint>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) { 
    // LISTEN state, waiting for SYN segment
    //   which state's status: _isn without value
    const auto &header = seg.header();
    if(!_isn.has_value()){
        if(!header.syn)
            return;
        _isn = header.seqno;
    }
    // if error happened, switch to ERROR state
    if(header.rst)
        set_error();
    
    // ERROR state, don't do any thing
    if(has_error()) 
        return;

    uint64_t checkpoint = stream_out().bytes_written();
    uint64_t abs_no = unwrap(header.seqno, _isn.value(), checkpoint);
    auto stream_index = abs_no - 1 + header.syn;
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
    
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn.has_value())
        return {};
    const auto stream_index = stream_out().bytes_written();
    const auto abs_no = stream_index + 1 + stream_out().input_ended();
    return wrap(abs_no, _isn.value());
}

size_t TCPReceiver::window_size() const {
    return _capacity <= stream_out().buffer_size() ? 0 : _capacity - stream_out().buffer_size();
}
