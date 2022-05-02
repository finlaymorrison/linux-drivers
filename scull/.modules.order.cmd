cmd_/home/finlay/src/linux-drivers/scull/modules.order := {   echo /home/finlay/src/linux-drivers/scull/scull.ko; :; } | awk '!x[$$0]++' - > /home/finlay/src/linux-drivers/scull/modules.order
