FROM gcc:15 AS build
RUN apt-get update && apt-get install -y --no-install-recommends cmake git libasio-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2

FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends libstdc++6 && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/ludo_server /usr/local/bin/ludo_server
ENV PORT=8080
EXPOSE 8080
CMD ["ludo_server"]
