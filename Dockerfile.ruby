# Use the Ruby base image
FROM ruby:latest

RUN apt update && apt install -y time

# Your Ruby application setup goes here
WORKDIR /app
COPY build ./build
COPY demos ./demos
COPY Makefile .

RUN make ruby_c_ext

CMD ["make", "ruby_demo"]