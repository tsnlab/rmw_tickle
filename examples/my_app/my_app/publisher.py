import rclpy
from rclpy.node import Node
from my_message.msg import MyMessage
from time import time_ns

def main():
    rclpy.init(args=None)
    node = Node('publisher_node')
    publisher = node.create_publisher(MyMessage, 'my_topic', 10)
    def pub_callback():
        msg = MyMessage()
        msg.header = 1
        msg.body = time_ns()
        publisher.publish(msg)
        node.get_logger().info(f'published time={msg.body}')
        
    timer = node.create_timer(1.0, pub_callback)
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
