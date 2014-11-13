KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686


MODULES = deviceMessaging.ko 
APPS = ioctl read write

obj-m += deviceMessaging.o 

all:	$(APPS)
	make -C  $(KERNELDIR) M=$(PWD) modules
	 

ioctl: ioctl.c
	gcc  -Wall -Werror -pthread -D _GNU_SOURCE -o ioctl ioctl.c

read: read.c
	gcc  -Wall -Werror -pthread -D _GNU_SOURCE -o read read.c

write: write.c
	gcc  -Wall -Werror -pthread -D _GNU_SOURCE -o write write.c

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	$(RM) -f $(APPS) *~

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra

