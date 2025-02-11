#!/bin/bash
#######################################################################################
# Copyright ETSI Contributors and Others.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#######################################################################################

if [ $# -ne 3 ]; then
    echo "Usage $0 <module> <tag_number>"
    echo "Example: $0 ro 2"
    echo "Example: $0 lcm 5"
    exit 1
fi

MODULE_NAME="$1"
TAG_NUMBER="$2"

set -x
IMAGE_NAME="opensourcemano/${MODULE_NAME}:devel-${TAG_NUMBER}"
docker tag opensourcemano/${MODULE_NAME}:devel ${IMAGE_NAME}
IMAGE_FILE="${MODULE_NAME}-devel-${TAG_NUMBER}.tar.gz"
docker save -o ${IMAGE_FILE} ${IMAGE_NAME}
sudo ctr -n=k8s.io images import ${IMAGE_FILE}
rm ${IMAGE_FILE}
sudo ctr -n=k8s.io images list |grep ${IMAGE_NAME}
echo $IMAGE_NAME
kubectl -n osm patch deployment ${MODULE_NAME} --patch '{"spec": {"template": {"spec": {"containers": [{"name": "'${MODULE_NAME}'", "image": "'${IMAGE_NAME}'"}]}}}}'
kubectl -n osm rollout restart deployment ${MODULE_NAME}

