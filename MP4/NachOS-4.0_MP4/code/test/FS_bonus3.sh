../build.linux/nachos -f
../build.linux/nachos -mkdir /t0
../build.linux/nachos -mkdir /t1
../build.linux/nachos -mkdir /t2
../build.linux/nachos -cp num_100.txt /t0/f1
../build.linux/nachos -mkdir /t0/aa
../build.linux/nachos -mkdir /t0/bb
../build.linux/nachos -mkdir /t0/cc
../build.linux/nachos -cp num_100.txt /t0/bb/f1
../build.linux/nachos -cp num_100.txt /t0/bb/f2
../build.linux/nachos -cp num_100.txt /t0/bb/f3
../build.linux/nachos -cp num_100.txt /t0/bb/f4
echo "===============do -lr===================="
../build.linux/nachos -lr /
echo "===============do -l====================="
../build.linux/nachos -l /t0
echo "===============do -r remove f3==lr========"
../build.linux/nachos -r /t0/bb/f3
../build.linux/nachos -lr /
echo "===============do -rr remove dir bb==lr==="
../build.linux/nachos -rr /t0/bb
../build.linux/nachos -lr /