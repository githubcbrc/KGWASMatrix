#!/bin/bash

cont="gwascont"
img="gwasimg"

echo "##############################"
echo "+++++ Starting container ${cont}"

docker_cont=$(docker ps -a -f name=${cont} | wc -l);

if [ $docker_cont -eq 2 ]; then
	echo "${cont} exists ... removing!"
	docker container stop ${cont} 1>/dev/null
	docker container rm -f ${cont} 1>/dev/null
	echo "${cont} removed!"
fi

projectDir=$PWD"/.."

id=`docker run --rm -d --name ${cont} -it -v $projectDir:/project ${img}`
echo "New Container instance ${cont} running with id $id"
#docker run -d --name gwascont -it -v $PWD:/project --user 1000:1000  gwascont
echo "##############################"