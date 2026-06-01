set -e

package_name=$1
xacro_filename=$2
echo "Converting: $package_name/$xacro_filename"

WORKDIR=$(pwd)

. /opt/ros/jazzy/setup.bash
cd /root/sas/vendor/
colcon build --packages-up-to "$package_name"

cd ..
#cat vendor/install/setup.bash
. vendor/install/setup.bash
#ros2 pkg list | grep "iiwa"
cd vendor/iiwa_stack/"$package_name"/urdf
cat "$xacro_filename".xacro
xacro "$xacro_filename".xacro prefix:="prefix_" package_name:="$package_name"> "$xacro_filename".urdf
check_urdf "$xacro_filename".urdf
gz sdf -p "$xacro_filename".urdf > "$xacro_filename".sdf || true
ls .

cp "$xacro_filename".urdf "$WORKDIR"
cp "$xacro_filename".sdf "$WORKDIR"
cd "$WORKDIR"
