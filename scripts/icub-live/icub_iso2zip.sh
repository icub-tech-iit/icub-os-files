#!/bin/bash
# Defaults
#set -x
SCRIPT_VERSION="1.0.1.20160309"
PERSISTENCE_SIZE_MB="4000" # leave empty to disable#
DEP_BIN_LIST="7z mkfs.ext3"
ISO_BASENAME=""
_CWD_=""
DEBUG_MODE="false"

print_defs ()
{
  echo "Script version is $SCRIPT_VERSION"
  echo "Default parameters are"
  echo "  PERSISTENCE_SIZE_MB is $PERSISTENCE_SIZE_MB"
}

usage ()
{
  echo "SCRIPT DESCRIPTION"
  echo "Usage: $0 [options] -f LIVE_ISO_FILE"
  echo
  echo "Create icub-live image."
  echo
  echo "options are :"
  echo "  -f LIVE_ISO_FILE : use file LIVE_ISO_FILE (MANDATORY)"
  echo "  -p PERSISTENCE_SIZE  : sets the size of overlay partition to PERSISTENCE_SIZE (in MB), set to 0 to disable overlay"
  echo "  -D : run in debug mode"
  echo "  -v : print script version"
  echo "  -d : print defaults"
  echo "  -h : print this help"
}

parse_opt() {
  while getopts hdp:f:vD opt
  do
    case "$opt" in
    D)
      DEBUG_MODE="true"
      ;;
    v)
      echo "Script version is $SCRIPT_VERSION"
      exit 0
      ;;
    f)
      LIVE_IMAGE_NAME="$OPTARG"
	    ;;
    p)
      PERSISTENCE_SIZE_MB="$OPTARG"
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
    if [ "$DEBUG_MODE" == " true" ]; then
      echo -e "\e[96m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] : checking $dep executable..\e[39m"
    fi
    ret=$(which $dep)
    if [ "$ret" == "" ]; then
      echo -e "\e[31mERROR : required executable $dep not found, please install the corresponing package.\e[39m"
      exit 1
    fi
  done
}

init()
{
  WHOAMI=$(whoami)
  if [ "$WHOAMI" != "root" ]
  then
    echo -e "\e[31m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR: Pleae execute this script as root.\e[39m"
    exit 1
  fi
  check_deps
  if [ "${LIVE_IMAGE_NAME}" == "" ]
  then
    echo -e "\e[31m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR: please specify the Live USB image file\e[39m"
    usage
    exit 1
  fi
  if [ ! -f "${LIVE_IMAGE_NAME}" ]
  then
    echo -e "\e[31m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR: live image  ${LIVE_IMAGE_NAME} not available\e[39m"
    exit 1
  fi
  if [ "$PERSISTENCE_SIZE_MB" -gt "4000" ]; then
    echo -e  "\e[33m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] WARNING : persistence size of $PERSISTENCE_SIZE_MB MB is too large, max size is 4000 MB (due to FAT32 limitations), setting persistence fiel size to 4000 MB\e[39m"
    PERSISTENCE_SIZE_MB="4000"
  fi
  ISO_BASENAME=$(basename ${LIVE_IMAGE_NAME} .iso)
  if [ -d "${ISO_BASENAME}" ]; then
    echo -n -e "\e[33m"
    read -r -p "[$(date +%d-%m-%Y) $(date +%H:%M:%S)] WARNING : path ${ISO_BASENAME} already exists and will be overwritten, are you sure (y/N)?] " response
    echo -n -e "\e[39m"
    response=${response,,}    # tolower
    case $response in
    [yY])
      echo -e "\e[96m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] : Removing ${ISO_BASENAME} as requested\e[39m"
      rm -rf ${ISO_BASENAME}
      ;;
    *)
      exit 1
      ;;
    esac
  fi
  mkdir ${ISO_BASENAME}
  echo -e "\e[32m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 started\e[39m"
}

