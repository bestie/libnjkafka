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
    # Only macOS gets fast builds, don't @ me.
    PLATFORM_NATIVE_IMAGE_FLAGS = "-Ob"
endif

GRAALVM_HOME ?=
ifeq ($(GRAALVM_HOME),)
$(warning GRAALVM_HOME is not set it to root of your GraalVM installation from which bin/native-image can be found.)
endif


PROJECT_HOME := $(CURDIR)

BUILD_BASE_DIR = build
BUILD_DIR = $(BUILD_BASE_DIR)/$(OS)-$(PLATFORM)

# JAVA compilation
JAVA_HOME = $(GRAALVM_HOME)
JAVAC = $(JAVA_HOME)/bin/javac
JAVAC_VERSION = 22
NATIVE_IMAGE = $(GRAALVM_HOME)/bin/native-image
NATIVE_IMAGE_FLAGS = $(PLATFORM_NATIVE_IMAGE_FLAGS) -cp $(CLASSPATH) --native-compiler-options="-I$(PROJECT_HOME)/$(BUILD_DIR)"  -H:ConfigurationFileDirectories=$(GRAALVM_AGENT_CONFIG_DIR)

# C compilation
C_SRC = csrc
C_SRCS = $(wildcard $(C_SRC)/*.c)
C_FLAGS = -Wall -Werror -std=c11 -fPIC -g

# JAVA source
JAVA_SRC = src/main/java/com/zendesk/libnjkafka
JAVA_BIN = build/java_bin
CLASSPATH = "$(KAFKA_HOME)/libs/*:src/main/resources:$(JAVA_BIN)"
GRAALVM_AGENT_CONFIG_DIR = $(BUILD_BASE_DIR)/graalvm_agent_build_configs
GRAALVM_DEPENDENCY_METADATA = $(GRAALVM_AGENT_CONFIG_DIR)/reachability-metadata.json
JAVA_ENTRYPOINTS = $(JAVA_BIN)/com/zendesk/libnjkafka/Entrypoints.class

# C source
C_API_SRC = $(C_SRC)/libnjkafka_c_api.c
STRUCT_DEFINITIONS = include/libnjkafka_structs.h
CALLBACK_DEFINITIONS = include/libnjkafka_callbacks.h
PUBLIC_C_API_HEADERS = include/libnjkafka.h

# Binaries
GRAALVM_NATIVE_OBJECT = $(BUILD_DIR)/libnjkafka_core.$(LIB_EXT)
C_API_OBJECT = $(BUILD_DIR)/libnjkafka_c_api.o
SHARED_LIBRARY_OBJECT = $(BUILD_DIR)/libnjkafka.$(LIB_EXT)

# Used for generating the dependency reachability metadata and demo programs
KAFKA_BROKERS ?= localhost:9092
KAFKA_TOPIC ?= libnjkafka-build-topic

.PHONY: all
all: native lib
	@echo "Build complete ✅ ✅ ✅"

.PHONY: lib
lib: $(SHARED_LIBRARY_OBJECT)

$(SHARED_LIBRARY_OBJECT): $(GRAALVM_NATIVE_OBJECT) $(C_API_OBJECT) $(PUBLIC_C_API_HEADERS)
	cp $(PUBLIC_C_API_HEADERS) $(BUILD_DIR)
	$(CC) -shared -o $(SHARED_LIBRARY_OBJECT) $(C_API_OBJECT) \
		-L$(PROJECT_HOME)/$(BUILD_DIR) \
		-lnjkafka_core

.PHONY: c-api
c-api: $(C_API_OBJECT)

$(C_API_OBJECT): $(C_API_SRC) $(STRUCT_DEFINITIONS) $(CALLBACK_DEFINITIONS)
	cp $(CALLBACK_DEFINITIONS) $(BUILD_DIR)
	$(CC) $(C_FLAGS) -I $(BUILD_DIR) -c $(C_API_SRC) -o $(C_API_OBJECT)

.PHONY: native
native: $(GRAALVM_NATIVE_OBJECT)

$(GRAALVM_NATIVE_OBJECT): $(GRAALVM_DEPENDENCY_METADATA) $(STRUCT_DEFINITIONS)
	mkdir -p $(BUILD_DIR)
	cp $(STRUCT_DEFINITIONS) $(BUILD_DIR)
	cp $(CALLBACK_DEFINITIONS) $(BUILD_DIR)
	$(NATIVE_IMAGE) -o libnjkafka_core --shared --static-nolibc -H:Name=$(BUILD_DIR)/libnjkafka_core $(NATIVE_IMAGE_FLAGS)

.PHONY: java-demo
java-demo: java
	./scripts/topic prepare
	KAFKA_BROKERS=$(KAFKA_BROKERS) KAFKA_TOPIC=$(KAFKA_TOPIC) java -cp $(CLASSPATH) com.zendesk.libnjkafka.JavaDemo

$(GRAALVM_DEPENDENCY_METADATA): $(JAVA_ENTRYPOINTS)
	./scripts/topic prepare
	mkdir -p $(GRAALVM_AGENT_CONFIG_DIR)
	KAFKA_BROKERS=$(KAFKA_BROKERS) KAFKA_TOPIC=$(KAFKA_TOPIC) timeout 10 java -agentlib:native-image-agent=config-output-dir=$(GRAALVM_AGENT_CONFIG_DIR) -cp $(CLASSPATH) com.zendesk.libnjkafka.JavaDemo

.PHONY: java
java: $(JAVA_ENTRYPOINTS)

$(JAVA_ENTRYPOINTS): $(JAVA_SRC)/*.java $(STRUCT_DEFINITIONS)
	mkdir -p $(JAVA_BIN)
	$(JAVAC) -cp $(CLASSPATH) -d $(JAVA_BIN) $(JAVA_SRC)/*

.PHONY: code-gen
code-gen: $(BUILD_DIR)/generator
	$(BUILD_DIR)/generator > $(JAVA_SRC)/StructSizeRegistry.java

$(BUILD_DIR)/generator: src/main/java/com/zendesk/code_gen/StructRegistryGenerator.java src/main/java/com/zendesk/libnjkafka/Structs.java include/libnjkafka_structs.h
	mkdir -p $(BUILD_DIR)
	javac -cp $(CLASSPATH) -d $(JAVA_BIN) src/main/java/com/zendesk/code_gen/StructRegistryGenerator.java src/main/java/com/zendesk/libnjkafka/Structs.java
	cp include/* $(BUILD_DIR)
	$(NATIVE_IMAGE) com.zendesk.code_gen.StructRegistryGenerator $(NATIVE_IMAGE_FLAGS) -o $(BUILD_DIR)/generator

## Docker #####################################################################

DOCKER_TAG ?= lib$(LIB_NAME):latest
DOCKER_PROJECT_HOME = /libnjkafka

.PHONY: docker-build
docker-build: build/.docker_build

build/.docker_build: Dockerfile Makefile $(C_SRCS) $(JAVA_SRC)/* include/* demos/*
	mkdir -p $(BUILD_BASE_DIR)
	docker build -t $(DOCKER_TAG) . && touch build/.docker_build

.PHONY: docker-make
docker-make: docker-build
	docker run \
		--rm \
		--network=host \
		-e KAFKA_BROKERS=host.docker.internal:9092 \
		-v $(PROJECT_HOME):/libnjkafka \
		$(DOCKER_TAG) \
		make

.PHONY: docker-demos
docker-demos: docker-build
	docker run \
		--rm \
		--network=host \
		-e KAFKA_BROKERS=host.docker.internal:9092 \
		-v $(PROJECT_HOME):/libnjkafka \
		$(DOCKER_TAG) \
		make c_demo

.PHONY: docker-bash
docker-bash: docker-build
	docker run \
		--rm --interactive --tty \
		--network=host \
		-e KAFKA_BROKERS=host.docker.internal:9092 \
		-v $(PROJECT_HOME):$(DOCKER_PROJECT_HOME) \
		$(DOCKER_TAG) \
		/bin/bash -l

## Demos ######################################################################

DEMO_DIR=$(PROJECT_HOME)/demos

## Ruby demo ##################################################################

.PHONY: ruby-clean
ruby-clean:
	rm -f $(DEMO_DIR)/ruby/build/*

RUBY_C_EXT_BUNDLE = $(DEMO_DIR)/ruby/build/libnjkafka.bundle

docker-ruby-demo: ruby_clean
	docker build -t libnjkafka-ruby-demo -f Dockerfile.ruby .
	docker run \
		--rm \
		--interactive --tty \
		--network=host \
		-e KAFKA_BROKERS=host.docker.internal:9092 \
		-v $(PROJECT_HOME):/libnjkafka \
		libnjkafka-ruby-demo \
		make ruby_demo

.PHONY: ruby-demo
ruby-demo: $(RUBY_C_EXT_BUNDLE)
	cd $(DEMO_DIR)/ruby && KAFKA_BROKERS=$(KAFKA_BROKERS) KAFKA_TOPIC=$(KAFKA_TOPIC) C_EXT_PATH=./build LD_LIBRARY_PATH=$(PROJECT_HOME)/$(BUILD_DIR) ruby --disable=gems demo.rb

.PHONY: ruby-c-ext
ruby-c-ext: $(RUBY_C_EXT_BUNDLE)

$(RUBY_C_EXT_BUNDLE): $(DEMO_DIR)/ruby/build/Makefile $(DEMO_DIR)/ruby/libnjkafka_ext.c
	cp $(DEMO_DIR)/ruby/libnjkafka_ext.c $(DEMO_DIR)/ruby/build
	cd $(DEMO_DIR)/ruby/build && LD_LIBRARY_PATH=$(DOCKER_PROJECT_HOME)/$(BUILD_DIR) make
	if [ "$(OS)" = "Darwin" ]; then \
		install_name_tool -change $(SHARED_LIBRARY_OBJECT) @rpath/libnjkafka.dylib $(PROJECT_HOME)/demos/ruby/build/libnjkafka_ext.bundle; \
		install_name_tool -add_rpath $(PROJECT_HOME)/$(BUILD_DIR) $(PROJECT_HOME)/demos/ruby/build/libnjkafka_ext.bundle; \
	fi

.PHONY: ruby-make-file
ruby-make-file: $(DEMO_DIR)/ruby/build/Makefile

$(DEMO_DIR)/ruby/build/Makefile: $(DEMO_DIR)/ruby/extconf.rb
	mkdir -p $(DEMO_DIR)/ruby/build
	cp $(DEMO_DIR)/ruby/extconf.rb $(DEMO_DIR)/ruby/build
	cp $(DEMO_DIR)/ruby/libnjkafka_ext.c $(DEMO_DIR)/ruby/build
	cd $(DEMO_DIR)/ruby/build && LIB_DIR=$(PROJECT_HOME)/$(BUILD_DIR) LD_LIBRARY_PATH=$(PROJECT_HOME)/$(BUILD_DIR) ruby extconf.rb

## C Demo #####################################################################

DEMO_C_LIBS = -L $(BUILD_DIR) -l njkafka
C_EXECUTABLE = $(BUILD_DIR)/libnjkafka_c_demo

.PHONY: c-demo
c-demo: $(C_EXECUTABLE)
	./scripts/topic prepare
	LD_LIBRARY_PATH=$(DOCKER_PROJECT_HOME)/$(BUILD_DIR) KAFKA_BROKERS=$(KAFKA_BROKERS) KAFKA_TOPIC=$(KAFKA_TOPIC) $(C_EXECUTABLE)

$(C_EXECUTABLE): $(DEMO_DIR)/c/demo.c
	LD_LIBRARY_PATH=$(DOCKER_PROJECT_HOME)/$(BUILD_DIR) $(CC) $(C_FLAGS) -I $(BUILD_DIR) $(DEMO_DIR)/c/demo.c $(DEMO_C_LIBS) -Wl,-rpath ./ -o $(C_EXECUTABLE)

## Misc #######################################################################

compile_flags.txt: Makefile
	echo $(C_FLAGS) | tr ' ' '\n' > compile_flags.txt
	echo -Iinclude >> compile_flags.txt
	echo -I$(BUILD_DIR) >> compile_flags.txt
	echo -L$(BUILD_DIR) >> compile_flags.txt

.PHONY: clean
clean:
	rm -rf ruby/build/*
	rm -rf $(JAVA_BIN)
	rm -rf $(GRAALVM_AGENT_CONFIG_DIR)
	rm -rf $(BUILD_BASE_DIR)/*
	rm -f *.log
	rm -f *.json
	rm -f $(BUILD_BASE_DIR)/.docker_build

.PHONY: topic-create
topic-create:
	KAFKA_TOPIC=$(KAFKA_TOPIC) KAFKA_BROKERS=$(KAFKA_BROKERS) ./scripts/topic create

.PHONY: topic-delete
topic-delete:
	KAFKA_TOPIC=$(KAFKA_TOPIC) KAFKA_BROKERS=$(KAFKA_BROKERS) ./scripts/topic delete

.PHONY: check-symbols
check-symbols:
	nm -gU $(BUILD_DIR)/libnjkafka.$(LIB_EXT) | grep libnjkafka

.PHONY: jdeps
jdeps: $(JAVA_BIN)
	jdeps -v -cp $(CLASSPATH) --multi-release $(JAVAC_VERSION) $(JAVA_BIN)
