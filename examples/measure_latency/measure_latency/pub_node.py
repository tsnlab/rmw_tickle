import rclpy
from rclpy.node import Node
from std_msgs.msg import Header

class LatencyPublisher(Node):
    def __init__(self):
        super().__init__('latency_publisher')
        self.publisher_ = self.create_publisher(Header, 'latency_topic', 1)
        self.timer = self.create_timer(0.1, self.timer_callback) # 10Hz
        self.get_logger().info("Publisher Node started. Sending timestamps...")

    def timer_callback(self):
        msg = Header()
        msg.stamp = self.get_clock().now().to_msg()
        self.publisher_.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = LatencyPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
