FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc libc6-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY Shell_project.c job_control.c job_control.h parse_redir.h ./
RUN gcc -Wall -Wextra -g Shell_project.c job_control.c -o shell -pthread

FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/shell /usr/local/bin/shell-linux

ENTRYPOINT ["/usr/local/bin/shell-linux"]
