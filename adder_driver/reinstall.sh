make clean
make
sudo rmmod ./adder_driver.ko
sudo rm /dev/adder
sudo insmod ./adder_driver.ko
sudo mknod /dev/adder c 246 0
