#!/usr/bin/env bash

# This script run ROS 2 command to evaluate performance of middleware and plot measurement result
# Test parameters: time interval (millisecond), payload size(byte) and number of messages (mimic options of ping: -i, -s, -c)
# Interval: 1000, 100, 10, 1 ms
# Payload size: 8, 16, 32, 64, 128, 256, 512, 1024, Max bytes

##################### Configuration #####################
MASTER_DESTINATION="tsnlab@192.168.1.121"

SLAVE_DESTINATION_LIST=(
    tsnlab@192.168.1.229
#   harim@localhost
)

SSH_COMMAND='ssh ${DESTINATION}'
SCP_COMMAND='scp ${DESTINATION}'
CSV_PATH="$HOME/ros2_jazzy/data/$(date +%g%m%d_%H%M%S)"

##################### Constants #####################
RMW_LIST=(
    "rmw_tickle"
    "rmw_fastrtps_cpp"
    "rmw_cyclonedds_cpp"
    "rmw_zenoh_cpp"
)

# unit: millisecond
INTERVAL_LIST=(
#   1 10 100 1000
    1 10 50 100 200
)

# unit: byte
PAYLOAD_SIZE_LIST=(
#   8 16 32 64 128 256 512 1024 # TODO: add payload size for MTU 
    8 16 32 64
)

# unit: seconds
TEST_DURATION=10

##################### Initialize #####################
# check if ROS 2 is enabled
if [[ ! $(export | grep "ROS_VERSION") ]]; then
    echo "source ROS 2 setup file"
    exit 1
fi

# Choose what to measure
if [[ "${1}" != "RTT" && "${1}" != "THROUGHPUT" ]]; then
    echo "first argument must be either \"RTT\" or \"THROUGHPUT\""
    exit 1
fi

if [[ ! -d ${CSV_PATH} ]]; then
    mkdir -p ${CSV_PATH}
fi

MEASURE="$1"

ROS_COMMAND=""
ROS_ARGUMENT=""
MASTER_COMMAND=""
SLAVE_COMMAND=""
CSV_FILENAME=""

if [[ ${MEASURE} == "RTT" ]]; then
    ROS_COMMAND="ros2 run measure_rtt "
    MASTER_COMMAND="ping"
    SLAVE_COMMAND="pong"
    CSV_FILENAME_TEMPLATE='rtt_${MW}_i${INTERVAL}_s${PAYLOAD_SIZE}_c${NUM_MESSAGES}.csv'
else
    ROS_COMMAND="ros2 run measure_throughput "
    MASTER_COMMAND="pub"
    SLAVE_COMMAND="sub"
    CSV_FILENAME_TEMPLATE=""
fi

ROS_ARGUMENT_TEMPLATE=' -c ${NUM_MESSAGES} -i ${INTERVAL} -s ${PAYLOAD_SIZE}'

##################### Functions #####################
function run_test() {
    MW=${1}

    for INTERVAL in ${INTERVAL_LIST[@]}; do
        for PAYLOAD_SIZE in ${PAYLOAD_SIZE_LIST[@]}; do
            echo ""
            echo "middleware=${MW}, interval=${INTERVAL}, payload size=${PAYLOAD_SIZE}"
            echo ""
            COMMAND="${SSH_COMMAND} "\''export RMW_IMPLEMENTATION='${MW}'; source ${HOME}/ros2_jazzy/install/setup.bash;'\'${ROS_COMMAND}
            for DESTINATION in ${SLAVE_DESTINATION_LIST[@]}; do
                eval ${COMMAND} ${SLAVE_COMMAND}&
            done
            sleep 2
            DESTINATION=${MASTER_DESTINATION}
#           NUM_MESSAGES=$((1000 * TEST_DURATION / INTERVAL))
            NUM_MESSAGES=1000
            ROS_ARGUMENT=$(eval echo ${ROS_ARGUMENT_TEMPLATE})
            eval ${COMMAND} ${MASTER_COMMAND} ${ROS_ARGUMENT}
            COMMAND="${SSH_COMMAND} "\'' /bin/kill $(ps -e | grep -w '${SLAVE_COMMAND}' | sed "s/^ *//" | cut -d " " -f 1)'\'
            for DESTINATION in ${SLAVE_DESTINATION_LIST[@]}; do
                eval ${COMMAND}
            done
            CSV_FILENAME=$(eval echo ${CSV_FILENAME_TEMPLATE})
            DESTINATION=${MASTER_DESTINATION}
            COMMAND="${SCP_COMMAND}:~/${CSV_FILENAME} ${CSV_PATH}/"
            eval ${COMMAND}
        done
    done
}

function draw_graph() {
    CSV_PATH=$1
    python3 plot.py ${CSV_PATH}
    return
}

##################### Main logic #####################
# choose middleware
for MW in ${RMW_LIST[@]}; do
    # run rtt or throughput test and acquire csv
    run_test ${MW}
done
# draw graph
draw_graph ${CSV_PATH}
