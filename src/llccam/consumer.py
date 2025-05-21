from proton import Url
from proton.reactor import Container, Selector
from proton.handlers import MessagingHandler

import os
import time
import statistics
from prometheus_client import start_http_server, Gauge

monitoring_port = int(os.getenv("MONITORING_PORT", 8081))
start_http_server(monitoring_port)
gauge = Gauge(
    "application_latency",
    "Application Latency."
)

username = os.getenv("AMQP_USER")
password = os.getenv("AMQP_PASS")
ip = os.getenv("AMQP_IP", "127.0.0.1")
port = os.getenv("AMQP_PORT", "5673")
topic = os.getenv("AMQP_TOPIC", "cits-large")

latencies = [0,0,0,0,0,0,0,0,0,0]
def add_latency(value):
    if len(latencies) >= 10:
        latencies.pop(0)  # Remove the first element
    latencies.append(value)

class Recv(MessagingHandler):
    def __init__(self, url):
        super(Recv, self).__init__()
        self.url = Url(url)
        self.received = 0

    def on_start(self, event):
        conn = event.container.connect(self.url)
        event.container.create_receiver(conn, self.url.path)

    def on_message(self, event):
        latency = time.time()*1000 - event.message.properties['timestamp']
        print(latency)
        add_latency(latency)
        gauge.set(statistics.mean(latencies))

        self.received += 1

# Configuration
url = f'amqp://{username}:{password}@{ip}:{port}/topic://{topic}'  # Replace with your broker URL

# Create and run the subscriber
Container(Recv(url)).run()
