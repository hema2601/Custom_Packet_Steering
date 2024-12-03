# Custom_Packet_Steering
Introduce pluggable packet steering interface and implement a protocol that minimizes Inter-Processor Interrupts


# Setup

Using linux kernel 6.10.8

1. Get the kernel from the linux archives(https://mirrors.edge.kernel.org/pub/)

wget https://mirrors.edge.kernel.org/pub/linux/kernel/v6.x/linux-6.10.8.tar.xz

tar -xvf linux-6.10.8.tar.xz

patch -p1 custom_kernel.patch 
