ARG FROM_IMAGE=ubuntu:20.04
ARG WS=/root/ws

FROM $FROM_IMAGE AS base
SHELL ["/bin/bash", "-c"]


RUN apt-get update && \
    apt-get install -y cmake && \
    apt-get install -y build-essential && \
    apt-get install -y gdb && \
    apt-get install -y libssl-dev && \
    apt-get install -y sudo && \
    apt-get install -y git && \
    apt-get install -y curl zip unzip tar && \
    apt-get install -y pkg-config && \
    apt-get install -y gcc g++ cmake-curses-gui uuid-dev && \
    # dependencies needed for Cyrus SASL support
    apt-get install -y libsasl2-2 libsasl2-dev libsasl2-modules && \
    # dependencies needed for bindings
    apt-get install -y swig python3-dev ruby-dev && \
    # dependencies needed for python docs 
    DEBIAN_FRONTEND=noninteractive apt-get install -y python3-sphinx && \
    apt-get install -y libjsoncpp-dev && \
    apt-get install -y libboost-all-dev

ARG WS
WORKDIR $WS/its_detectors

RUN mkdir lib

RUN cd $WS/its_detectors/lib && \
    git clone https://github.com/borglab/gtsam.git && \
    cd gtsam && \
    mkdir build && cd build && \
    cmake .. && \
    make install -j8 

RUN cd $WS/its_detectors/lib && \
    git clone https://gitlab.com/libeigen/eigen.git && \ 
    cd eigen && git checkout 3.4 && \
    mkdir build && cd build && cmake .. && make install -j8

RUN cd $WS/its_detectors/lib && \
    git clone https://github.com/chrberger/WGS84toCartesian.git && \
    git clone https://github.com/tysik/kalman_filters.git

RUN apt-get install -y libopenblas-dev liblapack-dev libarpack++2-dev libsuperlu-dev libarmadillo-dev && \
    echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib" >> .bashrc

RUN apt install -y libmysqlcppconn-dev libmysqlcppconn7v5

COPY lib/mlw_amqp lib/mlw_amqp
COPY lib/rapidjson lib/rapidjson
COPY lib/kalman lib/kalman
COPY resource resource
COPY test test

RUN cp -r lib/mlw_amqp/build/* /usr/local/lib
RUN cp -r lib/mlw_amqp/include/* /usr/local/include

RUN ln -s /usr/local/lib/libqpid-proton-proactor.so.1.7.0 /usr/local/lib/libqpid-proton-proactor.so.1
RUN ln -s /usr/local/lib/libqpid-proton-proactor.so.1  /usr/local/lib/libqpid-proton-proactor.so

RUN ln -s /usr/local/lib/libqpid-proton-core.so.10.10.0 /usr/local/lib/libqpid-proton-core.so.10
RUN ln -s /usr/local/lib/libqpid-proton-core.so.10  /usr/local/lib/libqpid-proton-core.so

RUN ln -s /usr/local/lib/libqpid-proton-cpp.so.12.7.1 /usr/local/lib/libqpid-proton-cpp.so.12
RUN ln -s /usr/local/lib/libqpid-proton-cpp.so.12  /usr/local/lib/libqpid-proton-cpp.so

RUN ln -s /usr/local/lib/libqpid-proton.so.11.11.0 /usr/local/lib/libqpid-proton.so.11
RUN ln -s /usr/local/lib/libqpid-proton.so.11  /usr/local/lib/libqpid-proton.so

COPY include include
COPY CMakeLists.txt CMakeLists.txt 
COPY src src

RUN sudo ldconfig

RUN mkdir build && cd build && cmake .. && make -j8

ENTRYPOINT ./build/main

## rm -rf /var/lib/apt/lists/*

######### BUILDING #################
# FROM base AS overlay

# ARG WS
# WORKDIR $WS
# COPY --from=base $WS $WS
# RUN source /opt/ros/humble/setup.bash && \
#     colcon build 

# COPY docker/humble_openrmf/entrypoint.sh /
# COPY docker/humble_openrmf/fastdds.xml /root/.ros

# RUN chmod 777 /entrypoint.sh
# ENTRYPOINT [ "/entrypoint.sh" ]
