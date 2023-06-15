#!/bin/bash

# Change to the desired directory
cd /home/samuel/redes2

# Loop 15 times to open 15 terminals
for i in {1..15}; do
    gnome-terminal -- bash -c "./client 127.0.0.1 50511; exec bash"
done
