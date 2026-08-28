FROM node:22-trixie-slim

# 1. Installation des outils système nécessaires (build-essential, gdb, git, etc.)
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y --no-install-recommends \
        build-essential \
        gdb \
        cmake \
        git \
        curl \
        sudo \
        alsa-utils \
        cmake \
        libboost-all-dev \
        libusb-1.0-0-dev \
        libssl-dev \
        libprotobuf-dev \
        protobuf-c-compiler \
        protobuf-compiler \
        qt6-base-dev \
        qt6-multimedia-dev \
        libqt6multimedia6 \
        libqt6multimediawidgets6 \
        libqt6bluetooth6 \
        qt6-connectivity-dev \
        pulseaudio \
        pulseaudio-module-bluetooth \
        librtaudio-dev \
        librtaudio7 \
        libkf6bluezqt-dev \
        libtag1-dev \
        qml6-module-qtquick \
        libglib2.0-dev \
        libgstreamer1.0-dev \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-base-apps \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-libav \
        gstreamer1.0-alsa \
        gstreamer1.0-qt6 \
        libgstreamer-plugins-base1.0-dev \
        qt6-declarative-dev \
        libgstreamer-plugins-bad1.0-dev \
        libunwind-dev \
        qml6-module-qtmultimedia \
        qt6-serialbus-dev \
        qt6-serialport-dev \
        qt6-websockets-dev \
        qt6-svg-dev \
        build-essential \
        libtool \
        autoconf \
        ffmpeg \
        libgpiod-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Création du répertoire et attribution des droits à l'utilisateur 'node'
RUN mkdir -p /workspaces/dash && chown -R node:node /workspaces

# 2. Utilisation de l'utilisateur 'node' par défaut présent dans l'image
USER node
WORKDIR /home/node

# 3. Configuration de npm pour installer les paquets globaux dans le dossier personnel de l'utilisateur 'node'
RUN mkdir -p /home/node/.npm-global \
    && npm config set prefix '/home/node/.npm-global'

# 3 bis. Ajout du dossier bin de npm au PATH
ENV PATH="/home/node/.npm-global/bin:${PATH}"

# 4. Installation globale de Claude Code
RUN mkdir -p /home/node/.claude \
    && npm install -g @anthropic-ai/claude-code

# 4 bis. Config du dossier Claude
ENV CLAUDE_CONFIG_DIR=/home/node/.claude

WORKDIR /workspaces/dash