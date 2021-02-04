#! /bin/bash
# Mount remote filesystems
MOUNTS_FILE="/etc/rc.iCub.d/mounts.list"
PACKETS_PER_PING=10
MAX_RETRIES_N=3
SLEEP_TIME_SEC=5
PING_DEST="10.0.0.1"
echo "Mounting remote filesystems.."
COUNT=0
RETVAL=1
while [ "$COUNT" -lt "$MAX_RETRIES_N" ] && [ "$RETVAL" != "0" ]
do
  ping -c $PACKETS_PER_PING -q $PING_DEST
  RETVAL=$?
  if [ "$RETVAL" != "0" ]
  then
    echo "Failed to contact $PING_DEST, sleeping for $SLEEP_TIME_SEC seconds.."
    sleep $SLEEP_TIME_SEC
  fi
  (( COUNT += 1 ))
done
if [ "$RETVAL" != "0" ]
then
  echo " Failed: impossible to contact $PING_DEST"
  exit 1
fi
if [ ! -f "$MOUNTS_FILE" ]
then
  echo " Failed: remote mounts file $MOUNTS_FILE not found"
  exit 1
fi
mapfile -t MOUNTS_ARRAY  < $MOUNTS_FILE
for i in $(seq ${#MOUNTS_ARRAY[*]}); do
  if [ "${MOUNTS_ARRAY[$i]}" != "" ] && [ "${MOUNTS_ARRAY[$i]:0:1}" != "#" ]
  then
    SOURCE=$( echo ${MOUNTS_ARRAY[$i]} | awk '{ print $1}' )
    MOUNTPOINT=$( echo ${MOUNTS_ARRAY[$i]} | awk '{ print $2}' )
    FS_TYPE=$( echo ${MOUNTS_ARRAY[$i]} | awk '{ print $3}' )
    OPTIONS=$( echo ${MOUNTS_ARRAY[$i]} | awk '{ print $4}' )
    COUNT=0
    RETVAL=1
    while [ "$COUNT" -lt "$MAX_RETRIES_N" ] && [ "$RETVAL" != "0" ]
    do
      sleep $SLEEP_TIME_SEC
      echo "Mounting $SOURCE"
      mount -t $FS_TYPE -o $OPTIONS $SOURCE $MOUNTPOINT
      RETVAL=$?
      if [ "$RETVAL" != "0" ]
      then
        sleep $SLEEP_TIME_SEC
      fi 
      (( COUNT += 1 ))
    done 
  fi
done

exit 0
