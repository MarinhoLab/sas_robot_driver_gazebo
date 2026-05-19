set -e

robot_name=$1
echo "Converting: $robot_name"

WORKDIR=$(pwd)

source vendor/install/setup.bash
cd vendor/Universal_Robots_ROS2_Description/urdf
xacro ur.urdf.xacro name:="$robot_name"_1 ur_type:="$robot_name" > "$robot_name".urdf
gz sdf -p "$robot_name".urdf > "$robot_name".sdf

cp "$robot_name".sdf "$WORKDIR"
cd "$WORKDIR"
