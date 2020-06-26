cd rfid
sudo rmmod rfid_dev
make clean
make
sudo insmod rfid_dev.ko
cd ..
rm rfid_server
gcc -o rfid_server rfid_server.c