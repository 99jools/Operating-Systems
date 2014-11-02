/*  firewallextension.c - simple version to check loading and unloading
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <net/tcp.h>
#include <linux/proc_fs.h> 
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include "kernel_comms.h"
 

MODULE_AUTHOR ("Julie Sewards <jrs300@student.bham.ac.uk>");
MODULE_DESCRIPTION ("Beginnings of firewallextension program") ;
MODULE_LICENSE("GPL");

#define BUFFERLENGTH 256
#define ADD_ENTRY 'A'
#define SHOW_TABLE 'L'
#define NEW_LIST 'N'

#define PROC_ENTRY_FILENAME "kernelReadWrite"


/********************************************************************************/
/* Variable declarations														*/
/********************************************************************************/

	DECLARE_RWSEM(list_sem); /* semaphore to protect list access */

	static struct proc_dir_entry *Our_Proc_File;

	struct listitem {
		int portno;
		char str_progpath[257];
		struct listitem* ptr_nextInList;	
	}; /* structure for a single item in rules list */

	struct listitem* ptr_headOfList = NULL; /* pointer to the head of the list */

/********************************************************************************/
/* add_entry - adds line from user space to the list kept in kernel space		*/
/********************************************************************************/
int add_entry (struct rule* ptr_ruleToAdd, int count) {
	struct listitem* ptr_newitem;	

  /* allocate memory for new list element */

	ptr_newitem = (struct listitem*) kmalloc (sizeof (struct listitem), GFP_KERNEL);
	if (!ptr_newitem) {
		return -ENOMEM; // can't add an element if no memory
	}

  	/* prepare list item */
	ptr_newitem->portno = ptr_ruleToAdd->portno;
	strcpy(ptr_newitem->str_progpath,ptr_ruleToAdd->str_program);

  	/* protect list access via semaphore */
  	down_write (&list_sem);

	//add to head of list
	ptr_newitem->ptr_nextInList = ptr_headOfList;
	ptr_headOfList = ptr_newitem;

	// update semaphore
  	up_write (&list_sem);

	return count;

} //end add_entry




/********************************************************************************/
/* clear_list - deletes all elements from current list and frees memory			*/
/********************************************************************************/
void clear_list (void) {
	struct listitem* ptr_temp;	

  	/* lock list for writing */
  	down_write (&list_sem);

	while (NULL != ptr_headOfList) {
		printk (KERN_INFO "Removing portno %i progpath %s from list\n", ptr_headOfList->portno,ptr_headOfList->str_progpath );
		ptr_temp = ptr_headOfList->ptr_nextInList;  //temporarily store pointer
		kfree(ptr_headOfList);
		ptr_headOfList = ptr_temp;		//head of list now points to next item
	}

	//release write lock
  	up_write (&list_sem);

	printk (KERN_INFO "List freed\n" );

} //end clear_list

/********************************************************************************/
/* show_table - displays the kernel table - uses printk for simplicity			*/
/********************************************************************************/
int show_table (int count) {

	struct listitem* ptr_currentitem;

	/* set semaphore for reading */
	down_read (&list_sem); 

	ptr_currentitem = ptr_headOfList;
	while (ptr_currentitem) {

		//print the current item in the list
		printk (KERN_INFO "kernelWrite:The next entry is  portno %i  program  %s\n", ptr_currentitem->portno, ptr_currentitem->str_progpath);

		//update the pointer to point to the next item
		ptr_currentitem = ptr_currentitem->ptr_nextInList; 

	}

	up_read (&list_sem); /* unlock reading */

	return count;
} //end show_table

/********************************************************************************/
/* kernelWrite - reads data from the proc-buffer and writes it into the kernel	*/
/********************************************************************************/

ssize_t kernelWrite (struct file* ptr_file, const char __user* ptr_userbuffer, size_t count, loff_t *ppoffset) {

	struct rule 	kernelRule;   

	if (sizeof(struct rule) != count ) {
		//we have a problem!
		return -EFAULT;
	} 

	/* copy rule from user space to kernel space  */
  	if (copy_from_user (&kernelRule, ptr_userbuffer, sizeof(kernelRule))) { 
    	return -EFAULT;
  	}	

	/* work out what is required based on op code */
  
	kernelRule.str_program[sizeof(kernelRule.str_program)-1] = '\0'; /* safety measure: ensure string termination */

	switch (kernelRule.op) {
	case ADD_ENTRY:
		return  (add_entry(&kernelRule, count));
	case SHOW_TABLE:
		return show_table (count);
	case NEW_LIST:
		clear_list();
		return  (add_entry(&kernelRule, count));
	default: 
		printk (KERN_INFO "kernelWrite: Illegal command %c \n", kernelRule.op);
		return -EFAULT;
	} //end switch

} //end kernelWRite



/********************************************************************************/
/* procfs_open -  called when proc file is opened to increment module ref count	*/
/********************************************************************************/
int procfs_open(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelReadWrite opened\n");
	try_module_get(THIS_MODULE);
	return 0;
} //end procfs_open


/********************************************************************************/
/* procfs_close - called when proc file is closed - decrements module reference count	*/
/********************************************************************************/
int procfs_close(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kerneReadWrite closed\n");
    module_put(THIS_MODULE);
    return 0;		/* success */
} //end procfs_close



//this stricture refers to kernelWrite so hasto be defined 
	const struct file_operations File_Ops_4_Our_Proc_File = {
		.owner 	= THIS_MODULE,
		.write 	= kernelWrite,
		.open 	= procfs_open,
		.release = procfs_close,
	};
/********************************************************************************/
/* the function called when the module is initially loaded						*/
/********************************************************************************/
int init_module(void) {
  
 	/* create the /proc file */
    Our_Proc_File = proc_create_data (PROC_ENTRY_FILENAME, 0644, NULL, &File_Ops_4_Our_Proc_File, NULL);
    
    /* check if the /proc file was created successfuly */
    	if (Our_Proc_File == NULL){
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		PROC_ENTRY_FILENAME);
		return -ENOMEM;
    }
    
	printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAME);
	printk(KERN_INFO "Firewall extensions module loaded\n");
    	return 0;	/* success - a non 0 return means init_module failed; module can't be loaded. */
}

/********************************************************************************/
/* cleanup_module -  called when the module is unloaded							*/
/********************************************************************************/
void cleanup_module(void) {

	remove_proc_entry (PROC_ENTRY_FILENAME, NULL);  /* now, no further module calls possible */
	printk(KERN_INFO "Firewall extensions module unloaded\n");
  	printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);  

  	/* free the list */
	clear_list();
  
  	printk(KERN_INFO "kernelWrite:Proc module unloaded.\n");

} //end cleanup_module 

