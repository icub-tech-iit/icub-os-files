#!/bin/bash
while true
do
        nmap -sn 10.0.0.0/24
        sleep 1
        nmap -sn 10.0.10.0/24
        sleep 60
done
