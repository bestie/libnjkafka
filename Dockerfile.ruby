FROM ruby:3.3.6-bookworm

RUN apt update && apt install -y time

# Mount project files here or use as base image
RUN mkdir /libnjkafka
WORKDIR /libnjkafka
