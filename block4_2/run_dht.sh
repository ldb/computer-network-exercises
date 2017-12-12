#!/bin/bash

echo "Running 5 servers for 60 seconds"

# First Peer
./rpc_server 1 localhost 8080 &
sleep 1
./rpc_server 26 localhost 8081 1 localhost 8080 &
sleep 2
./rpc_server 76 localhost 8082 1 localhost 8080 &
sleep 2
./rpc_server 53 localhost 8083 26 localhost 8081 &
sleep 2
./rpc_server 32 localhost 8084 53 localhost 8083 &

sleep 60

pkill -P $$
