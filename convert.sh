set -e
docker build -t srdg_convert --build-context repo=. docker/converter --no-cache

docker run -t -d --name extract_files srdg_convert /bin/bash
docker exec extract_files bash -c "cd /root/sas; . convert_ur.sh"
docker cp extract_files:/root/sas/ur3e.sdf ur3e.sdf
docker container stop extract_files