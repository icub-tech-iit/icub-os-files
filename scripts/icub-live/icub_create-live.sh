#!/bin/bash -e
#set -x
# #####################################################
# SCRIPT NAME: icub_create-live.sh
#
# DESCRIPTION: process server repository by updating
# docker enviroment and restarting containers
#
# AUTHOR : Matteo Brunettini <matteo.brunettini@iit.it>
#
# LATEST MODIFICATION DATE (2021-04-26):
#
SCRIPT_VERSION="1.20.0"          # Sets version variable
#
SCRIPT_TEMPLATE_VERSION="1.2.1" #
SCRIPT_NAME=$(realpath -s $0)
SCRIPT_PATH=$(dirname $SCRIPT_NAME)
#
# #####################################################
# COLORS
COL_ERROR=" \e[91m" # Light RED
COL_OK="\e[92m" # Light GREEN
COL_DONE="\e[96m" # Light CYAN
COL_ACTION="\e[34m" # BLUE
COL_WARNING="\e[33m" # YELLOW
COL_DEBUG="\e[[95m" # Light MAGENTA
COL_NORMAL="\e[0m"
# Defaults
LOG_FILE=""
# Locals
REQUIRED_PACKAGES_LIST="live-build imagemagick lsb-release"
SUPPORTED_DISTRO_LIST="buster"
CONFIG_FILENAME=".icub-live.config"
ARCHITECTURE="amd64"
FLAVOUR="rt-amd64"
CUSTOM_KERNEL=""
DISTRIB="buster"
AREAS="main contrib non-free"
MIRROR_BOOTSTRAP=""
MIRROR_SECURITY=""
LIVE_USERNAME="icub" # leave empty to use default
LIVE_DEFAULT_GROUPS="audio,dialout,sudo,video,dip,plugdev,netdev,powerdev"
LIVE_TIMEZONE="Europe/Rome"
LIVE_MODE="debian"
INSTALLER="live"
BOOTAPPEND="boot=live config quickreboot noeject net.ifnames=0 biosdevname=0 persistence"
VERSION="7.10-$(date  +%y%m%d)"
LIVE_NAME_PREFIX="icub-live_"
if [ "$FLAVOUR" == "" ]
then
  LIVE_IMAGE_NAME="${LIVE_NAME_PREFIX}${VERSION}-${ARCHITECTURE}.iso"
else
  LIVE_IMAGE_NAME="${LIVE_NAME_PREFIX}${VERSION}-${FLAVOUR}.iso"
fi
_CUR_PATH=$(pwd)
_FULL_SCRIPT_PATH_=$(realpath $0)
BUILD_PATH="$(dirname $_FULL_SCRIPT_PATH_)/live-build"
CONFIG_FILE="${BUILD_PATH}/.icub-live.config"
LB_BUILD_TARGET_FILE="live-image-amd64.hybrid.iso"
#
DEBUG=""
STAGE=""
MIRRORS=""
IMAGE_TYPE=""
ORIGINAL_ISO_FILENAME=""
CREATE_VMDK_DISK="false"
_TMP_LOG=""

log() {
 if [ "$LOG_FILE" != "" ]
  then
    echo -e "$(date +%d-%m-%Y) - $(date +%H:%M:%S) : ${1}" >> $_TMP_LOG
  else
    echo -e "$(date +%d-%m-%Y) - $(date +%H:%M:%S) : ${1}"
  fi
}

debug() {
  log "${COL_DEBUG}DEBUG - $1${COL_NORMAL}"
}

warn() {
  log "${COL_WARNING}WARNING - $1${COL_NORMAL}"
 }

error() {
  log "${COL_ERROR}ERROR - $1${COL_NORMAL}"
}

exit_err () {
  error "$1"
  cd $_CUR_PATH
  close_log
  exit 1
}

print_version() {
    echo "Script version is $SCRIPT_VERSION based of Template version $SCRIPT_TEMPLATE_VERSION"
}

