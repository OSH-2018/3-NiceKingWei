#!/bin/sh
cd ../
cd cmake-build-debug/
make -j

echo "test1"                    #
rm -rf mnt*
mkdir mnt1 mnt2
./memfs mnt2
../test/test1.sh mnt1           #
../test/test1.sh mnt2           #
cp mnt1/result.txt out1.std     #
cp mnt2/result.txt out1.my      #
diff out1.std out1.my           #
sudo umount mnt2

echo "test2"
rm -rf mnt*
mkdir mnt1 mnt2
./memfs mnt2
../test/test2.sh mnt1
../test/test2.sh mnt2
cp mnt1/result.txt out2.std
cp mnt2/result.txt out2.my
diff out2.std out2.my
sudo umount mnt2
