#!/bin/bash -e
#===============================================================================
# Title:        provision_icub_machine.sh
# Description:  this script setup a new machine for the iCub environment
# Date:         2020-AUG-19
# Usage:        bash provision_icub_machine.sh
#===============================================================================
#set -x
# Defaults
REQUIRED_PACKAGES="nfs-common python-tk portaudio19-dev libopencv-dev ntpdate vim ssh cmake-curses-gui iperf wget unzip git"
SUPERBUILD_REQUIRED_PACKAGES="libeigen3-dev build-essential cmake cmake-curses-gui coinor-libipopt-dev libboost-system-dev libboost-filesystem-dev libboost-thread-dev libtinyxml-dev libace-dev libgsl0-dev libopencv-dev libode-dev liblua5.1-dev lua5.1 git swig qtbase5-dev qtdeclarative5-dev qtmultimedia5-dev qml-module-qtquick2 qml-module-qtquick-window2 qml-module-qtmultimedia qml-module-qtquick-dialogs qml-module-qtquick-controls qml-module-qt-labs-folderlistmodel qml-module-qt-labs-settings libsdl1.2-dev libxml2-dev"
SERVER_REQUIRED_PACKAGES="ntp nfs-kernel-server nfs-common"
USE_ICUB_BINARIES="false"
USE_ICUB_SOURCES="false"
SERVER_LAPTOP="false"
SUPERBUILD="false"
ICUB_BINARIES="yarp icub"
ICUB_SOURCES_URL="https://github.com/robotology"
ROBOTOLOGY_SUPERBUILD="robotology-superbuild"
ICUB_SOURCES_REPOS="yarp icub-main icub-firmware-shared icub-firmware-build icub-basic-demos icub-contrib-common robots-configuration icub-tests robot-testing-framework"
BASHRC_ICUB_URL="https://git.robotology.eu/mbrunettini/icub-environment/raw/master/bashrc_iCub"
BASHRC_ICUB_SUPERBUILD_URL="https://git.robotology.eu/MBrunettini/icub-environment/raw/master/bashrc_iCub_superbuild"
PROVISIONED_USERNAME="icub"
INSTRUCTIONS_URL="http://wiki.icub.org/wiki/Generic_iCub_machine_installation_instructions"
#
ICUBRC_FILE=""
DISTRO_ISSUE_NAME=""

print_defs ()
{
  echo "Default parameters are"
  echo "  USE_ICUB_BINARIES is $USE_ICUB_BINARIES"
  echo "  USE_ICUB_SOURCES is $USE_ICUB_SOURCES"
  echo "  SUPERBUILD is $SUPERBUILD"
  echo "  ROBOTOLOGY_SUPERBUILD is $ROBOTOLOGY_SUPERBUILD"
  echo "  REQUIRED_PACKAGES are $REQUIRED_PACKAGES"
  echo "  SERVER_REQUIRED_PACKAGES are $SERVER_REQUIRED_PACKAGES"
  echo "  PROVISIONED_USERNAME is $PROVISIONED_USERNAME"
  echo "  ICUB_BINARIES are $ICUB_BINARIES"
  echo "  BASHRC_ICUB_URL is $BASHRC_ICUB_URL"
  echo "  BASHRC_ICUB_SUPERBUILD_URL is $BASHRC_ICUB_SUPERBUILD_URL"
  echo "  SERVER_LAPTOP is $SERVER_LAPTOP"
  echo "  INSTRUCTIONS_URL is $INSTRUCTIONS_URL"
  echo "  ICUB_SOURCES_URL is $ICUB_SOURCES_URL"
}

usage ()
{
  echo "Install a generic icub setup machine on a fresh installed linux"

  echo "Usage: $0 [options]"
  echo "options are :"
  echo "  -b : install also icub and yarp binaries from packages repository"
  echo "  -S : superbuild (WARNING: this forces the above flag to false)"
  echo "  -s : download sources"
  echo "  -u USERNAME : use USERNAME as user name"
  echo "  -l : for server laptop configuration (incompatible with superbuild flag)"
  echo "  -d : print defaults"
  echo "  -h : print this help"
}

log() {
  echo "$(date +%d-%m-%Y) - $(date +%H:%M:%S) : $1"
}

warn() {
  log "WARNING - $1"
}

exit_err() {
  log "ERROR - $1"
  exit 1
}


