FROM node:22-bookworm-slim

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
        libqt5multimedia5 \
        libqt5multimedia5-plugins \
        libqt5multimediawidgets5 \
        qtmultimedia5-dev \
        libqt5bluetooth5 \
        libqt5bluetooth5-bin \
        qtconnectivity5-dev \
        pulseaudio \
        pulseaudio-module-bluetooth \
        librtaudio-dev \
        librtaudio6 \
        libkf5bluezqt-dev \
        libtag1-dev \
        qml-module-qtquick2 \
        libglib2.0-dev \
        libgstreamer1.0-dev \
        gstreamer1.0-plugins-base-apps \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-libav \
        gstreamer1.0-alsa \
        libgstreamer-plugins-base1.0-dev \
        qtdeclarative5-dev \
        libgstreamer-plugins-bad1.0-dev \
        libunwind-dev \
        qml-module-qtmultimedia \
        libqt5serialbus5-dev \
        libqt5serialbus5-plugins \
        libqt5serialport5-dev \
        libqt5websockets5-dev \
		libqt5svg5-dev \
        build-essential \
        libtool \
        autoconf \
        ffmpeg \
        libgpiod-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

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

# 6. Définition du répertoire de travail final
WORKDIR /workspace
