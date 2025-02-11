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

set -ex

# Wrapper function for raw calls to Gitea API

HERE=$(dirname "$(readlink --canonicalize "$BASH_SOURCE")")
. "$HERE/../library/functions.sh"
. "$HERE/../library/trap.sh"

SERVER_URL=$1
TOKEN=$2
VERB=$3
URI=$4
DATA=$5

if [ -z "$DATA" ]; then
    kubectl exec statefulset/gitea --container=gitea --namespace=gitea --quiet -- \
    curl --silent --fail \
        "${SERVER_URL}/api/v1/${URI}" \
        --request "${VERB}" \
        --header "Authorization: token ${TOKEN}" \
        --header 'Accept: application/json'
else
    kubectl exec statefulset/gitea --container=gitea --namespace=gitea --quiet -- \
    curl --silent --fail \
        --request "$VERB" \
        "${SERVER_URL}/api/v1/${URI}" \
        --header "Authorization: token ${TOKEN}" \
        --header 'Accept: application/json' \
        --header 'Content-Type: application/json' \
        --data "${DATA}"
fi
