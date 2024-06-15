OS := $(shell uname -s)
PLATFORM := $(shell uname -m)
ifeq ($(OS),Linux)
    LIB_EXT = so
    CC ?= gcc
    PLATFORM_NATIVE_IMAGE_FLAGS = ""
endif
ifeq ($(OS),Darwin)
    LIB_EXT = dylib
    CC ?= clang
    PLATFORM_NATIVE_IMAGE_FLAGS = "-Ob"
endif

PROJECT_HOME := $(CURDIR)

BUILD_BASE_DIR = build
BUILD_DIR = $(BUILD_BASE_DIR)/$(OS)-$(PLATFORM)

# JAVA compilation
JAVAC = $(JAVA_HOME)/bin/javac
JAVAC_VERSION = 22
GRAALVM_HOME = $(JAVA_HOME)
NATIVE_IMAGE = $(GRAALVM_HOME)/bin/native-image
NATIVE_IMAGE_FLAGS = $(PLATFORM_NATIVE_IMAGE_FLAGS) -cp $(CLASSPATH) --native-compiler-options="-I$(PROJECT_HOME)/$(BUILD_DIR)"  -H:ConfigurationFileDirectories=$(GRAALVM_AGENT_CONFIG_DIR)

# C compilation	
C_SRC = csrc
C_SRCS = $(wildcard $(C_SRC)/*.c)
C_FLAGS = -Wall -Werror -fPIC -I $(BUILD_DIR) -g

# JAVA source
JAVA_PROJECT_STRUCTURE = main/java/com/zendesk/libnjkafka
JAVA_SRC = src/$(JAVA_PROJECT_STRUCTURE)
JAVA_BIN = bin
CLASSPATH = "$(KAFKA_HOME)/libs/*:src/main/resources:$(JAVA_BIN)"
GRAALVM_AGENT_CONFIG_DIR = $(BUILD_BASE_DIR)/graalvm_agent_build_configs
JNI_CONFIG = $(GRAALVM_AGENT_CONFIG_DIR)/jni-config.json
JAVA_ENTRYPOINTS = $(JAVA_BIN)/$(JAVA_SRC)/Entrypoints.class

# C source
C_API_SRC = $(C_SRC)/libnjkafka_c_api.c
STRUCT_DEFINITIONS = include/libnjkafka_structs.h
PUBLIC_C_API_HEADERS = include/libnjkafka.h

# Binaries
GRAALVM_NATIVE_OBJECT = $(BUILD_DIR)/libnjkafka_core.$(LIB_EXT)
C_API_OBJECT = $(BUILD_DIR)/libnjkafka_c_api.o
SHARED_LIBRARY_OBJECT = $(BUILD_DIR)/libnjkafka.$(LIB_EXT)


.PHONY: all
all: native lib

.PHONY: lib
lib: $(SHARED_LIBRARY_OBJECT)

$(SHARED_LIBRARY_OBJECT): $(GRAALVM_NATIVE_OBJECT) $(C_API_OBJECT)
	cp $(PUBLIC_C_API_HEADERS) $(BUILD_DIR)
	$(CC) -shared -o $(SHARED_LIBRARY_OBJECT) $(C_API_OBJECT) $(GRAALVM_NATIVE_OBJECT)

.PHONY: c_api
c_api: $(C_API_OBJECT)

$(C_API_OBJECT): $(C_API_SRC) $(STRUCT_DEFINITIONS)
	$(CC) $(C_FLAGS) -c $(C_API_SRC) -o $(C_API_OBJECT)

.PHONY: native
native: $(GRAALVM_NATIVE_OBJECT)

$(GRAALVM_NATIVE_OBJECT): $(JNI_CONFIG)
	mkdir -p $(BUILD_DIR)
	mkdir -p $(GRAALVM_AGENT_CONFIG_DIR)
	cp $(STRUCT_DEFINITIONS) $(BUILD_DIR)
	$(NATIVE_IMAGE) -o libnjkafka_core --shared -H:Name=$(BUILD_DIR)/libnjkafka_core $(NATIVE_IMAGE_FLAGS)

.PHONY: jni-config
jni-config: $(JNI_CONFIG)

$(JNI_CONFIG): $(JAVA_ENTRYPOINTS)
	timeout 10 java -agentlib:native-image-agent=config-output-dir=$(GRAALVM_AGENT_CONFIG_DIR) -cp $(CLASSPATH) src.main.java.com.zendesk.libnjkafka.JavaDemo

.PHONY: java
java: $(JAVA_ENTRYPOINTS)

$(JAVA_ENTRYPOINTS): $(JAVA_SRC)/*.java
	$(JAVAC) -cp $(CLASSPATH) -d $(JAVA_BIN) $(JAVA_SRC)/*

## Docker #####################################################################

DOCKER_TAG = lib$(LIB_NAME):latest
DOCKER_PROJECT_HOME = /libnjkafka

.PHONY: docker-bash
docker-bash: $(BUILD_BASE_DIR)/.docker_build
	docker run --interactive --tty $(DOCKER_TAG) /bin/bash --login

.PHONY: docker-build
docker-build: $(BUILD_BASE_DIR)/.docker_build

$(BUILD_BASE_DIR)/.docker_build: Dockerfile $(JAVA_SRC)/*.java
	docker build --network host -t $(DOCKER_TAG) . && touch $(BUILD_BASE_DIR)/.docker_build

## Ruby demo ##################################################################

.PHONY: ruby_clean
ruby_clean:
	rm -f ruby/build/*
	rm -f ruby/Makefile
	rm -f ruby/libnjkafka_ext.bundle
	rm -f ruby/libnjkafka_ext.o
	rm -f ruby/mkmf.log

.PHONY: ruby_demo
ruby_demo: ruby/build/libnjkafka.o
	cd ruby && C_EXT_PATH=./build DYLD_LIBRARY_PATH=$(PROJECT_HOME)/$(BUILD_DIR) ruby demo.rb

.PHONY: ruby_c_ext
ruby_c_ext: ruby/build/libnjkafka.o

ruby/build/libnjkafka.o: ruby/build/Makefile ruby/libnjkafka_ext.c
	cp ruby/libnjkafka_ext.c ruby/build
	cd ruby/build && make

ruby/build/Makefile: ruby/extconf.rb
	mkdir -p ruby/build
	cp ruby/extconf.rb ruby/build
	cp ruby/libnjkafka_ext.c ruby/build
	cd ruby/build && LIB_DIR=$(PROJECT_HOME)/$(BUILD_DIR) ruby extconf.rb

## C Demo #####################################################################

DEMO_C_LIBS = -L $(BUILD_DIR) -l njkafka
C_EXECUTABLE = $(BUILD_DIR)/libnjkafka_demo

.PHONY: c_demo
c_demo: $(C_EXECUTABLE)
	$(C_EXECUTABLE)

$(C_EXECUTABLE): $(C_SRCS)
	$(CC) $(C_FLAGS) $(DEMO_C_LIBS) -Wl,-rpath ./ $(C_SRC)/demo.c -o $(C_EXECUTABLE)

## Misc #######################################################################

.PHONY: clean
clean:
	rm -rf ruby/build/*
	rm -rf $(JAVA_BIN)/*
	rm -rf $(GRAALVM_AGENT_CONFIG_DIR)
	rm -rf $(BUILD_DIR)/*
	rm -f *.log
	rm -f *.json

.PHONY: topic
topic: $(BUILD_BASE_DIR)/.topic

.PHONY: delete_topic
delete_topic:
	$(KAFKA_HOME)/bin/kafka-topics.sh --bootstrap-server localhost:9092 --delete --topic test-topic && rm -f $(BUILD_BASE_DIR)/.topic

$(BUILD_BASE_DIR)/.topic:
	mkdir -p $(BUILD_BASE_DIR)
	$(KAFKA_HOME)/bin/kafka-topics.sh --bootstrap-server localhost:9092 --create --partitions=12 --topic test-topic && bash ./scripts/publish_messages.sh && touch $(BUILD_BASE_DIR)/.topic

check_symbols:
	nm -gU $(BUILD_DIR)/libnjkafka.$(LIB_EXT) | grep libnjkafka

jdeps: $(JAVA_BIN)
	jdeps -v -cp $(CLASSPATH) --multi-release $(JAVAC_VERSION) $(JAVA_BIN)