parse_opt() {
  while getopts hdbu:sSl opt
  do
    case "$opt" in
    S)
      SUPERBUILD="true"
      ;;
    u)
      PROVISIONED_USERNAME="$OPTARG"
      ;;
    s)
      USE_ICUB_SOURCES="true"
      ;;
    b)
      USE_ICUB_BINARIES="true"
      ;;
	  l)
      SERVER_LAPTOP="true"
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
  WHOAMI=$(whoami)
  if [ "$WHOAMI" != "root" ]
  then
    exit_err "Please execute this script as root"
  fi
  if [ "$PROVISIONED_USERNAME" == "" ]; then
    warning "Username not specified, using $DEFAULT_USERNAME as username"
    $PROVISIONED_USERNAME="$DEFAULT_USERNAME"
  fi
  if [ "$PROVISIONED_USERNAME" == "root" ]; then
    exit_err "root is not a valid username, please specify another username (see -u parameter)"
  fi
  if [ "$(grep $PROVISIONED_USERNAME /etc/passwd)" == "" ]; then
    exit_err "user $PROVISIONED_USERNAME is not valid"
  fi
  if [ "$USE_ICUB_SOURCES" == "true" ]; then
    warn "  You asked to download some sources, then if you use remote mounted sources, the /etc/fstab and related mountpoints must be configured already!"
    read -p "  Press ENTER to continue or CTRL-C to stop here"
  fi
  if [ "$SUPERBUILD" == "true" ]; then
    if [ "$SERVER_LAPTOP" == "true" ]; then
      exit_err " superbuild flag (-S) and serve laptop configuration flag (-l) are not compatible"
    fi
    if [ "$USE_ICUB_BINARIES" == "true"]; then
      warn "superbuild flag (-S) has been set, forcing USE_ICUB_BINARIES to false"
      USE_ICUB_BINARIES="false"
    fi
  fi

  log "$0 STARTED"
  DISTRO_ISSUE_NAME=$(cat /etc/issue | head --lines=1 | awk '{ print $1 }')
  case "$DISTRO_ISSUE_NAME" in
  "Ubuntu" )
    DISTRO_ISSUE_NAME=$(cat /etc/issue | head --lines=1 | awk '{ print $1, $2 }')
    ;;
  "Debian" )
    DISTRO_ISSUE_NAME=$(cat /etc/issue | head --lines=1 | awk '{ print $1, $2, $3 }')
    ;;
  *)
    exit_err "ERROR : unsupported distribution $DISTRO_ISSUE_NAME"
    ;;
  esac
  log "Detected $DISTRO_ISSUE_NAME"
}

fini()
{
  log "DONE"
  echo "  Don't forget to (if needed) :"
  echo "    1) setup Network configuration"
  echo "    2) setup ntp"
  if [ "$USE_ICUB_SOURCES" != "true" ]; then
    echo "    3) setup mountpoints and related /etc/fstab entries"
  fi
  echo "  See: http://wiki.icub.org/wiki/Generic_iCub_machine_installation_instructions"
  log "$0 ENDED "
}

set_sudo_guid()
{
  log "Adding user $PROVISIONED_USERNAME to sudo group.."
  if [ "$(grep '^sudo' /etc/group | grep '$PROVISIONED_USERNAME')" = "" ]
  then
    sed -e "/^sudo/s/$/,$PROVISIONED_USERNAME/" -i /etc/group
  fi
}

install_packages()
{
  apt-get --quiet --assume-yes update
  apt-get --quiet --assume-yes install $1
}

setup_system_packages()
{
  log "Installing system required packages.."
  DISTRO_SPECIFIC_PACKAGES=""
  case "$DISTRO_ISSUE_NAME" in
    "Ubuntu 14.04" | "Ubuntu 14.04.1" | "Ubuntu 14.04.2" | "Ubuntu 14.04.3" | "Ubuntu 14.04.4" | "Ubuntu 14.04.5" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
    "Ubuntu 16.04" | "Ubuntu 16.04.1" | "Ubuntu 16.04.2" | "Ubuntu 16.04.3" | "Ubuntu 16.04.4" | "Ubuntu 16.04.5" | "Ubuntu 16.04.6" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
      "Ubuntu 16.10" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
      "Ubuntu 17.04" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
      "Ubuntu 18.04" | "Ubuntu 18.04.1" | "Ubuntu 18.04.2" | "Ubuntu 18.04.3" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
      "Ubuntu 18.10" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
      "Ubuntu 19.04" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
      "Ubuntu 20.04" | "Ubuntu 20.04.1" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
    "Debian GNU/Linux 7" )
      echo "deb http://http.debian.net/debian wheezy-backports main" > /etc/apt/sources.list.d/backports.list
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
    "Debian GNU/Linux 8" )
      DISTRO_SPECIFIC_PACKAGES=""
      ;;
    "Debian GNU/Linux 9" )
      DISTRO_SPECIFIC_PACKAGES="dirmngr"
      ;;
    * )
      exit_err "ERROR : usupported or not recognized issue string $DISTRO_ISSUE_NAME in /etc/issue"
      ;;
  esac
  install_packages "$REQUIRED_PACKAGES $DISTRO_SPECIFIC_PACKAGES"
  if [ "$SUPERBUILD" == "true" ]; then
    install_packages "$SUPERBUILD_REQUIRED_PACKAGES"
  fi
  if [ "$USE_ICUB_SOURCES_LAPTOP" == "true" ]; then
    install_packages "$SERVER_REQUIRED_PACKAGES"
  fi
}

