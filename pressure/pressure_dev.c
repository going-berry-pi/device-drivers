#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define PRESSURE_MAJOR_NUMBER    506
#define PRESSURE_DEV_NAME        "pressure"

#define GPIO_BASE_ADDR    0x3f200000
#define SPI_BASE_ADDR     0x3f204000
#define GPFSEL0           0x00
#define GPFSEL1           0x04
#define GPSET0            0x1C
#define GPCLR0            0x28
#define GPLEV0            0x34 

static char *buffer = NULL;
int result=0;
static void __iomem *gpio_base;
static void __iomem *spi_base;
volatile unsigned int *gpfsel0;
volatile unsigned int *gpfsel1;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;

int pressure_open(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "PRESSURE_device open function called\n");
	gpio_base = ioremap(GPIO_BASE_ADDR,0x60);
	spi_base = ioremap(SPI_BASE_ADDR,0x18);
	gpfsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
	gpfsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
	gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
	gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
	
	*gpfsel0 = (1<<26);//gpio8->100
	*gpfsel0 |= (1<<29);//gpio9->100
	*gpfsel1 = (1<<2);//gpio10->100
	*gpfsel1 |= (1<<5);//gpio11->100
	
	return 0;
}

int pressure_release(struct inode *inode, struct file *filp){
	printk(KERN_ALERT "PRESSURE_device release function called\n");
	iounmap((void *)gpio_base);
	return 0;	
}

ssize_t pressure_read(struct file *filp, char *buf, size_t count, loff_t *f_pos){
		char tbuf[3];
		char rbuf[3];
		int adc_value;
		
		/*tbuf to send register number*/
 		tbuf[0] = 1;
		tbuf[1] = (8 + 0) << 4;
		tbuf[2] = 0;

		volatile uint32_t* paddr = spi_base + 0 ;
		volatile uint32_t* fifo = spi_base + 4 ;
		uint32_t TXCnt = 0;
		uint32_t RXCnt = 0;
		
		*paddr = *paddr| 0x00000030; //clear
		*paddr = *paddr| 0x00000080 ;/*!< Transfer Active */
		
		while ((TXCnt < 3) || (RXCnt < 3))
		{
		  /* TX fifo not full, so add some more bytes */
		  while ((*paddr & 0x00040000) && (TXCnt < 3))
		  {
			  *fifo = tbuf[TXCnt];
			  TXCnt++;
		  }
		  /* Rx fifo not empty, so get the next received bytes */
		  while (((*paddr & 0x00020000)) && (RXCnt < 3))
		  {
			  rbuf[RXCnt] = *fifo;
			  RXCnt++;
		  }
		}
		/* Wait for DONE to be set */
		while (!(*paddr & 0x00010000));
		
		uint32_t v = *paddr;
		v = (v&~0x00000080)|(0&0x00000080);
		*paddr = v;   /* Set TA = 0, and also set the barrier */

		copy_to_user(buf, &rbuf[2], sizeof(char));
	
	return count;
	}
	
static struct file_operations sys_fops = {
	.owner = THIS_MODULE,
	.read = pressure_read,
	.open = pressure_open,
	.release = pressure_release
	};

int __init pressure_init(void){
	if(register_chrdev(PRESSURE_MAJOR_NUMBER,PRESSURE_DEV_NAME,&sys_fops)<0)
		printk(KERN_ALERT "[pressure] driver init failed\n");
	else
		printk(KERN_ALERT "[pressure] driver init successful\n");
	buffer = (char*)kmalloc(1024,GFP_KERNEL);
	if(buffer !=NULL)
		memset(buffer, 0, 1024);
	return 0;
	}
	
void __exit pressure_exit(void){
	unregister_chrdev(PRESSURE_MAJOR_NUMBER,PRESSURE_DEV_NAME);
	printk(KERN_ALERT "[pressure] driver cleanup\n");
	kfree(buffer);
	
	}
	
module_init(pressure_init);
module_exit(pressure_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JUHYUN");
MODULE_DESCRIPTION("This is the hellow world example for device driver in systemp programming lecture");
