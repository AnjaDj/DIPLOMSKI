#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anja Dj.");
MODULE_DESCRIPTION("ADC Driver");
MODULE_VERSION("4.0");

#define I2C_CLIENT_NAME ("CLIENT_ADC")
#define I2C_CLIENT_ADDR (0x48)

static struct i2c_adapter *i2c_client_adapter = NULL;
static struct i2c_client  *i2c_client_device  = NULL;

/* Timer dependencies */
static struct hrtimer mytimer;
static ktime_t kt;

/* message for turning on the A/D conversion
 *   SD      - 1   (Single Ended Input)
 *   C2-C0   - 001 (CH1 Selected)
 *   PD1-PD0 - 11  (Internal Reference ON and A/D Converter ON)
 */
const char INIT_ADC_CONVERSION_MESSAGE = 0x9c;

/* message for turning off the A/D conversion
 *   SD      - 1   (Single Ended Input)
 *   C2-C0   - 001 (CH1 Selected)
 *   PD1-PD0 - 00  (Power Down Between A/D Converter Conversions)
 */
const char SHUTDOWN_ADC_CONVERSION_MESSAGE = 0x90;

int adc_driver_major; 

/*  Each 12-bit data word is returned in two bytes */
char digital_voltage_value[2];

/* Timer callback function */
enum hrtimer_restart timer_callback(struct hrtimer* timer);

/* List of I2C devices supported by this driver (ADC driver) */
static const struct i2c_device_id supported_devices[] = {
    { I2C_CLIENT_NAME , 0},
    { }
};

/* Function called when the slave (ADC) has been found */
static int driver_probe(struct i2c_client *client)
{
	return 0;
}
/* Function called when the slave (ADC) has been removed */
static void  driver_remove(struct i2c_client *client )
{
	i2c_master_send(i2c_client_device, &SHUTDOWN_ADC_CONVERSION_MESSAGE, 1);
	return;
}

MODULE_DEVICE_TABLE(i2c, supported_devices);

static struct i2c_driver driver = {
    .driver = {
        .name   = I2C_CLIENT_NAME,
        .owner  = THIS_MODULE,
    },
    .remove     = driver_remove, 	   // @remove: Function called when the slave (ADC) has been removed
    .probe 	    = driver_probe,        // @probe:  Function called when the slave (ADC) has been found
    .id_table   = supported_devices    // @id_table:  List of I2C devices supported by this driver
};

static struct i2c_board_info   board_info = {
	.type = I2C_CLIENT_NAME,
	.addr = I2C_CLIENT_ADDR
};


/*FILE OPERATIONS*/


/* Function called when the Device file has been opened */
static int etx_open(struct inode *inode, struct file *filp)
{
    return 0;
}
/* Function called when the Device file has been closed */
static int etx_release(struct inode *inode, struct file *filp)
{
    return 0;
}
/* Function called when the Device file has been written in */
static ssize_t etx_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
    return 0;
}
/* Function called when the Device file has been read from */
static ssize_t etx_read(struct file *filp, char *buf, size_t len, loff_t *f_pos)
{
    /* Size of valid data in bytes received from ADC */
    int data_size = 0;

    /* Average data value */
    uint32_t avg = 0;
	
    // 20 conversions
	for(int i = 0; i < 20; i++)
	{
    
	    /* Master (RPi) sends a message to the client (ADC) to start AD conversion */
	    i2c_master_send(i2c_client_device, &INIT_ADC_CONVERSION_MESSAGE, 1);
	    
	    hrtimer_start(&mytimer, ktime_set(1,0), CLOCK_MONOTONIC);

	    /* Receiving 2bytes raw converted data from ADC */
	    i2c_master_recv(i2c_client_device, digital_voltage_value, 2);
		
        /* Master (RPi) sends a message to the client (ADC) to end AD conversion */
	    i2c_master_send(i2c_client_device, &SHUTDOWN_ADC_CONVERSION_MESSAGE, 1);
	
	    data_size = strlen(digital_voltage_value);
	    
	    // ADC as first byte sends the MOST SIGNIFICANT byte
	    uint16_t temp = digital_voltage_value[0]<<8 | digital_voltage_value[1];
	    avg += temp;
	}
    
	avg /= 20;
	char uradi[4];
	uradi[0] = avg & 0x000000ff;
	uradi[1] = (avg >> 8)  & 0x000000ff;
	uradi[2] = (avg >> 16) & 0x000000ff;
	uradi[3] = (avg >> 24) & 0x000000ff;
	
	printk("avg = %x\n", avg);
	
	/* Sends data from kernel to user space */
	if (copy_to_user(buf, uradi, 4) != 0)
	{
	    printk(KERN_INFO "copy_to_user(buf,avg,4)");
        return -EFAULT;
    }
    else
    {
        (*f_pos) += data_size;
        *f_pos = 0;
        return data_size;
    }
}

/* Structure declaring usual file access functions */
static struct file_operations adc_fops =
{
    .owner    = THIS_MODULE,
    .read     = etx_read,			/* Function called when the Device file has been read from */
    .write    = etx_write, 			/* Function called when the Device file has been written in */
    .open     = etx_open,			/* Function called when the Device file has been opened */
    .release  = etx_release,        /* Function called when the Device file has been closed */
};



/* Module Init function */
static int      __init etx_driver_init(void)
{
    int ret = -1;
    int result = -1;
    i2c_client_adapter = i2c_get_adapter(1);

    if( i2c_client_adapter != NULL )
    {
        i2c_client_device = i2c_new_client_device(i2c_client_adapter, &board_info);
 
        if( i2c_client_device != NULL )
        {
	    int result = i2c_register_driver(THIS_MODULE, &driver);
            ret = 0;
        }

        i2c_put_adapter(i2c_client_adapter);
    }

    pr_info("I2C driver added\n");
    printk(KERN_INFO "Inserting ADC driver module\n");

    /* Registering device. */
    result = register_chrdev(0, "adc_driver", &adc_fops);
    if (result < 0)
    {
        printk(KERN_INFO "adc_driver: cannot obtain major number %d\n", adc_driver_major);
        return result;
    }

    adc_driver_major = result;
    printk(KERN_INFO "adc_driver major number is %d\n", adc_driver_major);

    /* Initialize timer to a given clock */
    hrtimer_init(&mytimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    mytimer.function = &timer_callback;

    return ret;
}

/* Module exit function */
static void __exit etx_driver_exit(void)
{
	printk(KERN_INFO "Removing adc_driver module\n");
	hrtimer_cancel(&mytimer);
	unregister_chrdev(adc_driver_major, "adc_driver"); 
	i2c_unregister_device(i2c_client_device); 
	i2c_del_driver(&driver);
}

/* Timer callback, called with hrtimer_start  */
enum hrtimer_restart timer_callback(struct hrtimer* timer)
{
    printk(KERN_INFO "hrtimer handler called!\n");
    return HRTIMER_NORESTART;
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);
