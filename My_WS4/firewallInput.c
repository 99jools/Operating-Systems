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
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/namei.h>


 

MODULE_AUTHOR ("Julie Sewards <jrs300@student.bham.ac.uk>");
MODULE_DESCRIPTION ("FIrewall Input module") ;
MODULE_LICENSE("GPL");

#define WRITEBUFFERLENGTH 512
#define READBUFFERLENGTH 65537

#define TRUE 1
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

	static struct proc_dir_entry *Our_Proc_File;

	spinlock_t   portList_lock;	
	unsigned long flags;

	struct struct_Listitem {
		struct list_head list;
		unsigned remotePort;
		int nextIndex;
		unsigned remoteAddr[32];
		unsigned localPort[32];
		int full;
	}; /* structure for a single item in rules list */

	struct list_head* ptr_PortList; //this ptr points to the currently active port list

	LIST_HEAD(dummyList); // creates and initialises dummy port list


/********************************************************************************/
/* 	monitor																			*/
/********************************************************************************/
void monitor(unsigned remoteip, unsigned remote, unsigned local){

	struct struct_Listitem* ptr_RetrievedListitem;

	//lock list
	spin_lock_irqsave(&portList_lock ,flags);

	//traverse list to see if port is being monitored
	list_for_each_entry(ptr_RetrievedListitem, ptr_PortList, list) {
		
		if (remote == ptr_RetrievedListitem->remotePort){
			
			//record packet information in array
			printk(KERN_INFO "monitor: am recording packet in index = %i \n",ptr_RetrievedListitem->nextIndex);	
			ptr_RetrievedListitem->localPort[ptr_RetrievedListitem->nextIndex]  = local;
			ptr_RetrievedListitem->remoteAddr[ptr_RetrievedListitem->nextIndex] = remoteip;

			//increment next free index 
			ptr_RetrievedListitem->nextIndex++;
			printk(KERN_INFO "monitor: nextIndex = %i \n",ptr_RetrievedListitem->nextIndex);
		
			if (32 == ptr_RetrievedListitem->nextIndex){
				printk(KERN_INFO "am in the 32 case\n");
				ptr_RetrievedListitem->nextIndex = 0;
				ptr_RetrievedListitem->full = TRUE;		
			} 

		}

	} //end traverse list

	//unlock list
	spin_unlock_irqrestore(&portList_lock ,flags);
	return;
} //end monitor

/********************************************************************************/
/* 																				*/
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
	struct iphdr *ip;
	struct struct_Listitem* ptr_RetrievedListitem;

ip = ip_hdr (skb);
    if (!ip) {
		printk (KERN_INFO "firewall: Cannot get IP header!\n!");
    }
    
    //    printk (KERN_INFO "The protocol received is %d\n", ip->protocol);
    if (ip->protocol == IPPROTO_TCP) { 
		//printk (KERN_INFO "TCP-packet received\n");

    	/* get the tcp-header for the packet */
		tcp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct tcphdr), &_tcph);
		if (!tcp) {
	    	printk (KERN_INFO "Could not get tcp-header!\n");
	    	return NF_ACCEPT;
       	}

		if (tcp->syn && tcp->ack) {
		   	printk (KERN_INFO "firewall: Received SYN-ACK-packet   ");
		   	printk (KERN_INFO "firewall: Source address = %u.%u.%u.%u", NIPQUAD(ip->saddr));
		   	printk (KERN_INFO "firewall: destination port = %d   ", htons(tcp->dest)); 
		   	printk (KERN_INFO "firewall: source port = %d\n", htons(tcp->source)); 

/*			//traverse list to see if port is being monitored
			list_for_each_entry(ptr_RetrievedListitem, ptr_PortList, list) {

//printk(KERN_INFO "traversing list - port from listitem %u, syn ack packet port = %u\n", ptr_RetrievedListitem->remotePort, htons(tcp->source));
		
				if (htons(tcp->source) == ptr_RetrievedListitem->remotePort){ */
					monitor(ip->saddr, htons(tcp->source), htons(tcp->dest));
		

			} //end check list
		}
    }
    return NF_ACCEPT;	

} //end FirewallExtensionHook






/********************************************************************************/
/* add_entry - adds line from user space to the list kept in kernel space		*/
/********************************************************************************/
int add_entry (int portNo, struct list_head* ptr_NewList) {

	struct struct_Listitem* ptr_NewListitem; 

	//get space for new list item  
	ptr_NewListitem = (struct struct_Listitem*) kmalloc (sizeof (struct struct_Listitem), GFP_KERNEL);	
	if (!ptr_NewListitem) {
		return 1; // can't add list item
	}

	// set up list item data
	memset(ptr_NewListitem,0,sizeof(*ptr_NewListitem));
	ptr_NewListitem->remotePort = portNo;
	INIT_LIST_HEAD(&ptr_NewListitem->list);  

	// add to tail of list and modify tail pointer
	list_add_tail(&ptr_NewListitem->list, ptr_NewList);
	return 0;
} //end add_entry

