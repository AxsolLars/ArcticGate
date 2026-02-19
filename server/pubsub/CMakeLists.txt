cmake_minimum_required(VERSION 3.10)
project(opcua_pubsub_example C)

set(CMAKE_C_STANDARD 99)

# Find open62541 (must be installed or built beforehand)
find_package(open62541 REQUIRED)

# Sources
set(SOURCES
    src/main.c
    src/pubsub/publish.c
    src/pubsub/subscribe.c
    src/pubsub/server.c
    src/pubsub/serverUtils.c
    src/pubsub/collection.c 
    src/pubsub/publishUtil.c 
    src/pubsub/subscribeUtil.c 
    src/config/parser.c
    src/config/printer.c
    src/config/free.c
    src/utils/toml.c
    src/utils/dotenv.c
    src/utils/nodeIdMap.c
)

# Executable
add_executable(opcua_pubsub ${SOURCES})

# Link open62541
target_link_libraries(opcua_pubsub open62541::open62541)

# Add both util and configParser to the include path
target_include_directories(opcua_pubsub PRIVATE
    src/pubsub
    src/config
    src/utils
)
