ENV ?= standard
ENV_FILE := env/$(ENV).env
DEBUG ?= false
PYTHON := .venv/bin/python3
GROUP_COUNT ?= 5
DATASET_COUNT ?= 10
FIELD_COUNT ?= 2
CONFIG_NAME ?= config.toml

CONFIG_SCRIPT ?= scripts/generate_sample_config.py
CONFIG_DIR ?= configs

include $(ENV_FILE)
export $(shell sed -n 's/=.*//p' $(ENV_FILE))
export VFS = $(shell expr $(PUB_COUNT) + $(SUB_COUNT))
build:
	@echo "Building Docker image: $(IMAGE_NAME)"
	docker build -t $(IMAGE_NAME) -f server/Dockerfile .
# Explanation of each line:
# port: dynamically set the port for the container in ENV SubBsePort needs to be Pub_Count away from PubBasePort
# docker run is the command to just run the docker, -d the flag so the logs don't occupy the terminal
# --name sets the name dynamically with the count of publishers
# -p gives the port. on the docker side its always 4840, it counts up in the before set port space
# -v gives the volumes, configs and the given env for the c program
# -e sets dynamic environment variables as to avoid args parsing
docker:
	@for i in $(shell seq 0 $$(($(PUB_COUNT) - 1))); do \
		port=$$((PUB_BASE_PORT + i)); \
		echo "Starting pub$$i on port $$port"; \
		docker run -d \
			--name pub$$i \
			-p $$port:4840 \
			-v $$(pwd)/configs:/$(CONFIG_DOCKER_DIR) \
			-v $$(pwd)/$(ENV_FILE):/app/.env:ro \
			-e ID=$$i \
			-e ROLE=pub \
			-e DEBUG=$(DEBUG) \
			$(IMAGE_NAME) ./opcua_pubsub; \
	done; \
	for i in $(shell seq 0 $$(($(SUB_COUNT) - 1))); do \
		port=$$((SUB_BASE_PORT + i)); \
		echo "Starting sub$$i on port $$port"; \
		docker run -d \
			--name sub$$i \
			-p $$port:4840 \
			-v $$(pwd)/configs:/app/configs \
			-v $$(pwd)/$(ENV_FILE):/app/.env:ro \
			-e ID=$$i \
			-e ROLE=sub\
			-e DEBUG=$(DEBUG) \
			$(IMAGE_NAME) ./opcua_pubsub; \
	done

up: build docker 

down:
	docker rm -f $$(docker ps -aq --filter "name=^pub[0-9]+") || true
	docker rm -f $$(docker ps -aq --filter "name=^sub[0-9]+") || true

