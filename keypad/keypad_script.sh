sudo rmmod keypad_dev
make clean
make
sudo insmod keypad_dev.ko
rm keypad_app
gcc -o keypad_app keypad_app.c
