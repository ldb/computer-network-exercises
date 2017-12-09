#!/bin/bash

echo "Running servers for 30 seconds"
echo "Connect to any node on ports 8080-8083"

./rpc_server 1 localhost 8080 &

sleep 1

./rpc_server 26 localhost 8083 1 localhost 8080 &

#./rpc_server 8081 26 8080 1 8082 51 localhost && sleep 1 &
#./rpc_server 8082 51 8081 26 8083 76 localhost && sleep 1 &
#./rpc_server 8083 76 8082 51 8080 1 localhost && sleep 1 &

sleep 10

pkill -P $$
