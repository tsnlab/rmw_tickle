import rclpy
from rclpy.node import Node
from std_msgs.msg import Header
import csv
import os
from collections import deque

class LatencyLogger(Node):
    def __init__(self):
        super().__init__('latency_logger')
        self.subscription = self.create_subscription(
            Header, 'latency_topic', self.listener_callback, 10)
        self.ts_queue = deque()
        
        # CSV Setup
        self.filename = 'latency_data.csv'
        self.file_exists = os.path.isfile(self.filename)
        self.csv_file = open(self.filename, mode='a', newline='')
        self.writer = csv.writer(self.csv_file)
        
        # Write header only if the file is new
        if not self.file_exists:
            self.writer.writerow(['count', 'timestamp_ns', 'latency_ms'])
        
        self.latencies_sum = 0
        self.count = 0
        self.get_logger().info(f"Logging latencies to {self.filename}")

    def listener_callback(self, msg):
        now = self.get_clock().now()
        sent_time = rclpy.time.Time.from_msg(msg.stamp)
        
        # Calculate latency
        diff = now - sent_time
        latency_ms = diff.nanoseconds / 1e6
        self.ts_queue.append(latency_ms)
        if len(self.ts_queue) > 100:
            oldest_latency = self.ts_queue.popleft()
            self.latencies_sum -= oldest_latency

        # Log to CSV
        self.count += 1
        self.writer.writerow([self.count, now, latency_ms])
        
        self.latencies_sum += latency_ms
        latencies_avg = self.latencies_sum / len(self.ts_queue)
        self.get_logger().info(f"Latency: {latency_ms:.4f} ms | Avg: {latencies_avg:.4f} ms")

    def destroy_node(self):
        self.csv_file.close()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args, signal_handler_options=None)
    node = LatencyLogger()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down, saving CSV...")
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
