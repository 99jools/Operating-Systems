KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

MODULES = charDeviceDriver.ko 

obj-m += charDeviceDriver.o 

all:	
	make -C  $(KERNELDIR) M=$(PWD) modules
	 

client: client.c
	gcc  -Wall -Werror -pthread -D _GNU_SOURCE -o client client.c

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	$(RM) -f client *~

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra

