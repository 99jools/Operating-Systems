/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>	/* for put_user */
#include "charDeviceDriver.h"
#include "ioctl.h"


MODULE_LICENSE("GPL");

/********************************************************************************/
/* Variable declarations														*/
/********************************************************************************/
DEFINE_MUTEX  (devLock);
//static int currentMaxTotal = 0;


/********************************************************************************/
/* ioctl  -  called when proc file is written to with ioctl command				*/
/********************************************************************************/

static long device_ioctl(struct file *file,	/* see include/linux/fs.h */
		 unsigned int ioctl_num,	/* number and param for ioctl */
		 unsigned long ioctl_param)	{

	/* 
	 * Switch according to the ioctl called 
	 */
	if (ioctl_num == SET_MAX_SIZE) {
	    printk(KERN_INFO "charDeviceDriver: I am in ioctl with parameter SET_MAX_SIZE/n");

	/* check that I am able to assign new Max Size */
	/* if ioctl_param < currentMaxTotal or ioctl_param < currentTotalSize */

	  
	    return 0; //success
	}

	else {
	    /* no operation defined - return failure */
	    return -EINVAL;

	}
}


/********************************************************************************/
/* init_module  -  the function called when the module is initially loaded		*/
/********************************************************************************/
int init_module(void)
{
        Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk(KERN_ALERT "charDeviceDriver: Registering char device failed with %d\n", Major);
	  return Major;
	}

	//create an empty list of messages


	printk(KERN_INFO "charDeviceDriver loaded - device assigned major number %d\n", Major);
	return SUCCESS;
}

/********************************************************************************/
/* cleanup_module -  called when the module is unloaded							*/
/********************************************************************************/
void cleanup_module(void)
{

	/* free the list */
//	clear_list();

	/*  Unregister the device */
	unregister_chrdev(Major, DEVICE_NAME);
  	printk(KERN_INFO "charDeviceDriver: module unloaded\n");
}









/********************************************************************************/
/* device_open -	Called when a process tries to open the device file, like 	*/
/*					"cat /dev/mycharfile"										*/
/********************************************************************************/ 

static int device_open(struct inode *inode, struct file *file)
{
    

    printk(KERN_INFO "charDeviceDriver: in device_open\n");
    return 0;
}

/********************************************************************************/
/* device_close -	Called when a process tries to close the device file	 	*/
/********************************************************************************/

static int device_release(struct inode *inode, struct file *file)
{

 	printk(KERN_INFO "charDeviceDriver: in device_close\n");
	return 0;
}

/********************************************************************************/
/* device_read -	Called when a process which already opened the dev file, 	*/
/* 					attempts to read from it.									*/
/********************************************************************************/  

static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	/*
	 * Number of bytes actually written to the buffer 
	 */
	int bytes_read = 0;


   printk(KERN_INFO "charDeviceDriver: I am in device_read/n");

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello  */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "charDeviceDriver: I am in device_write.\n");
	return -EINVAL;
}
