#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

int scull_major = 0; /* dynamic by default */
int scull_minor = 0;
int scull_nr_devs = 4;
int scull_quantum = 4000;
int scull_qset = 1000;


module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);

MODULE_AUTHOR("Finlay Morrison");
MODULE_LICENSE("Dual BSD/GPL");

struct scull_dev *scull_devices;

struct scull_dev
{
    struct scull_qset *data;
    int quantum;
    int qset;
    unsigned long size;
    unsigned int access_key;
    struct semaphore sem;
    struct cdev cdev;
    int minor_num;
};

struct scull_qset
{
    void **data;
    struct scull_qset *next;
};

int scull_trim(struct scull_dev *dev)
{
    struct scull_qset *next, *dptr;
    int qset = dev->qset;
    int i;

    for (dptr = dev->data; dptr; dptr = next)
    {
        if (dptr->data)
        {
            for (i = 0; i < qset; ++i)
            {
                kfree(dptr->data[i]);
            }
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }

    dev->size = 0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;
    return 0;
}

struct scull_qset* scull_follow(struct scull_dev *dev, int count)
{
    struct scull_qset* qptr = dev->data;
    if (!qptr)
    {
        qptr = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (qptr == NULL)
        {
            return NULL;
        }
        memset(qptr, 0, sizeof(struct scull_qset));
    }

    while (count--)
    {
        if (!qptr->next)
        {
            qptr->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
            if (!qptr->next)
            {
                return NULL;
            }
            memset(qptr->next, 0, sizeof(struct scull_qset));
        }
        qptr = qptr->next;
    }

    return qptr;
}

int scull_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev;

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);

    printk(KERN_WARNING "scull: open called on minor number %d\n", dev->minor_num);

    filp->private_data = dev;

    if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        scull_trim(dev);
    }
    return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev = filp->private_data;

    printk(KERN_WARNING "scull: release called on minor number %d\n", dev->minor_num);

    return 0;
}

ssize_t scull_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;
    struct scull_qset *dptr;
    int quantum = dev->quantum, qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    printk(KERN_WARNING "scull: read called for %ld bytes at position %lld on minor number %d\n", count, *f_pos, dev->minor_num);
    if (*f_pos >= dev->size)
    {
        goto out;
    }
    if (*f_pos + count > dev->size)
    {
        count = dev->size - *f_pos;
    }

    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    dptr = scull_follow(dev, item);

    if (!dptr || !dptr->data || !dptr->data[s_pos])
    {
        goto out;
    }

    if (count > quantum - q_pos)
    {
        count = quantum - q_pos;
    }

    if (copy_to_user(buff, dptr->data[s_pos] + q_pos, count))
    {
        printk(KERN_WARNING "scull: failed to copy to from user\n");
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

out:
    return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;
    struct scull_qset *dptr;
    int quantum = dev->quantum, qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = -ENOMEM;

    printk(KERN_WARNING "scull: write called for %ld bytes at position %lld on minor number %d\n", count, *f_pos, dev->minor_num);

    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    dptr = scull_follow(dev, item);

    if (dptr == NULL)
    {
        goto out;
    }
    
    if (!dptr->data)
    {
        dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
        if (!dptr->data)
        {
            goto out;
        }
        memset(dptr->data, 0, qset * sizeof(char*));
    }
    
    if (!dptr->data[s_pos])
    {
        dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!dptr->data[s_pos])
        {
            goto out;
        }
    }
    
    if (count > quantum - q_pos)
    {
        count = quantum - q_pos;
    }
    
    if (copy_from_user(dptr->data[s_pos] + q_pos, buff, count))
    {
        printk(KERN_WARNING "scull: failed to copy bytes from user\n");
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    if (dev->size < *f_pos)
    {
        dev->size = *f_pos;
    }

out:
    return retval;
}

struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .read = scull_read,
    .write = scull_write,
    .open = scull_open,
    .release = scull_release
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
    int err, devno = MKDEV(scull_major, scull_minor + index);

    cdev_init(&dev->cdev, &scull_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &scull_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_WARNING "Error %d adding scull%d\n", err, index);
    }
}

static void scull_exit(void)
{
    int i;
    dev_t dev = MKDEV(scull_major, scull_minor);

    printk(KERN_WARNING "scull: exit\n");

    if (scull_devices)
    {
        for (i = 0; i < scull_nr_devs; ++i)
        {
            scull_trim(scull_devices + i); // scull_trim expects pointer
            cdev_del(&scull_devices[i].cdev);
        }
        kfree(scull_devices);
    }

    unregister_chrdev_region(dev, scull_nr_devs);
}

static int __init scull_init(void)
{
    int result, i;
    dev_t dev = 0;

    printk(KERN_WARNING "scull: init");

    if (scull_major)
    {
        dev = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
        printk(KERN_WARNING "scull: allocated major device number %d\n", scull_major);
    }
    else
    {
        result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
        scull_major = MAJOR(dev);
        printk(KERN_WARNING "scull: allocated major device number %d\n", scull_major);
    }
    if (result < 0)
    {
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
        return result;
    }

    scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
    if (!scull_devices)
    {
        result = -ENOMEM;
        goto fail;
    }
    memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

    for (i = 0; i < scull_nr_devs; ++i)
    {
        scull_devices[i].quantum = scull_quantum;
        scull_devices[i].qset = scull_qset;
        scull_devices[i].minor_num = i + scull_minor;
        scull_setup_cdev(&scull_devices[i], i);
    }
    return 0;

fail:
    scull_exit();
    return result;
}

module_init(scull_init);
module_exit(scull_exit);