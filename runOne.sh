#!/bin/bash

touch lg.txt
s=$(./halite -d "$1 $1" "./MyBot" "./starkbot_linux_x64" -q | tail --lines=+5 | head -n 1) 
x=$(echo "$s" | awk '{print $2}')

echo $x >> lg.txt