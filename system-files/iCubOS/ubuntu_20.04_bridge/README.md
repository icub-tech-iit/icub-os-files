# iCub-head-cam (Xavier NX) network configuration

### eth0: (connected to icub-head)

IP: 10.0.0.3/24\
gateway: 10.0.0.1\
DNS: 10.0.0.1

### enP5p1s0: (only for troubleshoot)

IP: 10.10.10.10/24\
no gateway\
no DNS

# How to remove the bridge and use wireless adapter on Xavier NX

First you need to connect to the robot via ethernet setting an IP on the network 10.10.10.0/24 (e.g.: 10.10.10.5/24), now you should be able to reach the Xavier NX on 10.10.10.10/24. And of course have plugged the Wireless adapter in the Xavier.
Otherwise if you have the possibility to have a monitor and an usb keyboard connected to the icub-head you can direcetly perform the operations. 
The same thing is valid if the icub-head is connected to the wifi and it is reachable.
### connect to Xavier and jump on icub-head:

`ssh icub@10.10.10.10`\
`ssh icub@10.0.0.2`
 
 ### Once on the icub-head you need to modify the `netplan` configuration by replacing the symlink 50-icub.yaml with the desired configuration:
 
 `cd /etc/netplan`\
 `ls -la`
 at this point you should see the symlink `50-icub.yaml` pointing to the bridge configuration `50-icub-head-bridge.yaml.notload` replace the symlink with the desired configuration:
 
 e.g:
 `sudo ln -sf 50-icub-head-static.yaml.notload 50-icub.yaml`\
 check the configuration, rember to verify SSID and password of the wifi and set the correct ones
 
 `cat 50-icub.yaml`
 ```  version: 2
  ethernets:
    enp4s0:
      dhcp4: no
      dhcp6: no
      addresses: [10.0.2.2/24]
      optional: true
    enp5s0:
      dhcp4: no
      dhcp6: no
      addresses: [10.0.1.104/24]
      optional: true
  wifis:
    wlp3s0:
      dhcp4: yes
      dhcp6: no
      addresses: [10.0.0.2/24]
      access-points:
            "iCub3_000-Wifi":
              password: "iCubWifi"
      optional: true
 ```
 now verify if the configuration is correct and it doesn't have errors
 
 `sudo netplan generate`
 
 if the output it is clear you can go on.
 
 ### Disable the scripts for bridge and ping
 
 ``sudo systemctl disable bridge.service``
 
 ``sudo systemctl disable ping-everyone.service``
 
 return on Xavier, you'll reboot everything later for the configuration to take effect.
 
 `crtl+D` or `exit` 
 
 disable the `ping-everyone.service`  also on the Xavier:
 
 ``sudo systemctl disable ping-everyone.service``
 
 check the name of the wireless interface with `ip a`
 
 enter in the Network Manager UI and start configuring the wifi and the wired connection
 
 `sudo nmtui`
 
 select `edit a connection` using `arrows` and `spacebar`
 
 ![nmtui1](https://user-images.githubusercontent.com/74544142/194637528-1d322399-0d06-48d1-a6d0-40f6618cfb45.PNG)\
 
 select `add`and then `wifi`\
 
![nmtui2](https://user-images.githubusercontent.com/74544142/194637941-f0fe3ca0-7883-428e-b198-6d17513273ab.PNG)

fill the field as in the pictures, rememner to put the right interface name in `device`\

 ![nmtui3](https://user-images.githubusercontent.com/74544142/194638765-e3389780-dfb0-4df4-ac95-4d61f4cf3676.PNG)
 
 set the IP configuration to `manual` and the address to 10.0.0.3/24 like in the picture

![nmtui4](https://user-images.githubusercontent.com/74544142/194638876-249b785d-5031-41e4-9a5b-882df60f0ffd.PNG)

Scroll down to `OK` and then exit. 

Use the same procedure to edit the `Wired Connection 1` and set the IP address of the `eth0` of the Xavier to 10.0.2.3.

### Now you can reboot the whole robot and then you should reach 10.0.0.2 and 10.0.0.3 through the Wifi connection.


