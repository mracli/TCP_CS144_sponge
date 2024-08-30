Lab 5 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the NetworkInterface:
[
    在 NI 类中：
        - 记录本设备的 IP 地址以及 MAC 地址；
        - 同时使用 map 记录 邻居的 ip-arp 记录，每项记录保存 30 秒；
        - 同时如果本地没有邻居 ip-mac 地址对，则尝试向所有邻居发送 arp 请求，并记录在 _waiting_arp_response map,；
        - 为了防止 arp-flood 请求攻击，本地根据 arp 请求记录来判断是否需要重新发送，如果超过 5 秒则清空记录；
        - 如果 ip datagram 无法发送 (那么缓存该请求，并尝试向周围的邻居节点请求 ip-mac 地址对)
]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
