
import requests
import security


def discover_sb_service(tile,serviceName):
    headers = security.get_header_with_token()
    discovery_url="http://5gmeta-platform.eu/discovery-api"
    path=discovery_url+"/mec/tile/"+str(tile)
    r = requests.get(path, headers=headers)
    json_response=r.json()
    service_ip=-1
    service_port=-1
    if len(json_response) > 0: 
        for mec in json_response:
            for service in mec['sb_services']:
                if service['service_name'] == serviceName:
                    service_ip=service['ip']
                    service_port=service['port']
                    print(service)
                    return service_ip,service_port
    return service_ip,service_port




def register(dataflowmetadata,tile):
    service="registration-api"
    registrationAPI_ip,  registrationAPI_port = discover_sb_service(tile,service)
    if registrationAPI_ip == -1 or registrationAPI_port == -1:
        print(service+" service not found")
        exit(-1)

    url = "http://"+registrationAPI_ip+':'+str(registrationAPI_port)+'/dataflows'

    # Send the JSON of the dataflow's metadata, and receive the dataflowId and the topic where to publish the messages
    r = requests.post(url, json = dataflowmetadata)
    r=r.json()
    dataflowId = r['id']
    topic = r['topic']
    return dataflowId,topic


# This method will update dataflow Info in order to tell infrastructure that given dataflow is still available
def keepAliveDataflow(dataflowMetadata,dataflowId):
    print("Update dataflowId "+str(dataflowId))
    tile=dataflowMetadata["dataSourceInfo"]["sourceLocationInfo"]["locationQuadkey"]
    registration_ip, registration_port=discover_sb_service(tile,"registration-api")

    r = requests.put("http://"+registration_ip+":"+registration_port+"/dataflows/"+str(dataflowId), json = (dataflowMetadata))


