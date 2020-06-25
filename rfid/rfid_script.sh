sudo rmmod rfid_dev
make clean
make
sudo insmod rfid_dev.ko
rm rfid_app
gcc -o rfid_app rfid_app.c
