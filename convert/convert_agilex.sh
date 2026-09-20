set -e

robot_name=$1
echo "Converting: $robot_name"

WORKDIR=$(pwd)

. /opt/ros/jazzy/setup.bash
cd /root/sas/vendor
tree . | grep "$robot_name"

# There's a typo in the ranger_description packages, they're called "desription" instead of "description"
URDF_PATH=ugv_gazebo_sim/"${robot_name}"/"${robot_name}_desription"/urdf/"${robot_name}_desription.urdf"
stat "$URDF_PATH"
gz sdf -p "$URDF_PATH" > "${robot_name}".sdf

cp "${robot_name}".sdf "$WORKDIR"
cd "$WORKDIR"
