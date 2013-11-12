Blackbox Linux Kernel Module
============================

A simple kernel module to log certain kernel statistics at a 1 second
interval when load average goes over a trigger value (which defaults
to value of num_active_cpus().)

Configuration
-------------

Once inserted, the logging is enabled via proc/sysctl:

>
> echo 1 > /proc/sys/kernel/debug/blackbox/enable
>

or:

>
> sysctl -w debug.blackbox.enable=1
>

The load average trigger can be set with:

>
> echo $n > /proc/sys/kernel/debug/blackbox/load_trigger
>

CAVEAT
-------


This could be done in better, easier ways, such as with SystemTap,
etc. This is mostly my first foray into kernel development and also to
help debugging a specific performance issue I was dealing with where
SystemTap was a hard sell (long story.)



