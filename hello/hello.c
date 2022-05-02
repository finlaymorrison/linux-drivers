#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>

int num = 1;
module_param(num, int, S_IRUGO);
static char *name = "world";
module_param(name, charp, S_IRUGO);

static int __init hello_init(void)
{
    int i;
    for (i = 0; i < num; ++i)
    {
        printk(KERN_ALERT "Hello, %s!\n", name);
    }
    return 0;
}

static void __exit hello_exit(void)
{
    printk("Goodbye, cruel world\n");
}

MODULE_LICENSE("Dual BSD/GPL");

module_init(hello_init);
module_exit(hello_exit);