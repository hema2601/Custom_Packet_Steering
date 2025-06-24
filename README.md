# Custom_Packet_Steering
Introduce pluggable packet steering interface and implement a protocol that minimizes Inter-Processor Interrupts


# Setup

Using linux kernel 6.10.8

1. Get the kernel from the linux archives(https://mirrors.edge.kernel.org/pub/)
```
wget https://mirrors.edge.kernel.org/pub/linux/kernel/v6.x/linux-6.10.8.tar.xz

tar -xvf linux-6.10.8.tar.xz

patch -p1 -l < ./patches/custom_kernel.patch 
```
2. Compile the kernel
```
cd linux-6.10.8
make oldconfig # If they ask you for input, just hit enter (they are asking for your preference on new kernel configuration options)
sudo make -j`nproc` bzImage
sudo make -j`nproc` modules
sudo make INSTALL_MOD_STRIP=1 modules_install
sudo make install
```
then update grub and reboot

Check with uname -r whether the kernel booted correctly 

3. (Optional) Setup custom iperf3
```
git clone https://github.com/hema2601/iperf.git
cd iperf
./configure && make
sudo ln -s /path/to/iperf3/src/iperf3 /bin/iperf3_napi
```
4. To be continued
