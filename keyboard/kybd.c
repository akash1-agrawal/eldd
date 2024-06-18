#include <linux/module.h>
#include <linux/timer.h>
#include <linux/io.h>

//#define KBD_DATA_REG           0x60    /* I/O port for keyboard data */
//#define KBD_CONTROL_REG        0x64 

struct timer_list mytimer;
int ticks;
int count = 0;




void mytimer_function(struct timer_list *ptimer)
{
	printk(KERN_INFO " %s : mytimer_function : count = %d\n", THIS_MODULE->name, count);
	count++;
	mod_timer(&mytimer, jiffies + ticks);
	outb(0xAE, 0x60);
}

static __init int desd_init(void)
{
	int sec = 10;
	ticks = sec * HZ;		//750
	outb(0xAD, 0x64);
	timer_setup(&mytimer, mytimer_function, 0);
	mytimer.expires = jiffies + ticks;
	add_timer(&mytimer);

	printk(KERN_INFO " %s : Timer initialisation is done successfully\n", THIS_MODULE->name);
	
	return 0;
}

static __exit void desd_exit(void)
{
	del_timer_sync(&mytimer);
	printk(KERN_INFO " %s : Timer deinitialisation is done successfully\n", THIS_MODULE->name);
}
module_init(desd_init);
module_exit(desd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DESD @ Sunbeam");
MODULE_DESCRIPTION("This is demo of kernel timers");