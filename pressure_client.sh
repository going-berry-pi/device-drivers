cd pressure
sudo rmmod pressure_dev
make clean
make
sudo insmod pressure_dev.ko
rm pressure_app
gcc -o pressure_app pressure_app.c
cd ..
rm pressure_client
gcc -o pressure_client pressure_client.c -lpthread