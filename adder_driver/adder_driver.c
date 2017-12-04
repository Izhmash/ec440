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
int has_non_num(char *s);
int has_num(char *s);

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
 * "cat /dev/adder"
 */
static int device_open(struct inode *inode, struct file *file)
{
	// If two processes have opened the adder...
	if (Device_Open == 2)
		return -EBUSY;

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
 * Called when a process writes to dev file: echo "98" > /dev/adder
 */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    long value = 0;
    char *input = strdup(buff);
    char *token;
    char delim[] = " /,\n";
    int i;
    
    // End input buffer before garbage
    for (i = 0; i < len; i++) {
        if (input[i] == '\n') {
            printk(KERN_INFO "adder: enter is at index %d\n", i);
            input[i] = '\0';
        }
    }
    
    if (input != NULL) {
        printk(KERN_INFO "adder: input = %s\n", input);
        // Tokenize input and add to the current saved value
        
        if (has_non_num(input) || input[0] == '\0') {
            printk(KERN_ALERT "adder: bad input value\n");
            return -EINVAL;
        }

        while ((token = strsep(&input, delim)) != NULL) {
            printk(KERN_INFO "adder: token = %s\n", token);
            //if (!has_non_num(token) && token[0] != '\0') {
            if (has_num(token)) {
                sscanf(token, "%ld", &value);
                //printk(KERN_INFO "adder: input = %ld\n", value);
                number_holder += value;
            //} else {
            //    printk(KERN_ALERT "adder: bad input value\n");
            //    return -EINVAL;
            //}
            }
        }
    } else {
        printk(KERN_ALERT "adder: null input value\n");
    }
    printk(KERN_INFO "adder: val = %ld\n", number_holder);
    return len;
}

/*
 * Copy a string and return a pointer
*/
char *strdup(const char *str)
{
    int n = strlen(str) + 1;
    char *dup = kmalloc((unsigned long) n, GFP_USER);
    if(dup)
    {
        strcpy(dup, str);
    }
    return dup;
}


/*
 * Check if a string has any non-numbers in it
*/
int has_non_num(char *s)
{
    while (*s != '\0') {
        if (isdigit(*s) == 0 && *s != ' ') return 1;
        s++;
    }

    return 0;
}

/*
 * Check if a string has no numbers
*/
int has_num(char *s)
{
    while (*s != '\0') {
        if (isdigit(*s) == 1) return 1;
        s++;
    }

    return 0;
}
