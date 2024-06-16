#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include "ioctl.h"

static int pchar_open(struct inode *, struct file *);
static int pchar_close(struct inode *, struct file *);
static ssize_t pchar_read(struct file *, char *, size_t, loff_t *);
static ssize_t pchar_write(struct file *, const char *, size_t, loff_t *);
static long pchar_ioctl(struct file *, unsigned int, unsigned long);

#define MAX 32
static struct kfifo buf;
static dev_t devno;
static int major;
static struct class *pclass;
static struct cdev cdev;
static struct file_operations pchar_fops = {
    .owner = THIS_MODULE,
    .open = pchar_open,
    .release = pchar_close,
    .read = pchar_read,
    .write = pchar_write,
    .unlocked_ioctl = pchar_ioctl
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

    ret =kfifo_to_user(&buf,ubuf,size,&nbytes);
    if(ret < 0)
    {
        printk(KERN_ERR "%s pchar_read() to user failed\n",THIS_MODULE->name);
        return ret;
    }
    printk(KERN_INFO "%s pchar_read() to user success %d data copied to user\n",THIS_MODULE->name,nbytes);

    return nbytes;
}

static ssize_t pchar_write(struct file *pfile, const char *ubuf, size_t size, loff_t *poffset)
{
    int ret, nbytes;
    printk(KERN_INFO "%s pchar_write()\n",THIS_MODULE->name);

    ret = kfifo_from_user(&buf,ubuf,size,&nbytes);
    if(ret<0)
    {
        printk(KERN_ERR "%s pchar_write() failed\n",THIS_MODULE->name);
        return ret;
    }
    printk(KERN_INFO "%s pchar_write() success %d data copy from user\n",THIS_MODULE->name,nbytes);

    return nbytes;
}

static long pchar_ioctl(struct file *pfile, unsigned int cmd, unsigned long param)
{
    info_t info;
    
    switch(cmd)
    {
        case FIFO_CLEAR:
            printk(KERN_INFO "%s: ioctl() fifo clear\n",THIS_MODULE->name);
            kfifo_reset(&buf);
            break;

        case FIFO_INFO:
            printk(KERN_INFO "%s: ioctl() fifo info\n",THIS_MODULE->name);
            info.size = kfifo_size(&buf);
            info.avail = kfifo_avail(&buf);
            info.len = kfifo_len(&buf);
            copy_to_user((void*)param, &info,sizeof(info_t));
            break;

        case FIFO_RESIZE:
            static char *temp;
            int ret;
            int len = kfifo_len(&buf);

            temp = kmalloc(len,GFP_KERNEL);
            if(temp == NULL)
            {
                ret = -ENOMEM;
                printk(KERN_ERR  "%s: kmalloc failed()\n",THIS_MODULE->name);
            }

            ret = kfifo_out(&buf,temp,len);
            if (ret != len) 
            {
                printk(KERN_ERR "%s: kfifo_out() failed\n", THIS_MODULE->name);
                kfree(temp);
                return -EIO; // Input/output error
            }
            kfifo_free(&buf);

            ret = kfifo_alloc(&buf,64,GFP_KERNEL);
            if(ret != 0) 
            {
                printk(KERN_ERR "%s: kfifo_alloc() failed\n", THIS_MODULE->name);
            }
            
            kfifo_in(&buf,temp,64);
            kfree(temp);
            printk(KERN_INFO "%s: ioctl() fifo resize\n",THIS_MODULE->name);
            
            break;

        default:
            printk(KERN_INFO "%s: ioctl() unsupported cmd\n",THIS_MODULE->name);
            return -EINVAL;
    }
    return 0;
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("akash");
MODULE_DESCRIPTION("ioctl demo");