print_defs ()
{
  echo "Default parameters are"
  echo " SCRIPT_TEMPLATE_VERSION is $SCRIPT_TEMPLATE_VERSION"
  echo " SCRIPT_VERSION is $SCRIPT_VERSION"
  echo " SCRIPT_NAME is $SCRIPT_NAME"
  echo " SCRIPT_PATH is $SCRIPT_PATH"
  if [ "$LOG_FILE" != "" ]
  then
    echo "  log file is $LOG_FILE"
  fi
  echo "  REQUIRED_PACKAGES_LIST is $REQUIRED_PACKAGES_LIST"
  echo "  SUPPORTED_DISTRO_LIST is $SUPPORTED_DISTRO_LIST"
  echo "  ARCHITECTURE is $ARCHITECTURE"
  echo "  FLAVOUR is $FLAVOUR"
  echo "  CUSTOM_KERNEL is $CUSTOM_KERNEL"
  echo "  DISTRIB is $DISTRIB"
  echo "  AREAS is $AREAS"
  echo "  LIVE_USERNAME is $LIVE_USERNAME"
  echo "  LIVE_DEFAULT_GROUPS is $LIVE_DEFAULT_GROUPS"
  echo "  LIVE_TIMEZONE is $LIVE_TIMEZONE"
  echo "  BOOTAPPEND is $BOOTAPPEND"
  echo "  INSTALLER is $INSTALLER"
  echo "  VERSION is $VERSION"
  echo "  LIVE_NAME_PREFIX is $LIVE_NAME_PREFIX"
}

usage ()
{
  echo "SCRIPT DESCRIPTION"
  echo "Usage: $0 [options] -s all|config|build|clean|cleancache|cleanall"
  echo
  echo "Create icub-live image."
  echo "The parameter STAGE is mandatory and can be one of the following:"
  echo "  config - create config files"
  echo "  build - build image based on config stage"
  echo "  clean - cleanup build files"
  echo "  cleancache - clean only cache"
  echo "  cleanall - clean both build and cache files"
  echo "  cleaniso - clean all iso and related files"
  echo "  all - execute clean, config and build"
  echo
  echo "options are :"
  echo "  -L LOG_FILE : logs to file LOG_FILE"
  echo "  -D : compile the live system in debug mode"
  echo "  -d : print defaults"
  echo "  -h : print this help"
}

parse_opt() {
  while getopts hvdDs:L: opt
  do
    case "$opt" in
    L)
        LOG_FILE="$OPTARG"
        ;;
    s)
        STAGE="$OPTARG"
	      ;;
    D)
        DEBUG="--debug"
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

open_log()
{
  if [ "$LOG_FILE" != "" ]; then
    _TMP_LOG=$(mktemp)
  fi
}

close_log()
{
  if [ "$LOG_FILE" != "" ]; then
    cat $_TMP_LOG >> $LOG_FILE
    rm $_TMP_LOG
  fi
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
       apt-get --yes install $_package
     fi 
    done
}

################################################################################
# Purpose: read distrubution name (only for Debian and Ubuntu)
# Returns: the distro name
################################################################################
get_distro()
{
  local code_name="$(lsb_release -c | awk '{ print $2 }')"
  echo $code_name
}

################################################################################
# Purpose: check if the actual distro is supported
# Arguments:
#   $1 -Z the list of supported distro
# Returns: 
#   true if supported
#   false if not supported
################################################################################
check_supported_distro()
{
  local _supported="false"
  local _actual_distro_name=$(get_distro)
  local _distro
  for _distro in $1
    do
      if [[ "$_actual_distro_name" =~ ^$_distro.* ]]; then
        _supported="true"
      fi
    done
  echo $_supported
}


init()
{
  open_log
  WHOAMI=$(whoami)
  if [ "$WHOAMI" != "root" ]
  then
    exit_err "Pleae execute this script as root"
  fi
  if [ ! -d "${BUILD_PATH}/config" ]
  then
    exit_err "can't find configuration files in ${BUILD_PATH}/config"
    exit 1
  fi
  _is_supported=$(check_supported_distro "$SUPPORTED_DISTRO_LIST")
  if [ "$_is_supported" != "true" ]; then
    exit_err "distro is not supported"
  fi
  check_required_packages "$REQUIRED_PACKAGES_LIST"
  log "$0 started."
  cd $BUILD_PATH
}

fini()
{
  log "$0 ${COL_OK}ENDED${COL_NORMAL}"
  close_log
  cd ${_CUR_PATH}
  if [ "$?" != "0" ]; then
    warn "unable to cd to path ${_CUR_PATH}"
  fi
}

