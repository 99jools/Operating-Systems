/* Global definition for the example character device driver */

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long);

#define SUCCESS 0
#define TRUE 1
#define DEVICE_NAME "chardev"	
#define BUF_LEN 4096			/* Max length of the message from the device */
#define K 1024
#define INCREMENT 1
#define DECREMENT -1
#define ISZERO 0


/* 
 * Global variables are declared as static, so are global within the file. 
 */
struct cdev *my_cdev;
       dev_t dev_num;

static int Major;				/* Major number assigned to our device driver */

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.unlocked_ioctl = device_ioctl,
	.release = device_release
};

struct struct_Listitem {
		struct list_head list;
		char* ptr_msg;
		size_t msglen;
	}; // structure for a single item in msg queue




