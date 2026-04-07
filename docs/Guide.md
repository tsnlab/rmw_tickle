# rmw_tickle Usage Guide
This guide shows how to integrate TickLE and ROS 2 and run examples.

## Setup
### Requirements
- Ubuntu
- ROS 2 Jazzy

### Download ROS 2 source and install dependencies
Follow [ROS 2 Installation Guide](https://docs.ros.org/en/jazzy/Installation/Alternatives/Ubuntu-Development-Setup.html) until **Install additional RMW implementations (optional)** and return.

### Add RMW and typesupport
RMW and typesupport are mandatory packages that sits between middleware and ROS 2.


This repository has:
- `rmw_tickle`
- `rosidl_typesupport_tickle_c`

Add packages before building ROS 2.
```
cd ~/ros2_jazzy/src
git clone https://github.com/tsnlab/rmw_tickle
cd rmw_tickle
git checkout typesupport-dev
git submodule update --init
```

### Build ROS 2
Move to root of ROS 2 workspace and build.
```
cd ~/ros2_jazzy
```
#### Build all packages
```
colcon build
```
#### Build packages necessary to run example
```
colcon build --packages-up-to ros2run ros2interface demo_nodes_py
```
### Source setup script
Packages are installed in `~/ros2_jazzy/install` and environment variables should be updated accordingly. Run following command whenever build is complete or a new terminal is opened. Or you can add it to shell initialization file such as `~/.bashrc`.
```
source ~/ros2_jazzy/install/setup.bash
```

### Setup packages

#### Package for message
To define your own message, create package for message definition.
```
cd ~/ros2_jazzy/src
ros2 pkg create --build-type ament_cmake custom_message
cd custom_message
mkdir msg
```
Add custom message file at `msg` directory. Refer to [ROS 2 writing message definition](https://docs.ros.org/en/jazzy/Concepts/Basic/About-Interfaces.html) for detail.


Message definition file example:


**Simple.msg**
```
uint8 header
uint64 body
```

**CMakeList.txt**

Open `CMakeList.txt` and add following lines below `find_package(ament_cmake REQUIRED)`.
```
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
    "msg/Simple.msg"
)
```
**packages.xml**


Open `packages.xml` and add following lines below `<buildtool_depend>ament_cmake</buildtool_depend>`.
```
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```
**Build**
```
cd ~/ros2_jazzy
colcon build --packages-select custom_message
```
After build is done,
```
source install/setup.bash
```

**Check**


Check if message is accessible by ROS.
```
ros2 interface list | grep custom_message
```
Output should look like
```
    custom_message/msg/Simple
```
#### Package for example application
Create ROS package for python project.
```
cd ~/ros2_jazzy/src
ros2 pkg create --build-type ament_python tickle_app_py
cd tickle_app_py
```
**packages.xml**


Add following lines below `<license>` element.
```
<exec_depend>rclpy</exec_depend>
<exec_depend>custom_message</exec_depend>
```
**setup.py**


Modify `entry_points`.
```
entry_points={
    'console_scripts': [
        'publisher = tickle_app_py.publisher:main',
        'subscriber = tickle_app_py.subscriber:main'
    ],
},
```
`publisher` and `subscriber` in lhs are name of entry point. It is used when executing the command like `ros2 run tickle_app_py publisher`.


`tickle_app_py` is name of package.


`publisher` and `subscriber` in rhs are name of python script.


`main` is name of function to be called in the python script.


## Example code
### RCL
There are two ROS client libraries(RCL) maintained by ROS 2 team, `rclcpp` and `rclpy`. ROS application should be written in either C++ to use `rclcpp` or Python to use `rclpy`. Other option is using community maintained client libraries such as `rclc` for C, `rclrs` for Rust, etc.


### Language choice
Currently, TickLE provide only C typesupport. `rclpy` uses C typesupport and `rclcpp` uses C++ typesupport, so `rclpy` is only option if community maintained libraries are not considered. Example will be written in Python.


### Write example code
Move to directory where python scripts are stored.
```
cd tickle_app_py
```


**publisher.py**
```
import rclpy
from rclpy.node import Node
from custom_message.msg import Simple
from time import time_ns

def main():
    rclpy.init(args=None)
    node = Node('publisher_node')
    publisher = node.create_publisher(Simple, 'simple_topic', 10)
    def pub_callback():
        msg = Simple()
        msg.header = 1
        msg.body = time_ns()
        publisher.publish(msg)
        node.get_logger().info(f'published time={msg.body}')
        
    timer = node.create_timer(1.0, pub_callback)
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```
**subscriber.py**
```
import rclpy
from rclpy.node import Node
from custom_message.msg import Simple

def main():
    rclpy.init(args=None)
    node = Node('subscriber_node')
    def sub_callback(msg: Simple):
        if msg.header == 1:
            node.get_logger().info(f'time={msg.body}')
        else:
            node.get_logger().info(f'invalid data')
    subscriber = node.create_subscription(Simple, 'simple_topic', sub_callback, 10)
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
```

#### Build
```
cd ~/ros2_jazzy
colcon build --packages-select tickle_app_py
```

## Run example application
### RMW selection
Default RMW is `rmw_fastrtps_cpp`. To select RMW,
```
export RMW_IMPLEMENTATION=<rmw-name>
```
`<rmw-name>` is name of RMW such as `rmw_tickle`, `rmw_fastrtps_cpp`, `rmw_zenoh`, etc.
### Network namespace
Default configuration of TickLE requires nodes to be executed in separate network namespace.

To create the network namespace with default preset,
```
cd ~/ros2_jazzy/src/rmw_tickle/rmw_tickle/tickle
make createns
```
To remove,
```
make deletens
```

### Publisher
```
sudo ip netns exec ns1 bash
source install/setup.bash
export RMW_IMPLEMENTATION=rmw_tickle
ros2 run tickle_app_py publisher
```
Publisher node output:
```
[2026-04-08 14:03:01] [INFO] node_id=1
[2026-04-08 14:03:01] [INFO] Node open at 8282
[INFO] [1775624581.442785008] []: Created TickLE node: /publisher_node
[INFO] [1775624581.561424871] []: Created TickLE service: ...
[INFO] [1775624582.573426737] [publisher_node]: published time=1775624582565595329
[INFO] [1775624583.565765638] [publisher_node]: published time=1775624583565442748
...
```
### Subscriber
Execute subscriber in a new terminal.
```
sudo ip netns exec ns2 bash
source install/setup.bash
export RMW_IMPLEMENTATION=rmw_tickle
ros2 run tickle_app_py subscriber
```
Publisher node output:
```
[2026-04-08 14:03:02] [INFO] node_id=2
[2026-04-08 14:03:02] [INFO] Node open at 8282
[INFO] [1775624581.442785008] []: Created TickLE node: /subscriber_node
[INFO] [1775624581.561424871] []: Created TickLE service: ...
[INFO] [1775624582.573838782] [subscriber_node]: time=1775624582565595329
[INFO] [1775624583.565964993] [subscriber_node]: time=1775624583565442748
```
They must have different `node_id`. Check if `time` values are equal.

