import os
import sys
import array
import argparse
import csv
import rclpy
from collections import OrderedDict
from rclpy.node import Node
from rtt_messages.msg import Ping, Pong
from measure_rtt import rtt_common

# NOTE: use either size of transmission unit or size of payload
# temporary value, it should be MTU_PAYLOAD_SIZE = MTU - size of headers
MTU_PAYLOAD_SIZE = 1400
DEFAULT_INTERVAL_US = 100 * rtt_common.MILLISECOND // rtt_common.MICROSECOND
DEFAULT_QOS_PROFILE = 1

BYTEORDER = 'little'
TTL = 1 * rtt_common.SECOND

LOG_TERMINAL = True
LOG_INTERVAL = rtt_common.SECOND

class RTTPing(Node):
    def __init__(self, interval: int, payload_size: int):
        # ROS API invocation
        super().__init__('rtt_ping')
        self.publisher_ = self.create_publisher(Ping, 'rtt_ping', DEFAULT_QOS_PROFILE)
        self.subscriber_ = self.create_subscription(Pong, 'rtt_pong', self.pong_callback, DEFAULT_QOS_PROFILE)
        self.ping_timer_ = self.create_timer(interval / rtt_common.SECOND , self.ping_callback)
        self.logger_timer_ = self.create_timer(LOG_INTERVAL / rtt_common.SECOND, self.log_callback)

        # message
        self.payload_size_: int = payload_size
        self.timestamps_ = OrderedDict()
        self.sequence_id_: int = 0
        self.oldest_sequence_id: int = 0

        # statistics
        self.count_: int = 0
        self.loss_count_: int = 0
        self.rtt_sum_: int = 0

        # CSV Setup
        filename = 'rtt_data.csv'
        file_exists = os.path.isfile(filename)
        self.csv_file_ = open(filename, mode='a', newline='')
        self.writer_ = csv.writer(self.csv_file_)
        self.get_logger().info(f"Ping Node started, ping interval={interval / rtt_common.MILLISECOND} ms")

        # Write header only if the file is new
        if not file_exists:
            self.writer_.writerow(['count', 'timestamp_ns', 'rtt_ms'])
        self.get_logger().info(f"Logging RTTs to {filename}")
        

    def ping_callback(self):
        # publish Ping message (sequence ID + payload)
        msg = Ping()
        msg.payload = array.array('B', self.sequence_id_.to_bytes(8, byteorder=BYTEORDER))
        msg.payload.frombytes(array.array('B', bytearray(self.payload_size_ - 8)))
        timestamp: int = self.get_clock().now().nanoseconds
        self.timestamps_[self.sequence_id_] = timestamp
        self.publisher_.publish(msg)

        # check if oldest message exceeds TTL and discard if so
        while True:
            while self.oldest_sequence_id not in self.timestamps_:
                self.oldest_sequence_id += 1
            if timestamp - self.timestamps_[self.oldest_sequence_id] > TTL:
                self.timestamps_.pop(self.oldest_sequence_id)
                self.loss_count_ += 1
            else:
                break

        self.sequence_id_+= 1 

    def pong_callback(self, msg: Pong):
        # receive pong message
        arrival_timestamp: int = self.get_clock().now().nanoseconds
        sequence_id = int.from_bytes(msg.payload[:8], byteorder=BYTEORDER)
        timestamp = self.timestamps_.pop(sequence_id)
        rtt = arrival_timestamp - timestamp

        # Log to CSV
        self.writer_.writerow([sequence_id, timestamp, rtt])

        self.rtt_sum_ += rtt
        self.count_+= 1

    def log_callback(self):
        if self.count_ > 0:
            avg_rtt = self.rtt_sum_ / self.count_
        else:
            avg_rtt = 0
        avg_rtt /= rtt_common.MILLISECOND
        self.get_logger().info(f"avg rtt={avg_rtt:.6f} ms, received={self.count_}, loss={self.loss_count_}")
        self.rtt_sum_ = 0
        self.loss_count_ = 0
        self.count_ = 0

    def destroy_node(self):
        self.csv_file_.close()
        super().destroy_node()

def main(args=sys.argv):
    parser = argparse.ArgumentParser(prog='RTT Ping node')
    parser.add_argument("-i", "--interval", help="message transmit interval in microseconds", type=int, default=DEFAULT_INTERVAL_US)
    parser.add_argument("-s", "--payload-size", help="payload size in bytes. equal to or larger than 16 bytes", type=int, default=16, choices=[16, 32, 64, 128, 256, 512, 1024, MTU_PAYLOAD_SIZE])
    args = parser.parse_args()
    rclpy.init(args=sys.argv)
    node = RTTPing(args.interval * rtt_common.MICROSECOND, args.payload_size)
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down, saving CSV...")
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
