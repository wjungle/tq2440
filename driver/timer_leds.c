#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <plat/regs-timer.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <linux/interrupt.h>
#include <mach/regs-irq.h>

#define DEVICE_NAME "ex-leds"
#define LED_MAJOR 232

#define IOCTL_LED_ON 1
#define IOCTL_LED_OFF 0

unsigned long gpbcon;
unsigned long gpbdat;

unsigned long tcfg0;
unsigned long tcfg1;
unsigned long tcon;
unsigned long tcntb0;
unsigned long tcmpb0;
unsigned long tcnto0;

unsigned long srcpnd;
unsigned long intmask;
unsigned long intpnd;
unsigned long intoffset;
unsigned long pclk;

static irqreturn_t timer2_interrupt(int irq, void *dev_id, struct pt_regs *regs);

#if 0
static unsigned long led_table[]=
{
	S3C2410_GPB5,
	S3C2410_GPB6,
	S3C2410_GPB7,
	S3C2410_GPB8,
};

static unsigned int led_cfg_table[]=
{
	S3C2410_GPB5_OUTP,
	S3C2410_GPB6_OUTP,
	S3C2410_GPB7_OUTP,
	S3C2410_GPB8_OUTP,
};
#endif

static void timer2_init(void)
{
	//s3c2410_gpio_cfgpin(S3C2410_GPB1, S3C2410_GPB1_TOUT1);

	/* stop the Timer2 
	tcon = __raw_readl(S3C2410_TCON);
	tcon &= ~(15<<8);
	__raw_writel(tcon, S3C2410_TCON);
	*/
	
	tcfg0 = __raw_readl(S3C2410_TCFG0);
	tcfg0 &= ~S3C2410_TCFG_PRESCALER1_MASK;
	tcfg0 |= 255<<8;
	__raw_writel(tcfg0, S3C2410_TCFG0);

	tcfg1 = __raw_readl(S3C2410_TCFG1);
	tcfg1 &= ~S3C2410_TCFG1_MUX1_MASK;
	tcfg1 |= S3C2410_TCFG1_MUX1_DIV16;
	__raw_writel(tcfg1, S3C2410_TCFG1);

	/* prescaler = 255; divider = 1/16; so 50MHz/(255+1)/16 = 12.2070kHz(81.9188 us) */ 
	/* 4ms = 4000us, 4000/81.9188 = 48.82 */
	/* 49 * 81.9188 ~= 4014 */

	//pclk = clk_get_rate(clk_get(NULL, "pclk"));
	tcntb0 = 50000000/(256 * 16);		//12200*82~=1,000,400us
	__raw_writel(tcntb0, S3C2410_TCNTB(2));

	//tcmpb0 = 49>>1;
	//__raw_writel(tcmpb0, S3C2410_TCMPB(2));

	/* start the Timer2 */
	tcon = __raw_readl(S3C2410_TCON);
	tcon &= ~(15<<12);
	tcon |= (0xb<<12); 
	//S3C2410_TCON_T2RELOAD | S3C2410_TCON_T2INVERT | S3C2410_TCON_T2MANUALUPD | S3C2410_TCON_T2START;
	__raw_writel(tcon, S3C2410_TCON);	
	tcon &= ~(2<<12);
	__raw_writel(tcon, S3C2410_TCON);	

	
	printk(DEVICE_NAME" timer2 initialized\n");
}

