import os
import sys
import array
import argparse
import csv
import rclpy
from collections import OrderedDict
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rtt_messages.msg import Ping, Pong
from measure_rtt import rtt_common

# NOTE: use either size of transmission unit or size of payload
# temporary value, it should be MTU_PAYLOAD_SIZE = MTU - size of headers
MTU_PAYLOAD_SIZE = 1400
DEFAULT_INTERVAL = 100 * rtt_common.MILLISECOND
DEFAULT_COUNT = 600
DEFAULT_QOS_PROFILE = 1

BYTEORDER = 'little'
TTL = 1 * rtt_common.SECOND

LOG_INTERVAL = rtt_common.SECOND
MONITOR_INTERVAL = rtt_common.SECOND

class RTTPing(Node):
    def __init__(self, interval_ms: int, payload_size: int, message_count: int):
        interval_s = interval_ms / 1000

        # ROS API invocation
        super().__init__('rtt_ping')
        self.publisher_ = self.create_publisher(Ping, 'rtt_ping', DEFAULT_QOS_PROFILE)
        self.subscriber_ = self.create_subscription(Pong, 'rtt_pong', self.pong_callback, DEFAULT_QOS_PROFILE)
        self.ping_timer_ = self.create_timer(interval_s, self.ping_callback)
        self.logger_timer_ = self.create_timer(LOG_INTERVAL / rtt_common.SECOND, self.log_callback)
        self.monitor_timer_ = self.create_timer(MONITOR_INTERVAL / rtt_common.SECOND, self.monitor_callback)

        # message
        self.payload_size_: int = payload_size
        self.timestamps_ = OrderedDict()
        self.sequence_id_: int = 0
        self.oldest_sequence_id_: int = 0
        self.stop_spin_ = False

        # statistics
        self.received_: int = 0
        self.loss_count_: int = 0
        self.rtt_sum_: int = 0
        self.ping_count_ = message_count

        # CSV Setup
        middleware_name = os.getenv('RMW_IMPLEMENTATION')
        csv_filename = f'rtt_{middleware_name}_i{interval_ms}_s{payload_size}_c{message_count}.csv'
        file_exists = os.path.isfile(csv_filename)
        self.csv_file_ = open(csv_filename, mode='a', newline='')
        self.writer_ = csv.writer(self.csv_file_)
        self.get_logger().info(f"Ping Node started, ping interval={interval_s} s, ping count={self.ping_count_}")

        # Write header only if the file is new
        if not file_exists:
            self.writer_.writerow(['count', 'timestamp_ns', 'rtt_ns'])
        self.get_logger().info(f"Logging RTTs to {csv_filename}")
    
    def monitor_callback(self):
        # check if oldest message exceeds TTL and discard if so
        timestamp: int = self.get_clock().now().nanoseconds
        while self.oldest_sequence_id_ < self.sequence_id_:
            if self.oldest_sequence_id_ not in self.timestamps_:
                self.oldest_sequence_id_ += 1
            elif timestamp - self.timestamps_[self.oldest_sequence_id_] > TTL:
                self.timestamps_.pop(self.oldest_sequence_id_)
                self.oldest_sequence_id_ += 1
                self.loss_count_ += 1
            else:
                break;

        if self.ping_count_ == 0 and len(self.timestamps_) == 0:
            self.stop_spin_ = True

    def ping_callback(self):
        # publish Ping message (sequence ID + payload)
        msg = Ping()
        msg.payload = array.array('B', self.sequence_id_.to_bytes(8, byteorder=BYTEORDER))
        msg.payload.frombytes(array.array('B', bytearray(self.payload_size_ - 8)))
        timestamp: int = self.get_clock().now().nanoseconds
        self.publisher_.publish(msg)
        self.timestamps_[self.sequence_id_] = timestamp
        self.sequence_id_+= 1 
        self.ping_count_ -= 1
        if self.ping_count_ == 0:
            self.ping_timer_.cancel()

    def pong_callback(self, msg: Pong):
        # receive pong message
        arrival_timestamp: int = self.get_clock().now().nanoseconds
        sequence_id = int.from_bytes(msg.payload[:8], byteorder=BYTEORDER)

        # drop timed out message
        if sequence_id not in self.timestamps_:
            return
        timestamp = self.timestamps_.pop(sequence_id)
        rtt = arrival_timestamp - timestamp

        # Log to CSV
        self.writer_.writerow([sequence_id, timestamp, rtt])

        self.rtt_sum_ += rtt
        self.received_+= 1

    def log_callback(self):
        if self.received_ > 0:
            avg_rtt = self.rtt_sum_ / self.received_
        else:
            avg_rtt = 0
        avg_rtt /= rtt_common.MILLISECOND
        self.get_logger().info(f"avg rtt={avg_rtt:.6f} ms, received={self.received_}, loss={self.loss_count_}")
        self.rtt_sum_ = 0
        self.loss_count_ = 0
        self.received_ = 0

    def stop_spin(self) -> bool:
        return self.stop_spin_

    def destroy_node(self):
        self.csv_file_.close()
        super().destroy_node()

def main(args=sys.argv):
    parser = argparse.ArgumentParser(prog='RTT Ping node')
    parser.add_argument("-i", "--interval", help="message transmit interval in millisecond (1 ms granularity)", type=int, default=(DEFAULT_INTERVAL / rtt_common.MILLISECOND))
    parser.add_argument("-s", "--payload-size", help="payload size in bytes. equal to or larger than 16 bytes", type=int, default=16, choices=[8, 16, 32, 64, 128, 256, 512, 1024, MTU_PAYLOAD_SIZE])
    parser.add_argument("-c", "--count", help="number of messages to be sent", type=int, default=DEFAULT_COUNT)
    args = parser.parse_args()
    rclpy.init(args=sys.argv)
    node = RTTPing(args.interval, args.payload_size, args.count)
    try:
        while not node.stop_spin():
            rclpy.spin_once(node)
        last_second: int = node.get_clock().now().nanoseconds + rtt_common.SECOND
        while node.get_clock().now().nanoseconds < last_second:
            rclpy.spin_once(node)
    except (KeyboardInterrupt, ExternalShutdownException) as error:
        node.get_logger().info("Shutting down, saving CSV...")
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
