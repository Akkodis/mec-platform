#!/bin/bash
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

set +eux

# K3s releases: https://github.com/k3s-io/k3s/releases/
K8S_VERSION="v1.29.3+k3s1"

# configure registry
function configure_registry() {
    if [ -n "${DOCKER_PROXY_URL}" ]; then
        echo "Configuring docker proxy URL in /etc/rancher/k3s/registries.yaml"
        cat << EOF | sudo tee /etc/rancher/k3s/registries.yaml > /dev/null
mirrors:
  docker.io:
    endpoint:
      - "${DOCKER_PROXY_URL}"
EOF
    fi
    if [ -n "${DOCKER_REGISTRY_URL}" ]; then
        echo "Configuring docker private registry in /etc/rancher/k3s/registries.yaml"
        cat << EOF | sudo tee -a /etc/rancher/k3s/registries.yaml > /dev/null
configs:
  ${DOCKER_REGISTRY_URL}:
    auth:
      username: ${DOCKER_REGISTRY_USER}
      password: ${DOCKER_REGISTRY_PASSWORD}
EOF
    fi
}

# installs k3s
function install_k3s() {
    [ -z "${DEBUG_INSTALL}" ] || DEBUG beginning of function
    export INSTALL_K3S_EXEC="--disable traefik"
    curl -sfL https://get.k3s.io | INSTALL_K3S_VERSION=${K8S_VERSION} sh -s -
    sudo chmod 644 /etc/rancher/k3s/k3s.yaml
    [ -z "${DEBUG_INSTALL}" ] || DEBUG end of function
}

# updates service nodeport range
function update_service_nodeport_range() {
    [ -z "${DEBUG_INSTALL}" ] || DEBUG beginning of function
    sudo k3s server --kube-apiserver-arg=service-node-port-range=80-32767
    [ -z "${DEBUG_INSTALL}" ] || DEBUG end of function
}

# checks cluster readiness
function check_for_readiness() {
    [ -z "${DEBUG_INSTALL}" ] || DEBUG beginning of function
    # Check for Ready node, takes ~30 seconds
    echo "Waiting for K8s nodes to be ready"
    local time_for_failure=60    # seconds broken
    local sampling_period=5     # seconds
    local counter=0
    local cluster_ready=""
    while (( counter < time_for_failure ))
    do
        kubectl get nodes |grep master |grep -v none | grep Ready
        if [ $? -eq 0 ] ; then
            echo "K8s cluster is ready"
            cluster_ready="y"
            break
        else
            echo "K8s cluster is not ready yet"
            counter=$((counter + sampling_period))
            sleep ${sampling_period}
        fi
    done
    [ -n "$cluster_ready" ] || FATAL_TRACK k8scluster "K3s cluster nodes not ready after $time_for_failure seconds."

    echo "Waiting for pods to be ready"
    local time_for_readiness=20   # seconds ready
    local time_for_failure=100    # seconds broken

    # Equivalent number of samples
    oks_threshold=$((time_for_readiness/${sampling_period}))     # No. ok samples to declare the system ready
    failures_threshold=$((time_for_failure/${sampling_period}))  # No. nok samples to declare the system broken
    failures_in_a_row=0
    oks_in_a_row=0
    ####################################################################################
    # Loop to check system readiness
    ####################################################################################
    K3S_NAMESPACE=kube-system
    while [[ (${failures_in_a_row} -lt ${failures_threshold}) && (${oks_in_a_row} -lt ${oks_threshold}) ]]
    do
        # State of pods rather than completed jobs
        K3S_PODS_STATE=$(kubectl get pod -n ${K3S_NAMESPACE} --no-headers |grep -v Completed 2>&1)
        K3S_PODS_READY=$(echo "${K3S_PODS_STATE}" | awk '$2=="1/1" || $2=="2/2" {printf ("%s\t%s\t\n", $1, $2)}')
        K3S_PODS_NOT_READY=$(echo "${K3S_PODS_STATE}" | awk '$2!="1/1" && $2!="2/2" {printf ("%s\t%s\t\n", $1, $2)}')
        COUNT_K3S_PODS_READY=$(echo "${K3S_PODS_READY}"| grep -v -e '^$' | wc -l)
        COUNT_K3S_PODS_NOT_READY=$(echo "${K3S_PODS_NOT_READY}" | grep -v -e '^$' | wc -l)

        # OK sample
        if [[ ${COUNT_K3S_PODS_NOT_READY} -eq 0 ]]
        then
            ((++oks_in_a_row))
            failures_in_a_row=0
            echo -ne ===\> Successful checks: "${oks_in_a_row}"/${oks_threshold}\\r
        # NOK sample
        else
            ((++failures_in_a_row))
            oks_in_a_row=0
            echo
            echo Bootstraping... "${failures_in_a_row}" checks of ${failures_threshold}

            # Reports failed pods in K3S
            if [[ "${COUNT_K3S_PODS_NOT_READY}" -ne 0 ]]
            then
                echo "K3S kube-system: Waiting for ${COUNT_K3S_PODS_NOT_READY} of $((${COUNT_K3S_PODS_NOT_READY}+${COUNT_K3S_PODS_READY})) pods to be ready:"
                echo "${K3S_PODS_NOT_READY}"
                echo
            fi
        fi

        #------------ NEXT SAMPLE
        sleep ${sampling_period}
    done

    ####################################################################################
    # OUTCOME
    ####################################################################################
    if [[ (${failures_in_a_row} -ge ${failures_threshold}) ]]
    then
        echo
        FATAL_TRACK k8scluster "K8S CLUSTER IS BROKEN"
    else
        echo
        echo "K8S CLUSTER IS READY"
    fi
    [ -z "${DEBUG_INSTALL}" ] || DEBUG end of function
}

