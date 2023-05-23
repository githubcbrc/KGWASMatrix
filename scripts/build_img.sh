#!/bin/bash
# BUILD IMAGE
echo "##############################"
echo "+++++ Building img gwasimg ..."
docker build $1 -t gwasimg .
echo "##############################"
