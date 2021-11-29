#!/bin/bash
#===============================================================================
# Title:        wifi-PowerSave_disable.sh
# Description:  disable WiFi Power save via iwconfig
# Date:         2021-NOV-29
# Author:       Matteo Brunettini <matteo.brunettini@iit.it>
# Usage:        bash wifi-PowerSave_disable.sh
# NOTE:
#===============================================================================
# Parameters
_WIRELESS_INTERFACE="wlp2s0"
# Locals
_IWCONFIG_BIN=$(which iwconfig)

if [ "$_IWCONFIG_BIN" == "" ]; then
  exit 1
fi
iwconfig $_WIRELESS_INTERFACE power off
exit $?
