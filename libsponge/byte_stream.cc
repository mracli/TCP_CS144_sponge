#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    if (!remaining_capacity())
        return 0;
    size_t write_size = std::min(remaining_capacity(), data.size());
    _buff.append(data.substr(0, write_size));
    _bytes_pushed += write_size;
    return write_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    if (buffer_empty())
        return {};
    size_t peek_size = std::min(len, buffer_size());
    return _buff.substr(0, peek_size);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (buffer_empty())
        return;
    size_t pop_size = std::min(len, buffer_size());
    _buff = _buff.substr(pop_size);
    _bytes_popped += pop_size;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    if(buffer_empty())
        return {};
    std::string result = peek_output(len);
    pop_output(result.size());
    return result;
}

void ByteStream::end_input() { _is_finished = true; }

bool ByteStream::input_ended() const { return _is_finished; }

size_t ByteStream::buffer_size() const { return _buff.size(); }

bool ByteStream::buffer_empty() const { return _buff.empty(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_pushed; }

size_t ByteStream::bytes_read() const { return _bytes_popped; }

size_t ByteStream::remaining_capacity() const { return buffer_size() >= _capacity ? 0 : _capacity - buffer_size(); }
