
GRAALVM_HOME ?=
ifeq ($(GRAALVM_HOME),)
$(warning GRAALVM_HOME is not set it to root of your GraalVM installation from which bin/native-image can be found.)
endif

PROJECT_HOME := $(CURDIR)
OS := $(shell uname -s)
PLATFORM := $(shell uname -m)
BUILD_BASE_DIR = build
BUILD_DIR = $(BUILD_BASE_DIR)/$(OS)-$(PLATFORM)

ifeq ($(OS),Linux)
	LIB_EXT = so
	CC ?= gcc
	PLATFORM_NATIVE_IMAGE_FLAGS = -Ob
	SHARED_LIBRARY_LINKER_FLAGS = -soname,libnjkafka.so
	NATIVE_IMAGE_LINKER_FLAGS = -soname,libnjkafka_core.so
	LD_LIBRARY_PATH=$(PROJECT_HOME)/$(BUILD_DIR)
endif
ifeq ($(OS),Darwin)
	LIB_EXT = dylib
	CC ?= clang
	PLATFORM_NATIVE_IMAGE_FLAGS = -Ob
	SHARED_LIBRARY_LINKER_FLAGS = -install_name,@rpath/libnjkafka.dylib
	NATIVE_IMAGE_LINKER_FLAGS = -install_name,@rpath/libnjkafka_core.dylib
	LD_LIBRARY_PATH=
endif

# JAVA compilation
JAVA_HOME = $(GRAALVM_HOME)/bin/javac
JAVAC = $(GRAALVM_HOME)/bin/javac
JAVAC_VERSION = 22
NATIVE_IMAGE = $(GRAALVM_HOME)/bin/native-image
NATIVE_IMAGE_FLAGS = $(PLATFORM_NATIVE_IMAGE_FLAGS) -cp $(CLASSPATH) \
 --native-compiler-options="-I$(PROJECT_HOME)/$(BUILD_DIR)" \
 -H:ConfigurationFileDirectories=$(GRAALVM_AGENT_CONFIG_DIR)

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
COMBINED_HEADER=$(BUILD_DIR)/include/libnjkafka.h

# Binaries
GRAALVM_NATIVE_OBJECT = $(BUILD_DIR)/libnjkafka_core.$(LIB_EXT)
C_API_OBJECT = $(BUILD_DIR)/libnjkafka_c_api.o
SHARED_LIBRARY_OBJECT = $(BUILD_DIR)/libnjkafka.$(LIB_EXT)

# Used for generating the dependency reachability metadata and demo programs
KAFKA_BROKERS ?= localhost:9092
KAFKA_TOPIC ?= libnjkafka-build-topic

.PHONY: dist
dist: lib $(COMBINED_HEADER)
	mkdir -p $(BUILD_DIR)/dist
	mkdir -p $(BUILD_DIR)/dist/include
	mkdir -p $(BUILD_DIR)/dist/lib
	cp $(COMBINED_HEADER) $(BUILD_DIR)/dist/include
	cp $(SHARED_LIBRARY_OBJECT) $(BUILD_DIR)/dist/lib
	cp $(C_API_OBJECT) $(BUILD_DIR)/dist/lib

.PHONY: lib
lib: $(SHARED_LIBRARY_OBJECT)
	@echo "Build complete ✅ ✅ ✅"

.PHONY: c-api
c-api: $(C_API_OBJECT)

.PHONY: native
native: $(GRAALVM_NATIVE_OBJECT)

.PHONY: java-demo
java-demo: java
	./scripts/topic prepare
	KAFKA_BROKERS=$(KAFKA_BROKERS) KAFKA_TOPIC=$(KAFKA_TOPIC) java -cp $(CLASSPATH) com.zendesk.libnjkafka.JavaDemo

