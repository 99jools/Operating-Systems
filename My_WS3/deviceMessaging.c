/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "deviceMessaging.h"
#include "ioctl.h"


MODULE_LICENSE("GPL");

/********************************************************************************/
/* Variable declarations														*/
/********************************************************************************/
DEFINE_MUTEX  (devLock);

static int g_TotalAllowable = 2 * K * K;
static int g_CurrentTotal = 0;	


LIST_HEAD(msgQueue); // creates and initialises message queue


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
	    printk(KERN_INFO "deviceMessaging: I am in ioctl with parameter SET_MAX_SIZE\n");

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
	  printk(KERN_ALERT "deviceMessaging: Registering char device failed with %d\n", Major);
	  return Major;
	}

	printk(KERN_INFO "deviceMessaging loaded - device assigned major number %d\n", Major);
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
  	printk(KERN_INFO "deviceMessaging: module unloaded\n");
}



/********************************************************************************/
/* device_open -  called when a process tries to open the device file		*/
/********************************************************************************/
static int device_open(struct inode *inode, struct file *file)
{
    
   printk(KERN_INFO "deviceMessaging: am in device_open\n");
   try_module_get(THIS_MODULE);
    
   return 0;
}

/********************************************************************************/
/* device_release -  called when a process closes the device file		*/
/********************************************************************************/
static int device_release(struct inode *inode, struct file *file)
{
  	printk(KERN_INFO "deviceMessaging: am in device_release\n");
	/* 
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);

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


   printk(KERN_INFO "deviceMessaging: I am in device_read\n");

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
}

/********************************************************************************/
/* do_count_under_lock - manages locking to increment and decrement count 	*/
/* 																				*/
/********************************************************************************/ 
int do_count_under_lock(int op, size_t len){

	int rc = 0;
	
	// acquire lock
	mutex_lock (&devLock);

	// perform operation

	switch (op){
	case (INCREMENT):
		//add to total if room
		if ( (g_CurrentTotal + len) <= g_TotalAllowable  ){
			g_CurrentTotal = g_CurrentTotal + len;
			rc = SUCCESS;
		} else {
			rc = -EAGAIN;  // reject - no space
		}
		break;
	case (DECREMENT):
		g_CurrentTotal = g_CurrentTotal - len;
		rc = SUCCESS;
		break; 
	}

	
	//release lock
 	mutex_unlock (&devLock);
	return rc;

} //end do_count_under_lock


/********************************************************************************/
/* device_write -	Called when a process attempts to write to a device 	 	*/
/* 																				*/
/********************************************************************************/ 
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	char* ptr_NewMsg;  
	struct struct_Listitem* ptr_NewListitem;  
	
	//reject if user is trying to write a message longer than 4k
	if ( (len < 1) | (len > 4*K) ) {
		return -EINVAL;
	}

	/* if we have space to add message, increment count */
	if (do_count_under_lock(INCREMENT,len) < 0) {
		return -EAGAIN;  //no room
	}

	// get space to store message (remembering to handle any kmalloc error)
	ptr_NewMsg = (char*)kmalloc (len, GFP_KERNEL);	
	if (!ptr_NewMsg) {
		do_count_under_lock(DECREMENT,len);  // need to reset the count as can't add after all
		return -ENOMEM; // can't add an element if no memory
	}

	/* copy msg from user space to kernel space  */
  	if (copy_from_user (ptr_NewMsg, buff, len) != 0) { 
		do_count_under_lock(DECREMENT,len);
    	return -EFAULT;
  	}

	//get space for new list entry 
	ptr_NewListitem = (struct struct_Listitem*) kmalloc (sizeof (struct struct_Listitem), GFP_KERNEL);	
	if (!ptr_NewListitem) {
		do_count_under_lock(DECREMENT,len);  
		return -ENOMEM; // can't add list item
	}

	// set up list item data
	ptr_NewListitem->ptr_msg = ptr_NewMsg;
	ptr_NewListitem->msglen = len;
	INIT_LIST_HEAD(&ptr_NewListitem->list);  

	/* NOW WE ARE GOING TO ADD IT TO THE LIST */

	// reaquire lock in order to modify the list
	mutex_lock (&devLock);

	// add to tail of list and modify tail pointer

	list_add_tail(&ptr_NewListitem->list, &msgQueue);

	printk(KERN_INFO "Message of length %zd added to queue\n", len);
	
	mutex_unlock (&devLock);
	return len;  //return bytes written
	

} //end device_write