setup_icub_packages()
{
  log "Adding iCub packages repository for $DISTRO_ISSUE_NAME.."
  ICUB_REPO_STRING=""
  case "$DISTRO_ISSUE_NAME" in
    "Ubuntu 14.04" | "Ubuntu 14.04.1" | "Ubuntu 14.04.2" | "Ubuntu 14.04.3" | "Ubuntu 14.04.4" | "Ubuntu 14.04.5" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu trusty contrib/science"
      ;;
    "Ubuntu 16.04" | "Ubuntu 16.04.1" | "Ubuntu 16.04.2" | "Ubuntu 16.04.3" | "Ubuntu 16.04.4" | "Ubuntu 16.04.5" | "Ubuntu 16.04.6" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu xenial contrib/science"
      ;;
	  "Ubuntu 16.10" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu yakkety contrib/science"
      ;;
    "Ubuntu 17.04" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu zesty contrib/science"
      ;;
    "Ubuntu 18.04" | "Ubuntu 18.04.1"  | "Ubuntu 18.04.2" | "Ubuntu 18.04.3" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu bionic contrib/science"
      ;;
    "Ubuntu 18.10" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu cosmic contrib/science"
      ;;
    "Ubuntu 19.04" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu disco contrib/science"
      ;;
    "Ubuntu 20.04.1" | "Ubuntu 20.04.1" )
      ICUB_REPO_STRING="deb http://www.icub.org/ubuntu focal contrib/science"
      ;;
    "Debian GNU/Linux 7" )
      ICUB_REPO_STRING="deb http://www.icub.org/debian wheezy contrib/science"
      ;;
    "Debian GNU/Linux 8" )
      ICUB_REPO_STRING="deb http://www.icub.org/debian jessie contrib/science"
      ;;
    "Debian GNU/Linux 9" )
      ICUB_REPO_STRING="deb http://www.icub.org/debian stretch contrib/science"
      ;;
    * )
      exit_err "ERROR : usupported or not recognized issue string $DISTRO_ISSUE_NAME in /etc/issue"
      ;;
  esac
  echo "$ICUB_REPO_STRING" > /etc/apt/sources.list.d/icub.list
  apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 57A5ACB6110576A6
  log "Installing icub common packages.."
  ICUB_COMMON_PACKAGES="icub-common"
  install_packages "$ICUB_COMMON_PACKAGES"
}

insert_text ()
{
  if [ "$1" == "" ]; then
    return
  fi
  cat << 'EOF' >> $1
#Load the iCub custom bashrc
if [ "$HOME" != "" ]; then
  ICUBRC_FILE="${HOME}/.bashrc_iCub"
else
  ICUBRC_FILE="/home/icub/.bashrc_iCub"
fi
if [ -f "$ICUBRC_FILE" ]; then
  source "$ICUBRC_FILE"
fi
EOF

}

setup_user_env()
{
  log "Installing $PROVISIONED_USERNAME user enviroment.."
  BASHRC_FILE="/home/${PROVISIONED_USERNAME}/.bashrc"
  if [ ! -f "$BASHRC_FILE" ]; then
    warn ".bashrc file not found in user $PROVISIONED_USERNAME home directory, using /etc/skel/.bashrc instead"
    BASHRC_FILE="/etc/skel/.bashrc"
  fi
  WHERE_TO_INSERT="# If not running interactively, don't do anything"
  BASHRC_TEMPIFLE="${BASHRC_FILE}.new"
  if [ -f "$BASHRC_TEMPIFLE" ]; then
   rm -f $BASHRC_TEMPIFLE
  fi
  while IFS='' read -r line; do
    if [ "$line" == "$WHERE_TO_INSERT" ]; then
      insert_text $BASHRC_TEMPIFLE
    fi
    printf "%s\n" "$line" >> $BASHRC_TEMPIFLE
  done < "$BASHRC_FILE"
  if [ -f "$BASHRC_FILE" ]; then
    mv "$BASHRC_FILE" "${BASHRC_FILE}.old"
  fi
  mv "$BASHRC_TEMPIFLE" "$BASHRC_FILE"
  chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME "$BASHRC_FILE"
  ICUBRC_FILE="/home/${PROVISIONED_USERNAME}/.bashrc_iCub"
  if [ "$SUPERBUILD" == "true" ]; then
    wget $BASHRC_ICUB_SUPERBUILD_URL --quiet --output-document="$ICUBRC_FILE"
  else
    wget $BASHRC_ICUB_URL --quiet --output-document="$ICUBRC_FILE"
  fi
  chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME "$ICUBRC_FILE"
}

