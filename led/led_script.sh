sudo rmmod led_dev
make clean
make
sudo insmod led_dev.ko
rm led_app
gcc -o led_app led_app.c