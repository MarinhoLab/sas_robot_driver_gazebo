# Scoped Set Pose Plugin for Gazebo Harmonic

This is a small Gazebo Harmonic system plugin that adds a service accepting scoped
entity names, then forwards the request to Gazebo's default world set-pose
service.

It does **not** replace or modify `/world/<world>/set_pose`. It advertises a new
service, by default:

```text
/world/<world>/set_pose_scoped
```

The service has the same Gazebo Transport wire type as the default Gazebo
service:

```text
Request:  gz.msgs.Pose
Response: gz.msgs.Boolean
```

The plugin accepts names such as:

```text
robot/base_link
my_world/robot/base_link
robot::base_link
```

It resolves them to the current Gazebo entity ID and forwards the request to:

```text
/world/<world>/set_pose
```

## Build standalone

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
export GZ_SIM_SYSTEM_PLUGIN_PATH="$PWD/build:${GZ_SIM_SYSTEM_PLUGIN_PATH:-}"
```

## SDF usage

Make sure the default `UserCommands` system is loaded, because it provides
`/world/<world>/set_pose`.

```xml
<plugin
    filename="gz-sim-user-commands-system"
    name="gz::sim::systems::UserCommands"/>

<plugin
    filename="libScopedSetPose.so"
    name="scoped_set_pose::ScopedSetPose">
  <service>/world/ur3e_world/set_pose_scoped</service>
  <target_service>/world/ur3e_world/set_pose</target_service>
  <timeout_ms>1000</timeout_ms>
</plugin>
```

## Call it

```bash
gz service -s /world/ur3e_world/set_pose_scoped \
  --reqtype gz.msgs.Pose \
  --reptype gz.msgs.Boolean \
  --timeout 1000 \
  --req 'name: "robot/base_link", position: {x: 1.0, y: 0.0, z: 0.5}, orientation: {w: 1.0}'
```

The plugin keeps compatibility with Gazebo's default service by using
`gz.msgs.Pose -> gz.msgs.Boolean`, only adding scoped-name resolution before the
request reaches the default `/world/<world>/set_pose` service.

## In a parent ROS 2 package

Add this directory as a subdirectory from the top-level CMakeLists.txt:

```cmake
add_subdirectory(plugins/scoped_set_pose)
```

If installing into your package instead of this standalone project, adjust the
install destination, for example:

```cmake
install(
  TARGETS ScopedSetPose
  LIBRARY DESTINATION share/${PROJECT_NAME}/plugins
  ARCHIVE DESTINATION share/${PROJECT_NAME}/plugins
  RUNTIME DESTINATION share/${PROJECT_NAME}/plugins
)
```
