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

# add devkitPro repo + install pacman + switch toolchain
RUN curl -fsSL https://apt.devkitpro.org/devkitpro-pub.gpg -o /usr/share/keyrings/devkitpro-pub.gpg \
  && echo "deb [signed-by=/usr/share/keyrings/devkitpro-pub.gpg] https://apt.devkitpro.org stable main" > /etc/apt/sources.list.d/devkitpro.list \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y devkitpro-pacman \
  && dkp-pacman --noconfirm -S switch-dev \
;

WORKDIR /app/

ENV DEVKITPRO=/opt/devkitpro

ENTRYPOINT ["make"]
