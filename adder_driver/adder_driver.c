/*
 *  adder_driver.c: Accumulates integer values written by the user
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
				 * Used to prevent greater than R+W access */
static char msg[BUF_LEN];	/* The msg the device will give when asked */
static char *msg_Ptr;

static char input_buf[BUF_LEN];
static char *input;
static int input_idx = 0;

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
    input = input_buf;

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
	if (Device_Open >= 2)
		return -EBUSY;

	Device_Open++;
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

    printk(KERN_INFO "Reading!\n");

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
    char cur_char;
    const char local_input[BUF_LEN];
    long value = 0;
    char *cur_input = strdup(buff);
    //char *input_Ptr = strdup(buff);

    char *token;
    char delim[] = " /,\n";
    int i;
    int idx = 0;

    if (cur_input != NULL) {
        printk(KERN_INFO "adder: input = %s\n", cur_input);
        for (i = 0; i < len; i++) {
            *input = cur_input[i];
            input++;
            input_idx++;
        }
    } 

    idx += input_idx;
    memcpy((void *) local_input, (void *) input_buf, BUF_LEN);

    // XXX TESTING
    //*input = '\0';
    cur_char = local_input[idx - 1];
    printk(KERN_INFO "adder: input_buf = %s\n", input_buf);
    printk(KERN_INFO "adder: idx = %d\n", input_idx);
    printk(KERN_INFO "adder: cur_char = %d\n", (int)cur_char);

    
    if (/*input_buf[idx - 1] ==  ' ' || input_buf[idx - 2] == '\0' ||*/
            (int) cur_char == 32 || (int) cur_char == 10) {
    printk(KERN_INFO "adder: adding value(s)...\n");
    if (has_non_num(input_buf)) {
        // clear buffer
        memset(input_buf, 0, BUF_LEN);
        printk(KERN_ALERT "adder: bad input value\n");
        return -EINVAL;
    }
    //*input = '\n';
    input = input_buf;
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
            if (has_num(token)) {
                sscanf(token, "%ld", &value);
                number_holder += value;
            }
        }

        // Reset the input buffer pointer
        input = input_buf;
        input_idx = 0;
        /*for (i = 0; i < BUF_LEN; i++) {
            input_buf[i] = 'X';
        }*/
        memset((void *) input_buf, 0, BUF_LEN);
        //memset((void *) input, 0, BUF_LEN);
    } else {
        printk(KERN_ALERT "adder: null input value\n");
    }
    //else {
        // twiddle yer thumbs
    //}
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
    // Return immediately if there are no numbers
    if (!has_num(s)) return 1;

    while (*s != '\0' && *s != '\n') {
        // Allow spaces and dashes for negative numbers
        if (isdigit(*s) == 0 && *s != ' ' && *s != '-') return 1;
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

/*
 * Check if an input has a space or null char in it
*/
int is_finished(char *s, int len)
{
    int i;
    while (i < len) {
        if (s[i] == ' ' || s[i] == '\0') {
            return 1;
        }
        i++;
    }
    return 0;
}
