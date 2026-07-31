# TickLE Testbed guide for ROS 2 middleware evaluation
This package includes RTT application and test script.
* RTT application applies to remote RPi
* Test script applies to host

## RTT application (remote RPi)
Before using RTT application, install OS and ROS 2 in RPi. refer to [RPI-GUIDE.md](RPI-GUIDE.md)
Then, proceed to following procedure.

### How to build
```
cd <ros2-root>
source install/setup.bash
colcon build --packages-select rtt_messages measure_rtt
source install/setup.bash
```

### How to run
```
# Ping node
ros2 run measure_rtt ping -i <interval in ms> -c <ping count> -s <payload size>

# Pong node
ros2 run measure_rtt pong
```
Ping node create CSV file where timestamp, RTT fields are recorded.

## Test script (Host)

### Configure remote IP addresses
1. open script([resource/test_and_draw.sh](resource/test_and_draw.sh)) with text editor
2. assign destination (pair of username and IP address) of ping node `MASTER_DESTINATION=<username>@<ping-node-address>`
3. assign destination of pong node `SLAVE_DESTINATION_LIST=<username>@<pong-node-address>`. more than one address in `SLAVE_DESTINATION_LIST` may be needed for measuring throughput, but it is not covered in this guide.
4. set path where CSV will be stored. `CSV_PATH=<csv-path>`. default is `$HOME/ros2/data`
5. `RMW_LIST` is list of RMW. comment/uncomment RMW name as needed
6. `INTERVAL_LIST` is list of intervals (X-axis of RTT graph)
7. `PAYLOAD_SIZE_LIST` is list of payload size
8. `NUM_MESSAGES` is number of ping messages

### SSH remote connection
Transfer SSH public key to remote RPi to login without password
```
ssh-copy-id <username>@<ping-node-address>
ssh-copy-id <username>@<pong-node-address>
```

### How to run
```
cd resource
bash test_and_draw.sh RTT
```
After measure is done, graph will be generated.
