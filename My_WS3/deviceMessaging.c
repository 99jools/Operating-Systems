#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
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

	if ( !(ioctl_num == SET_MAX_SIZE)) {
 		return -EINVAL;  // no valid operation requested
	}

	/* about to modify global variable so need lock */
	mutex_lock (&devLock);

	if ( (ioctl_param <= g_CurrentTotal) ) {
		mutex_unlock (&devLock);
		printk(KERN_INFO "Current total is: %i,  Current maximum: %i Size requested %lx reset rejected\n",g_CurrentTotal, 					g_TotalAllowable,ioctl_param);
		return -EINVAL;	
	}

	// otherwise do reset 
	g_TotalAllowable = ioctl_param;
	mutex_unlock (&devLock);
	printk(KERN_INFO "Current total is: %i,  New maximum: %i Size requested %lx reset accepted\n",g_CurrentTotal, 					g_TotalAllowable,ioctl_param);
   	return 0;
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
	//need to traverse the list and free the space as I go	



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

	int bytes_read = 0;  
//	struct list_head* ptr_first;
	struct struct_Listitem* ptr_RetrievedListitem;

	/* lock queue, remove first item, decrement count and unlock */ 
	mutex_lock (&devLock);

	if (list_empty(&msgQueue) ){
		//list is empty 
		mutex_unlock (&devLock);
		return -EAGAIN;  //nothing to read
	}

	ptr_RetrievedListitem = list_first_entry(&msgQueue, struct struct_Listitem,list);
//	ptr_RetrievedListitem = container_of(ptr_first,struct struct_Listitem,list);
	list_del(&ptr_RetrievedListitem->list);
	g_CurrentTotal = g_CurrentTotal - ptr_RetrievedListitem->msglen;
	mutex_unlock (&devLock);

	/* process retrieved item to decide how many bytes we are going to write to user */ 

	bytes_read = (length < ptr_RetrievedListitem->msglen ) ? length : ptr_RetrievedListitem->msglen;


	/* copy to user buffer and check return code */
	if (copy_to_user (buffer, ptr_RetrievedListitem->ptr_msg, bytes_read) != 0) { 
    	return -EFAULT;
  	}


	/* free memory  */
	kfree(ptr_RetrievedListitem->ptr_msg);  // free memory for message
	kfree(ptr_RetrievedListitem);			// free memory for list item

	return bytes_read;  /* will return smaller of length of message or length of buffer */
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

	mutex_unlock (&devLock);
	return len;  //return bytes written
	

} //end device_write
