# Computer Networks
This is the lab projects for the course 'Computer Networks I' from the University of Toronto.

## Wireshark Labs
This lab involves using the Wireshark, a packet sniffer software to observe various network in action. The wireshark lab includes five specific labs:
1. HTTP: The basic GET/response interaction, HTTP message formats, retrieving large HTML files, retrieving objects, and HTTP authentication/security.
2. UDP: Capturing UDP traces.
3. TCP: Sequence and acknowledgememt numbers, flow-control, congestion-control, TCP connection setup.
4. IP: Analyze IP traces with traceroute, IP datagram, IP fragmentation.
5. Ethernet: Ethernet and ARP protocol.

## The File Transfer Lab
Implemented a simple client/server program, which interact each other to transfer a file in a connectionless manner. This (fake) file transfer protocol is based on UNIX sockets, and the following features have been implemented:
* This protocol is based on UDP, but uses acknowledgements to guarantee reliable transfer 
* Flow control
* Congestion control
* Domain Name Resolution
* Various file formats (but not binary files)

## The Text Conferencing Lab
This lab uses UNIX TCP sockets to implement a simple text conferencing application. The following features have been implemented:
* Multiple users
* Users must log in with credentials. Server maintains a database for registered users 
* Group chat support
* Domain name resolution
* Allow inviting other users to a group chat
* Browse which users are currently online
