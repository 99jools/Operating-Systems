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
#define SHOW_TABLE 'S'
#define NEW_LIST 'N'

#define PROC_ENTRY_FILENAME "kernelReadWrite"


/********************************************************************************/
/* Variable declarations														*/
/********************************************************************************/

	DECLARE_RWSEM(list_sem); /* semaphore to protect list access */

	static struct proc_dir_entry *Our_Proc_File;

	struct rule_listitem {
		int portno;
		char* pprog_filename;
		struct rule_listItem* nextInList;	
	}; /* structure for a single item in rules list */

	struct rule_listitem* pheadOfList = NULL; /* pointer to the head of the list */

/********************************************************************************/
/* add_entry - adds line from user space to the list kept in kernel space		*/
/********************************************************************************/
struct rule_listitem* add_entry (struct rule_listitem* pHead, struct ruleops* pruleToAdd) {

  struct rule_listitem* pnewEntry; //defines a pointer for the new list item

  /* allocate memory for new list element */
  pnewEntry = kmalloc (sizeof (struct rule_listitem), GFP_KERNEL);
  if (!pnewEntry) {
    return NULL;
  }

  newEntry->portno = pruleToAdd.

  /* protect list access via semaphore */
  down_write (&list_sem);
  newEntry->next = lineList;
  lineList = newEntry;
  up_write (&list_sem);

  /* return new list */
  return lineList;

} //end add_entry


/********************************************************************************/
/* clear_list - deletes all elements from current list and frees memory			*/
/********************************************************************************/
void clear_list () {





} //end clear_list

/********************************************************************************/
/* show_table - displays the kernel table - uses printk for simplicity			*/
/********************************************************************************/
int show_table (int count) {

	struct rule_listitem* pcurrent_item;

	down_read (&list_sem); /* lock for reading */
	pcurrent_item
	while (tmp) {
		printk (KERN_INFO "kernelWrite:The next entry is %s\n", tmp->line);
		tmp = tmp->next;
	}
	up_read (&list_sem); /* unlock reading */
	return count;
} //end show_table

/********************************************************************************/
/* kernelWrite - reads data from the proc-buffer and writes it into the kernel	*/
/********************************************************************************/

ssize_t kernelWrite (struct file *pfile, const char __user *puserbuffer, size_t count, loff_t *ppoffset) {

	struct ruleops* pruleToKernel = (struct ruleops*) puserbuffer;  
	struct ruleops 	kernelRule;   

	if (sizeof(struct ruleops) != count ) {
		//we have a problem!
		return -EFAULT;
	} 

	/* copy rule from user space to kernel space */
  	if (copy_from_user (kernelRule, puserbuffer, sizeof(kernelRule))) { 
    	return -EFAULT;
  	}	
	printk (KERN_INFO "in kernelWrite: op cod is %c\n",kernelRule.op);	

	/* work out what is required based on op code */




	kernelBuffer = kmalloc (BUFFERLENGTH, GFP_KERNEL); /* allocate memory */
   
	if (!kernelBuffer) {
		return -ENOMEM;
	}


	if (count > BUFFERLENGTH) { /* make sure we don't get buffer overflow */
		kfree (kernelBuffer);
		return -EFAULT;
	}
  
      
	kernelRule.prog_filename[sizeof(kernelRule.prog_filename)-1] = '\0'; /* safety measure: ensure string termination */

	switch (kernelRule.op) {
	case ADD_ENTRY:
		return  (add_entry(&kernelRule, count);
	case SHOW_TABLE:
		return show_table (count);
	case NEW_LIST:
		clear_list();
		return  (add_entry(&kernelRule, count);
	default: 
		printk (KERN_INFO "kernelWrite: Illegal command \n");
		return -EFAULT;
	}
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

  struct lineList *tmp;

	remove_proc_entry (PROC_ENTRY_FILENAME, NULL);  /* now, no further module calls possible */
	printk(KERN_INFO "Firewall extensions module unloaded\n");
  	printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);  

  	/* free the list */
	while (kernelList) {
		tmp = kernelList->next;
		kfree (kernelList->line);
		kfree (kernelList);
		kernelList = tmp;
	}
  
  	printk(KERN_INFO "kernelWrite:Proc module unloaded.\n");

} //end cleanup_module 

