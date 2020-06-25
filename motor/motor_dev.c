#include <linux/init.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/uaccess.h> 
#include <linux/slab.h>

#include <asm/mach/map.h> 
#include <asm/uaccess.h> 

#include <linux/delay.h>
#include <linux/timer.h>

#define MOTOR_MAJOR_NUMBER   507
#define MOTOR_DEV_NAME      "motor_dev" 

#define GPIO_BASE_ADDRESS   0x3F200000

#define GPFSEL1 0x04
#define GPSET0	0x1C
#define GPCLR0	0x28

#define RANGE 20

#define MOTOR_RIGHT 0
#define MOTOR_RIGHT_TIME 3
#define MOTOR_LEFT 1
#define MOTOR_CLOSE_TER -1

#define MOTOR 18
#define RIGHT 0
#define LEFT 1

#define DEGREE90 20 //how much up down

static void __iomem * gpio_base;
volatile unsigned int * gpfsel1;
volatile unsigned int * gpset0;
volatile unsigned int * gpclr0;

void setMotor(int cmd, int motor_dev)
{
  // PWM : Pulse Width Modulation : 펄스 폭을 조절
  //일정한 주기내에서 Duty비를 변화 시켜서 평균 전압을 제어 
  // mdelay(n) : 'n' ms(밀리초) 단위의 지연
  // udelay(n) : 'n' us(마이크로초) 단위의 지연
  // ndelay(n) : 'n' ns(나노초) 단위의 지연

  //모터는 20ms이 한텀 

    int i;
    
      if(motor_dev > 1)
      {
      //폭이 1.5ms :0도
      // 2ms 일때 90도
      // 1ms 일때 -90도
      if(cmd == 0 || cmd == 3)  //위로, OPEN
      {        
        printk( "PIN%d RIGHT\n", motor_dev);
         for(i = 0; i < DEGREE90 ; i++)
         {
          *gpset0 |= (1 << motor_dev);
          // mdelay(0.8);//right 90
          mdelay(0.99);//right 90

          *gpclr0 = (1 << motor_dev);
          mdelay(RANGE);
        }
      }

      else if(cmd == 1 || cmd == -1) //아래로, CLOSE
      {
        printk( "PIN%d LEFT\n", motor_dev);
        for(i=0; i < DEGREE90; i++)
        {
          *gpset0 |= (1 << motor_dev);
          //udelay(0.01);
          // mdelay(1.8);//90 left
          mdelay(1.99);//90 left
          *gpclr0 = (1 << motor_dev);
          mdelay(RANGE);
          }
       }
        
      //   else if(cmd == -1) //문닫고 종료
      //     {
      //   printk( "PIN%d LEFT\n", motor_dev);
      //   for(i=0; i < DEGREE90; i++)
      //   {
      //     *gpset0 |= (1 << motor_dev);
      //     //udelay(0.01);
      //     mdelay(1.95);//90 left
      //     *gpclr0 = (1 << motor_dev);
      //     mdelay(RANGE-1);
      //     }
      //  }

    }
  //   else
  //   {
  //       if(cmd == 0)
  //     {        
  //       printk("cmd==0??\n");
  //       printk( "PIN%d RIGHT\n", motor_dev);
  //       for(i = 0; i < DEGREE90 + 8; i++)
  //      {
  //        *gpset0 |= (1 << motor_dev);
  //         //usleep_range(1.5, 1.5);
  //         ndelay(100);
  //         *gpclr0 = (1 << motor_dev);
  //         mdelay(PWM_RANGE-1);
  //      }
  //     }
      
  //     else if(cmd == 1)
  //    {
  //       printk( "PIN%d LEFT\n", motor_dev);
  //       for(i=0; i < DEGREE90 + 6; i++)
  //       {
  //         *gpset0 |= (1 << motor_dev);
  //         //usleep_range(1.5, 1.5);
  //         ndelay(100);
  //         *gpclr0 = (1 << motor_dev);
  //         mdelay(PWM_RANGE-1);
  //       }
  //      }
  //  }
      
}

int motor_open(struct inode * inode, struct file * filp)
{
    printk(KERN_ALERT "MOTOR driver open\n"); 
    gpio_base = ioremap(GPIO_BASE_ADDRESS, 0x60);
    
    
    gpfsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
    gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
    gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
    
    *gpfsel1 |= (1 << 24);
    *gpfsel1 |= (1 << 27);
    udelay(500);
    return 0;
    

}

int motor_release(struct inode * inode, struct file * filp) { 
    printk(KERN_ALERT "MOTOR driver close\n");
    iounmap((void *)gpio_base); 
    return 0; 
}

long motor_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{ 
    switch (cmd){ 
        case MOTOR_RIGHT: 
        printk(KERN_ALERT "CMD : %d, MOTOR OPEN(RIGHT)\n", cmd);
        setMotor(RIGHT, MOTOR);
        break; 
    
        case MOTOR_LEFT: 
        printk(KERN_ALERT "CMD : %d, MOTOR CLOSE(LEFT)\n", cmd);
        setMotor(LEFT, MOTOR);
        break;

        case MOTOR_RIGHT_TIME: 
        printk(KERN_ALERT "CMD : %d, MOTOR RIGHT DURING 10 SEC\n", cmd);
        setMotor(RIGHT, MOTOR);
        break;

        case MOTOR_CLOSE_TER: 
        printk(KERN_ALERT "CMD : %d, MOTOR CLOSE AND TERMINATE\n", cmd);
        setMotor(LEFT, MOTOR);
        break;
	
	default :
        printk(KERN_ALERT "ioctl : command error\n");
    }
    
    return 0; 
}

static struct file_operations motor_fops = { 
    .owner = THIS_MODULE, 
    .open = motor_open, 
    .release = motor_release,
    .unlocked_ioctl = motor_ioctl,
};

int __init motor_init (void) { 
    if(register_chrdev(MOTOR_MAJOR_NUMBER, MOTOR_DEV_NAME, &motor_fops) < 0)
        printk(KERN_ALERT "MOTOR driver initalization failed\n"); 
    else 
        printk(KERN_ALERT "MOTOR driver initalization succeed\n");
    
    return 0; 
}

void __exit motor_exit(void){ 
    unregister_chrdev(MOTOR_MAJOR_NUMBER, MOTOR_DEV_NAME); 
    printk(KERN_ALERT "MOTOR driver exit"); 
}

module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Park Hyosung");
MODULE_DESCRIPTION("SERVO MOTOR DEVICE DRIVER");
