/*  firewallextension.c - simple version to check loading and unloading
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
 


MODULE_AUTHOR ("Julie Sewards <jrs300@student.bham.ac.uk>");
MODULE_DESCRIPTION ("Beginnings of firewallextension program") ;
MODULE_LICENSE("GPL");

int init_module(void)
{
    printk(KERN_INFO "Firewall extensions module loaded\n");

   // A non 0 return means init_module failed; module can't be loaded.
   return 0;
}


void cleanup_module(void)
{
     printk(KERN_INFO "Firewall extensions module unloaded\n");
}  
