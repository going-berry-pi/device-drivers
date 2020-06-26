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

#define KEYPAD_MAJOR_NUMBER     505
#define KEYPAD_DEV_NAME         "keypad_dev"

#define GPIO_BASE_ADDR          0x3F200000
#define GPFSEL1                 0x04
#define GPFSEL2                 0x08
#define GPSET0                  0x1c
#define GPCLR0                  0x28
#define GPLEV0                  0x34
#define GPPUD0                  0x94

static void __iomem *gpio_base;
volatile unsigned int *gpfsel1;
volatile unsigned int *gpfsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;
volatile unsigned int *gppud0;

static int line = 0;

int keypad_open(struct inode *inode, struct file *filp) {
    printk(KERN_ALERT "Keypad driver open!!\n");

    gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
    gpfsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpfsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
    gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
    gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
    gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
    gppud0 = (volatile unsigned int *)(gpio_base + GPPUD0);

    *gpfsel2 = (0x000<<18);
    *gpfsel1 = (0x000<<9);
    *gpfsel1 = (0x000<<18);

    *gpfsel1 |= (1<<6);
    *gpfsel1 |= (1<<27);
    *gpfsel2 |= 1;
    *gpfsel2 |= (1<<3);
    *gppud0 |= 0x01;//pull down enable

    return 0;
}

int keypad_release(struct inode *inode, struct file *filp) {
    printk(KERN_ALERT "Keypad driver closed!!\n");
    *gpclr0 |= (1<<12);
    *gpclr0 |= (1<<19);
    *gpclr0 |= (1<<20);
    *gpclr0 |= (1<<21);
    iounmap((void *)gpio_base);
    return 0;
}

ssize_t keypad_read(struct file *filp, char *buf, size_t size, loff_t *f_pos) {
    printk(KERN_ALERT "Keypad read function called - line : %d\n", line);
    
    udelay(20);

    int index = -1;

    if((((*gplev0) & (1<<26)) >> 26) != 0) {
        index = 0;
    }
    else if((((*gplev0) & (1<<13)) >> 13) != 0) {
        index = 1;
    }
    else if((((*gplev0) & (1<<16)) >> 16) != 0) {
        index = 2;
    }
    
    udelay(20);

    copy_to_user(buf, &index, sizeof(int));

    udelay(20);

    if(line == 2) {
        *gpclr0 |= (1<<19);
    }
    else if(line == 7) {
        *gpclr0 |= (1<<21);
    }
    else if(line == 6) {
        *gpclr0 |= (1<<20);
    }
    else if(line == 4) {
        *gpclr0 |= (1<<12);
    }
    
    mdelay(20);

    return size;
}

ssize_t keypad_write(struct file *filp, const char *buf, size_t size, loff_t *f_pos) {
    copy_from_user(&line, buf, sizeof(int));
    printk(KERN_ALERT "Keypad write function called - line :%d\n", line);

    udelay(20);

    if(line == 2) {
        *gpset0 |= (1<<19);
        *gpclr0 = (1<<21);
        *gpclr0 = (1<<20);
        *gpclr0 = (1<<12);
        
    }
    else if(line == 7) {
        *gpset0 |= (1<<21);
        *gpclr0 = (1<<19);
        *gpclr0 = (1<<20);
        *gpclr0 = (1<<12);
    }
    else if(line == 6) {
        *gpset0 |= (1<<20);
        *gpclr0 = (1<<21);
        *gpclr0 = (1<<19);
        *gpclr0 = (1<<12);
    }
    else if(line == 4) {
        *gpset0 |= (1<<12);
        *gpclr0 = (1<<19);
        *gpclr0 = (1<<20);
        *gpclr0 = (1<<21);
        
    }

    mdelay(20);

    return size;
}

static struct file_operations keypad_fops = {
    .owner = THIS_MODULE,
    .read = keypad_read,
    .write = keypad_write,
    .open = keypad_open,
    .release = keypad_release
};

int __init keypad_init(void) {
    if(register_chrdev(KEYPAD_MAJOR_NUMBER, KEYPAD_DEV_NAME, &keypad_fops) < 0)
        printk(KERN_ALERT "Keypad driver initialization fail\n");
    else
        printk(KERN_ALERT "Keypad driver initialization success\n");
    return 0;
}

void __exit keypad_exit(void) {
    unregister_chrdev(KEYPAD_MAJOR_NUMBER, KEYPAD_DEV_NAME);
    printk(KERN_ALERT "Keypad driver exit done\n");
}

module_init(keypad_init);
module_exit(keypad_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Younho Choo");
MODULE_DESCRIPTION("des");
