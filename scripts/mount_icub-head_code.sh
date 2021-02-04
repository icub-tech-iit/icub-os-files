#!/bin/bash -e
#===============================================================================
# Title:        mount_icub-head_code.sh
# Description:  this script mount a remote ICUB_CODE path locally via ssh
# Date:         2017-OCT-05
# Usage:        bash mount_icub-head_code.sh
#===============================================================================
#set -x
# Defaults
CODE_MOUNTPOINT="/home/icub/icub-head_files"
CODE_SOURCE_HOSTNAME="icub-head"
CODE_SOURCE_PATH="/usr/local/src/robot"
CODE_SOURCE_USERNAME="icub"
#
MOUNTED_PATH_RES=""
CREATE_MOUNTPOINT="false"

print_defs ()
{
  echo "Default parameters are"
  echo "  CODE_MOUNTPOINT is $CODE_MOUNTPOINT"
  echo "  CODE_SOURCE_HOSTNAME is $CODE_SOURCE_HOSTNAME"
  echo "  CODE_SOURCE_PATH is $CODE_SOURCE_PATH"
  echo "  CREATE_MOUNTPOINT is $CREATE_MOUNTPOINT"
  echo "  CODE_SOURCE_USERNAME is $CODE_SOURCE_USERNAME"
}

usage ()
{
  echo "SCRIPT DESCRIPTION"

  echo "Usage: $0 [options]"
  echo "options are :"
  echo "  -u : unmount insted of mounting"
  echo "  -c : create mountpoint path $CODE_MOUNTPOINT (if it does not exist)"
  echo "  -s HOSTNAME : use HOSTNAME as source host name"
  echo "  -p PATH : use PATH as source host path"
  echo "  -m PATH : mount code in path PATH"
  echo "  -t USERNAME : use USERNAME as remote user"
  echo "  -d : print defaults"
  echo "  -h : print this help"
}

log() {
  echo "$(date +%d-%m-%Y) - $(date +%H:%M:%S) : $1"
}

warn() {
  echo "$(date +%d-%m-%Y) - $(date +%H:%M:%S) WARNING : $1"
 }
 
error() {
  echo "$(date +%d-%m-%Y) - $(date +%H:%M:%S) ERROR : $1"
}

exit_err () {
	error "$1"
	exit 1
}


parse_opt() {
  while getopts hdm:up:s:ct: opt
  do
    case "$opt" in
    t)
      CODE_SOURCE_USERNAME="$$OPTARG"
      ;;
	c)
	  CREATE_MOUNTPOINT="true"
	  ;;
	s)
	  CODE_SOURCE_HOSTNAME="$OPTARG"
	  ;;
	p)
	  CODE_MOUNTPOINT="$OPTARG"
	  ;;
	m)
	  CODE_SOURCE_PATH="$OPTARG"
	  ;;
	u)
      unmount_path
      exit 0
      ;;
    h)
      usage
      exit 0
      ;;
    d)
      print_defs
      exit 0
      ;;
    \?) # unknown flag
      usage
      exit 1
      ;;
    esac
  done
}

init()
{
 SSHFS_BIN=$(which sshfs)
 if [ "$SSHFS_BIN" == "" ]; then
   exit_err "SSHFS binaries not found, (sshfs package not installed?)"
 fi
 if [ ! -d "$CODE_MOUNTPOINT" ]; then
   if [ "$CREATE_MOUNTPOINT" == "true" ]; then
     mkdir $CODE_MOUNTPOINT
	 if [ "$?" != "0" ]; then
	   exit_err "unable to create pathh $CODE_MOUNTPOINT"
	 fi
   else
     exit_err " mount point $CODE_MOUNTPOINT not found"
   fi
 fi
 ping -c 3 -q $CODE_SOURCE_HOSTNAME >/dev/null 2>&1
 if [ "$?" != "0" ]; then
    exit_err "remote host $CODE_SOURCE_HOSTNAME is not reachable"
 fi
 
}

fini()
{
  log "remote path ${CODE_SOURCE_USERNAME}@${CODE_SOURCE_HOSTNAME}:${CODE_SOURCE_PATH} mounted to ${CODE_MOUNTPOINT}"
}

mount_path()
{
  MOUNTED_PATH_RES=$(mount | grep "$CODE_MOUNTPOINT" | awk '{print $1}')
  if [ "$MOUNTED_PATH_RES" != "" ]; then
    exit_err "remote path $MOUNTED_PATH_RES already mounted to $CODE_MOUNTPOINT"
  fi
  sshfs ${CODE_SOURCE_USERNAME}@${CODE_SOURCE_HOSTNAME}:${CODE_SOURCE_PATH} ${CODE_MOUNTPOINT}
  if [ "$?" != "0" ]; then
    exit_err "Problems mounting ${CODE_SOURCE_USERNAME}@${CODE_SOURCE_HOSTNAME}:${CODE_SOURCE_PATH} to ${CODE_MOUNTPOINT} (remote path not exists?)"
  fi
}

unmount_path()
{
  MOUNTED_PATH_RES=$(mount | grep "$CODE_MOUNTPOINT" | awk '{print $1}')
  if [ "$MOUNTED_PATH_RES" == "" ]; then
    warn "no remote path is mounted to $CODE_MOUNTPOINT"
  else
    fusermount -u $CODE_MOUNTPOINT
	if [ "$?" != "0" ]; then
	  error "Errors while unmounting path $CODE_MOUNTPOINT"
	else
	  log "path $CODE_MOUNTPOINT unmounted correcyly"
	fi
	
  fi
}

main()
{
  mount_path
}

parse_opt "$@"
init
main
fini
exit 0