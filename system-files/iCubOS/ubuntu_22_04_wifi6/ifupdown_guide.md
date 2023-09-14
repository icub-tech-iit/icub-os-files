# How to uninstall `netplan.io` and install `ifupdown` on Ubuntu 22.04 LTS:

#### First install `ifupdown` 

`sudo apt update`\
`sudo apt install ifupdown`

#### Configure your `/etc/network/interfaces` file with configuration:

> source /etc/network/interfaces.d/*
>
>#The loopback network interface\
>auto lo\
>iface lo inet loopback\
>
>allow-hotplug enp0s3\
>auto enp0s3\
>iface enp0s3 inet static\
>  address 10.0.0.1\
>  netmask 255.255.255.0\
>  broadcast 10.0.0.255\
>  gateway 10.0.0.1\
>  dns-nameservers 10.0.0.1

#### then **stop** and **disable** `netplan` services:

>  systemctl stop systemd-networkd.socket \
>  systemctl stop systemd-networkd \
>  systemctl stop networkd-dispatcher\
>  systemctl stop systemd-networkd-wait-online\
>  systemctl disable systemd-networkd.socket \
>  systemctl disable systemd-networkd \
>  systemctl disable networkd-dispatcher\
>  systemctl disable systemd-networkd-wait-online

#### then **mask** the services so they wont pop up anymore :
  
>  systemctl mask systemd-networkd.socket \
>  systemctl masksystemd-networkd \
>  systemctl masknetworkd-dispatcher\ 
>  systemctl mask systemd-networkd-wait-online

#### Now it's time to uninstall `netplan`

> sudo apt purge netplan\
> sudo apt autoremove

#### Enable `Networking` service:

> systemctl enable networking\
> systemctl restart networking

#### Install `resolvconf` in order to make resolution name work:

`sudo apt install resolvconf`

#### *stop* and *disable* `systemd-resolved` service:

>sudo systemctl stop systemd-resolved\
>sudo systemctl disable systemd-resolved

#### well done!

