FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y \
    build-essential \
    cmake \
    clang-tools \
    g++ \
    git \
    libpoco-dev \
    libre2-dev \
    clang-format clang-tidy

RUN mkdir /app/
RUN mkdir /app/src/
RUN mkdir /app/src/build/

ADD bot /app/src/bot
ADD cmake /app/src/cmake
ADD contrib /app/src/contrib
ADD util /app/src/util
COPY CMakeLists.txt /app/src/

RUN cd /app/src/build && \
    cmake .. && \
    make all

RUN /app/src/build/test_telegram

RUN mv /app/src/build/bot-run /app/bot-run && rm -rf /app/src/
WORKDIR /app
CMD ./bot-run config/credentials.yml config/offset_backup.data
