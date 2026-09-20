set -e

robot_name=$1
echo "Converting: $robot_name"

WORKDIR=$(pwd)

. /opt/ros/jazzy/setup.bash
cd vendor
colcon build --packages-select ur_description
cd ..
source vendor/install/setup.bash
cd vendor/Universal_Robots_ROS2_Description/urdf
xacro ur.urdf.xacro name:="$robot_name"_1 ur_type:="$robot_name" > "$robot_name".urdf
gz sdf -p "$robot_name".urdf > "$robot_name".sdf

cp "$robot_name".sdf "$WORKDIR"
cd "$WORKDIR"
