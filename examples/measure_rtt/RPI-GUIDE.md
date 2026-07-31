# RPi + rmw_tickle setup guide
1. Prerequisite
2. Install OS in microSD
3. Install ROS 2 in RPi
4. Install TickLE
5. Setup network

## Prerequisite
- 2 or more Raspberry Pi 4 model B (or 5)
- micro SD card per 1 RPi
- RPi Imager (>=2.0.0)

## Install OS in microSD
Open RPi Imager in host machine.
- Device: Raspberry Pi 4 model B (substitute 5 for 4 if RPi 5 is used)
- OS: Other general-purpose OS - Ubuntu - Ubuntu Server 24.04.4 LTS
- Configure account, Wi-Fi and enable SSH.
- Insert the microSD card to RPi. Rest of the steps are done in RPi.

## Install ROS 2 in RPi
```bash
sudo apt update
sudo apt install software-properties-common
sudo add-apt-repository universe

sudo apt update && sudo apt install curl -y
export ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | grep -F "tag_name" | awk -F'"' '{print $4}')
curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo ${UBUNTU_CODENAME:-${VERSION_CODENAME}})_all.deb"
sudo dpkg -i /tmp/ros2-apt-source.deb

sudo apt update && sudo apt install ros-dev-tools
sudo apt upgrade
sudo apt install ros-jazzy-ros-base
source /opt/ros/jazzy/setup.bash
```

## Install TickLE
### Resolve ROS 2 application dependencies
Some ROS 2 message packages must be re-built to use newly added TickLE typesupport.
```bash
cd $HOME
mkdir -p ros2/src
cd ..

# clone ROS 2 packages
git clone https://github.com/tsnlab/rmw_tickle.git --recurse-submodules -b demo
git clone https://github.com/ros2/rcl_interfaces.git -b jazzy
git clone https://github.com/ros2/common_interfaces.git -b jazzy
git clone https://github.com/ros2/rosidl.git -b jazzy
mv rosidl/rosidl_typesupport_interface .
rm -r rosidl
cd ..

# build and resolve dependency repeatedley
source /opt/ros/jazzy/setup.bash
colcon build --packages-select rosidl_typesupport_interface rosidl_typesupport_tickle_c rmw_tickle
source install/setup.bash # source is necessary after build is done
colcon build --packages-select builtin_interfaces service_msgs std_msgs
source install/setup.bash
colcon build --packages-select rcl_interfaces type_description_interfaces
source install/setup.bash
colcon build --packages-select rosgraph_msgs
source install/setup.bash
```

## Setup network
### NetworkManager
For easier network setup, use network-manager instead of netplan.
```bash
sudo apt install network-manager
```
Add `renderer` field under `network` in `/etc/netplan/50-cloud-init.yaml`
```bash
network:
  renderer: NetworkManager
...
```
Apply configuration
```bash
sudo netplan apply
```
Check NetworkManager status
```bash
sudo systemctl status NetworkManger
```

### IP address
IP address should be assigned for each RPI.

#### TUI method using nmtui
```bash
sudo nmtui
```
1. Select "Edit a connection"
2. Select eth1
3. IPv4 configuration -> Manual
4. Addresses -> `192.168.10.<2~254>/24`
5. Return to option menu
6. Select "Activate a connection"
7. Deactivate and activate eth1
8. Select OK to exit

#### CLI method using nmcli
```bash
sudo nmcli connection modify "Wired connection 2" ipv4.method manual ipv4.addr "192.168.10.<2~254>/24" ipv6.method disable
sudo nmcli connection up ifname eth1
sudo systemctl restart NetworkManager
```

Check 10Base-T1S LEDs are on and ping to ensure if connection is normal.
```bash
ping 192.168.10.<master_id>/24
ping 192.168.10.<slave_id>/24
```
