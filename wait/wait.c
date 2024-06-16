#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/wait.h>

static int pchar_open(struct inode *, struct file *);
static int pchar_close(struct inode *, struct file *);
static ssize_t pchar_read(struct file *, char *, size_t, loff_t *);
static ssize_t pchar_write(struct file *, const char *, size_t, loff_t *);

#define MAX 32
static struct kfifo buf;
static dev_t devno;
static int major;
static struct class *pclass;
static struct cdev cdev;
static wait_queue_head_t wr_wq;
static wait_queue_head_t rd_wq;

static struct file_operations pchar_fops = {
    .owner = THIS_MODULE,
    .open = pchar_open,
    .release = pchar_close,
    .read = pchar_read,
    .write = pchar_write
};

static __init int init_mod(void)
{
    int ret, minor;
    struct device *pdevice;

    printk(KERN_INFO "%s init_mod() called\n",THIS_MODULE->name);
    // 1) fifo
    ret = kfifo_alloc(&buf,MAX,GFP_KERNEL);
    if(ret != 0)
    {
        printk(KERN_ERR "%s kfifo_alloc() failed\n",THIS_MODULE->name);
        goto kfifo_alloc_failed;
    }
    printk(KERN_INFO "%s kfifo_alloc() sucess\n",THIS_MODULE->name);

    //2) mjor minor no / device no
    //int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name);
    ret = alloc_chrdev_region(&devno, 0, 1,"pchar");
    if(ret != 0)
    {
        printk(KERN_ERR "%s allco_chrdev_region() failed\n",THIS_MODULE->name);
        goto allco_chrdev_region_failed;
    }
    major = MAJOR(devno);
    minor = MINOR(devno);
    printk(KERN_INFO "%s allco_chrdev_region() done : devno = %d/%d\n",THIS_MODULE->name,major,minor);
    ///proc/devices
    //3) device class
    //Device class is created under /sys/class 
    pclass = class_create(THIS_MODULE, "pchar_class");
    if(IS_ERR(pclass))
    {
        printk(KERN_ERR "%s class_create() failed\n",THIS_MODULE->name);
        ret = -1;
        goto class_create_failed;    
    }
    printk(KERN_INFO "%s class_create() completed\n",THIS_MODULE->name);

    //4) device create
    //Device file is create under /sys/devices/ virtual/class_name/devname and /dev/devname
    pdevice = device_create(pclass,NULL,devno,NULL,"pchar%d",0);
    if(IS_ERR(pdevice))
    {
        printk(KERN_ERR "%s device_create() failed\n",THIS_MODULE->name);
        ret = -1;
        goto device_create_failed;    
    }
    printk(KERN_INFO "%s device_create() completed\n",THIS_MODULE->name);

    //5) cdev add / add device
    cdev_init(&cdev,&pchar_fops);
    ret = cdev_add(&cdev,devno,1);
    if(ret != 0)
    {
        printk(KERN_ERR "%s cdev_add() failed\n",THIS_MODULE->name);
        goto cdev_add_failed;
    }
    printk(KERN_INFO "%s cdev_add() completed\n",THIS_MODULE->name);
    
    // write waiting queue init
    init_waitqueue_head(&wr_wq);
    printk(KERN_INFO "%s: init_waitqueue_head() write wait queue\n",THIS_MODULE->name);
    
    // read waiting queue init
    init_waitqueue_head(&rd_wq);
    printk(KERN_INFO "%s: init_waitqueue_head() read wait queue\n",THIS_MODULE->name);
    
    printk(KERN_INFO "%s init_mod() completed\n",THIS_MODULE->name);
    
    return 0;

cdev_add_failed:
    device_destroy(pclass,devno);
device_create_failed:    
    class_destroy(pclass);
class_create_failed:
    unregister_chrdev_region(devno,1);
allco_chrdev_region_failed:
    kfifo_free(&buf);
kfifo_alloc_failed:
    return ret;
}

static __exit void exit_mod(void)
{
    printk(KERN_INFO "%s exit_mod() called\n",THIS_MODULE->name);

    cdev_del(&cdev);
    printk(KERN_INFO "%s cdev_del() done\n",THIS_MODULE->name);

    device_destroy(pclass,devno);
    printk(KERN_INFO "%s device_destroy() done\n",THIS_MODULE->name);

    class_destroy(pclass);
    printk(KERN_INFO "%s class_destroy() done\n",THIS_MODULE->name);

    unregister_chrdev_region(devno,1);
    printk(KERN_INFO "%s unregister_chrdev_region() done\n",THIS_MODULE->name);

    kfifo_free(&buf);
    printk(KERN_INFO "%s kfifo_free() released\n",THIS_MODULE->name);

    printk(KERN_INFO "%s exit_mod() completed\n",THIS_MODULE->name);
}

static int pchar_open(struct inode *pinode, struct file *pfile)
{
    printk(KERN_INFO "%s pchar_open()\n",THIS_MODULE->name);

    return 0;
}

static int pchar_close(struct inode *pinode, struct file *pfile)
{
    printk(KERN_INFO "%s pchar_close()\n",THIS_MODULE->name);

    return 0;
}

static ssize_t pchar_read(struct file *pfile, char *ubuf, size_t size, loff_t *poffset)
{
    int ret ,nbytes;
    printk(KERN_INFO "%s pchar_read()\n",THIS_MODULE->name);

    // interruptible sleep
    ret = wait_event_interruptible(rd_wq, !kfifo_is_empty(&buf));
    if(ret != 0)
    {
        printk(KERN_INFO "%s : pchar_read() wake-up due to signal\n",THIS_MODULE->name);
        return -ERESTARTSYS;
    }


    ret =kfifo_to_user(&buf,ubuf,size,&nbytes);
    if(ret < 0)
    {
        printk(KERN_ERR "%s pchar_read() to user failed\n",THIS_MODULE->name);
        return ret;
    }
    printk(KERN_INFO "%s pchar_read() to user success %d data copied to user\n",THIS_MODULE->name,nbytes);

    if(nbytes > 0)
        wake_up_interruptible(&wr_wq);

    return nbytes;
}

static ssize_t pchar_write(struct file *pfile, const char *ubuf, size_t size, loff_t *poffset)
{
    int ret, nbytes;
    printk(KERN_INFO "%s pchar_write()\n",THIS_MODULE->name);
    // interruptible sleep
    ret = wait_event_interruptible(wr_wq, !kfifo_is_full(&buf));
    if(ret != 0)
    {
        printk(KERN_INFO "%s : pchar_write() wake-up due to signal\n",THIS_MODULE->name);
        return -ERESTARTSYS;
    }

    ret = kfifo_from_user(&buf,ubuf,size,&nbytes);
    if(ret<0)
    {
        printk(KERN_ERR "%s pchar_write() failed\n",THIS_MODULE->name);
        return ret;
    }
    printk(KERN_INFO "%s pchar_write() success %d data copy from user\n",THIS_MODULE->name,nbytes);
    
    if(nbytes > 0)
        wake_up_interruptible(&rd_wq);

    return nbytes;
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("akash");
MODULE_DESCRIPTION("ioctl demo");
