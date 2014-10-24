/*  Example for transferring data from user space into the kernel 
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/proc_fs.h> 
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/list.h>

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Writing data to kernel") ;
MODULE_LICENSE("GPL");

#define BUFFERLENGTH 256
#define ADD_ENTRY 'A'
#define SHOW_TABLE 'S'

#define PROC_ENTRY_FILENAME "kernelWrite"

DECLARE_RWSEM(list_sem); /* semaphore to protect list access */

static struct proc_dir_entry *Our_Proc_File;

struct lineList {
  char *line;
  struct lineList *next;
}; /* the list-structure for keeping the data in the kernel */

struct lineList *kernelList = NULL; /* the global list of words */

/* adds line from user space to the list kept in kernel space */
struct lineList *add_entry (struct lineList *lineList, char *line) {

  struct lineList *newEntry;
  /* allocate memory for new list element */
  newEntry = kmalloc (sizeof (struct lineList), GFP_KERNEL);
  if (!newEntry) {
    return NULL;
  }

  newEntry->line = line;

  /* protect list access via semaphore */
  down_write (&list_sem);
  newEntry->next = lineList;
  lineList = newEntry;
  up_write (&list_sem);

  /* return new list */
  return lineList;

}

/* displays the kernel table - for simplicity via printk */
void show_table (struct lineList *lineList) {

  struct lineList *tmp;
  down_read (&list_sem); /* lock for reading */
  tmp = lineList;
  while (tmp) {
    printk (KERN_INFO "kernelWrite:The next entry is %s\n", tmp->line);
    tmp = tmp->next;
  }
  up_read (&list_sem); /* unlock reading */

}

/* This function reads in data from the user into the kernel */
ssize_t kernelWrite (struct file *file, const char __user *buffer, size_t count, loff_t *offset) {


  char *kernelBuffer; /* the kernel buffer */
 
  
  struct lineList *tmp;

  printk (KERN_INFO "kernelWrite entered\n");

  kernelBuffer = kmalloc (BUFFERLENGTH, GFP_KERNEL); /* allocate memory */
   
  if (!kernelBuffer) {
    return -ENOMEM;
  }


  if (count > BUFFERLENGTH) { /* make sure we don't get buffer overflow */
    kfree (kernelBuffer);
    return -EFAULT;
  }

  
  /* copy data from user space */
  if (copy_from_user (kernelBuffer, buffer, count)) { 
    kfree (kernelBuffer);
    return -EFAULT;
  }
      
  kernelBuffer[BUFFERLENGTH -1]  ='\0'; /* safety measure: ensure string termination */
  printk (KERN_INFO "kernelWrite: Having read %s\n", kernelBuffer);

  switch (kernelBuffer[0]) {
    case ADD_ENTRY:
      tmp = add_entry (kernelList, &(kernelBuffer[1]));
      if (!tmp) {
	kfree (kernelBuffer);
	return -EFAULT;
      }
      else {
	kernelList = tmp;
      }
      break;
    case SHOW_TABLE:
      show_table (kernelList);
      break;
    default: 
      printk (KERN_INFO "kernelWrite: Illegal command \n");
  }
  return count;
}
  


/* 
 * The file is opened - we don't really care about
 * that, but it does mean we need to increment the
 * module's reference count. 
 */
int procfs_open(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelWrite opened\n");
	try_module_get(THIS_MODULE);
	return 0;
}

/* 
 * The file is closed - again, interesting only because
 * of the reference count. 
 */
int procfs_close(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelWrite closed\n");
    module_put(THIS_MODULE);
    return 0;		/* success */
}

const struct file_operations File_Ops_4_Our_Proc_File = {
    .owner = THIS_MODULE,
    .write 	 = kernelWrite,
    .open 	 = procfs_open,
    .release = procfs_close,
};


int init_module(void)
{
	

    
    /* create the /proc file */
    Our_Proc_File = proc_create_data (PROC_ENTRY_FILENAME, 0644, NULL, &File_Ops_4_Our_Proc_File, NULL);
    
    /* check if the /proc file was created successfuly */
    if (Our_Proc_File == NULL){
	printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
	       PROC_ENTRY_FILENAME);
	return -ENOMEM;
    }
    
    printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAME);
    
    return 0;	/* success */

}
  
void cleanup_module(void)
{

  struct lineList *tmp;


  remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
  printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);  


  /* free the list */
  while (kernelList) {
      tmp = kernelList->next;
      kfree (kernelList->line);
      kfree (kernelList);
      kernelList = tmp;
  }
  
  printk(KERN_INFO "kernelWrite:Proc module unloaded.\n");
  
}  
