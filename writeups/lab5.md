Lab 5 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the NetworkInterface:
[
在 NetworkInterface 类中：
- 记录本设备的 IP 地址以及 MAC 地址；
- 同时使用`_arp_table` 记录 邻居的 ip-arp 记录，每项记录保存 30 秒；
- 根据待发送的 datagrams 执行如下操作：
  同时如果本地没有邻居 ip-mac 地址对，则尝试向所有邻居发送 arp 请求，并记录在 `_waiting_arp_response_table`, 同时将该 datagram 缓存在 `_waiting_internet_dgrams_table`；
- 为了防止 arp-flood 问题，本地根据 arp 请求记录来判断是否需要重新发送，如果超过 5 秒则清空记录.
- 接收到其他 frame ：
  判断为payload为arp请求，如果是arp请求，且opcode为request，则返回本机 ip-mac 地址的 arp reply；对于接收到的任意arp请求，当尝试记录在 `_arp_table` 中
  判断为payload为datagram，则返回该datagram。
]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
