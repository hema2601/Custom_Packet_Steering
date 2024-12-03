MY_PATH=/home/hema/Custom_Packet_Steering/module/ipi_latency_module


obj-m += ipi_latency.o


#EXTRA_CFLAGS += -O0

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(MY_PATH) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(MY_PATH) clean
