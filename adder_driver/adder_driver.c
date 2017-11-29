/*
 *  adder.c: Accumulates integer values written by the user
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <asm/uaccess.h>	/* for put_user */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ian Ballou");
MODULE_DESCRIPTION("A sample driver that accumulates a value");
MODULE_VERSION("0.1");

/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

char *strdup(const char *str);
int has_letter(const char *s);

#define SUCCESS 0
#define DEVICE_NAME "adder"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
				 * Used to prevent multiple access to device */
static char msg[BUF_LEN];	/* The msg the device will give when asked */
static char *msg_Ptr;

//static char message[256] = {0};
//static short size_of_message;
static long number_holder;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
        Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk(KERN_ALERT "Registering adder failed with %d\n", Major);
	  return Major;
	}

	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	number_holder = 0;

	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
	/* 
	 * Unregister the device 
	 */
  //	int ret = unregister_chrdev(Major, DEVICE_NAME);
	unregister_chrdev(Major, DEVICE_NAME);
  //	if (ret < 0)
  //		printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{

	//if (Device_Open)
	//	return -EBUSY;

	Device_Open++;
	//sprintf(msg, "I already told you %d times Hello world!\n", counter++);
	printk(KERN_INFO "Open!\n");
	sprintf(msg, "Current adder value: %ld\n", number_holder);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;		/* We're now ready for our next caller */

	/* 
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);

	return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	/*
	 * Number of bytes actually written to the buffer 
	 */
	int bytes_read = 0;

	/*
	 * If we're at the end of the message, 
	 * return 0 signifying end of file 
	 */
	if (*msg_Ptr == 0)
		return 0;

    // Update the read message
	sprintf(msg, "Current adder value: %ld\n", number_holder);
	msg_Ptr = msg;

	/* 
	 * Actually put the data into the buffer 
	 */
	while (length && *msg_Ptr) {

		/* 
		 * The buffer is in the user data segment, not the kernel 
		 * segment so "*" assignment won't work.  We have to use 
		 * put_user which copies data from the kernel data segment to
		 * the user data segment. 
		 */
		put_user(*(msg_Ptr++), buffer++);

		length--;
		bytes_read++;
	}


	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
}

/*  
 * Called when a process writes to dev file: echo "hi" > /dev/hello 
 */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    long value = 0;
    char *input = strdup(buff);
    //char *end_ptr;
    char *token;
    char delim[] = " /,\n";
    int first = 1;
    //sprintf(message, "%s(%zu characters)", buff, len);
    if (buff != NULL) {
        //token = strsep(&input, delim);
        printk(KERN_INFO "adder: input = %s\n", input);
        while ((token = strsep(&input, delim)) != NULL) {
            printk(KERN_INFO "adder: token = %s\n", token);
            if (!has_letter(token) && token[0] != '\0') {
                sscanf(token, "%ld", &value);
                //number_holder += simple_strtol(token, &end_ptr, 10);
                printk(KERN_INFO "adder: input = %ld\n", value);
                //if (first) {
                //    number_holder += value - 1; 
                //    first = 0;
                //} else {
                    number_holder += value;
                //}
                //token = strsep (&input, delim);
            }
        }
    } else {
        printk(KERN_ALERT "adder: null input value\n");
    }
    printk(KERN_INFO "adder: val = %ld\n", number_holder);
    return len;
}

char *strdup(const char *str)
{
    int n = strlen(str) + 1;
    char *dup = vmalloc((unsigned long) n);
    if(dup)
    {
        strcpy(dup, str);
    }
    return dup;
}


int has_letter(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 1;
    }

    return 0;
}
