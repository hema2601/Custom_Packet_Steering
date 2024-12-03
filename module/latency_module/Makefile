MY_PATH=/home/hema/Custom_Packet_Steering/module/latency_module


obj-m += latency_measure.o


#EXTRA_CFLAGS += -O0

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(MY_PATH) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(MY_PATH) clean
