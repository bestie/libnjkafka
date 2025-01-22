FROM ubuntu:plucky-20241213

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
        time \
    && apt-get clean

WORKDIR /home

ARG ARCHITECTURE=aarch64

# Download GraalVM
ARG JDK_VERSION=23
ARG GRAALVM_ARCHIVE=graalvm-jdk-${JDK_VERSION}_linux-${ARCHITECTURE}_bin.tar.gz
# See https://www.oracle.com/java/technologies/jdk-script-friendly-urls/
RUN wget --quiet https://download.oracle.com/graalvm/${JDK_VERSION}/latest/${GRAALVM_ARCHIVE}

# Download Kafka client
ARG KAFKA_ARCHIVE=kafka_2.13-3.9.0.tgz
RUN wget --quiet https://downloads.apache.org/kafka/3.9.0/${KAFKA_ARCHIVE}

ENV JDK_VERSION=$JDK_VERSION
ARG GRAALVM_HOME=/home/graalvm
ENV GRAALVM_HOME=$GRAALVM_HOME
# For arm64, set env var for other architectures
ARG GRAALVM_SHA256=1835a98b87c439c8c654d97956c22d409855952e5560a8127f56c50f3f919d7d

# RUN echo "$GRAALVM_SHA256 *${GRAALVM_ARCHIVE}" | sha256sum --strict --check
RUN tar -xzf ${GRAALVM_ARCHIVE}
RUN rm ${GRAALVM_ARCHIVE}
RUN bash -c "ln -s /home/graalvm-jdk-${JDK_VERSION}.* $GRAALVM_HOME"

ARG KAFKA_HOME=/home/kafka
ENV KAFKA_HOME=${KAFKA_HOME}
ARG KAFKA_SHA256=abc44402ddf103e38f19b0e4b44e65da9a831ba9e58fd7725041b1aa168ee8d1

# RUN echo "$KAFKA_SHA256 *${KAFKA_ARCHIVE}" | sha256sum --strict --check
RUN tar -xzf ${KAFKA_ARCHIVE}
RUN rm ${KAFKA_ARCHIVE}
RUN bash -c "ln -s /home/kafka* $KAFKA_HOME"

ENV PATH=$GRAALVM_HOME/bin:$PATH
ENV PATH=$KAFKA_HOME/bin:$PATH

# Mount project files here or use as base image
RUN mkdir /libnjkafka
WORKDIR /libnjkafka
