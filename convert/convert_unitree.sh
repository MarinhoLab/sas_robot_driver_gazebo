set -e

robot_name=$1
echo "Converting: $robot_name"

WORKDIR=$(pwd)

. /opt/ros/jazzy/setup.bash
cd /root/sas/vendor
colcon build --packages-select "$robot_name"_description

cd ..
source vendor/install/setup.bash
cd vendor/unitree_ros/robots/"$robot_name"_description/xacro
xacro robot.xacro name:="$robot_name"_1 DEBUG:=False> "$robot_name".urdf
gz sdf -p "$robot_name".urdf > "$robot_name".sdf

cp "$robot_name".sdf "$WORKDIR"
cd "$WORKDIR"
