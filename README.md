# sas_robot_driver_gazebo

## Licenses

Check any licenses in repositories in `vendor` if you decide to use them. This repository's owner holds no
liability for misuse. 

## Folder structure

The two folders below have special meanings, the others follow usual naming.

| Folder    | Meaning                                                                                                    |
|-----------|------------------------------------------------------------------------------------------------------------|
| `sdf`     | Base Gazebo scene files.                                                                                   |
| `vendor`  | Vendor models. Check the licenses if you decide to use them in your work. They are not shared in this repo |

## Technical Overview

Each relevant joint to be controlled should have a plugin specification similar to the following, using `gz::sim::systems::JointPositionController`.

```xml
<plugin
filename="gz-sim-joint-position-controller-system"
name="gz::sim::systems::JointPositionController">
    <joint_name>shoulder_pan_joint</joint_name>
    <use_velocity_commands>True</use_velocity_commands>
    <cmd_max>0.25</cmd_max>
</plugin>
```

### Technical Overview

Joints are read from a default `gz::sim::systems::JointStatePublisher`.

```xml
<plugin
    filename="gz-sim-joint-state-publisher-system"
    name="gz::sim::systems::JointStatePublisher">
</plugin>
```

## Considerations

- `gz::sim::systems::PosePublisher` has been considered to read poses of entities. However, it's more convenient for `tf2` given how the frames are described. 