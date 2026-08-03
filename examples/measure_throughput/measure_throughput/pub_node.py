import sys
import rclpy
import array
from rclpy.node import Node
from my_message.msg import Payload

# subscriber receive rate is around 2000~3000 messages per second. 
# 10000 ensures maximum throughput
DEFAULT_FREQUENCY = 10000

class ThroughputPublisher(Node):
    def __init__(self, payload_size, frequency):
        super().__init__('throughput_publisher')
        self.publisher_ = self.create_publisher(Payload, 'throughput_topic', 1)
        self.timer = self.create_timer(1 / frequency, self.timer_callback)
        self.payload_size = payload_size
        self.get_logger().info("Publisher Node started. Sending timestamps...")

    def timer_callback(self):
        msg = Payload()
        msg.payload = array.array('B', [0xAA] * (self.payload_size))
        self.publisher_.publish(msg)

def main(args=sys.argv):
    if len(args) < 2 or len(args) > 3:
        print(f"usage:  ros2 run measure_throughput pub payload_size [frequency]")
        print(f"        payload_size    payload size in bytes")
        print(f"        frequency       sending frequency in Hz. Default value is 10,000")
        return
    payload_size = int(args[1])
    if len(args) == 3:
        frequency = float(args[2])
    else:
        frequency = DEFAULT_FREQUENCY
    rclpy.init(args=None)
    node = ThroughputPublisher(payload_size, frequency)
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
