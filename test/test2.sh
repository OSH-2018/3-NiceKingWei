#!/bin/sh
cd $1
mkdir dir1
cd dir1/
mkdir dir11
ls >> ../result.txt
echo "hello" > x
cd ../
cat dir1/x >> result.txt
rm -rf dir1
ls >> result.txt