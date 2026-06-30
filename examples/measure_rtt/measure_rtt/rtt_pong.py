import rclpy
from rclpy.node import Node
from rtt_messages.msg import Ping, Pong

DEFAULT_QOS_PROFILE = 1

class RTTPong(Node):
    def __init__(self):
        # ROS API invocation
        super().__init__('rtt_pong')
        self.publisher_ = self.create_publisher(Pong, 'rtt_pong', DEFAULT_QOS_PROFILE)
        self.subscriber_ = self.create_subscription(Ping, 'rtt_ping', self.ping_callback, DEFAULT_QOS_PROFILE)
        self.get_logger().info("Pong Node started")

    def ping_callback(self, ping_msg: Ping):
        pong_msg = Pong()
        pong_msg.payload = ping_msg.payload
        self.publisher_.publish(pong_msg)

    def destroy_node(self):
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = RTTPong()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()

if __name__ == '__main__':
    main()
