FROM debian:bookworm-slim

# Arguments pour l'utilisateur non-root
ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Installation des outils de base C++, Node.js, sudo et git
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y --no-install-recommends \
        build-essential \
        gdb \
        cmake \
        git \
        curl \
        sudo \
        ca-certificates \
    && curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

# Création de l'utilisateur non-root et configuration des droits sudo sans mot de passe
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Installation globale de Claude Code
RUN npm install -g @anthropic-ai/claude-code

# Passage à l'utilisateur non-root
USER $USERNAME
WORKDIR /workspace