static int led_open(struct inode *inode, struct file *file)
{
	int err;
	s3c2410_gpio_cfgpin(S3C2410_GPB5, S3C2410_GPB5_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB6, S3C2410_GPB6_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB7, S3C2410_GPB7_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPB8, S3C2410_GPB8_OUTP);
	s3c2410_gpio_setpin(S3C2410_GPB5, 0);
	s3c2410_gpio_setpin(S3C2410_GPB6, 1);
	s3c2410_gpio_setpin(S3C2410_GPB7, 0);
	s3c2410_gpio_setpin(S3C2410_GPB8, 1);

	err = request_irq(IRQ_TIMER2, timer2_interrupt, IRQF_DISABLED, "TIMER2", NULL);
	if(err)
	{
		disable_irq(IRQ_TIMER2);
		free_irq(IRQ_TIMER2, NULL);
		return -EBUSY;
	}
	else
		printk("drive open ok\n");	
	
	/* enable timer interrupt*/
	intmask = __raw_readl(S3C2410_INTMSK);
	intmask &= ~(1<<12);
	__raw_writel(intmask, S3C2410_INTMSK);
	
/*
	srcpnd = __raw_readl(S3C2410_SRCPND);
	srcpnd |= (1<<12);
	__raw_writel(srcpnd, S3C2410_SRCPND);

	intpnd = __raw_readl(S3C2410_INTPND);
	intpnd |= (1<<12);
	__raw_writel(intpnd, S3C2410_INTPND);
*/	
	return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
	free_irq(IRQ_TIMER2, NULL);
	printk("drive close ok\n");
	return 0;
}

unsigned int toggle_data = 0;

static irqreturn_t timer2_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	srcpnd = __raw_readl(S3C2410_SRCPND);
	srcpnd &= ~(1<<12);
	__raw_writel(srcpnd, S3C2410_SRCPND);

	intpnd = __raw_readl(S3C2410_INTPND);
	intpnd &= ~(1<<12);
	__raw_writel(intpnd, S3C2410_INTPND);
	
	toggle_data ^= 1;
	s3c2410_gpio_setpin(S3C2410_GPB5, toggle_data);
	printk("toggle=%d\n", toggle_data);
	
	return IRQ_RETVAL(IRQ_HANDLED);
}

static int led_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
#if 0
	if(arg > 4){
		return -EINVAL;	//Invalid argument
	}
	switch(cmd)
	{
		case IOCTL_LED_ON:
#if 0	//for linux-2.6.32
			s3c2410_gpio_setpin(S3C2410_GPB(arg+5), 0);
#else
			s3c2410_gpio_setpin(led_table[arg], 0);
#endif		
			return 0;

		case IOCTL_LED_OFF:
#if 0
			s3c2410_gpio_setpin(S3C2410_GPB(arg+5), 1);
#else	
			s3c2410_gpio_setpin(led_table[arg], 1);
#endif
			return 0;

		default:
			return -EINVAL;
	}
#endif
	return 0;
}



static struct file_operations led_fops = 
{
	.owner = THIS_MODULE,
	.open = led_open,
	.ioctl = led_ioctl,
	.release = led_release,
};

//static char __initdata banner[] = "***TQ2440 LEDS*** (c)2014\n";
static struct cdev leds_cdev;

static int __init led_init(void)
{
	int ret;

	//向系統申請裝置編號; 此用於已知起始裝置的裝置編號，而alloc_chrdev_region()用於裝置編號未知
	//使用MKDEV巨集可透過master, minor生成dev_t
	ret = register_chrdev_region(MKDEV(LED_MAJOR, 0), 1, DEVICE_NAME);
	if(ret < 0){
		printk(DEVICE_NAME" can't register major number\n");
		return ret;
	}
	cdev_init(&leds_cdev, &led_fops);	//用於初始化cdev的成員，並建立cdev與file_operations的連接
	leds_cdev.owner = THIS_MODULE;
	//向系統添加一個cdev，來完成字元裝置的註冊
	cdev_add(&leds_cdev, MKDEV(LED_MAJOR, 0), 1);

	printk(DEVICE_NAME" initialized\n");

	timer2_init();

	return 0;
}

static void __exit led_exit(void)
{
	cdev_del(&leds_cdev);
	unregister_chrdev_region(MKDEV(LED_MAJOR, 0), 1);
}

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("www.embedsky.net");
MODULE_DESCRIPTION("TQ2440 LED Driver");
MODULE_LICENSE("GPL");