save_config()
{
  if [ -f "${CONFIG_FILE}" ]
  then
    warn "overwriting existing config file"
    rm ${CONFIG_FILE}
  fi
  echo -e "ARCHITECTURE=\"$ARCHITECTURE\"" >> $CONFIG_FILE
  echo -e "FLAVOUR=\"$FLAVOUR\"" >> $CONFIG_FILE
  echo -e "CUSTOM_KERNEL=\"$CUSTOM_KERNEL\"" >> $CONFIG_FILE
  echo -e "DISTRIB=\"$DISTRIB\"" >> $CONFIG_FILE
  echo -e "AREAS=\"$AREAS\"" >> $CONFIG_FILE
  echo -e "MIRROR_BOOTSTRAP=\"$MIRROR_BOOTSTRAP\"" >> $CONFIG_FILE
  echo -e "MIRROR_SECURITY=\"$MIRROR_SECURITY\"" >> $CONFIG_FILE
  echo -e "LIVE_USERNAME=\"$LIVE_USERNAME\"" >> $CONFIG_FILE
  echo -e "LIVE_DEFAULT_GROUPS=\"$LIVE_DEFAULT_GROUPS\"" >> $CONFIG_FILE
  echo -e "LIVE_TIMEZONE=\"$LIVE_TIMEZONE\"" >> $CONFIG_FILE
  echo -e "INSTALLER=\"$INSTALLER\"" >> $CONFIG_FILE
  echo -e "BOOTAPPEND=\"$BOOTAPPEND\"" >> $CONFIG_FILE
  echo -e "VERSION=\"$VERSION\"" >> $CONFIG_FILE
  echo -e "LIVE_IMAGE_NAME=\"$LIVE_IMAGE_NAME\"" >> $CONFIG_FILE
  echo -e "DEBUG=\"$DEBUG\"" >> $CONFIG_FILE
  echo -e "MIRRORS=\"$MIRRORS\"" >> $CONFIG_FILE
}

load_config()
{
  if [ ! -f "${CONFIG_FILE}" ]
  then
    error "config file ${CONFIG_FILE} not found, please execute the config stage"
    fini
    exit 1
  fi
  source ${CONFIG_FILE}
}

config_live()
{
  log "Starting CONFIGURE stage.."
  IMAGE_TYPE="iso-hybrid"
  ORIGINAL_ISO_FILENAME="live-image-${ARCHITECTURE}.hybrid.iso"
  if [ ! -d "config/includes.binary/isolinux" ]
  then
    mkdir -p "config/includes.binary/isolinux"
  fi
  convert templates/splash.png -font helvetica -fill white -pointsize 26 -annotate +180+50 "iCub PC104 LIVE\n\nVersion ${VERSION}\nBased on Linux ${DISTRIB} ${ARCHITECTURE} " config/includes.binary/isolinux/splash.png
  if [ "$LIVE_USERNAME" != "" ]
  then
    BOOTAPPEND="${BOOTAPPEND} username=${LIVE_USERNAME}"
  fi
  if [ "$LIVE_DEFAULT_GROUPS" != "" ]
  then
    BOOTAPPEND="${BOOTAPPEND} user-default-groups=${LIVE_DEFAULT_GROUPS}"
  fi
  if [ "$LIVE_TIMEZONE" != "" ]
  then
    BOOTAPPEND="${BOOTAPPEND} timezone=${LIVE_TIMEZONE}"
  fi

  BOOTAPPEND="${BOOTAPPEND} ip=frommedia"

  if [ "$MIRROR_BOOTSTRAP" != "" ]
  then
    MIRRORS="$MIRRORS --mirror-bootstrap $MIRROR_BOOTSTRAP"
  fi

  if [ "$MIRROR_SECURITY" != "" ]
  then
    MIRRORS="$MIRRORS --mirror-binary-security $MIRROR_SECURITY --mirror-chroot-security $MIRROR_SECURITY"
  fi

  if [ "$FLAVOUR" != "" ]
  then
    LINUX_FLAVOURS="--linux-flavours $FLAVOUR"
  fi

  if [ "$CUSTOM_KERNEL" != "" ]
  then
    LINUX_PACKAGES="--linux-packages $CUSTOM_KERNEL"
  fi

  if [ "$DEBUG" != "" ]; then
    debug 'lb config --mode "$LIVE_MODE" --distribution "$DISTRIB" --architectures "$ARCHITECTURE" $LINUX_FLAVOURS $LINUX_PACKAGES --archive-areas "$AREAS" $MIRRORS --bootappend-live "$BOOTAPPEND" --debian-installer "$INSTALLER" --binary-images "iso-hybrid" $DEBUG'
  fi
  lb config --mode "$LIVE_MODE" --distribution "$DISTRIB" --architectures "$ARCHITECTURE" $LINUX_FLAVOURS $LINUX_PACKAGES --archive-areas "$AREAS" $MIRRORS --bootappend-live "$BOOTAPPEND" --debian-installer "$INSTALLER" --binary-images "iso-hybrid" $DEBUG
  if [ "$?" != "0" ]; then
    exit_err "errors while configuring live environment"
  fi

  # Create version file
  VERSION_FILE="config/includes.chroot/VERSION_PC104"
  echo "iCub LIVE image for PC104 version $VERSION" > ${VERSION_FILE}
  echo "Based on $DISTRIB $ARCHITECTURE" >> ${VERSION_FILE}
  echo "Build on $( date +%d-%b-%Y )" >> ${VERSION_FILE}
  cp ${VERSION_FILE} config/includes.binary/VERSION
  # Copy README file
  README_FILE="config/includes.chroot/README_PC104"
  if [ -f "$README_FILE" ]; then
    cp $README_FILE config/includes.binary/README
  fi
  # Copy ChangeLog file
  CHANGELOG_FILE="config/includes.chroot/ChangeLog_PC104"
  if [ -f "$CHANGELOG_FILE" ]; then
    cp $CHANGELOG_FILE config/includes.binary/ChangeLog
  fi
  log "CONFIGURE stage completed."
}

