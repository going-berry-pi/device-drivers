sudo rmmod motor_dev
make clean
make
sudo insmod motor_dev.ko
gcc -o motor_app motor_app.c
