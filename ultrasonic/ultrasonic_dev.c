#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/time.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define ULTRASONIC_MAJOR_NUMBER     504
#define ULTRASONIC_NAME             "ultrasonic"

#define GPIO_BASE_ADDR              0x3F200000
#define GPFSEL1                     0x04
#define GPFSEL2                     0x08
#define GPSET0                      0x1c
#define GPCLR0                      0x28
#define GPLEV0                      0x34

#define ECHO_PIN                    19
#define TRIG_PIN                    21

static void __iomem *gpio_base;
volatile unsigned int *gpfsel1;
volatile unsigned int *gpfsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;

int ultrasonic_open(struct inode *inode, struct file *filp) {
    printk(KERN_ALERT "Ultrasonic driver open!!\n");

    gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
    gpfsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpfsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
    gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
    gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
    gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);

    *gpfsel1 &= (0xC7FFFFFF);    // GPIO 19 (echo) is an input
    *gpfsel2 |= (1<<3);          // GPIO 21 (trig) is an output
    
    return 0;
}

int ultrasonic_release(struct inode *inode, struct file *filp) {
    printk(KERN_ALERT "Ultrasonic driver closed!!\n");
    iounmap((void *)gpio_base);
    return 0;
}

ssize_t ultrasonic_read(struct file *filp, char *buf, size_t size, loff_t *f_pos) {
    unsigned long pulse_start = 0;
    unsigned long pulse_end = 0;
    unsigned long pulse_duration = 0;
    unsigned long distance = 0;
    int echo_state = 0;
    struct timeval tv;
    
    printk(KERN_ALERT "Ultrasonic read function called!!\n");

    *gpclr0 |= (1<<TRIG_PIN);
    mdelay(5);
    *gpset0 |= (1<<TRIG_PIN);
    udelay(10);
    *gpclr0 |= (1<<TRIG_PIN);

    while(echo_state == 0) {    
        echo_state = ((*gplev0) & (1<<ECHO_PIN)) >> ECHO_PIN;
        do_gettimeofday(&tv);
    }
    pulse_start = (unsigned long)tv.tv_usec;
    
    while(echo_state == 1) {
        echo_state = ((*gplev0) & (1<<ECHO_PIN)) >> ECHO_PIN;
        do_gettimeofday(&tv);
    }
    pulse_end = (unsigned long)tv.tv_usec;
    
    pulse_duration = pulse_end - pulse_start;
    distance = pulse_duration * 17 / 1000;
    
    printk(KERN_ALERT "Distance : %lu cm", distance);

    copy_to_user(buf, &distance, 4);
    
    return size;
}

static struct file_operations ultrasonic_fops = {
    .owner = THIS_MODULE,
    .read = ultrasonic_read,
    .open = ultrasonic_open,
    .release = ultrasonic_release
};

int __init ultrasonic_init(void) {
    if(register_chrdev(ULTRASONIC_MAJOR_NUMBER, ULTRASONIC_NAME, &ultrasonic_fops) < 0)
        printk(KERN_ALERT "Ultrasonic driver initialization fail\n");
    else
        printk(KERN_ALERT "Ultrasonic driver initialization success\n");

    return 0;
}

void __exit ultrasonic_exit(void) {
    unregister_chrdev(ULTRASONIC_MAJOR_NUMBER, ULTRASONIC_NAME     );
    printk(KERN_ALERT "ultrasonic driver exit done\n");
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hyosung park");
MODULE_DESCRIPTION("des");
