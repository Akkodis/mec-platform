FROM ubuntu:20.04

# Disable Prompt During Packages Installation
ARG DEBIAN_FRONTEND=noninteractive

# Update Ubuntu Software repository
RUN apt update -y
RUN apt install -y python3 python3-pip ninja-build libsoup2.4-1 libsoup2.4-dev libssl1.1 libssl-dev \ 
                   libsrtp2-1 libsrtp2-dev libgnutls30 libgnutls28-dev \ 
                   libgnutls-openssl27 libnice10 libnice-dev libwebsockets15 \
                   libwebsockets-dev flex bison \
                   iputils-ping libpango-1.0-0 libpango1.0-dev libpango-1.0-0 \
                   libpango1.0-dev libpangocairo-1.0-0 libjson-glib-1.0-0 \ 
                   libjson-glib-dev libavfilter7 libavfilter-dev \
                   libavformat58 libavformat-dev libavresample4 \
                   libavresample-dev libavcodec58 libavcodec-dev libavutil56 \
                   libavutil-dev libtool git \
                   python3-gst-1.0 \
                   libgirepository1.0-dev \
                   libgstreamer-plugins-base1.0-dev \
                   libcairo2-dev \
                   gir1.2-gstreamer-1.0 \
                   python3-gi \
                   python-gi-dev \
                   libjson-glib-1.0-0 libjson-glib-dev

RUN pip3 install -Iv meson==0.53.2
RUN git clone https://github.com/sctplab/usrsctp
WORKDIR usrsctp
RUN git checkout c1d6cb3bf4344e01e7d0b3157f4e7103d55b0fbf
RUN chmod +x bootstrap
RUN ./bootstrap
RUN ./configure --prefix /usr/
RUN make
RUN make install

RUN update-ca-certificates
RUN git config --global http.sslverify false

RUN git clone --branch 1.18 https://github.com/GStreamer/gst-build.git
WORKDIR gst-build
RUN mkdir build
RUN meson --prefix=/usr build
RUN ninja -C build
COPY icestream.c ./gst-plugins-bad/ext/webrtc
RUN ninja -C build
RUN meson install -C build

RUN pip3 install --user websockets


RUN pip3 install websockets
RUN pip3 install pygeotile requests


WORKDIR /opt/
COPY requirements.txt ./
RUN pip3 install --no-cache-dir -r requirements.txt

COPY webrtc_proxy.py simple_server.py amqp_manager.py udpvideo2amqp.py content.py webrtcRX ./

EXPOSE 8443
EXPOSE 55000-55099/udp

ENTRYPOINT ["python3"]

CMD ["webrtc_proxy.py"]
