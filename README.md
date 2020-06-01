# Real Time Embedded Systems Project

## Project Information

Using a *Raspberry Pi* Zero, our goal was to create a *C* application that uses *sockets* and threads and can interact with other *Raspberry Pi*'s exchanging messages.

More specifically, by using the same *SSID* network, the devices will belong to the same network and can be searched using their gateway private IP. Upon connection, a message queue is used to store received and generated messages.

*PThreads* were used in order to implement a client/server design on the **same** device. The general network was a *Peer-To-Peer architecture*.

The design of my implementation is shown in the following flow chart

![Flow Chart](./Asset/fchart.png)