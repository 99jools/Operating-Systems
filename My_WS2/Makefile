KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

MODULES = firewallExtension.ko 

obj-m += firewallExtension.o 

all:	client
	make -C  $(KERNELDIR) M=$(PWD) modules
	 

client: client.c
	gcc  -Wall -Werror -pthread -D _GNU_SOURCE -o client.o client.c

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	$(RM) -f client.o *~

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra


