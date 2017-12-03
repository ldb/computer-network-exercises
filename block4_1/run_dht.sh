#!/bin/bash

echo "Running servers for 5 seconds"

./server 8080 1 8083 76 8081 26 localhost & 
./server 8081 26 8080 1 8082 51 localhost & 
./server 8082 51 8081 26 8083 76 localhost & 
./server 8083 76 8082 51 8080 1 localhost &

sleep 15

pkill -P $$