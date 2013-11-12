/*
 * "blackbox" kernel module.
 *
 * monitors load1 avg, when it goes over a user-defined threshold it
 * starts to log useful information
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/cpumask.h>



static int blackbox_enable_sysctl(struct ctl_table *,
                                  int,
                                  void __user *,
                                  size_t *,
                                  loff_t *);

static unsigned long get_load_avg(void);

static struct timer_list stats_timer;

static int load_trigger;

static int blackbox_enable;
static int blackbox_enable_sc;

static ctl_table blackbox_table[] = {
        {
                .procname     = "enable",
                .maxlen       = sizeof(int),
                .mode         = 0644,
                .proc_handler = blackbox_enable_sysctl,
                .data         = &blackbox_enable_sc,
        },
        {
                .procname     = "load_trigger",
                .maxlen       = sizeof(int),
                .mode         = 0644,
                .proc_handler = proc_dointvec,
                .data         = &load_trigger,

        },
        { }
};

static ctl_table blackbox_blackbox_table[] = {
        {
                .procname     = "blackbox",
                .maxlen       = 0,
                .mode         = 555,
                .child        = blackbox_table,
        },
        { }
};

static ctl_table blackbox_root_table[] = {
        {
                .procname     = "debug",
                .maxlen       = 0,
                .mode         = 0555,
                .child        = blackbox_blackbox_table,
        },
        { }
};


static struct ctl_table_header *blackbox_sysctl_header;

DEFINE_MUTEX(blackbox_enable_mutex);



static int blackbox_enable_sysctl(struct ctl_table *table,
                                  int write,
                                  void __user *buffer,
                                  size_t *lenp,
                                  loff_t *ppos) {

        int ret;

        ret = proc_dointvec(table, write, buffer, lenp, ppos);
        blackbox_enable_sc = !!blackbox_enable_sc;


        /* not entirely sure if the mutex is 100% required here -
           wanted to avoid the (unlikely?) possibility of having
           add_timer() called twice.
        */

        if (write) { 
                mutex_lock(&blackbox_enable_mutex);
                
                /*
                 * if currently disabled, start the timer up.  if it's
                 * currently enabled, the timer will catch the flag
                 * and not restart when the timer expires. this has
                 * the effect of doing one more pass after your
                 * disable it.
                 */
                if (!blackbox_enable && blackbox_enable_sc) {
                        printk(KERN_INFO "blackbox: enabling\n");
                        stats_timer.expires = jiffies + HZ;
                        add_timer(&stats_timer);
                }

                blackbox_enable = blackbox_enable_sc;
                mutex_unlock(&blackbox_enable_mutex);
        }

        return ret;
}


static void print_stats(unsigned long x)
{
        unsigned long l = get_load_avg();

        if (l >= load_trigger) {
                printk(KERN_ALERT "blackbox: load avg is %lx\n", l);
        }

        if (likely(blackbox_enable)) {
                stats_timer.expires = jiffies + HZ;
                add_timer(&stats_timer);
        }
}

static int blackbox_init(void) 
{
        printk(KERN_INFO "blackbox: module inserted.\n");
        init_timer(&stats_timer);
        stats_timer.function = print_stats;

        blackbox_sysctl_header = register_sysctl_table(blackbox_root_table);

        /* default load trigger is #cpus */
        load_trigger = num_active_cpus();

        return 0;
}

static unsigned long get_load_avg(void) 
{
        return (avenrun[0] >> FSHIFT);
}

static void blackbox_exit(void)
{
        del_timer_sync(&stats_timer);

        if (blackbox_sysctl_header) 
                unregister_sysctl_table(blackbox_sysctl_header);

        printk(KERN_ALERT "blackbox: module removed.\n");
}

module_init(blackbox_init);
module_exit(blackbox_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jon Philpott");
MODULE_DESCRIPTION("Provides additional logging when loadavg goes over a specific threshold.");


