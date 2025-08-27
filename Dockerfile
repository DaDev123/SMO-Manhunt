FROM ubuntu:20.04 as builder

# install dependencies
RUN apt-get update \
  && apt-get install -y \
    curl \
    wget \
    ca-certificates \
    apt-transport-https \
    python3 \
    python3-pip \
  && pip install keystone-engine \
;

# install devkitPro using official installer
RUN wget https://apt.devkitpro.org/install-devkitpro-pacman \
  && chmod +x install-devkitpro-pacman \
  && ./install-devkitpro-pacman \
  && dkp-pacman --noconfirm -S switch-dev \
;

WORKDIR /app/

ENV DEVKITPRO /opt/devkitpro
ENTRYPOINT ["make"]
