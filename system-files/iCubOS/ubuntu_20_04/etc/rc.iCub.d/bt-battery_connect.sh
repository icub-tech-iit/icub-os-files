#!/bin/bash
#===============================================================================
# Title:        bt-battery_connect.sh
# Description:  this script 
# Date:         2017-OCT-05
# Author:	Stefano Dafarra <stefano.dafarra@iit.it>
# Usage:        bash mount_icub-head_code.sh
# NOTE:		please replace RNBT_ADDRESS with your actual BT address
#===============================================================================

# Please change the following accorgding to your actual BT address
address="RNBT_ADDRESS"

if [ "$address" == "RNBT_ADDRESS"]; then
  echo "ERROR, please replace RNBT_ADDRESS with your actual Bluetooth address"
  echo "see https://icub-tech-iit.github.io/documentation/icub_operating_systems/icubos/bluetooth/"
  exit 1
fi

connected=0
i=0

echo Started

while (( connected == 0 && i < 10 ))
do
    echo Attempt $i
    rfcomm release 0 # Close eventual previous connections
    rfcomm -r connect 0 $address > /tmp/connect_out 2>&1 & #executes rfcomm in background to check that bluetooth is working
    sleep 5
    pid=$! #stores executed process id in pid
    count=$(ps -A| grep $pid |wc -l) #check whether process is still running
    if [[ $count -eq 0 ]] #if process is already terminated, then there were issues in connecting
    then
        cat /tmp/connect_out
        echo Connect failed
        if grep -q "No route to host" /tmp/connect_out; then # There are issues with the bluetooth
            echo "There might be a problem with the bluetooth. If it persists, try running sudo service bluetooth restart"
        fi
    else
        rfcomm release 0
        echo Released connection
        sleep 1
        cat /tmp/connect_out
        if grep -q "$address" /tmp/connect_out; then # If the connection was successfull, the address should be displayed in the output
            echo Connect successfull
            rfcomm bind 0 $address > /tmp/bind_out 2>&1
            cat /tmp/bind_out
            if [[ -s /tmp/bind_out ]]; then #if bind is successfull does not print anything
                echo  Bind returned error
            else
                echo Bind successfull
                sleep 1
                echo Calling stty
                stty -F /dev/rfcomm0 raw
                stty_return=$?
                if [[ $stty_return -eq 0 ]]
                then
                    echo stty successfull
                    connected=1
                else
                    echo stty failed
                fi
            fi
        else
            echo Connect failed
        fi
    fi
    i=$((i+1))
done
