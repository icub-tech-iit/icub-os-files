#!/bin/sh -e
# insert sound driver modules
modprobe snd-pcm-oss
modprobe snd-mixer-oss

# Distinguish between MIC input and output amplifier (DAC): for standard pc104 they are
# both inside the integrated sound card, for new hardware version they are on 2 external
# usb cards (no integrated audio cards anymore).

if [ -d /proc/asound/card1 ]
then
   echo "2 audio devices found"
   DAC_number=$(cat /proc/asound/cards | awk '/\[DAC/{print $1}')
   dongle_number=$(cat /proc/asound/cards | awk '/\[Dongle/{print $1}')
   echo "DAC number ${DAC_number}, dongle number ${dongle_number} "

   amixer -c ${DAC_number} sset PCM,0 80%,80% unmute cap

   amixer -c ${dongle_number} sset Mic,0 80%

else
   echo "1 audio devices found, using usual config"
   # Configure volume parameters
   amixer -c 0 sset PCM,0 80%,80% unmute cap
   amixer -c 0 sset Line,0 80%,80% unmute cap
   amixer -c 0 sset Capture,0 80%,80% unmute cap
fi
exit 0
