set -e

robot_name=$1
echo "Converting: $robot_name"

WORKDIR=$(pwd)

. /opt/ros/jazzy/setup.bash
cd /root/sas/vendor

# The <robot>_description packages are ROS 1 (catkin), so colcon cannot build
# them on ROS 2. The xacro only uses $(find <robot>_description) to locate its
# sibling xacro files, so register the package in a minimal ament stub instead.
STUB=/tmp/srdg_ament_stub
mkdir -p "$STUB/share/ament_index/resource_index/packages"
touch "$STUB/share/ament_index/resource_index/packages/${robot_name}_description"
mkdir -p "$STUB/share/${robot_name}_description"
cp unitree_ros/robots/"$robot_name"_description/package.xml "$STUB/share/${robot_name}_description/"
ln -sfn "$PWD/unitree_ros/robots/${robot_name}_description/xacro" "$STUB/share/${robot_name}_description/xacro"
export AMENT_PREFIX_PATH="$STUB"

cd unitree_ros/robots/"$robot_name"_description/xacro
xacro robot.xacro name:="$robot_name"_1 DEBUG:=False> "$robot_name".urdf
gz sdf -p "$robot_name".urdf > "$robot_name".sdf

cp "$robot_name".sdf "$WORKDIR"
cd "$WORKDIR"
