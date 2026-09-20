FROM debian:trixie-slim

RUN apt-get update && \
    apt-get install -y \
    g++ \
    cmake \
    ninja-build \
    libdrogon-dev \
    libsqlite3-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN mkdir -p database

RUN cmake -S . -B build -G Ninja
RUN cmake --build build

EXPOSE 10000

CMD ["./build/BhavanaMart"]