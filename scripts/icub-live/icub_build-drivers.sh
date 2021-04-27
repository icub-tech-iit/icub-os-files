#!/bin/bash -e
#set -x
# ######################################################
# SCRIPT NAME: icub_build_modules.sh
#
# DESCRIPTION: build custom kernel driver modules for
#  PC104 live image
#
# AUTHOR : Matteo Brunettini <matteo.brunettini@iit.it>
#
# LATEST MODIFICATION DATE (YYYY-MM-DD):
#
SCRIPT_VERSION="0.9"          # Sets version variable
SCRIPT_TEMPLATE_VERSION="1.2.1" #
SCRIPT_NAME=$(realpath -s $0)
SCRIPT_PATH=$(dirname $SCRIPT_NAME)
#
# ######################################################
# COLORS
COL_NORMAL="\e[0m"
COL_ERROR=" \e[31m"
COL_OK="\e[32m"
COL_DONE="\e[96m"
COL_ACTION="\033[1;90m"
COL_WARNING="\e[33m"
# Defaults
LOG_FILE=""
CHROOT_PATH=""
SOURCES_PATH=""
TARGET=""
TMP_BUILD_PATH="/tmp/cfw_module"
ARCHIVE_FILE_NAME="cfw002_driver"
# locals
REQUIRED_PACKAGES_LIST="schroot"
# ######################################################

print_defs ()
{
  echo "Default parameters are"
  echo " SCRIPT_TEMPLATE_VERSION is $SCRIPT_TEMPLATE_VERSION"
  echo " SCRIPT_VERSION is $SCRIPT_VERSION"
  if [ "$LOG_FILE" != "" ]
  then
    echo "  log file is $LOG_FILE"
  fi
  echo "Local parameters are"
  echo "  REQUIRED_PACKAGES_LIST is $REQUIRED_PACKAGES_LIST"
  echo "  TMP_BUILD_PATH is $TMP_BUILD_PATH"
  echo "  ARCHIVE_FILE_NAME is $ARCHIVE_FILE_NAME"
}

usage ()
{
  echo "SCRIPT DESCRIPTION"

  echo "Usage: $0 [options]"
  echo "options are :"

  echo "  -c CHROOT_PATH : use the chroot system in path CHROOT_PAT [mandatory]"
  echo "  -s SOURCES_PATH : use driver sources from path SOURCES_PATH [mandatory]"
  echo "  -t TARGET : build for target $TARGET instead of $(shell uname -r)"
  echo "  -l LOG_FILE : write logs to file LOG_FILE"
  echo "  -d : print defaults"
  echo "  -v : print version"
  echo "  -h : print this help"
}

log() {
 if [ "$LOG_FILE" != "" ]
  then
    echo -e "$(date +%d-%m-%Y) - $(date +%H:%M:%S) : ${1}${COL_NORMAL}" >> $LOG_FILE
  else
    echo -e "$(date +%d-%m-%Y) - $(date +%H:%M:%S) : ${1}${COL_NORMAL}"
  fi
}

warn() {
  log "${COL_WARNING}WARNING - $1"
 }

error() {
  log "${COL_ERROR}ERROR - $1"
}

exit_err () {
  error "$1"
  exit 1
}

print_version() {
    echo "Script version is $SCRIPT_VERSION based of Template version $SCRIPT_TEMPLATE_VERSION"
}

parse_opt() {
  while getopts hdvl:c:s:t: opt
  do
    case "$opt" in
    "t")
      TARGET="$OPTARG"
      ;;
    "c")
      CHROOT_PATH="$OPTARG"
      ;;
    "s")
      SOURCES_PATH="$OPTARG"
      ;;
    "l")
      LOG_FILE="$OPTARG"
      ;;
    h)
      usage
      exit 0
      ;;
    d)
      print_defs
      exit 0
      ;;
    v)
      print_version
      exit 0
      ;;
    \?) # unknown flag
      usage
      exit 1
      ;;
    esac
  done
}

################################################################################
# Purpose: check if packages are installed and if needed, install them (only for
# Debian and Ubuntu)
# Arguments:
#   $1 -> the list of packages to check
################################################################################
check_required_packages()
{
  local _package
  for _package in $1
    do
     log "checking package $_package"
     local _package_installed=$(dpkg-query -W --showformat='${Status}\n' $_package )
     if [ "$_package_installed" != "install ok installed" ]; then
       warn "The package $_package is not installed, installing now"
       sudo apt-get --yes install $_package
     fi 
    done
}


init()
{
  WHOAMI=$(whoami)
  if [ "$WHOAMI" != "root" ]
  then
    exit_err "Pleae execute this script as root"
  fi

  if [ "$CHROOT_PATH" == "" ]; then
    exit_err "chroot path parameter not specified"
  fi
  if [ ! -d "$CHROOT_PATH" ]; then
    exit_err "path $CHROOT_PATH is not valid"
  fi
  if [ "$SOURCES_PATH" == "" ]; then
    exit_err "chroot path parameter not specified"
  fi
  if [ ! -d "$SOURCES_PATH" ]; then
    exit_err "path $CHROOT_PATH is not valid"
  fi
  check_required_packages "$REQUIRED_PACKAGES_LIST"
  mkdir -p ${CHROOT_PATH}${TMP_BUILD_PATH}
  log "$0 ${COL_OK}STARTED"
}

