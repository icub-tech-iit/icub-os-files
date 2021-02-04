#! /bin/sh
#===============================================================================
# Title:        set-hostname-from-net.sh
# Description:  this script set a blade machine hostname by contacting icub-srv
# Date:         2017-OCT-05
# Usage:        bash set-hostname-from-net.sh
#===============================================================================
_HOSTNAME_=""
LEASES_FILE_PATH="/var/lib/dhcp"

get_hostname_from_dns() {
  if [ "$1" != "" ]
  then
        IP_ADDR=$( ip addr show $1 | grep inet | grep $1 | awk '{ print $2 }' | sed -e 's/\/24//')
        if [ "$IP_ADDR" != "" ]
        then
                _HOSTNAME_=$( host $IP_ADDR |  awk '{ print $5 }' | cut -d'.' -f1 )
        fi
  fi
}

get_hostname_from_leases() {
  if [ "$1" != "" ]
  then
    if [ -f "${LEASES_FILE_PATH}/dhclient.leases" ]
    then
      _HOSTNAME_=$( grep "host-name" ${LEASES_FILE_PATH}/dhclient.leases | awk '{ print $3 }' | sed -e 's/"//g' -e 's/;//g' | head -1 )
    fi
  fi
}

get_hostname_from_dns eth0
if [ "${_HOSTNAME_}" = "" ]
then
        get_hostname_from_leases eth0
fi
if [ "${_HOSTNAME_}" = "" ]
then
        exit 1
fi
echo $_HOSTNAME_
hostname "$_HOSTNAME_"
exit $?
