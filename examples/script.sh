#!/bin/bash
for file in `ls *CACHE*`; do
    newfile=`echo $file | sed -e 's/CACHE/NOCACHE/g'`
    cp $file $newfile
    sed -e 's/NOPINT-CACHE/NOPINT-NOCACHE/g' < $newfile > tmp
    cp tmp $newfile
    sed -e 's/AccountingConsumer/AccountingRandomConsumer/g' < $newfile > tmp
    cp tmp $newfile
    rm tmp
done
