#!/usr/bin/env bash

# This script run ROS 2 command to evaluate performance of middleware and plot measurement result
# Test parameters: time interval (millisecond), payload size(byte) and number of messages (mimic options of ping: -i, -s, -c)
# Interval: 1000, 100, 10, 1 ms
# Payload size: 8, 16, 32, 64, 128, 256, 512, 1024, Max bytes

##################### Configuration #####################
MASTER_DESTINATION=

SLAVE_DESTINATION_LIST=(
)

CSV_PATH="$HOME/ros2/data/$(date +%g%m%d_%H%M%S)"

##################### Constants #####################
RMW_LIST=(
    "rmw_tickle"
    "rmw_fastrtps_cpp"
    "rmw_cyclonedds_cpp"
#   "rmw_zenoh_cpp" # TODO: fix zenoh node not recognizing zenoh router
)

# unit: millisecond
INTERVAL_LIST=(
#   1 10 100 1000
    1 10 50 100 200
)

# unit: byte
PAYLOAD_SIZE_LIST=(
#   8 16 32 64 128 256 512 1024 # TODO: add payload size for MTU 
#   8 16 32 64
    8
)

# number of ping messages
NUM_MESSAGES=1000

##################### Initialize #####################

# Check configuration
if [[ "${MASTER_DESTINATION}" == "" ]]; then
    echo "invalid configuration: MASTER_DESTINATION is empty"
    exit 1
fi
if [[ "${#SLAVE_DESTINATION_LIST[@]}" == "0" ]]; then
    echo "invalid configuration: SLAVE_DESTINATION_LIST is empty"
    exit 1
fi
if [[ "${CSV_PATH}" == "" ]]; then
    echo "invalid configuration: CSV_PATH is empty"
    exit 1
fi
if [[ "${#RMW_LIST[@]}" == "0" ]]; then
    echo "invalid configuration: RMW_LIST is empty"
    exit 1
fi
if [[ "${#INTERVAL_LIST[@]}" == "0" ]]; then
    echo "invalid configuration: INTERVAL_LIST is empty"
    exit 1
fi
if [[ "${#PAYLOAD_SIZE_LIST[@]}" == "0" ]]; then
    echo "invalid configuration: PAYLOAD_SIZE_LIST is empty"
    exit 1
fi
if [[ "${NUM_MESSAGES}" == "" ]]; then
    echo "invalid configuration: NUM_MESSAGES is empty"
    exit 1
fi

# Choose what to measure
if [[ "${1}" != "RTT" && "${1}" != "THROUGHPUT" ]]; then
    echo "first argument must be either \"RTT\" or \"THROUGHPUT\""
    exit 1
fi

if [[ ! -d ${CSV_PATH} ]]; then
    echo "${CSV_PATH} does not exist; will be created"
    mkdir -p ${CSV_PATH}
fi

echo "checking node connection"
MASTER_ADDR=$(echo ${MASTER_DESTINATION} | cut -d '@' -f 2)
ping $MASTER_ADDR -c 1 -W 1 > /dev/null
if [[ $? -ne 0 ]]; then
    echo "unable to connect MASTER_DESTINATION ${MASTER_DESTINATION}"
    exit 1
else
    ssh -o BatchMode=yes ${MASTER_DESTINATION} exit
    if [[ $? -ne 0 ]]; then
        echo "unable to ssh with key MASTER_DESTINATION ${MASTER_DESTINATION}"
    fi
    echo "master node ${MASTER_DESTINATION} normal"
fi

for SLAVE_DESTINATION in ${SLAVE_DESTINATION_LIST[@]}; do
    SLAVE_ADDR=$(echo ${SLAVE_DESTINATION} | cut -d '@' -f 2)
    ping $SLAVE_ADDR -c 1 -W 1 > /dev/null
    if [[ $? -ne 0 ]]; then
        echo "unable to connect SLAVE_DESTINATION ${SLAVE_DESTINATION}"
        exit 1
    else
        ssh -o BatchMode=yes ${SLAVE_DESTINATION} exit
        if [[ $? -ne 0 ]]; then
            echo "unable to ssh with key SLAVE_DESTINATION ${SLAVE_DESTINATION}"
        fi
        echo "slave node ${SLAVE_DESTINATION} normal"
    fi
done

MEASURE="$1"

ROS_COMMAND=""
ROS_ARGUMENT=""
MASTER_COMMAND=""
SLAVE_COMMAND=""
SSH_COMMAND='ssh ${DESTINATION}'
SCP_COMMAND='scp ${DESTINATION}'
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

    # run zenoh router
    if [[ ${MW} == "rmw_zenoh_cpp" ]]; then
        COMMAND="${SSH_COMMAND} "\''export RMW_IMPLEMENTATION='${MW}'; source ${HOME}/ros2/install/setup.bash;'\'" ros2 run rmw_zenoh_cpp rmw_zenohd&"
        eval ${COMMAND}
    fi

    for INTERVAL in ${INTERVAL_LIST[@]}; do
        for PAYLOAD_SIZE in ${PAYLOAD_SIZE_LIST[@]}; do
            echo ""
            echo "middleware=${MW}, interval=${INTERVAL}, payload size=${PAYLOAD_SIZE}"
            echo ""

            # run pong(RTT) or sub(throughput)
            COMMAND="${SSH_COMMAND} "\''export RMW_IMPLEMENTATION='${MW}'; source ${HOME}/ros2/install/setup.bash;'\'${ROS_COMMAND}
            for DESTINATION in ${SLAVE_DESTINATION_LIST[@]}; do
                eval ${COMMAND} ${SLAVE_COMMAND}&
            done
            sleep 2

            # run ping(RTT) or pub(throughput)
            DESTINATION=${MASTER_DESTINATION}
            ROS_ARGUMENT=$(eval echo ${ROS_ARGUMENT_TEMPLATE})
            eval ${COMMAND} ${MASTER_COMMAND} ${ROS_ARGUMENT}

            # kill pong(RTT) or sub(throughput)
            COMMAND="${SSH_COMMAND} "\'' /bin/kill $(ps -e | grep -w '${SLAVE_COMMAND}' | sed "s/^ *//" | cut -d " " -f 1)'\'
            for DESTINATION in ${SLAVE_DESTINATION_LIST[@]}; do
                eval ${COMMAND}
            done

            # copy CSV from RPi to here
            CSV_FILENAME=$(eval echo ${CSV_FILENAME_TEMPLATE})
            DESTINATION=${MASTER_DESTINATION}
            COMMAND="${SCP_COMMAND}:~/${CSV_FILENAME} ${CSV_PATH}/"
            eval ${COMMAND}
        done
    done

    # kill zenoh router
    if [[ ${MW} == "rmw_zenoh_cpp" ]]; then
        COMMAND="${SSH_COMMAND} pkill -9 -f ros && ros2 daemon stop"
        eval ${COMMAND}
    fi
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
draw_graph ${CSV_PATH}