build_live()
{
  log "Starting BUILD stage.."

  if [ "$DEBUG" != "" ]; then
    debug 'lb build'
  fi
  lb build
  if [ "$?" != "0" ]; then
    exit_err "errors while building live"
  fi

  if [ -f "${ORIGINAL_ISO_FILENAME}" ]; then
    mv ${ORIGINAL_ISO_FILENAME} ${LIVE_IMAGE_NAME}
    md5sum ${LIVE_IMAGE_NAME} > ${LIVE_IMAGE_NAME}.md5
    if [ -f "${ORIGINAL_ISO_FILENAME}.zsync" ]; then
      mv ${ORIGINAL_ISO_FILENAME}.zsync ${LIVE_IMAGE_NAME}.zsync
    fi
    log "Build stage completed: live image $LIVE_IMAGE_NAME produced."
  else
    error "live image not available (errors while building? check above output)"
    fini
    exit 1
  fi
}

clean_live()
{
  log "Starting CLEAN-LIVE stage.."
  PATH_TO_DELETE="config/hooks"
  DIR_TO_DELETE="config/apt config/debian-installer config/includes config/includes.bootstrap config/includes.installer/ config/includes.source includes/packages includes/packages.binary includes/preseed includes/rootfs"
  FILES_TO_DELETE="config/source config/chroot config/common config/bootstrap config/binary config/build config/includes.binary/VERSION config/includes.binary/README config/includes.binary/ChangeLog config/includes.binary/isolinux/splash.png config/includes.chroot/VERSION_PC104 includes/packages includes/packages.binary includes/preseed includes/rootfs $CONFIG_FILE"
  for _file in $FILES_TO_DELETE
  do
    if [ -e "$_file" ]; then
      rm $_file
    fi
  done
  for _dir in $DIR_TO_DELETE
  do
    if [ -e "$_dir" ]; then
      rmdir --ignore-fail-on-non-empty $_dir
    fi
  done
  for _path in $PATH_TO_DELETE
  do
    if [ -e "$_path" ]; then
      rm -r $_path
    fi
  done

  if [ "$DEBUG" != "" ]; then
    debug 'clean --all'
  fi
  lb clean --all
  if [ "$?" != "0" ]; then
    warn "errors while cleaning live"
  fi

  log "CLEAN-LIVE stage completed."
}

cleancache_live()
{
  log "Starting CLEAN-CACHE stage.."

  if [ "$DEBUG" != "" ]; then
    debug 'lb clean --cache'
  fi
  lb clean --cache
  if [ "$?" != "0" ]; then
    warn "errors while cleaning cache"
  fi

 log "CLEAN-CACHE stage completed."
}

clean_iso()
{
  log "Starting CLEAN-ISO stage.."
  if ls ${LIVE_NAME_PREFIX}*.iso > /dev/null 2>&1; then
    rm ${LIVE_NAME_PREFIX}*.iso
  fi
  if ls ${LIVE_NAME_PREFIX}*.md5 > /dev/null 2>&1; then
    rm ${LIVE_NAME_PREFIX}*.md5
  fi
  if ls ${LIVE_NAME_PREFIX}*.zsync > /dev/null 2>&1; then
    rm ${LIVE_NAME_PREFIX}*zsync
  fi
  log "CLEAN-ISO stage completed."
}

main()
{
  case "$STAGE" in
    "clean")
       clean_live
       ;;
    "cleaniso")
       clean_iso
       ;;
    "cleancache")
       cleancache_live
       ;;
    "cleanall")
       clean_live
       cleancache_live
       clean_iso
       ;;
    "config")
       config_live
       save_config
       ;;
    "build")
       load_config
       build_live
       ;;
    "all")
       clean_live
       clean_iso
       config_live
       build_live
       ;;
     *) # unknown flag
       error "ERROR invalid or missing STAGE parameter"
       fini
       exit 1
       ;;
  esac
}

parse_opt "$@"
init
main
fini
exit 0
