# 5GMETA Platform deployment using Ansible

Ansible is used to deploy all the components forming the 5GMETA Edge Stack. The playbook i contained in this repository will install all the dependencies and components and repositories necessary for the 5GMETA platform's MEC server.
The playbook will the deploy the following components:

- Open Source MANO (OSM): v17 for Ubuntu 22.04 
- Notary and Connaisseur for managing security in the cluster
- 5GMETA MEC APIs and Base Components

## Deploying an instance of the 5GMETA MEC Platform

To deploy the MEC Platform using Ansible, type the following command:

```bash
ansible-playbook -i mec-platform-inventory --private-key yourprivatekey 5gmeta-mec-platform-playbook.yaml  
```

Prior to installing the MEC Platform, the values of the parameters of MEC and Cloud hosts must be adapted to your deployment. 


Once deployed you can access the different services in the next ports:
- [OSM UI in port]
- OSM API (Orchestration API) in port 9999
- [Grafana UI] 
- Grafana/Loki UI 
- Prometheus UI 
- Alert Manager UI 
- 5GMETA MEC Platfrom  API Server
- 5GMETA Message-Broker 
- 5GMETA Video-Broker 

For more details see [README](https://github.com/Akkodis/mec-platform/blob/main/README.md)


## Check the Installation

Also you can check the status of OSM ressources managed by Kubernetes in the following way:

```bash
kubectl get all -n osm
kubectl get all -n osm
```
	
## CREDITS
- Djibrilla Amadou Kountche
- Mikel Serón Esnal [GitHub](https://github.com/mikelseron))
