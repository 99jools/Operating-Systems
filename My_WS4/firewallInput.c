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


 

MODULE_AUTHOR ("Julie Sewards <jrs300@student.bham.ac.uk>");
MODULE_DESCRIPTION ("FIrewall Input module") ;
MODULE_LICENSE("GPL");

#define BUFFERLENGTH 512

#define PROC_ENTRY_FILENAME "iptraffic"

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

	struct struct_Listitem {
		struct list_head list;
		int remotePort;
		int nextIndex;
		int remoteAddr[32];
		int localPort[32];
		int full;
	}; /* structure for a single item in rules list */

	LIST_HEAD(portList); // creates and initialises the port list




/********************************************************************************/
/* 																				*/
/********************************************************************************/
 




/********************************************************************************/
/* 																				*/
/********************************************************************************/



/********************************************************************************/
/*                      							*/
/********************************************************************************/


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

	ip = ip_hdr (skb);
    if (!ip) {
	printk (KERN_INFO "firewall: Cannot get IP header!\n!");
    }
    
    //    printk (KERN_INFO "The protocol received is %d\n", ip->protocol);
    if (ip->protocol == IPPROTO_TCP) { 
	//	printk (KERN_INFO "TCP-packet received\n");

		/* get the tcp-header for the packet */
		tcp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct tcphdr), &_tcph);
		if (!tcp) {
			printk (KERN_INFO "Could not get tcp-header!\n");
			return NF_ACCEPT;
		}

		if (tcp->syn && tcp->ack) {
	
			printk (KERN_INFO "firewall: Received SYN-ACK-packet \n");
			printk (KERN_INFO "firewall: Source address = %u.%u.%u.%u\n", NIPQUAD(ip->saddr));
			printk (KERN_INFO "firewall: destination port = %d\n", htons(tcp->dest)); 
			printk (KERN_INFO "firewall: source port = %d\n", htons(tcp->source)); 
		
		/* NEED TO PUT THE CALL TO THE MONITORING CODE HERE */

		} // end if syn-ack packet
	}

	return NF_ACCEPT;	

} //end FirewallExtensionHook






/********************************************************************************/
/* add_entry - adds line from user space to the list kept in kernel space		*/
/********************************************************************************/
int add_entry (int portNo) {

	printk (KERN_INFO "I a\n" );


} //end add_entry


/********************************************************************************/
/* clear_list - deletes all elements from current list and frees memory			*/
/********************************************************************************/
void clear_list (void) {
	
	struct struct_Listitem* ptr_RetrievedListitem;
	struct struct_Listitem* ptr_temp;

	//need to traverse the list and free the space as I go	
    	list_for_each_entry_safe(ptr_RetrievedListitem, ptr_temp, &msgQueue, list) {
    		list_del(&(ptr_RetrievedListitem->list));

		printk(KERN_INFO "Freeing list item containing msg %x\n", *(ptr_RetrievedListitem->ptr_msg));
    		kfree(ptr_RetrievedListitem);
    	}
	printk (KERN_INFO "List freed\n" );

} //end clear_list

/********************************************************************************/
/* kernelRead - reads the monitoring data and writes it to the proc file		*/
/********************************************************************************/
ssize_t kernelRead (struct file *fp,
		 char __user *buffer,  /* the destination buffer */
		 size_t buffer_size,  /* size of buffer */
		 loff_t *offset  /* offset in destination buffer */
	        ) {
  int retval = 0;  /* number of bytes read; return value for function */





  printk (KERN_INFO "procfile read returned %d byte\n", retval);
  return retval;
}

/********************************************************************************/
/* kernelWrite - reads data from the proc-buffer and writes it into the kernel	*/
/********************************************************************************/

ssize_t kernelWrite (struct file* ptr_file, const char __user* ptr_userbuffer, size_t count, loff_t *ppoffset) {

  	char* ptr_KernelBuffer; /* the kernel buffer */

	int items = 0;
	const char str_delim[2] = ",";
	char* ptr_NextTok;
	int portNo;

	/* check length of input buffer to avoid tying up kernel trying to allocate excessive amounts of memory */
	if ( (count < 1) || (count > BUFFERLENGTH ) ) {
		return -EINVAL;		
	}


	/* get space to copy the port from the user (remembering to handle any kmalloc error) */
	ptr_kernelBuffer = kmalloc (count+1, GFP_KERNEL); 
   
	if (!ptr_kernelBuffer) {
	return -ENOMEM;
	} //error allocating memory


	/* copy data from user space */
	if (copy_from_user (ptr_KernelBuffer, buffer, count)) { 
		kfree (ptr_KernelBuffer);
		return -EFAULT; //error copying from user
	}
	
	// make sure of null terminated string
	*(ptr_KernelBuffer + count) = '\0';

	//create a new list
	LIST_HEAD(newList); // creates and initialises a new port list

	// set pointer to first token
	ptr_NextTok = ptr_KernelBuffer;



	/* validate the input data and add to list of port monitoring items*/
	while (ptr_NextTok != NULL) ){  

		if ( (strlen(ptr_NextTok) < 1) || (64==items) )  {
			kfree(ptr_KernelBuffer);
			return -EINVAL;  // zero length token or too many ports specified
		} 

		
		// convert token to integer
 		if (!kstrtoui(ptr_NextTok, 10, &portNo){
			kfree(ptr_KernelBuffer);
			return -EINVAL;

		}

		// add this port to the list
		add_entry(portNo);

		// increment the count
		items++;

		//get next token
		ptr_NextTok = strsep(&ptr_NextTok, str_delim)); 

	} //end while

	if (64 == items) {
		kfree(ptr_KernelBuffer);
		return -EINVAL;  // more than 64 ports in list
	}

/********************* if we are here, we have created a list of ports - now it is time to make this the active list for monitoring */

	



	/* all done - free the buffer and return bytes written */
	kfree (ptr_kernelBuffer);
	return count;  

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
	.read   = kernelRead,
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

