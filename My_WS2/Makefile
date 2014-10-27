KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686
CFLAGS = -Wall -Werror -pthread -D _GNU_SOURCE
GCC = gcc $(CFLAGS)

MODULES = firewallExtension.ko 

obj-m += firewallExtension.o 

all:
	make -C  $(KERNELDIR) M=$(PWD) modules

client: client.c
	$(GCC) -o client client.c

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm -f client

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra



