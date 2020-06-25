cd led
sudo rmmod led_dev
make clean
make
sudo insmod led_dev.ko
rm led_app
gcc -o led_app led_app.c

cd ..
cd ultrasonic
sudo rmmod ultrasonic_dev
make clean
make
sudo insmod ultrasonic_dev.ko
rm ultrasonic_app
gcc -o ultrasonic_app ultrasonic_app.c

cd ..
cd infrared-thermopile
sudo rmmod infrared_thermopile_dev
make clean
make
sudo insmod infrared_thermopile_dev.ko
rm infrared_thermopile_app
gcc -o infrared_thermopile_app infrared_thermopile_app.c

cd ..
rm temperature_app
gcc -o temperature_app temperature_app.c -lpthread
