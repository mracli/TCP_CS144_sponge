#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    // pre_check if now should be pushed
    if (stream_out().error() || !_output.remaining_capacity())
        return;
    if (eof) {
        _last_index = index + data.size();
    }
    if (data.empty()){
        if(eof)
            _output.end_input();
        return;
    }

    // 1. check if data is in valid range, eg: data already fully pushed, or out of current capacity range
    size_t right_index = index + data.size();
    size_t begin_index = stream_out().bytes_written();
    size_t end_index = begin_index + _capacity;
    if (index >= end_index || right_index <= begin_index)
        return;

    // 2.  if data valid then try to insert into _buff
    // 2.1 pre-process data: stich unused part of data
    size_t stich_left = std::max(begin_index, index);
    size_t sitch_right = std::min(end_index, right_index);

    // 2.3 virtually insert data which exactly fitted in current reassembler's status
    if(sitch_right > stich_left)
        insert_buffer(data.substr(stich_left - index, sitch_right - stich_left), stich_left);

    // 3   check if it's satisfied situation to insert _buff into _output
    if(_output.bytes_written() == _buff.front().first)
        pop_from_buffer();
    // 4   finnally if all data popped into _output, then we close writer
    if (_last_index.has_value() && (_last_index.value() == _output.bytes_written()))
        _output.end_input();
}

void StreamReassembler::insert_buffer(std::string &&data, const size_t index) {
    // DUMMY_CODE(data);
    // DUMMY_CODE(index);
    auto begin_index = index;
    const auto end_index = index + data.size();
    for (auto it = _buff.begin(); it != _buff.end() && begin_index < end_index;) {
        // check if current data not valid to insert
        if (begin_index >= it->first) {
            begin_index = std::max(it->first + it->second.size(), begin_index);
            it++;
            continue;
        }
        if (begin_index == index && end_index <= it->first) {
            _buffed_size += (end_index - begin_index);
            _buff.emplace(it, begin_index, std::move(data));
            return;
        }
        const auto right_index = std::min(end_index, it->first);
        const auto len = (right_index - begin_index);
        it = _buff.emplace(it, begin_index, data.substr(begin_index - index, len));
        _buffed_size += len;
        begin_index = right_index;
    }
    // if _buff is empty or still leave some data could be inserted, we just insert it into _buff
    if (begin_index < end_index) {
        _buffed_size += end_index - begin_index;
        _buff.emplace_back(begin_index, data.substr(begin_index - index));
    }
}

void StreamReassembler::pop_from_buffer() {
    auto &stream = stream_out();
    for (auto it = _buff.begin(); it != _buff.end();) {
        auto begin_index = stream.bytes_written();
        if (it->first > begin_index)
            break;
        stream.write(it->second);
        _buffed_size -= it->second.size();
        it = _buff.erase(it);
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _buffed_size; }

bool StreamReassembler::empty() const { return _buff.empty(); }
