from gz.msgs10.actuators_pb2 import Actuators
from gz.msgs10.model_pb2 import Model
from gz.transport13 import Node

import time

joint_positions = []

def joint_state_cb(msg: Model):
    global joint_positions
    joints = msg.joint

    joint_positions = [
        joints[1].axis1.position,
        joints[2].axis1.position,
        joints[3].axis1.position,
        joints[4].axis1.position,
        joints[5].axis1.position,
        joints[6].axis1.position
    ]
    print(f'{joint_positions}')

def main():
    # create a transport node
    node = Node()
    topic = "/world/default/model/test/joint_state"

    publisher_actuators = node.advertise("/actuators", Actuators)
    actuators_msg = Actuators()
    actuators_msg.position.append(6)
    actuators_msg.position.append(6)
    actuators_msg.position.append(6)
    actuators_msg.position.append(6)
    actuators_msg.position.append(6)
    actuators_msg.position.append(6)

    if node.subscribe(Model, topic, joint_state_cb):
        print("Subscribing to type {} on topic [{}]".format(
            Model, topic))
    else:
        print("Error subscribing to topic [{}]".format(topic))
        return

    # wait for shutdown
    try:
        while True:
            time.sleep(0.001)
            if not publisher_actuators.publish(actuators_msg):
                print("?")
    except KeyboardInterrupt:
        pass
    print("Done")

if __name__ == "__main__":
    main()
