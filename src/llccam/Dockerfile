FROM python:3.8-slim
WORKDIR /app
RUN apt update && apt install -y gcc
RUN pip3 install --upgrade pip
RUN pip3 install prometheus_client 
RUN pip3 install python-qpid-proton
COPY . /app
ENTRYPOINT ["python", "consumer.py"]