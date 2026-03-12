FROM ubuntu:questing

# I would prefer not to
ENV DEBIAN_FRONTEND=noninteractive

# All the hits
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        libssl-dev \
        time \
        unzip \
        wget \
        zlib1g-dev \
        # QoL packages: GraalVM makes this image more and 1GB, so why not?
        bash-completion \
        btop \
        curl \
        git \
        less \
        man \
        ncurses-bin \
        tree \
        vim \
    && apt-get clean

WORKDIR /home

ARG ARCHITECTURE=aarch64

# Download GraalVM
ARG JDK_VERSION=25
ARG GRAALVM_ARCHIVE=graalvm-jdk-${JDK_VERSION}_linux-${ARCHITECTURE}_bin.tar.gz
# See https://www.oracle.com/java/technologies/jdk-script-friendly-urls/
RUN wget --quiet https://download.oracle.com/graalvm/${JDK_VERSION}/latest/${GRAALVM_ARCHIVE} \
   && tar -xzf ${GRAALVM_ARCHIVE} \
   && rm ${GRAALVM_ARCHIVE}

# Download Kafka client
ARG KAFKA_ARCHIVE=kafka_2.13-3.9.2.tgz
RUN wget --quiet https://downloads.apache.org/kafka/3.9.2/${KAFKA_ARCHIVE} \
    && tar -xzf ${KAFKA_ARCHIVE} \
    && rm ${KAFKA_ARCHIVE}

ENV JDK_VERSION=$JDK_VERSION
ARG GRAALVM_HOME=/home/graalvm
ENV GRAALVM_HOME=$GRAALVM_HOME

RUN bash -c "ln -s /home/graalvm-jdk-${JDK_VERSION}.* $GRAALVM_HOME"

ARG KAFKA_HOME=/home/kafka
ENV KAFKA_HOME=${KAFKA_HOME}
ARG KAFKA_SHA256=abc44402ddf103e38f19b0e4b44e65da9a831ba9e58fd7725041b1aa168ee8d1

RUN bash -c "ln -s /home/kafka* $KAFKA_HOME"

ENV PATH=$GRAALVM_HOME/bin:$PATH
ENV PATH=$KAFKA_HOME/bin:$PATH

# Mount project files here or use as base image
RUN mkdir /libnjkafka
WORKDIR /libnjkafka
