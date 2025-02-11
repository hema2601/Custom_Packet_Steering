MY_PATH=/home/hema/Custom_Packet_Steering/module/pkt_steering/


obj-m += pkt_steer_module.o


#EXTRA_CFLAGS += -O0

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(MY_PATH) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(MY_PATH) clean
