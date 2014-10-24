/*  hello.c - The simplest kernel module.
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
#include "entry.h"


#define BUFFERSIZE 80				//for readinodes

#define BUFFERLENGTH 256			//for kernelWrite
#define ADD_ENTRY 'A'				//for kernelWrite
#define SHOW_TABLE 'S'				//for kernelWrite
#define PROC_ENTRY_FILENAME "kernelWrite"


#define BUFFERSIZER 10  			// for kernelRead
#define PROC_ENTRY_FILENAMER "kernelRead"


/* make IP4-addresses readable for firewallExtension*/
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]






 



/*******************************************************************************************************************************************/

/*  Example for transferring data from user space into the kernel 
 */






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



  
 


/*******************************************************************************************************************************************/

/*  Example for transferring data from kernel into user space
 */






int counter = 0; /* number of entries to be transferred in total */

static struct proc_dir_entry *Our_Proc_File;


/* the function called to write data into the proc-buffer */
ssize_t kernelRead (struct file *fp,
		 char __user *buffer,  /* the destination buffer */
		 size_t buffer_size,  /* size of buffer */
		 loff_t *offset  /* offset in destination buffer */
	        ) {
  char *pos;    /* the current position in the buffer */
  struct entry_t entry;
  static int finished = 0;
  int retval = 0;  /* number of bytes read; return value for function */
  int i = 0;
  printk (KERN_INFO "procfile_read called with offset of %lld and buffer size %ld\n",  *offset, buffer_size);

  if (finished) {
      printk (KERN_INFO "procfs_read: END\n");
      finished = 0;
      return 0;
  }
  
  pos = buffer;
  while (pos + sizeof (struct entry_t) <= buffer + buffer_size ) {
    entry.field1 = i;  /* create some data */
    entry.field2 = -i; /* create some data */
    copy_to_user (pos, &entry, sizeof (struct entry_t)); /* copy it into user buffer */
    pos += sizeof (struct entry_t); /* increase the counters */
    counter++;
    i ++;
    retval = retval + sizeof (struct entry_t);
  }
  if (counter == BUFFERSIZER) {
      finished = 1;
      counter = 0;
  }
  printk (KERN_INFO "procfile read returned %d byte\n", retval);
  return retval;
} //end KernelRead


/* 
 * The file is opened - we don't really care about
 * that, but it does mean we need to increment the
 * module's reference count. 
 */
int procfs_open(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelRead opened\n");
	try_module_get(THIS_MODULE);
	return 0;
} //end procfs_open

/* 
 * The file is closed - again, interesting only because
 * of the reference count. 
 */
int procfs_close(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kernelRead closed\n");
	module_put(THIS_MODULE);
	return 0;		/* success */
} //end procfs_close

const struct file_operations File_Ops_4_Our_Proc_File = {
    .owner = THIS_MODULE,
    .read 	 = kernelRead,
    .open 	 = procfs_open,
    .release = procfs_close,
};






  



/*******************************************************************************************************************************************/









struct nf_hook_ops *reg;

unsigned int FirewallExtensionHook (const struct nf_hook_ops *ops,
				    struct sk_buff *skb,
				    const struct net_device *in,
				    const struct net_device *out,
				    int (*okfn)(struct sk_buff *)) {

    struct tcphdr *tcp;
    struct tcphdr _tcph;
    struct mm_struct *mm;
    struct sock *sk;


  sk = skb->sk;
  if (!sk) {
    printk (KERN_INFO "firewall: netfilter called with empty socket!\n");;
    return NF_ACCEPT;
  }

  if (sk->sk_protocol != IPPROTO_TCP) {
    printk (KERN_INFO "firewall: netfilter called with non-TCP-packet.\n");
    return NF_ACCEPT;
  }

    

    /* get the tcp-header for the packet */
    tcp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct tcphdr), &_tcph);
    if (!tcp) {
	printk (KERN_INFO "Could not get tcp-header!\n");
	return NF_ACCEPT;
    }
    if (tcp->syn) {
	struct iphdr *ip;
	
	printk (KERN_INFO "firewall: Starting connection \n");
	ip = ip_hdr (skb);
	if (!ip) {
	    printk (KERN_INFO "firewall: Cannot get IP header!\n!");
	}
	else {
	    printk (KERN_INFO "firewall: Destination address = %u.%u.%u.%u\n", NIPQUAD(ip->daddr));
	}
	printk (KERN_INFO "firewall: destination port = %d\n", htons(tcp->dest)); 
		
	

	if (in_irq() || in_softirq() || !(mm = get_task_mm(current)) || IS_ERR (mm)) {
		printk (KERN_INFO "Not in user context - retry packet\n");
		return NF_ACCEPT;
	}

	
	if (htons (tcp->dest) == 80) {
	    tcp_done (sk); /* terminate connection immediately */
	    return NF_DROP;
	}
    }
    return NF_ACCEPT;	
}

EXPORT_SYMBOL (FirewallExtensionHook);

static struct nf_hook_ops firewallExtension_ops = {
	.hook    = FirewallExtensionHook,
	.owner   = THIS_MODULE,
	.pf      = PF_INET,
	.priority = NF_IP_PRI_FIRST,
	.hooknum = NF_INET_LOCAL_OUT
};



/*******************************************************************************************************************************************/

int init_module(void)
{
	/* initialise for kernelWrite */
    
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
int init_module (void) {

	/* initialise for kernelRead
    /* create the /proc file */
    Our_Proc_File = proc_create_data (PROC_ENTRY_FILENAMER, 0644, NULL, &File_Ops_4_Our_Proc_File, NULL);
    
    /* check if the /proc file was created successfuly */
    if (Our_Proc_File == NULL){
	printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
	       PROC_ENTRY_FILENAMER);
	return -ENOMEM;
    }
    
    printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAMER);
    
    return 0;	/* success */



	//initialise firewallExtension
  int errno;

  errno = nf_register_hook (&firewallExtension_ops); /* register the hook */
  if (errno) {
    printk (KERN_INFO "Firewall extension could not be registered!\n");
  } 
  else {
    printk(KERN_INFO "Firewall extensions module loaded\n");
  }

  // A non 0 return means init_module failed; module can't be loaded.
  return errno;


	/* initialise for readinodes */
    struct path path;
    pid_t mod_pid;
    struct dentry *procDentry;
    struct dentry *parent;

    char cmdlineFile[BUFFERSIZE];
    int res;
    
    printk (KERN_INFO "readInodes module loading\n");
    mod_pid = current->pid;
    snprintf (cmdlineFile, BUFFERSIZE, "/proc/%d/exe", 5515); 
    res = kern_path (cmdlineFile, LOOKUP_FOLLOW, &path);
    if (res) {
	printk (KERN_INFO "Could not get dentry for %s!\n", cmdlineFile);
	return -EFAULT;
    }
    
    procDentry = path.dentry;
    printk (KERN_INFO "The name is %s\n", procDentry->d_name.name);
    parent = procDentry->d_parent;
    printk (KERN_INFO "The name of the parent is %s\n", parent->d_name.name);

    return 0;
    
} //end init_module 

/*******************************************************************************************************************************************/
void cleanup_module(void)

	struct lineList *tmp;

	//cleanup kernelWrite
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

	//cleanup kernelRead
	remove_proc_entry (PROC_ENTRY_FILENAMER, NULL);  /* now, no further module calls possible */
  	printk (KERN_INFO "kernel Read  removed \n");
  


	// cleanup readinodes
	printk(KERN_INFO "readInodes module unloaded \n");

	// cleanup firewallExtension
    nf_unregister_hook (&firewallExtension_ops); /* restore everything to normal */
    printk(KERN_INFO "Firewall extensions module unloaded\n");
  
} //end cleanup_module