/********************************************************************************/
/* clear_list - deletes all elements from a list and frees memory				*/	
/*				no need to lock as list has already been disconnected when this */
/*				function is called											    */
/********************************************************************************/
void clear_list (struct list_head* ptr_OldList) {
	
	struct struct_Listitem* ptr_RetrievedListitem;
	struct struct_Listitem* ptr_temp;

	//need to traverse the list and free the space as I go	
	list_for_each_entry_safe(ptr_RetrievedListitem, ptr_temp, ptr_OldList, list) {
		printk(KERN_INFO "Freeing list item containing port %i\n", ptr_RetrievedListitem->remotePort);
		list_del(&(ptr_RetrievedListitem->list));
		kfree(ptr_RetrievedListitem);
	}


} //end clear_list


/********************************************************************************/
/* copy_to_buffer                     											*/
/********************************************************************************/
int copy_to_buffer(char* ptr_temp, int buffer_len){

	struct struct_OutputLine{
		unsigned rp;
		char colon = ":";
		unsigned ra;
		char colon2 = ":";
		unsigned lp;
		char nl= "\n";
	};

	struct struct_Listitem* ptr_RetrievedListitem;

	int size_line;
	int copied = 0;
	int i;
	int toprint;

	size_line = sizeof(struct struct_OutputLine);

	list_for_each_entry(ptr_RetrievedListitem, ptr_PortList, list) {
		printk(KERN_INFO "reading list item containing port %i\n", ptr_RetrievedListitem->remotePort);

		//decide how many items we are going to print from the array
		if (TRUE==ptr_RetrievedListitem->full){
			printk(KERN_INFO "am in full case \n");
			toprint = 32;
		} else {
			printk(KERN_INFO "am in partial case \n");
			toprint = ptr_RetrievedListitem->nextIndex;
		}

		//write output lines to buffer
		for (i=0; i<toprint; i++){
			printk(KERN_INFO "%i:%u.%u.%u.%u:%i\n", ptr_temp-> remotePort, NIPQUAD(ptr_temp-> remoteAddr[1]), ptr_temp-> localPort[1]);
		}


		//check whether there is enough space
		if ((buffer_len - copied) < size_line) {
			printk(KERN_INFO "buffer full - returning partial data \n");
			return copied;  //not able to fit all of the data into the buffer	
		}

		
		} else {
					printk(KERN_INFO "am in not full case, nextIndex = %i \n",ptr_RetrievedListitem->nextIndex);
			//just print full items
			i=0;
			while (i<ptr_RetrievedListitem->nextIndex){
				printk(KERN_INFO "ptr_RetrievedListitem->remoteAddr= %u, ptr_RetrievedListitem->localPort= %i\n",ptr_RetrievedListitem->remoteAddr[i],ptr_RetrievedListitem->localPort[i]);
				i++;
			}


		}


		// copy data into buffer and advanced pointer
		memcpy( ptr_temp,ptr_RetrievedListitem, size_Listitem);
		ptr_temp = ptr_temp + size_Listitem;	
		copied = copied + size_Listitem;
		
	}
	return copied;  //complete data written to buffer
}

/********************************************************************************/
/* kernelRead - reads the monitoring data and writes it to the proc file		*/
/********************************************************************************/
ssize_t kernelRead (struct file *fp, char __user *buffer,  /* the destination buffer */
		 size_t buffer_size,  /* size of buffer */
		 loff_t *offset ) {

	ssize_t bytes_read;  /* number of bytes read; return value for function */
	char* ptr_KernelReadBuffer; 

	/* check length of input buffer to avoid tying up kernel trying to allocate excessive amounts of memory */
	if ( (buffer_size < 1) || (buffer_size > READBUFFERLENGTH ) ) {
		return -EINVAL;		
	}

	/* get space to copy the port from the user (remembering to handle any kmalloc error) */
	ptr_KernelReadBuffer = kmalloc (buffer_size, GFP_KERNEL); 
	if (!ptr_KernelReadBuffer) {
		return -ENOMEM;
	} 

	
	//lock the list
	spin_lock_irqsave(&portList_lock ,flags);
	
	//copy list contents to buffer
	bytes_read = copy_to_buffer(ptr_KernelReadBuffer, buffer_size);
	

	//unlock the list
	spin_unlock_irqrestore(&portList_lock ,flags);

	/* copy to user buffer and check return code */
	if (copy_to_user (buffer, ptr_KernelReadBuffer, bytes_read) != 0) { 
		kfree(ptr_KernelReadBuffer);
    	return -EFAULT;
  	}


	kfree(ptr_KernelReadBuffer);
  	return bytes_read;
}

