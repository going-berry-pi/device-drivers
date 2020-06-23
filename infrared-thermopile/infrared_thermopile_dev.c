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

#define INFRARED_THERMOPILE_MAJOR_NUMBER     502
#define INFRARED_THERMOPILE_NAME             "infrared_thermopile"

#define GPIO_BASE_ADDR              0x3F200000
#define GPFSEL0                     0x00
#define GPSET0                      0x1c
#define GPCLR0                      0x28

#define BSC1_BASE                   0x3F804000
#define C                           0x0
#define S                           0x4
#define DLEN                        0x8
#define A                           0xc
#define FIFO                        0x10
#define DIV			                    0x14

#define BSC_C_I2CEN                 0x00008000
#define BSC_C_ST 		                0x00000080
#define BSC_C_CLEAR_1 		          0x00000020
#define BSC_C_READ 		              0x00000001

#define BSC_S_CLKT 		              0x00000200
#define BSC_S_ERR 		              0x00000100
#define BSC_S_RXD 		              0x00000020
#define BSC_S_DONE 		              0x00000002

#define CLOCK_DIVIDER               148

static void __iomem *gpio_base;
volatile unsigned int *gpfsel0;
volatile unsigned int *gpfsel1;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;

static void __iomem *bsc1_base;
volatile unsigned int *control;
volatile unsigned int *status;
volatile unsigned int *dlen;
volatile unsigned int *a_register;
volatile unsigned int *fifo;
volatile unsigned int *div;

int infrared_thermopile_open(struct inode *inode, struct file *filp){
    printk(KERN_ALERT "Infrared Thermopile device open function called\n");
    gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
    bsc1_base = ioremap(BSC1_BASE, 0x20);
    gpfsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
    gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
    gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
    
    *gpfsel0 |= (1<<8);
    *gpfsel0 |= (1<<11);
    
    control = (volatile unsigned int *)(bsc1_base + C);
    status = (volatile unsigned int *)(bsc1_base + S);
    dlen = (volatile unsigned int *)(bsc1_base + DLEN);
    a_register = (volatile unsigned int *)(bsc1_base + A);
    fifo = (volatile unsigned int *)(bsc1_base + FIFO);
    div = (volatile unsigned int *)(bsc1_base + DIV);

    *div = 148;
    *a_register = 0x40;

    /* Clear FIFO */
    *control |= BSC_C_CLEAR_1;
    /* Clear Status */
    *status = BSC_S_CLKT | BSC_S_ERR | BSC_S_DONE;
    /* Set Data Length */
    *dlen = 1;
    *fifo = 0x03;
    *control = BSC_C_I2CEN | BSC_C_ST;
    
    return 0;
}

int infrared_thermopile_release(struct inode *inode, struct file *filp) {
    printk(KERN_ALERT "Infrared Thermopile driver closed!!\n");
    iounmap((void *)gpio_base);
    iounmap((void *)bsc1_base);
    return 0;
}

ssize_t infrared_thermopile_read(struct file *filp, char *buf, size_t size, loff_t *f_pos) {
    printk(KERN_ALERT "Infrared Thermopile read function called!!\n");

    int remaining = 2;
    int i = 0;
    int reason = 0;
    char data[2] = {0,};

    /* Clear FIFO */
    *control |= BSC_C_CLEAR_1;
    /* Clear Status */
    *status = BSC_S_CLKT | BSC_S_ERR | BSC_S_DONE;
    /* Set Data Length */
    *dlen = 2;
    /* Start read */
    *control = BSC_C_I2CEN | BSC_C_ST | BSC_C_READ;

    /* Wait for transfer to complete */
    while(!(*status & BSC_S_DONE)) {
        /* we must empty the FIFO as it is populated and not use any delay */
        while(*status & BSC_S_RXD) {
            /* Read from FIFO */
            data[i] = *fifo;
            i++;
            remaining--;
        }
    }

    /* transfer has finished - grab any remainig stuff in FIFO */
    while(remaining && (*status & BSC_S_RXD)) {
        data[i] = *fifo;
        i++;
        remaining--;
    }
    
    *control |= BSC_S_DONE;
    
    copy_to_user(buf, &data, sizeof(char) * 2);

    return size;
}

static struct file_operations infrared_thermopile_fops = {
    .owner = THIS_MODULE,
    .read = infrared_thermopile_read,
    .open = infrared_thermopile_open,
    .release = infrared_thermopile_release
};

int __init infrared_thermopile_init(void) {
    if(register_chrdev(INFRARED_THERMOPILE_MAJOR_NUMBER, INFRARED_THERMOPILE_NAME, &infrared_thermopile_fops) < 0)
        printk(KERN_ALERT "Infrared Thermopile driver initialization fail\n");
    else
        printk(KERN_ALERT "Infrared Thermopile driver initialization success\n");

    return 0;
}

void __exit infrared_thermopile_exit(void) {
    unregister_chrdev(INFRARED_THERMOPILE_MAJOR_NUMBER, INFRARED_THERMOPILE_NAME);
    printk(KERN_ALERT "Infrared Thermopile driver exit done\n");
}

module_init(infrared_thermopile_init);
module_exit(infrared_thermopile_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Younho Choo");
MODULE_DESCRIPTION("des");