install_binaries()
{
  log "Installing binaries.."
  install_packages "$ICUB_BINARIES"
}

install_sources()
{
  log "Installing sources.."
  if [ "$SERVER_LAPTOP" = "true" ]; then
    _SOURCES_PATH="/exports/code"
    _SOURCES_PATH2="/usr/local/src/robot"
    _SOURCES_PATH3="/exports/local_yarp"
    _SOURCES_PATH4="/home/icub/.local/share"
    if [ ! -f "$_SOURCES_PATH" ]; then
      mkdir -p $_SOURCES_PATH
      chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $_SOURCES_PATH -R
    fi
    cd $_SOURCES_PATH
    for p in $ICUB_SOURCES_REPOS ; do
      if [ ! -d $p ]; then
        git clone "${ICUB_SOURCES_URL}/${p}.git"
      else
        cd $p
        git fetch
        cd ..
      fi
      chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $p -R
    done
    if [ ! -f "$_SOURCES_PATH2" ]; then
      mkdir -p $_SOURCES_PATH2
      chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $_SOURCES_PATH2 -R
      ln -s $_SOURCES_PATH/* $_SOURCES_PATH2
    fi
    if [ ! -f "$_SOURCES_PATH3" ]; then
      mkdir -p $_SOURCES_PATH3
      chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $_SOURCES_PATH3 -R
    fi
    if [ ! -f "$_SOURCES_PATH4" ]; then
      mkdir -p $_SOURCES_PATH4
      chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $_SOURCES_PATH4 -R
       ln -s $_SOURCES_PATH3 $_SOURCES_PATH4
    fi
  else
    _SOURCES_PATH="/usr/local/src/robot"
	if [ ! -f "$_SOURCES_PATH" ]; then
	  mkdir -p $_SOURCES_PATH
	  chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $_SOURCES_PATH -R
	fi
	cd $_SOURCES_PATH
	for p in $ICUB_SOURCES_REPOS ; do
	  if [ ! -d $p ]; then
	   git clone "${ICUB_SOURCES_URL}/${p}.git"
	  else
	    cd $p
	    git fetch
	    cd ..
	  fi
	  chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $p -R
	done
  fi
}

install_superbuild()
{
  _SOURCES_PATH="/usr/local/src/robot"
  if [ ! -f "$_SOURCES_PATH" ]; then
    mkdir -p $_SOURCES_PATH
    chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $_SOURCES_PATH -R
  fi
  cd $_SOURCES_PATH
  if [ ! -d $ROBOTOLOGY_SUPERBUILD ]; then
    git clone "${ICUB_SOURCES_URL}/${ROBOTOLOGY_SUPERBUILD}.git"
  else
    cd $ROBOTOLOGY_SUPERBUILD
    git fetch
    cd ..
  fi
  chown $PROVISIONED_USERNAME:$PROVISIONED_USERNAME $ROBOTOLOGY_SUPERBUILD -R
}

add_exports()
{
   echo "/exports/code 10.0.0.0/255.255.255.0(rw,sync,no_root_squash,no_subtree_check)" >> /etc/exports
   echo "/exports/local_yarp 10.0.0.0/255.255.255.0(rw,sync,no_root_squash,no_subtree_check)" >> /etc/exports
}

add_hosts()
{
   echo "10.0.0.1	icubsrv" >> /etc/hosts
   echo "10.0.0.2	icub-head" >> /etc/hosts
}

main()
{
  setup_system_packages
  setup_icub_packages
  if [ "$USE_ICUB_BINARIES" == "true" ]; then
    install_binaries
  fi
  if [ "$USE_ICUB_SOURCES" == "true" ]; then
    if [ "$SUPERBUILD" == "true" ]; then
      install_superbuild
    else
      install_sources
    fi
  fi

  if [ "$SERVER_LAPTOP" == "true" ]; then
    add_exports
    add_hosts
  fi
  set_sudo_guid
  setup_user_env
}

parse_opt "$@"
init
main
fini
exit 0
