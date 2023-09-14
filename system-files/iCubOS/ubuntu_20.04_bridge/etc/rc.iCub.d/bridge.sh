#!/bin/bash -e
#===============================================================================
# Title:        bridge.sh
# Description:  this script allows to set a network bridge between eth interface with wireless interface 
# Date:         20022-Set-30
# Author:		    Davide Lasagna davide.lasagna@iit.it
# Usage:        bash bridge.sh
#NOTE:			    see also netplan configuration
#===============================================================================
#set -x
# Defaults
bridge_ip="10.0.0.2"
wireless_if_mac=""
xavier_ip="10.0.0.3"
xavier_mac=""
wifi_if="wlp3s0"
#
# bring up wireless interface
iw dev wlp3s0 set 4addr on
# add wireless interface to bridge 
brctl addif br0 wlp3s0
# enable L2 routing toward wireless interface
ebtables -t nat -A POSTROUTING -o $wifi_if -j snat --to-src  $wireless_if_mac --snat-arp --snat-target ACCEPT
# enable clients L2 routing through wireless interface 
ebtables -t nat -A PREROUTING -p IPv4 -i wlp3s0 --ip-dst $xavier_ip -j dnat --to-dst $xavier_mac --dnat-target ACCEPT
ebtables -t nat -A PREROUTING -p ARP -i wlp3s0 --arp-ip-dst $xavier_ip -j dnat --to-dst $xavier_mac --dnat-target ACCEPT

exit $?
