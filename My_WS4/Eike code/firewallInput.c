#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <net/tcp.h>
#include <linux/namei.h>

MODULE_AUTHOR ("Eike Ritter <E.Ritter@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Extensions to the firewall") ;
MODULE_LICENSE("GPL");


/* make IP4-addresses readable */

#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]


struct nf_hook_ops *reg;

unsigned int FirewallExtensionHook (const struct nf_hook_ops *ops,
				    struct sk_buff *skb,
				    const struct net_device *in,
				    const struct net_device *out,
				    int (*okfn)(struct sk_buff *)) {

    struct tcphdr *tcp;
    struct tcphdr _tcph;
    struct iphdr *ip;

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
		
	   if (htons (tcp->source) == 80) {
	       return NF_DROP;
	   }
       }
    }
    return NF_ACCEPT;	
}


static struct nf_hook_ops firewallExtension_ops = {
	.hook    = FirewallExtensionHook,
	.owner   = THIS_MODULE,
	.pf      = PF_INET,
	.priority = NF_IP_PRI_FIRST,
	.hooknum = NF_INET_LOCAL_IN
};

int init_module(void)
{

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
}


void cleanup_module(void)
{

    nf_unregister_hook (&firewallExtension_ops); /* restore everything to normal */
    printk(KERN_INFO "Firewall extensions module unloaded\n");
}  
