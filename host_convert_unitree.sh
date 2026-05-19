set -e

robot_name=$1
echo "Converting: $robot_name"

docker build -t srdg_convert --build-context repo=. docker/converter --no-cache

docker stop extract_files || true
docker container rm extract_files || true
docker run -t -d --name extract_files srdg_convert /bin/bash
docker exec extract_files bash -c "cd /root/sas; . convert_unitree.sh $robot_name"
docker cp extract_files:/root/sas/"$robot_name".sdf "$robot_name".sdf
docker container stop extract_files