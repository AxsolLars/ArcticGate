# ================================
# Build stage
# ================================
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config libmbedtls-dev python3 \
    && rm -rf /var/lib/apt/lists/*

# ---- build open62541 ----
WORKDIR /deps
COPY server/open62541/ .   
RUN rm -rf /deps/build
RUN cmake -B /deps/build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DUA_BUILD_EXAMPLES=OFF \
    -DUA_BUILD_TOOLS=OFF \
    -DUA_ENABLE_PUBSUB=ON \
    -DUA_ENABLE_MQTT=ON \
    -DUA_ENABLE_PUBSUB_MQTT=ON \
    -DUA_ENABLE_JSON_ENCODING=ON && \
    cmake --build /deps/build -j $(nproc) && \
    cmake --install /deps/build --prefix /usr/local
# RUN git clone --recursive --branch v1.5.2 https://github.com/open62541/open62541.git && \
#     cd open62541 && \
#     cmake -B ../build -S . \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DUA_BUILD_EXAMPLES=OFF \
#     -DUA_BUILD_TOOLS=OFF \
#     -DUA_ENABLE_PUBSUB=ON \
#     -DUA_ENABLE_MQTT=ON \
#     -DUA_ENABLE_PUBSUB_MQTT=ON \
#     -DUA_ENABLE_JSON_ENCODING=ON && \
#     cmake --build ../build -j $(nproc) && \
#     cmake --install ../build --prefix /usr/local

# ---- build your app ----
WORKDIR /src
COPY server/pubsub .
RUN rm -rf build && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j

# ================================
# Runtime stage
# ================================
FROM debian:bookworm-slim

# Install only runtime tools you want (optional)
RUN apt-get update && apt-get install -y \
    tcpdump iproute2 iputils-ping net-tools netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only the final binary from build stage
COPY --from=build /src/build/opcua_pubsub /usr/local/bin/opcua_pubsub

# Optional: expose OPC UA ports
EXPOSE 4840/tcp
EXPOSE 4840/udp

# ENTRYPOINT means you only pass args in compose
ENTRYPOINT ["opcua_pubsub"]

# No CMD → you override with docker-compose
