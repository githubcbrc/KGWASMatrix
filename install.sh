#!/bin/bash

cd init
bash ../scripts/build_img.sh #--no-cache
bash ../scripts/start_cont.sh
docker exec -it gwascont bash /project/scripts/build_binaries.sh
