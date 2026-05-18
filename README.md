# sas_robot_driver_gazebo

gz transport version on MacOS
gz-transport13-13.5.0_11.arm64_sequoia.bottle.tar.gz

Things to add

```xml
<world name="default">
    <plugin
      filename="gz-sim-physics-system"
      name="gz::sim::systems::Physics">
    </plugin>
    <plugin
      filename="gz-sim-scene-broadcaster-system"
      name="gz::sim::systems::SceneBroadcaster">
    </plugin>
    
    
<plugin
 filename="gz-sim-joint-controller-system"
 name="gz::sim::systems::JointController">
 <joint_name>shoulder_pan_joint</joint_name>
 <initial_velocity>1.0</initial_velocity>
 </plugin>
 
<plugin
 filename="gz-sim-joint-controller-system"
 name="gz::sim::systems::JointController">
  <joint_name>shoulder_lift_joint</joint_name>
 <initial_velocity>1.0</initial_velocity>
</plugin>

<plugin
 filename="gz-sim-joint-controller-system"
 name="gz::sim::systems::JointController">
  <joint_name>elbow_joint</joint_name>
 <initial_velocity>1.0</initial_velocity>
</plugin>
  </model>
  
  </world>
```
