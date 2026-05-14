import rclpy
from rclpy.node import Node
from my_message.msg import MyMessage

def main():
    rclpy.init(args=None)
    node = Node('subscriber_node')
    def sub_callback(msg: MyMessage):
        if msg.header == 1:
            node.get_logger().info(f'time={msg.body}')
        else:
            node.get_logger().info(f'invalid data')
    subscriber = node.create_subscription(MyMessage, 'my_topic', sub_callback, 10)
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