$(COMBINED_HEADER): $(STRUCT_DEFINITIONS) $(CALLBACK_DEFINITIONS) $(PUBLIC_C_API_HEADERS)
	mkdir -p $(BUILD_DIR)/include
	echo "" > $@
	echo "#ifndef LIBNJKAFKA_H" >> $@
	echo "#define LIBNJKAFKA_H" >> $@
	cat $(STRUCT_DEFINITIONS)   | grep -v "^#" >> $@
	cat $(CALLBACK_DEFINITIONS) | grep -v "^#" >> $@
	cat $(PUBLIC_C_API_HEADERS) | grep -v "^#" >> $@
	echo "#endif" >> $@

$(SHARED_LIBRARY_OBJECT): $(GRAALVM_NATIVE_OBJECT) $(C_API_OBJECT) $(PUBLIC_C_API_HEADERS)
	cp $(PUBLIC_C_API_HEADERS) $(BUILD_DIR)
	$(CC) -shared -o $@ $(C_API_OBJECT) \
	-Wl,$(SHARED_LIBRARY_LINKER_FLAGS) -L$(BUILD_DIR) -lnjkafka_core

$(C_API_OBJECT): $(C_API_SRC) $(STRUCT_DEFINITIONS) $(CALLBACK_DEFINITIONS)
	cp $(CALLBACK_DEFINITIONS) $(BUILD_DIR)
	$(CC) $(C_FLAGS) -I $(BUILD_DIR) -c $(C_API_SRC) -o $(C_API_OBJECT)

$(GRAALVM_NATIVE_OBJECT): $(JAVA_ENTRYPOINTS) $(STRUCT_DEFINITIONS) $(GRAALVM_DEPENDENCY_METADATA)
	mkdir -p $(BUILD_DIR)
	cp $(STRUCT_DEFINITIONS) $(BUILD_DIR)
	cp $(CALLBACK_DEFINITIONS) $(BUILD_DIR)

	$(NATIVE_IMAGE) -o libnjkafka_core --shared --static-nolibc \
		-H:Name=$(BUILD_DIR)/libnjkafka_core $(NATIVE_IMAGE_FLAGS) \
		-H:NativeLinkerOption=-Wl,$(NATIVE_IMAGE_LINKER_FLAGS)

$(GRAALVM_DEPENDENCY_METADATA): $(JAVA_ENTRYPOINTS)
	@echo "Creating directory $(GRAALVM_AGENT_CONFIG_DIR)"
	mkdir -p $(GRAALVM_AGENT_CONFIG_DIR)
	@echo "👍 metadata dir"
	export KAFKA_BROKERS=$(KAFKA_BROKERS)
	export KAFKA_TOPIC=$(KAFKA_TOPIC)
	java -agentlib:native-image-agent=config-output-dir=$(GRAALVM_AGENT_CONFIG_DIR) \
		-cp $(CLASSPATH) \
		com.zendesk.libnjkafka.JavaDemo

.PHONY: java
java: $(JAVA_ENTRYPOINTS)

