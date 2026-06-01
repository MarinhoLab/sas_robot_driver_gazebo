set -e

package_name=$1
xacro_filename=$2
echo "Converting: $package_name/$xacro_filename"

docker container stop extract_files || true
docker container rm extract_files || true
docker build -t srdg_convert --build-context repo=. docker/converter --no-cache
docker run -t -d --name extract_files srdg_convert /bin/bash
docker exec extract_files bash -c "cd /root/sas; . convert_kuka.sh $package_name $xacro_filename"
docker cp extract_files:/root/sas/"$xacro_filename".sdf "$xacro_filename".sdf
docker container stop extract_files
