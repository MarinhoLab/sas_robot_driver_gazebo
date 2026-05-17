set -e
xacro ur.urdf.xacro name:=test ur_type:=ur3e > ur3e.urdf
gz sdf -p ur3e.urdf > ur3e.sdf
