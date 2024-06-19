#include <linux/module.h>
#include <linux/timer.h>
#include <linux/io.h>

struct timer_list mytimer;
int ticks;
//int count = 0;

void mytimer_function(struct timer_list *ptimer)
{	
	//count++;
	//mod_timer(&mytimer, jiffies + ticks);
	outb(0xAE, 0x64);
	printk(KERN_INFO "%s kybd enabled\n",THIS_MODULE->name);
}

static __init int desd_init(void)
{
	int sec = 10;
	ticks = sec * HZ;		//750
	printk(KERN_INFO "%s kybd disable for %d second\n",THIS_MODULE->name,sec);
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