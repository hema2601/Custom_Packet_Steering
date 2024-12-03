# Custom_Packet_Steering
Introduce pluggable packet steering interface and implement a protocol that minimizes Inter-Processor Interrupts


# How to Access Graphs

1. Clone the repository

```
git clone https://github.com/hema2601/Custom_Packet_Steering.git
```

2. Run a webserver in the root directory

```
cd Custom_Packet_Steering
python3 -m http.server 8090
```

3. Access Webserver from browser (if on local machine, use 127.0.0.1:8090)

4. Type in the path in the data directory of the data you want to see (Example: FinalData_TermProject_12-03/BigMsg_GRO_Comb)

5. Press 'Get Value!' Button

# Setup

Using linux kernel 6.10.8

1. Get the kernel from the linux archives(https://mirrors.edge.kernel.org/pub/)

wget https://mirrors.edge.kernel.org/pub/linux/kernel/v6.x/linux-6.10.8.tar.xz

tar -xvf linux-6.10.8.tar.xz

patch -p1 custom_kernel.patch 
