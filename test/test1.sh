#!/bin/sh
cd $1
echo "hello fs" > 1.txt
echo "hello" >> 1.txt
echo "hello fs2" > 2.txt
ls >> result.txt
echo "safdssdg" > 3.txt
rm 3.txt
cat 1.txt 2.txt >> result.txt