$(JAVA_ENTRYPOINTS): $(JAVA_SRC)/*.java $(STRUCT_DEFINITIONS)
	@echo "👉 JAVA_ENTRYPOINTS compiling"
	mkdir -p $(JAVA_BIN)
	$(JAVAC) -cp $(CLASSPATH) -d $(JAVA_BIN) $(JAVA_SRC)/*
	@echo "👍 JAVA_ENTRYPOINTS"

## Docker #####################################################################

DEFAULT_DOCKER_TAG ?= lib$(LIB_NAME):latest
DOCKER_TAG ?= $(DEFAULT_DOCKER_TAG)
DOCKER_PROJECT_HOME = /libnjkafka
ifeq ($(PLATFORM),x86_64)
	NATIVE_DOCKER_PLATFORM = linux/amd64
endif
ifeq ($(PLATFORM),arm64)
	NATIVE_DOCKER_PLATFORM = linux/arm64
endif
ifeq ($(PLATFORM),aarch64)
	NATIVE_DOCKER_PLATFORM = linux/arm64
endif

# Set the target platform for docker build, default to the host platform
# override by setting environment variable DOCKER_PLATFORM
DOCKER_PLATFORM ?= $(NATIVE_DOCKER_PLATFORM)
DOCKER_TARGET_PLATFORM = $(DOCKER_PLATFORM)
DOCKER_PLATFORM_FILE_FRIENDLY = $(subst /,-,$(DOCKER_TARGET_PLATFORM))
DOCKER_BUILD_STUB = $(BUILD_BASE_DIR)/docker_build_stub_$(DOCKER_PLATFORM_FILE_FRIENDLY)

ifeq ($(DOCKER_TARGET_PLATFORM),linux/amd64)
	GRAALVM_ARCH = x64
endif
ifeq ($(DOCKER_TARGET_PLATFORM),linux/arm64)
	GRAALVM_ARCH = aarch64
endif

.PHONY: docker-build
docker-build: $(DOCKER_BUILD_STUB)
# 	@echo "NATIVE_DOCKER_PLATFORM=$(NATIVE_DOCKER_PLATFORM)"
# 	@echo "DOCKER_TARGET_PLATFORM=$(DOCKER_TARGET_PLATFORM)"
# 	@echo "DOCKER_PLATFORM_FILE_FRIENDLY=$(DOCKER_PLATFORM_FILE_FRIENDLY)"
# 	@echo "DOCKER_BUILD_STUB=$(DOCKER_BUILD_STUB)"
	@echo "Docker build complete ✅ ✅ ✅"

$(DOCKER_BUILD_STUB): Makefile Dockerfile
	mkdir -p $(BUILD_BASE_DIR)
	docker build --platform=$(DOCKER_TARGET_PLATFORM) \
		--build-arg GRAALVM_ARCH=$(GRAALVM_ARCH) \
		--tag $(DOCKER_TAG) \
		. \
		&& touch $(DOCKER_BUILD_STUB)

build/scripts/docker-run: FORCE
	@mkdir -p build/scripts
	@echo '#!/usr/bin/env sh' > $@
	@echo 'docker run \\' >> $@
	@echo '  --platform=$(DOCKER_TARGET_PLATFORM) \\' >> $@
	@echo '  --rm \\' >> $@
	@echo '  --network=host \\' >> $@
	@echo '  --env KAFKA_BROKERS=$(KAFKA_BROKERS) \\' >> $@
	@echo '  --env KAFKA_TOPIC=$(KAFKA_TOPIC) \\' >> $@
	@echo '  --volume $(PROJECT_HOME):/libnjkafka \\' >> $@
	@echo '  $(DOCKER_TAG) "$$@"' >> $@
	@chmod +x $@

.PHONY: docker-make
docker-make: build/scripts/docker-run
	./build/scripts/docker-run make

.PHONY: docker-c-demo
docker-c-demo: build/scripts/docker-run
	./build/scripts/docker-run make c-demo

.PHONY: docker-bash
docker-bash: build/scripts/docker-run
	./build/scripts/docker-run bash --login

## Demos ######################################################################

DEMO_DIR=$(PROJECT_HOME)/demos

## Ruby demo ##################################################################

.PHONY: ruby-clean
ruby-clean:
	rm -rf $(DEMO_DIR)/ruby/build/*

RUBY_C_EXT_BUNDLE = $(DEMO_DIR)/ruby/build/libnjkafka.bundle
RUBY_DOCKER_SCRIPT = ./build/scripts/docker-ruby-run
RUBY_DOCKER_TAG = libnjkafka-ruby-demo

$(RUBY_DOCKER_SCRIPT): Makefile
	@mkdir -p build/scripts
	@echo '#!/usr/bin/env sh' > $@
	@echo 'docker run \\' >> $@
	@echo '  --platform=$(DOCKER_TARGET_PLATFORM) \\' >> $@
	@echo '  --rm \\' >> $@
	@echo '  --interactive --tty \\' >> $@
	@echo '  --network=host \\' >> $@
	@echo '  --env KAFKA_BROKERS=host.docker.internal:9092 \\' >> $@
	@echo '  --env KAFKA_TOPIC=$(KAFKA_TOPIC) \\' >> $@
	@echo '  --volume $(PROJECT_HOME):/libnjkafka \\' >> $@
	@echo '  $(RUBY_DOCKER_TAG) "$$@"' >> $@
	@chmod +x $@

docker-ruby-demo: $(RUBY_DOCKER_SCRIPT)
	docker run \
		--interactive --tty \
		--rm \
		--network=host \
		--env KAFKA_BROKERS=host.docker.internal:9092 \
		--env KAFKA_TOPIC=libnjkafka-build-topic \
		--volume /Users/stephenbest/code/libnjkafka:/libnjkafka \
		--workdir /libnjkafka \
		ruby:3.4.4-bookworm \
		make ruby-demo

.PHONY: ruby-demo
ruby-demo: $(RUBY_C_EXT_BUNDLE)
	cd $(DEMO_DIR)/ruby && KAFKA_BROKERS=$(KAFKA_BROKERS) KAFKA_TOPIC=$(KAFKA_TOPIC) LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) C_EXT_PATH=./build ruby --disable=gems demo.rb

.PHONY: ruby-c-ext
ruby-c-ext: $(RUBY_C_EXT_BUNDLE)

$(RUBY_C_EXT_BUNDLE): $(DEMO_DIR)/ruby/build/Makefile $(DEMO_DIR)/ruby/libnjkafka_ext.c
	cp $(DEMO_DIR)/ruby/libnjkafka_ext.c $(DEMO_DIR)/ruby/build
	cd $(DEMO_DIR)/ruby/build && LD_LIBRARY_PATH=$(PROJECT_HOME)/$(BUILD_DIR) make

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
	cd $(BUILD_DIR) && LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) KAFKA_BROKERS=$(KAFKA_BROKERS) KAFKA_TOPIC=$(KAFKA_TOPIC) ./$(notdir $(C_EXECUTABLE))

$(C_EXECUTABLE): $(DEMO_DIR)/c/demo.c $(SHARED_LIBRARY_OBJECT) $(COMBINED_HEADER)
	LD_LIBRARY_PATH=$(PROJECT_HOME)/$(BUILD_DIR) \
	$(CC) $(C_FLAGS) \
		-I $(BUILD_DIR)/include $(DEMO_DIR)/c/demo.c $(DEMO_C_LIBS) \
		-Wl,-rpath,@executable_path \
		-o $(C_EXECUTABLE)

## Misc #######################################################################

compile_flags.txt: Makefile
	echo $(C_FLAGS) | tr ' ' '\n' > compile_flags.txt
	echo -Iinclude >> compile_flags.txt
	echo -I$(BUILD_DIR) >> compile_flags.txt
	echo -L$(BUILD_DIR) >> compile_flags.txt


.PHONY: clean-all
clean-all: ruby-clean
	rm -rf build
	mkdir $(BUILD_BASE_DIR)

.PHONY: clean
clean: ruby-clean
	rm -rf $(JAVA_BIN)
	rm -rf $(GRAALVM_AGENT_CONFIG_DIR)
	rm -rf $(BUILD_DIR)
	rm -f *.log
	rm -f *.json
	rm -f $(BUILD_BASE_DIR)/.docker_build

.PHONY: topic-create
topic-create:
	KAFKA_TOPIC=$(KAFKA_TOPIC) KAFKA_BROKERS=$(KAFKA_BROKERS) ./scripts/topic create && ./scripts/publish_messages.sh

.PHONY: topic-delete
topic-delete:
	KAFKA_TOPIC=$(KAFKA_TOPIC) KAFKA_BROKERS=$(KAFKA_BROKERS) ./scripts/topic delete

.PHONY: check-symbols
check-symbols:
	nm -gU $(BUILD_DIR)/libnjkafka.$(LIB_EXT) | grep libnjkafka

.PHONY: jdeps
jdeps: $(JAVA_BIN)
	jdeps -v -cp $(CLASSPATH) --multi-release $(JAVAC_VERSION) $(JAVA_BIN)

.PHONY: FORCE
FORCE:
