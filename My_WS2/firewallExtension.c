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

/* make IP4-addresses readable */

#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]


/********************************************************************************/
/* Variable declarations														*/
/********************************************************************************/

	struct nf_hook_ops *reg;

	DECLARE_RWSEM(list_sem); /* semaphore to protect list access */

	static struct proc_dir_entry *Our_Proc_File;

	struct listitem {
		int portno;
		char str_progpath[257];
		struct listitem* ptr_nextInList;	
	}; /* structure for a single item in rules list */

	struct listitem* ptr_headOfList = NULL; /* pointer to the head of the list */

/********************************************************************************/
/* docheck - check rules helper to do actual list searching						*/
/********************************************************************************/
int docheck (int dest){
	int portfound = 0;
	struct listitem* ptr_currentitem;

	/*  check against rules to see if port exists in list */
	ptr_currentitem = ptr_headOfList;

	while ( (!portfound) && ptr_currentitem) {
		// check port number
		if (dest==ptr_currentitem->portno) {
			portfound = 1;
		} else {
			ptr_currentitem = ptr_currentitem->ptr_nextInList; /* only move on if not found */
		} 
	} //end while

	if (!portfound) 
		return 0;  /*we must have reached the end of the list - don't drop */

	/* we have reached our first match so need to start checking filenames */

	return 1;  //just for testing this part

} //end docheck


/********************************************************************************/
/* check_rules - 	used by firewall extension 				*/
/*			searches list to see if packet allowed			*/
/********************************************************************************/
int check_rules (int dest){

	int drop = 0;

	/*  check against rules list to see in found */
	down_read (&list_sem); /* set semaphore for reading */

	drop = docheck (dest); /* do actual checking of list */

	up_read (&list_sem); /* unlock reading */

	return drop;

} //end check rules

/********************************************************************************/
/* FirewallExtensionHook - applies firewall extension							*/
/********************************************************************************/

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
		} else {
	    	printk (KERN_INFO "firewall: Destination address = %u.%u.%u.%u\n", NIPQUAD(ip->daddr));
		}
		printk (KERN_INFO "firewall: destination port = %d\n", htons(tcp->dest)); 
	
		if (in_irq() || in_softirq() || !(mm = get_task_mm(current))) {
			printk (KERN_INFO "Not in user context - retry packet\n");
			return NF_ACCEPT;
		}
		mmput (mm); /* decrease counter controlling access to memory mapping tables */


		/* now check against firewall extension rules */
		if (check_rules(htons (tcp->dest))){
			printk (KERN_INFO "firewall: terminate connection\n!");
			tcp_done (sk); /* terminate connection immediately */
			return NF_DROP;
		}

    }

    return NF_ACCEPT;	
} //end FirewallExtensionHook






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
/* procfs_close - called when proc file is closed - decrements module ref count	*/
/********************************************************************************/
int procfs_close(struct inode *inode, struct file *file)
{
    printk (KERN_INFO "kerneReadWrite closed\n");
    module_put(THIS_MODULE);
    return 0;		/* success */
} //end procfs_close

/********************************************************************************/
/* structure definitions - refers to other functions so have to be declared after*/
/********************************************************************************/

EXPORT_SYMBOL (FirewallExtensionHook);

static struct nf_hook_ops firewallExtension_ops = {
	.hook    = FirewallExtensionHook,
	.owner   = THIS_MODULE,
	.pf      = PF_INET,
	.priority = NF_IP_PRI_FIRST,
	.hooknum = NF_INET_LOCAL_OUT
};

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

	int errno;
  
 	/* create the /proc file */
    Our_Proc_File = proc_create_data (PROC_ENTRY_FILENAME, 0644, NULL, &File_Ops_4_Our_Proc_File, NULL);
    
    /* check if the /proc file was created successfuly */
    if (Our_Proc_File == NULL){
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		PROC_ENTRY_FILENAME);
		return -ENOMEM;
    }
    printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAME);

	//register hook
	errno = nf_register_hook (&firewallExtension_ops); 
	if (errno) {
    	printk (KERN_INFO "Firewall extension could not be registered!\n");
		return errno;
  	}

	printk(KERN_INFO "Firewall extensions module loaded\n");
    return 0;	/* success - a non 0 return means init_module failed; module can't be loaded. */
} //end init_module


/********************************************************************************/
/* cleanup_module -  called when the module is unloaded							*/
/********************************************************************************/
void cleanup_module(void) {

	nf_unregister_hook (&firewallExtension_ops); /* restore everything to normal */
  	printk(KERN_INFO "filewall extension: hook unregistered.\n");

	remove_proc_entry (PROC_ENTRY_FILENAME, NULL);  /* now, no further module calls possible */
  	printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);  

  	/* free the list */
	clear_list();

  	printk(KERN_INFO "Firewall extension: module unloaded\n");

} //end cleanup_module 

