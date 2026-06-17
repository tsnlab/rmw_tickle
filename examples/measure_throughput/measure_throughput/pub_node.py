import sys
import rclpy
import array
from rclpy.node import Node
from my_message.msg import Payload

class ThroughputPublisher(Node):
    def __init__(self, payload_size):
        super().__init__('throughput_publisher')
        self.publisher_ = self.create_publisher(Payload, 'throughput_topic', 1)
        self.timer = self.create_timer(0.00001, self.timer_callback) # 100,000Hz
        self.payload_size = payload_size
        self.get_logger().info("Publisher Node started. Sending timestamps...")

    def timer_callback(self):
        msg = Payload()
        msg.payload = array.array('Q', [0xAAAAAAAAAAAAAAAA] * (self.payload_size // 8))
        self.publisher_.publish(msg)

def main(args=sys.argv):
    if len(args) != 2:
        print(f"usage:  ros2 run measure_throughput pub payload_size")
        print(f"        payload size is not given")
        return
    rclpy.init(args=None)
    node = ThroughputPublisher(int(args[1]))
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
