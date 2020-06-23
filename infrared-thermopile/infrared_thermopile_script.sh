sudo rmmod infrared_thermopile_dev
make clean
make
sudo insmod infrared_thermopile_dev.ko
rm infrared_thermopile_app
gcc -o infrared_thermopile_app infrared_thermopile_app.c
