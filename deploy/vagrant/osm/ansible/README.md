# 5GMETA Platform deployment using Ansible

Ansible is used to deploy all the components forming the 5GMETA Edge Stack. The playbook i contained in this repository will install all the dependencies and components and repositories necessary for the 5GMETA platform's MEC server.
The playbook will the deploy the following components:

- Open Source MANO (OSM): v17 for Ubuntu 22.04 
- Notary and Connaisseur for managing security in the cluster
- 5GMETA MEC APIs and Base Components

## Deploying the Edge Stack

Once deployed you can access the different services in the next ports:
- K8s API in port 6443
- K8s UI in port 8080
- OSM UI in port [80](http://127.0.0.1:80)
- OSM API (Orchestration API) in port 9999
- Grafana UI in port [3000](http://127.0.0.1:3000)
- Grafana/Loki UI in port [7000](http://127.0.0.1:7000)
- Prometheus UI in port [9090](http://127.0.0.1:9090)
- Alert Manager UI in port [9093](http://127.0.0.1:9093)
- 5GMETA Edge Instance API in port 5000
- 5GMETA Registration API in port 12346
- 5GMETA Message-Broker in port 5673 (SB) and 61616 (NB)
- 5GMETA Video-Broker in port 8443

Also you can check the status of OSM ressources managed by Kubernetes in the following way:

```bash
kubectl get all -n osm
kubectl get all -n mec-platform
```
	
## CREDITS
- Mikel Serón Esnal [GitHub](https://github.com/mikelseron))
