sudo rmmod ultrasonic_dev
make clean
make
sudo insmod ultrasonic_dev.ko
rm ultrasonic_app
gcc -o ultrasonic_app ultrasonic_app.c
