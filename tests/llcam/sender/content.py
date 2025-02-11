from proton import Message
from proton import symbol, ulong, PropertyDict
import base64
import sys
import random

messages = [Message(subject='s%d' % i, body=u'b%d' % i) for i in range(10)]


def messages_generator(num, msgbody):
    messages.clear()

    print("Sender prepare the messages... ")
    import time
    random.seed(time.time()*1000)
    for i in range(num):        
        props = {
                    "dataType": "cits", 
                    "dataSubType": "cam", 
                    "dataFormat": "asn1_jer",
                    "sourceId": 1, 
                    "locationQuadkey": "12022301011102", 
                    "timestamp": time.time()*1000,
                    "body_size": str(sys.getsizeof(msgbody))
                    }
        messages.append(Message(body= msgbody, properties=props))
        #print(messages[i])

    print("Message array done! \n")