# Initializes kubeconfig file
function save_kubeconfig() {
    [ -z "${DEBUG_INSTALL}" ] || DEBUG beginning of function
    KUBEDIR="${HOME}/.kube"
    KUBEFILE="$KUBEDIR/config"
    mkdir -p "${KUBEDIR}"
    K3S_KUBECONFIG="/etc/rancher/k3s/k3s.yaml"
    sudo cp "${K3S_KUBECONFIG}" "${KUBEFILE}"
    sudo chown $(id -u):$(id -g) "${KUBEFILE}"
    sed -i "s#server: https://127.0.0.1#server: https://${DEFAULT_IP}#g" "${KUBEFILE}"
    chmod 700 "${KUBEFILE}"
    echo
    echo "Credentials saved at ${KUBEFILE}"
    echo
    [ -z "${DEBUG_INSTALL}" ] || DEBUG end of function
}

# main
while getopts ":D:i:p:d:u:P:-: " o; do
    case "${o}" in
        i)
            DEFAULT_IP="${OPTARG}"
            ;;
        D)
            OSM_DEVOPS="${OPTARG}"
            ;;
        p)
            DOCKER_PROXY_URL="${OPTARG}"
            ;;
        d)
            DOCKER_REGISTRY_URL="${OPTARG}"
            ;;
        u)
            DOCKER_REGISTRY_USER="${OPTARG}"
            ;;
        P)
            DOCKER_REGISTRY_PASSWORD="${OPTARG}"
            ;;
        -)
            [ "${OPTARG}" == "debug" ] && DEBUG_INSTALL="y" && continue
            echo -e "Invalid option: '--$OPTARG'\n" >&2
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument" >&2
            exit 1
            ;;
        \?)
            echo -e "Invalid option: '-$OPTARG'\n" >&2
            exit 1
            ;;
        *)
            exit 1
            ;;
    esac
done

DEBUG_INSTALL=${DEBUG_INSTALL:-}
DEFAULT_IP=${DEFAULT_IP:-"127.0.0.1"}
OSM_DEVOPS=${OSM_DEVOPS:-"/usr/share/osm-devops"}
DOCKER_PROXY_URL=${DOCKER_PROXY_URL=-}
DOCKER_REGISTRY_URL=${DOCKER_REGISTRY_URL=-}
DOCKER_REGISTRY_USER=${DOCKER_REGISTRY_USER=-}
DOCKER_REGISTRY_PASSWORD=${DOCKER_REGISTRY_PASSWORD=-}
echo "DEBUG_INSTALL=${DEBUG_INSTALL}"
echo "DEFAULT_IP=${DEFAULT_IP}"
echo "OSM_DEVOPS=${OSM_DEVOPS}"
echo "DOCKER_PROXY_URL=${DOCKER_PROXY_URL}"
echo "DOCKER_REGISTRY_URL=${DOCKER_REGISTRY_URL}"
echo "DOCKER_REGISTRY_USER=${DOCKER_REGISTRY_USER}"
echo "HOME=$HOME"

source $OSM_DEVOPS/common/logging
source $OSM_DEVOPS/common/track

configure_registry
install_k3s
track k8scluster k3s_install_ok
check_for_readiness
track k8scluster k3s_node_ready_ok
# update_service_nodeport_range
# check_for_readiness
# track k8scluster k3s_update_nodeport_range_ok
save_kubeconfig
track k8scluster k3s_creds_ok
