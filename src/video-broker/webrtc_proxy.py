import os
import sys, json, requests, random, string

from amqp_manager import AMQP

# Device type
deviceType  = "webrtc_proxy"

# Parameters from MEC Message Broker
amqp_ip=os.getenv("AMQP_IP") 
amqp_port=os.getenv('AMQP_PORT')
username=os.getenv('AMQP_USER')
password=os.getenv('AMQP_PASS')

#Server URL to get the MEC information from 5GMETA Cloud Infrastructure
server_url     = ""

# These methods randomly creates id and ip.
def id_generator(size=10, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

def ip_generator():
  return ".".join(map(str, (random.randint(0, 255) 
                            for _ in range(4))))


if __name__ == '__main__':
    # AMQP configuration
    server_url="amqp://"+username+":"+password+"@"+amqp_ip+":"+str(amqp_port)+"/topic://"

    # Parameters to the AMQP system
    parameters = {
        'deviceType' : deviceType, 
        'serverURL' : server_url
    }

    # Topics to be subscribed to. Get notified with new or terminated video streams
    topics = ['newdataflow', 'terminatedataflow']
    try:
        listener = AMQP(subscription=topics, param=parameters)
    except:
        print('Error running webrtc_proxy')
        raise Exception('Error running webrtc_proxy')