/********************************************************************************/
/* kernelWrite - reads data from the proc-buffer and writes it into the kernel	*/
/********************************************************************************/

ssize_t kernelWrite (struct file* ptr_file, const char __user* ptr_userbuffer, size_t count, loff_t *ppoffset) {

  	char* ptr_KernelBuffer; /* the kernel buffer */
	char* ptr_NextTok;
	char* ptr_ThisTok;

	int items = 0;
	const char* str_delim = ",";

	int portNo;
	struct list_head* ptr_NewList;
	struct list_head* ptr_OldList;
	
	/* check length of input buffer to avoid tying up kernel trying to allocate excessive amounts of memory */
	if ( (count < 1) || (count > WRITEBUFFERLENGTH ) ) {
		return -EINVAL;		
	}


	/* get space to copy the port from the user (remembering to handle any kmalloc error) */
	ptr_KernelBuffer = kmalloc (count+1, GFP_KERNEL); 
   
	if (!ptr_KernelBuffer) {
	return -ENOMEM;
	} //error allocating memory


	/* copy data from user space */
	if (copy_from_user (ptr_KernelBuffer, ptr_userbuffer, count)) { 
		kfree (ptr_KernelBuffer);
		return -EFAULT; //error copying from user
	}
	
	// make sure of null terminated string
	*(ptr_KernelBuffer + count) = '\0';

	//create a new list 
	ptr_NewList = kmalloc(sizeof(struct list_head), GFP_KERNEL);
	if (!ptr_NewList) {
		return -ENOMEM;
	} 
	INIT_LIST_HEAD(ptr_NewList); //initialise list_head pointers

	// initialise pointer to start of buffer
	ptr_NextTok = ptr_KernelBuffer;

	/* validate the input data and add to list of port monitoring items*/


	while (ptr_NextTok != NULL) {  
  
		//get token
		ptr_ThisTok = strsep(&ptr_NextTok, str_delim); 

		// check token exists and we haven't exceed 64 items
		if ( ( strlen(ptr_ThisTok) < 1) || (64==items) )  {
			kfree(ptr_KernelBuffer);
			return -99;  
		} 

		// convert token to integer
 		if (kstrtouint(ptr_ThisTok, 10, &portNo) ){  
			kfree(ptr_KernelBuffer);
			return -89;
		}

		// add this port to the list
		if (add_entry(portNo, ptr_NewList) ){
			kfree(ptr_KernelBuffer);
			return -79;  // we were not able to add the list item
		}
		// increment the count
		items++;


	} //end while

    printk (KERN_INFO "before lock\n");

	/*  make this the active list for monitoring */
	
	//lock list
	spin_lock_irqsave(&portList_lock ,flags);

	//switch the pointers
	ptr_OldList = ptr_PortList;
	ptr_PortList = ptr_NewList;

	//unlock list
	spin_unlock_irqrestore(&portList_lock ,flags);

	// clear the old list
	if (ptr_OldList != NULL){
		clear_list(ptr_OldList);
	}

	/* all done - free the buffer and return bytes written */
	kfree (ptr_KernelBuffer);
	return count;  

} //end kernelWRite


/********************************************************************************/
/* procfs_open -  called when proc file is opened to increment module ref count	*/
/********************************************************************************/
int procfs_open(struct inode *inode, struct file *file)
{
//    printk (KERN_INFO "iptraffic opened\n");
	try_module_get(THIS_MODULE);
	return 0;
} //end procfs_open


/********************************************************************************/
/* procfs_close - called when proc file is closed - decrements module ref count	*/
/********************************************************************************/
int procfs_close(struct inode *inode, struct file *file)
{
//    printk (KERN_INFO "iptraffic closed\n");
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
	.hooknum = NF_INET_LOCAL_IN
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
    	printk (KERN_INFO "Firewall input could not be registered!\n");
		return errno;
  	}


	// set pointer to the empty list
	ptr_PortList = &dummyList;
	

	// initialise spin lock
	spin_lock_init(&portList_lock);

	printk(KERN_INFO "Firewall input module loaded\n");
    return 0;	/* success - a non 0 return means init_module failed; module can't be loaded. */
} //end init_module


/********************************************************************************/
/* cleanup_module -  called when the module is unloaded							*/
/********************************************************************************/
void cleanup_module(void) {

	nf_unregister_hook (&firewallExtension_ops); /* restore everything to normal */
  	printk(KERN_INFO "filewall input: hook unregistered.\n");

	remove_proc_entry (PROC_ENTRY_FILENAME, NULL);  /* now, no further module calls possible */
  	printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);  

  	/* free the list - no need for lock as can only by one instance of rmmod*/
	clear_list(ptr_PortList);

  	printk(KERN_INFO "Firewall input: module unloaded\n");

} //end cleanup_module 

