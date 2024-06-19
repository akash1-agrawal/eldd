TARGET=work_queue

obj-m = $(TARGET).o

modules:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C /home/akash/linux/ M=`pwd` modules
	
copy:
	scp `pwd`/$(TARGET).ko debian@192.168.7.2:/home/debian/akash

clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean

.phony: modules clean copy