fini()
{ 
  if [ -d "$TMP_BUILD_PATH" ]; then
    rm -rf "$TMP_BUILD_PATH"
  fi
  log "$0 ${COL_OK}ENDED"
}

build_cfw()
{
  # copy sourcesa
  mkdir -p ${CHROOT_PATH}/${TMP_BUILD_PATH}
  cp -ar ${SOURCES_PATH}/* ${CHROOT_PATH}/${TMP_BUILD_PATH}
  chroot ${CHROOT_PATH} /bin/bash -c "cd ${TMP_BUILD_PATH}/src && make clean KVER=$TARGET"
  chroot ${CHROOT_PATH} /bin/bash -c "cd ${TMP_BUILD_PATH}/src && make all KVER=$TARGET"
}

pack_cfw_binaries()
{
  _tmp_path=$(mktemp -d)

  mkdir -p ${_tmp_path}/lib/firmware
  mkdir -p ${_tmp_path}/lib/modules/${TARGET}/iCubDrivers/cfw002/LinuxDriver/API
  mkdir -p ${_tmp_path}/lib/modules/${TARGET}/iCubDrivers/cfw002/LinuxDriver/tests
  mkdir -p ${_tmp_path}/usr/lib
  mkdir -p ${_tmp_path}/usr/local/src

  cp ${ARCHIVE_FILE_NAME}_sources.tar.gz ${_tmp_path}/usr/local/src/
  cp ${CHROOT_PATH}/${TMP_BUILD_PATH}/src/API/libcfw002.so ${_tmp_path}/usr/lib/
  cp ${CHROOT_PATH}/${TMP_BUILD_PATH}/src/firmware/cfw002_fw.bin ${_tmp_path}/lib/firmware/
  cp ${CHROOT_PATH}/${TMP_BUILD_PATH}/src/module/cfw002.ko ${_tmp_path}/lib/modules/${TARGET}/iCubDrivers/cfw002/LinuxDriver/
  cp ${CHROOT_PATH}/${TMP_BUILD_PATH}/src/API/cfw002_api.h ${_tmp_path}/lib/modules/${TARGET}/iCubDrivers/cfw002/LinuxDriver/API/
  cp ${CHROOT_PATH}/${TMP_BUILD_PATH}/src/API/libcfw002.h ${_tmp_path}/lib/modules/${TARGET}/iCubDrivers/cfw002/LinuxDriver/API/
  cp ${CHROOT_PATH}/${TMP_BUILD_PATH}/src/API/libcfw002.so ${_tmp_path}/lib/modules/${TARGET}/iCubDrivers/cfw002/LinuxDriver/API/
  cp ${CHROOT_PATH}/${TMP_BUILD_PATH}/src/tests/test_audio ${_tmp_path}/lib/modules/${TARGET}/iCubDrivers/cfw002/LinuxDriver/tests/
  
  _pwd=$(pwd)
  if [ -f "${ARCHIVE_FILE_NAME}.tar" ]; then
    warn "removing old file ${ARCHIVE_FILE_NAME}.tar"
    rm ${ARCHIVE_FILE_NAME}.tar
  fi
  if [ -f "${ARCHIVE_FILE_NAME}.tar.gz" ]; then
    warn "removing old file ${ARCHIVE_FILE_NAME}.tar.gz"
    rm ${ARCHIVE_FILE_NAME}.tar.gz
  fi
  cd ${_tmp_path}
  tar cvf ${_pwd}/${ARCHIVE_FILE_NAME}.tar usr lib
  cd $_pwd
  gzip ${ARCHIVE_FILE_NAME}.tar
  rm ${ARCHIVE_FILE_NAME}_sources.tar.gz

  if [ -d "$_tmp_path" ]; then
    rm -rf $_tmp
  fi
}

pack_cfw_sources()
{ 
  if [ -f "${ARCHIVE_FILE_NAME}_sources.tar" ]; then
    warn "removing old file ${ARCHIVE_FILE_NAME}_sources.tar"
    rm ${ARCHIVE_FILE_NAME}_sources.tar
  fi
  if [ -f "${ARCHIVE_FILE_NAME}_sources.tar.gz" ]; then
    warn "removing old file ${ARCHIVE_FILE_NAME}_sources.tar.gz"
    rm ${ARCHIVE_FILE_NAME}_sources.tar.gz
  fi
  tar cvf ${ARCHIVE_FILE_NAME}_sources.tar ${SOURCES_PATH}
  gzip ${ARCHIVE_FILE_NAME}_sources.tar
}

main()
{
  build_cfw
  pack_cfw_sources
  pack_cfw_binaries
}

parse_opt "$@"
init
main
fini
exit 0
