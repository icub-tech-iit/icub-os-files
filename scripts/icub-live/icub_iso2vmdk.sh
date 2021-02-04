#!/bin/bash
# Defaults
#set -x
SCRIPT_VERSION="1.0.20170831"
DEP_BIN_LIST="qemu-img"
#
LIVE_IMAGE_NAME=""

log()
{
  echo "[ $(date +%d-%m-%Y) - $(date +%H:%M:%S) ] $1"
}

warn() {
  echo "[ $(date +%d-%m-%Y) - $(date +%H:%M:%S) ] WARNING : $1"
 }
 
error()
{
  echo "[ $(date +%d-%m-%Y) - $(date +%H:%M:%S) ] ERROR : $1"
}

exit_err () 
{
  error "$1"
  exit 1
}

print_defs ()
{
  echo "Script version is $SCRIPT_VERSION"
  echo "Default parameters are"
  echo " DEP_BIN_LIST is $DEP_BIN_LIST"
}

usage ()
{
  echo "SCRIPT DESCRIPTION"
  echo "Usage: $0 [options] -f LIVE_ISO_FILE"
  echo
  echo "Create a vmdk disk image from iCub live iso file."
  echo 
  echo "options are :"
  echo "  -f LIVE_ISO_FILE : use file LIVE_ISO_FILE (MANDATORY)"
  echo "  -v : print script version"
  echo "  -d : print defaults"
  echo "  -h : print this help"
}

parse_opt() {
  while getopts hdf:v opt
  do
    case "$opt" in
    v)
        echo "Script version is $SCRIPT_VERSION"
        exit 0
        ;;
    f)
        LIVE_IMAGE_NAME="$OPTARG"
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

check_deps()
{
  for dep in $DEP_BIN_LIST ; do
    log "checking $dep executable.."
    which $dep
    if [ "$?" != "0" ]; then
      exit_err "required executable $dep not found, please install the corresponing package"
      exit 1
    fi
  done
}

init()
{
  check_deps
  if [ "${LIVE_IMAGE_NAME}" == "" ]
  then
    log "please specify the Live iso image file"
    usage
    exit 1
  fi
  if [ ! -f "${LIVE_IMAGE_NAME}" ]
  then
    exit_err "live image  ${LIVE_IMAGE_NAME} not available"
  fi
  
  log "$0 started."
}

fini()
{
  echo "$0 ended successfully."
}

main()
{
  qemu-img convert -O vmdk "${LIVE_IMAGE_NAME}" "${LIVE_IMAGE_NAME}.vmdk"
  log "vmware disk ${LIVE_IMAGE_NAME}.vmdk produced."
}

parse_opt "$@"
init
main
fini
exit 0
