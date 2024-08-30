#include "network_interface.hh"

#include "arp_message.hh"
#include "buffer.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "ipv4_datagram.hh"
#include "ipv4_header.hh"
#include "parser.hh"

#include <cstdint>
#include <iostream>
#include <memory>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    if(dgram.header().proto != IPv4Header::PROTO_TCP) return;

    // check if our _table has corresponding next_hop_ip's ethernet addr.
    auto it = _table.find(next_hop_ip);
    if(it != _table.end()){
        _send_frame(it->second.addr, EthernetHeader::TYPE_IPv4, dgram.serialize());
    } else{
        // either _table nor _waiting_arp_response_table has record before
        if(_waiting_arp_response_table.find(next_hop_ip) == _waiting_arp_response_table.end()){
            ARPMessage arp_msg;
            arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
            arp_msg.sender_ethernet_address = _ethernet_address;
            arp_msg.target_ip_address = next_hop_ip;
            arp_msg.target_ethernet_address = {};
            arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
            _send_frame(ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, arp_msg.serialize());
            _waiting_arp_response_table[next_hop_ip] = ARP_RESP_TTL_MS;
        }
        _waiting_internet_dgrams_table[next_hop_ip].emplace_back(dgram, next_hop_ip);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    const auto &header = frame.header();
    if(header.dst != ETHERNET_BROADCAST && header.dst != _ethernet_address){
        return nullopt;
    }

    if(header.type == EthernetHeader::TYPE_IPv4){
        InternetDatagram ret;
        if(ret.parse(frame.payload()) != ParseResult::NoError) return nullopt;
        return ret;
    }

    if(header.type == EthernetHeader::TYPE_ARP){
        ARPMessage msg;
        if(msg.parse(frame.payload()) != ParseResult::NoError) return nullopt;
        if(msg.opcode == ARPMessage::OPCODE_REQUEST && 
           msg.target_ip_address == _ip_address.ipv4_numeric()){     
            ARPMessage recv_msg;
            recv_msg.sender_ethernet_address = _ethernet_address;
            recv_msg.sender_ip_address = _ip_address.ipv4_numeric();
            recv_msg.target_ethernet_address = msg.sender_ethernet_address;
            recv_msg.target_ip_address = msg.sender_ip_address;
            recv_msg.opcode = ARPMessage::OPCODE_REPLY;
            _send_frame(msg.sender_ethernet_address, EthernetHeader::TYPE_ARP, recv_msg.serialize());
        }

        _table[msg.sender_ip_address] = {ARP_TTL_MS, msg.sender_ethernet_address};
        // No neet to process OPCODE_RESPONSE ARPMessage
        // If we have corresponding next_hop updated, try to send local bufferred datagrams
        auto it = _waiting_internet_dgrams_table.find(msg.sender_ip_address);
        if(it != _waiting_internet_dgrams_table.end()){
            auto &ip_list = it->second;
            for(auto it1 = ip_list.begin(); it1 != ip_list.end(); it1++){
                const auto &[dgram, next_hop] = *it1;
                _send_frame(msg.sender_ethernet_address, EthernetHeader::TYPE_IPv4, dgram.serialize());
            }
            _waiting_internet_dgrams_table.erase(it);
        }
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    // delete timeout arp_addr in arp_table
    for(auto it = _table.begin(); it != _table.end();){
        if(it->second.ttl > ms_since_last_tick){
            it->second.ttl -= ms_since_last_tick;
            it++;
        }else{
            auto it2 = _waiting_internet_dgrams_table.find(it->first);
            if(it2 != _waiting_internet_dgrams_table.end()){
                _waiting_internet_dgrams_table.erase(it2);
            }
            it = _table.erase(it);
        }
    }

    // delete timeout arp_req in waiting_arp_response_table
    for(auto it = _waiting_arp_response_table.begin(); it != _waiting_arp_response_table.end();){
        if(it->second > ms_since_last_tick){
            it->second -= ms_since_last_tick;
            it++;
        }else{
            auto it2 = _waiting_internet_dgrams_table.find(it->first);
            if(it2 != _waiting_internet_dgrams_table.end()){
                _waiting_internet_dgrams_table.erase(it2);
            }
            it = _waiting_arp_response_table.erase(it);
        }
    }
}

void NetworkInterface::_send_frame(const EthernetAddress &dst, const uint16_t type, BufferList &&payload){
    DUMMY_CODE(dst, type, payload);
    EthernetFrame frame;
    frame.header().src = _ethernet_address;
    frame.header().dst = dst;
    frame.header().type = type;
    frame.payload() = std::move(payload);
    frames_out().emplace(std::move(frame));
}