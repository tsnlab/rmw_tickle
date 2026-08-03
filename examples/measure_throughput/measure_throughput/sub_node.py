import array
import rclpy
import csv
import os
from collections import deque
from rclpy.node import Node
from my_message.msg import Payload

class ThroughputLogger(Node):
    def __init__(self):
        super().__init__('throughput_logger')
        self.subscription = self.create_subscription(
            Payload, 'throughput_topic', self.listener_callback, 10)
        self.throughput_queue = deque()
        
        # CSV Setup
        self.filename = 'throughput_data.csv'
        self.file_exists = os.path.isfile(self.filename)
        self.csv_file = open(self.filename, mode='a', newline='')
        self.writer = csv.writer(self.csv_file)
        self.logger_timer = self.create_timer(1, self.logger_callback)
        
        # Write header only if the file is created
        if not self.file_exists:
            self.writer.writerow(['elapsed_second', 'message_per_second', 'throughput_mbps'])
        
        self.total_throughput_sum = 0
        self.throughput_sum_per_second = 0
        self.elapsed_second = 1
        self.message_count = 0
        self.get_logger().info(f"Logging throughput to {self.filename}")

    def logger_callback(self):
        throughput_avg = self.total_throughput_sum / self.elapsed_second
        self.get_logger().info(f"Throughput: {self.throughput_sum_per_second / 1000000:.4f} Mbps | Message/second: {self.message_count} | Avg: {throughput_avg / 1000000:.4f} Mbps")
        self.writer.writerow([self.elapsed_second, self.message_count, self.throughput_sum_per_second / 1000000])
        self.throughput_sum_per_second = 0
        self.message_count = 0
        self.elapsed_second += 1

    def listener_callback(self, msg: Payload):
        arr = msg.payload
        msg_size_bits = len(arr) * 8

        for elem in arr:
            if elem != 0xAA:
                self.get_logger().warning("invalid data")

        self.total_throughput_sum += msg_size_bits
        self.throughput_sum_per_second += msg_size_bits
        self.message_count += 1

    def destroy_node(self):
        self.csv_file.close()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args, signal_handler_options=None)
    node = ThroughputLogger()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down, saving CSV...")
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
