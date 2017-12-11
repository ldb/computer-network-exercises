#!/bin/bash

echo "Running 5 servers for 30 seconds"

./rpc_server 1 localhost 8080 &

sleep 1

./rpc_server 26 localhost 8081 1 localhost 8080 &

sleep 3

./rpc_server 76 localhost 8082 1 localhost 8080 &

sleep 3

./rpc_server 53 localhost 8083 26 localhost 8081 &

sleep 3

./rpc_server 32 localhost 8084 53 localhost 8083 &
#./rpc_server 8082 51 8081 26 8083 76 localhost && sleep 1 &
#./rpc_server 8083 76 8082 51 8080 1 localhost && sleep 1 &

sleep 30

pkill -P $$
