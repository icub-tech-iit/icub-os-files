# Steps to correctly setup the network once in Xprize:

1. On `icub-head` modify the file `/etc/netplan/50-icub.yaml` with the correct SSID and Password of the Xprize Network and change the IP Address of the Bridge with the one from the Xprize network 
```
network:
  version: 2
  renderer: networkd
  ethernets:
    enp4s0:
      dhcp4: no
      dhcp6: no
    enp5s0:
      dhcp4: no
      dhcp6: no
      addresses: [10.0.1.104/24]
      optional: true
  wifis:
    wlp3s0:
      dhcp4: no
      dhcp6: no
      access-points:
        "SSID Xprize Wifi":
          password: "password Xprize wifi"
  bridges:
    br0:
#set a macaddress for the bridge interface    
      macaddress: 68:54:5a:ce:d7:c3
      dhcp4: no
      dhcp6: no
      addresses: [IP you've chosen from xprize network\24]
      gateway4: [Gateway if needed always from xprize network]
      interfaces: [wlp3s0, enp4s0]
      nameservers:
        addresses: [put here the address of the DNS if needed]
```
Check if  everything OK with `netplan generate`.
2. Modify the variables `bridge_ip` and `xavier_ip` the script `/etc/rc.iCub.d/bridge.sh`  with the desired IPs of the Xprize network.
```
bridge_ip="put here the IP of the Xprize network that you've chosen for the bridge"
wireless_if_mac="this is already set"
xavier_ip="put here the IP of the Xprize network that you've chosen for the Xavier NX"
xavier_mac="this is already set"
wifi_if="wlp3s0"
```

3. Modify the script `/usr/local/source/robot/ping-everyone.sh` changing the argument of the `nmap`  with the new network.
