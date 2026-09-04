FROM node:20-bookworm AS panel
WORKDIR /app/panel
COPY panel/package*.json ./
RUN npm ci
COPY panel/ ./
RUN npm run build

FROM ubuntu:22.04 AS build
RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    libopencv-dev curl ca-certificates \
    qt6-base-dev qt6-websockets-dev qt6-webengine-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
COPY --from=panel /app/panel/dist ./panel/dist

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DFOVEA_ENABLE_ONNX=OFF -DFOVEA_ENABLE_QT=ON -DFOVEA_ENABLE_VIDEO=ON \
    && cmake --build build

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libopencv-dev libgl1 libglib2.0-0 libxkbcommon0 \
    qt6-base-dev qt6-websockets-dev qt6-webengine-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/build/app/fovea_app /usr/local/bin/fovea_app
COPY --from=panel /app/panel/dist /app/panel/dist
COPY assets /app/assets

ENV FOVEA_PANEL_DIST=/app/panel/dist/index.html
CMD ["fovea_app", "--grok"]
