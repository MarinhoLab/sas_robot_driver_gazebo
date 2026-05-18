set -e

WORKDIR=$(pwd)

source vendor/install/setup.bash
cd vendor/Universal_Robots_ROS2_Description/urdf
xacro ur.urdf.xacro name:=test ur_type:=ur3e > ur3e.urdf
gz sdf -p ur3e.urdf > ur3e.sdf

cp ur3e.sdf "$WORKDIR"
cd "$WORKDIR"
