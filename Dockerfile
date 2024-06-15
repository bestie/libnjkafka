FROM ubuntu:mantic-20240216

# I would prefer not to
ENV DEBIAN_FRONTEND=noninteractive

# All the hits
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        git \
        wget \
        unzip \
        libssl-dev \
        zlib1g-dev \
    && apt-get clean

WORKDIR /home

RUN wget -q https://download.oracle.com/graalvm/22/latest/graalvm-jdk-22_linux-x64_bin.tar.gz && \
    tar -xzf graalvm-jdk-22_linux-x64_bin.tar.gz

RUN wget https://downloads.apache.org/kafka/3.7.0/kafka_2.13-3.7.0.tgz && \
    tar -xzf kafka_2.13-3.7.0.tgz

ENV JAVA_HOME=/home/graalvm-jdk-22.0.1+8.1
ENV PATH=$JAVA_HOME/bin:$PATH

ENV KAFKA_HOME=/home/kafka_2.13-3.7.0

RUN mkdir /libnjkafka
WORKDIR /libnjkafka

COPY src ./src
COPY csrc ./csrc
COPY include ./include
COPY scripts ./scripts
COPY Makefile .

RUN make all

# ENTRYPOINT [ "make", "all" ]
