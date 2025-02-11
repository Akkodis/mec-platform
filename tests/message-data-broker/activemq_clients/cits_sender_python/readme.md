
The initial exampla for this client is at the link: https://github.com/apache/activemq/tree/main/assembly/src/release/examples/amqp/python

## Overview
This is an example of how to use the python AMQP [Qpid Proton](https://qpid.apache.org/proton/index.html) reactor API with ActiveMQ.

## Prereqs
- linux
- python 3.5+
- you have successfully installed [python-qpid-proton](https://pypi.python.org/pypi/python-qpid-proton) - including any of its [dependencies](https://github.com/apache/qpid-proton/blob/master/INSTALL.md)
- $PYTHONPATH can search this folder

## Running the Examples
In one terminal window run:

    python3 sender.py

    or

    python3 sender_with_sd_database_support.py

Depending if your are running your S&D connected to an database or not


If you want to do some debugging and check if your messages are being sent run in another terminal to receive messages (you have to modify address.py in order to put the appropriate ip, port and topic given by your message broker) :

    python receiver.py

Use the ActiveMQ admin web page to check Messages Enqueued / Dequeued counts match. 

You can control which AMQP server the examples try to connect to and the messages they send by changing the values in config.py

You have to take into account that any modification made on dataflowmetadata must be applied too into the content.py file in order to generate the appropriate content.

## Pseudo movement example

Added some fake movement around a fixed GPS position in order to simulate movement in example

[cits_send_moving_location.py](cits_send_moving_location.py)

This way we can move around a MEC that covers tiles:

* 031333123201033
* 031333123201211

and a secondary one that covers tiles:

* 031333123201212
* 031333123201213
* 031333123201223
* 031333123202223

