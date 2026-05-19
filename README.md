# rmw_tickle
ROS 2 RMW implementation for TickLE, fastest ROS 2 middleware optimized for 10Base-T1S. 

## Features
- Supports `rclpy` API in application
- Topic (pub/sub)
- Typesupport implementation

## Installtaion
### Requirements
- Ubuntu
- ROS 2 Jazzy

### ROS 2
To install ROS 2 Jazzy, Follow [ROS 2 Installation Guide](https://docs.ros.org/en/jazzy/Installation/Alternatives/Ubuntu-Development-Setup.html) until **Install additional RMW implementations (optional)** and return.

### Add TickLE and packages
```bash
cd ~/ros2_jazzy/src
git clone https://github.com/tsnlab/rmw_tickle
cd rmw_tickle
git checkout demo # branch name containing this README.md
git submodule update --init
```

### Build
Move to root of ROS 2 workspace and build.
```bash
cd ~/ros2_jazzy
```
#### Build all packages
```bash
colcon build
```
#### Build packages necessary to run example
```bash
colcon build --packages-up-to ros2run ros2interface demo_nodes_py
```
### Source setup script
Run following command whenever build is complete or a new terminal is opened. Or add it to shell init file like `.bashrc`.
```bash
source ~/ros2_jazzy/install/setup.bash
```

### Select RMW
Default RMW is `rmw_fastrtps_cpp`. To select RMW,
```bash
export RMW_IMPLEMENTATION=<rmw-name>
```
`<rmw-name>` is name of RMW such as `rmw_tickle`, `rmw_fastrtps_cpp`.


## Run demo
#### Network namespace
By default, network namespaces are required for TickLE nodes to work.

To create the network namespaces,
```bash
make -C ~/ros2_jazzy/src/rmw_tickle/rmw_tickle/tickle createns
```
To remove,
```bash
make -C ~/ros2_jazzy/src/rmw_tickle/rmw_tickle/tickle deletens
```
#### Publisher
```bash
sudo ip netns exec ns1 bash
source install/setup.bash
export RMW_IMPLEMENTATION=rmw_tickle
ros2 run demo_nodes_py talker
```
#### Subscriber
Execute subscriber in a new terminal.
```bash
sudo ip netns exec ns2 bash
source install/setup.bash
export RMW_IMPLEMENTATION=rmw_tickle
ros2 run demo_nodes_py listener
```

## Application example
Refer to following links to create custom message.
- [ROS 2 Creating custom msg files](https://docs.ros.org/en/foxy/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.html)
- [ROS 2 Message](https://docs.ros.org/en/jazzy/Concepts/Basic/About-Interfaces.html)

Refer to following links to create application.
- [ROS 2 Writing Pub/Sub application](https://docs.ros.org/en/foxy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Publisher-And-Subscriber.html)

### Message
Ues prepared message package: [my_message](examples/my_message)
```bash
cd ~/ros2_jazzy
colcon build --packages-select my_message
```

#### Message definition
Message is defined at [MyMessage.msg](examples/my_message/msg/MyMessage.msg)
```
uint8 header
uint64 body
```

#### Build
```bash
cd ~/ros2_jazzy
colcon build --packages-select my_message
source install/setup.bash
```

#### Verify
Check if message is accessible.
```bash
ros2 interface list | grep custom_message
```
Output should look like
```
    custom_message/msg/Simple
```


### Application
Ues prepared application package: [my_app](examples/my_app)

List of example sources
- [publisher.py](examples/my_app/my_app/publisher.py)
- [subscriber.py](examples/my_app/my_app/subscriber.py)

#### Build
```bash
cd ~/ros2_jazzy
colcon build --packages-select my_app
```

#### Publisher
```bash
sudo ip netns exec ns1 bash
source install/setup.bash
export RMW_IMPLEMENTATION=rmw_tickle
ros2 run tickle_app_py publisher
```
#### Subscriber
Execute subscriber in a new terminal.
```bash
sudo ip netns exec ns2 bash
source install/setup.bash
export RMW_IMPLEMENTATION=rmw_tickle
ros2 run tickle_app_py subscriber
```

## License
GPLv3 or proprietary license on request
