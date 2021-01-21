make clean
make
../build.linux/nachos -f
../build.linux/nachos -cp FS_test1 /FS_test1
../build.linux/nachos -e /FS_test1  
../build.linux/nachos -cp num_100.txt /f1 
../build.linux/nachos -cp num_1000.txt /f2 
echo "========================================="
../build.linux/nachos -lr /
echo "========================================="
echo "========================================="
../build.linux/nachos -D /
../build.linux/nachos -lr /