fini()
{
  if [ -d "${ISO_BASENAME}" ]; then
    rm -rf ${ISO_BASENAME}
  fi
  echo -e "\e[32m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ended successfully.\e[39m"
}


main()
{
  cp ${LIVE_IMAGE_NAME} ${ISO_BASENAME}
  cd ${ISO_BASENAME}
  echo -e "\e[96m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] : extracting iso file ${LIVE_IMAGE_NAME}..\e[39m"
  7z x ${LIVE_IMAGE_NAME}
  if [ "$?" != "0" ]; then
    echo -e "\e[31m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR : failed to extract iso file ${LIVE_IMAGE_NAME}\e[39m"
    exit 1
  fi
  rm ${LIVE_IMAGE_NAME}
  _FULL_SCRIPT_PATH_=$(realpath $0)
  _BUILD_PATH="$(dirname $_FULL_SCRIPT_PATH_)"
  cp ${_BUILD_PATH}/templates/utils.zip .
  7z x utils.zip
  if [ "$?" != "0" ]; then
    echo -e "\e[31m[$mkfs.ext3(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR : failed to extract utils zip file\e[39m"
    exit 1
  fi
  rm utils.zip
  cd ..
  pwd
  mv ${ISO_BASENAME}/isolinux ${ISO_BASENAME}/syslinux
  cp ${ISO_BASENAME}/syslinux/isolinux.cfg  ${ISO_BASENAME}/syslinux/syslinux.cfg
  if [ "$PERSISTENCE_SIZE_MB" != "" ] && [ "$PERSISTENCE_SIZE_MB" != "0" ]
  then
    echo -e "\e[96m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] : creating persistence file..\e[39m"
    dd if=/dev/zero of=${ISO_BASENAME}/persistence bs=1M count=${PERSISTENCE_SIZE_MB}
    if [ "$?" != "0" ]; then
      echo -e "\e[31m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR : failed to create persistence file\e[39m"
      exit 1
    fi
    mkfs.ext3 -L persistence ${ISO_BASENAME}/persistence
    _TEMP_PATH=$(mktemp -d)
    mount  ${ISO_BASENAME}/persistence $_TEMP_PATH
    if [ "$?" != "0" ]; then
      echo -e "\e[31m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR : failed to mount persistency file ${ISO_BASENAME}/persistence on $_TEMP_PATH\e[39m"
      exit 1
    fi
    echo "/ union" > ${_TEMP_PATH}/persistence.conf
    sync
    sleep 3
    umount -f $_TEMP_PATH
    sed -i 's/\(append boot=.*\)$/\1 persistence/'  ${ISO_BASENAME}/syslinux/live.cfg
    if [ -d "$_TEMP_PATH" ]; then
      rmdir  $_TEMP_PATH
    fi
  fi
  if [ -f "${ISO_BASENAME}.zip" ]; then
    echo -n -e "\e[33m"
    read -r -p "[$(date +%d-%m-%Y) $(date +%H:%M:%S)] WARNING : zip archive ${ISO_BASENAME}.zip already exists and will be overwritten, are you sure (y/N)?] " response
    echo -n -e "\e[39m"
    response=${response,,}    # tolower  WHOAMI=$(whoami)
    case $response in
    [yY])
      echo -e "\e[96m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] : Removing ${ISO_BASENAME}.zip as requested\e[39m"
      rm ${ISO_BASENAME}.zip
      ;;
    *)
      exit 1
      ;;
    esac
  fi
  echo -e "\e[96m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] : compressing archive ${ISO_BASENAME}.zip..\e[39m"
  cd ${ISO_BASENAME}
  7z a ../${ISO_BASENAME}.zip * .disk
  if [ "$?" != "0" ]; then
    echo -e "\e[31m[$(date +%d-%m-%Y) $(date +%H:%M:%S)] $0 ERROR : failed create destination zip file ${ISO_BASENAME}.zip\e[39m"
    exit 1
  fi
  cd ..
}

parse_opt "$@"
init
main
fini
exit 0
