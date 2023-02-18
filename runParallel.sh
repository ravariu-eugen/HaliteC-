#!/bin/bash

make MyBot


ATTEMPS=$2
Counter=50
mkdir temp
cp ./MyBot.cpp ./temp/
rm lg.txt 
touch lg.txt
for ((c=1; c<=ATTEMPS; c++))
do
    ./runOne.sh $1 &
    
    while [ $c -gt $(( $Counter + $(wc -l lg.txt | awk '{print $1}') )) ]
    do 
        echo "Running"
    done 
    echo $c
done


while [ $(wc -l lg.txt | awk '{print $1}') -lt $ATTEMPS ]
do
    echo "Running"
done

x=$(wc -l lg.txt | awk '{print $1}')
y=$(grep "1" lg.txt | wc -l | awk '{print $1}')
rm *.hlt
rm lg.txt
echo "$y / $x"

newFile="$y-$x##$(date +%m-%d-%Y)##$(date +%s)"

mv ./temp ./$newFile
cp ./MyBot.cpp ./$newFile/