# Guide to configure bonding active-backup using `ifenslave`:

### Official documentation at https://help.ubuntu.com/community/UbuntuBonding

### Installation

`sudo apt-get install ifenslave`

### Edit your /etc/modules configuration:

`sudo vi /etc/modules`

### Ensure that the bonding module is loaded:

>#/etc/modules: kernel modules to load at boot time.\
>#This file contains the names of kernel modules that should be loaded\
>#at boot time, one per line. Lines beginning with "#" are ignored.
>
>loop\
>lp\
>rtc\
>bonding

### Step 2: Configure network interfaces
Ensure that your network is brought down:

`sudo stop networking`

Then load the bonding kernel module:

`sudo modprobe bonding`

Now you are ready to configure your NICs.

### Edit your interfaces configuration:

 `sudo vi /etc/network/interfaces`
 
## For example, to combine enp3s0 and wlp2s0 as slaves to the bonding interface bond0 using a simple active-backup setup, with eth0 being the primary interface:


>#eth0 is manually configured, and slave to the "bond0" bonded NIC\
>auto enp3s0\
>iface enp3s0 inet manual\
>    bond-master bond0\
>    bond-primary enp3s0
>
>#wlp2s0 ditto, thus creating a 2-link bond. Since wlp2s0 it is a wifi interface were gonne use wpa-conf to connect to our wifi network.\
>auto wlp2s0\
>iface wlp2s0 inet manual\
>    bond-master bond0\
>    wpa-conf /etc/wpa_supplicant/wpa_supplicant.conf\
>#bond0 is the bonding NIC and can be used like any other normal NIC.\
>#bond0 is configured using static network information.\
>auto bond0\
>iface bond0 inet static\
>    address 10.0.0.2\
>    gateway 10.0.0.1\
>    netmask 255.255.255.0\
>    bond-mode active-backup\
>    bond-miimon 100\
>    bond-slaves enp3s0 wlp2s0

The bond-primary directive, if needed, needs to be part of the slave description (eth0 in the example), instead of the master. Otherwise it will be ignored.

For bonding-specific networking options please consult the documentation available at [BondingModuleDocumentation.](https://www.kernel.org/doc/Documentation/networking/bonding.txt)

### Finally, bring up your network again:

`sudo start networking`
