FROM ubuntu:20.04

RUN apt update && apt install apt-transport-https curl gnupg git -y
RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg
RUN mv bazel-archive-keyring.gpg /usr/share/keyrings
RUN echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
RUN apt update && apt install -y bazel
RUN apt -y full-upgrade
RUN apt install -y bazel-5.2.0

# Local directory would be mounted under /opt/proxy-wasm-cli/
# Local bazel cache would be mounted under /opt/bazel-cache

RUN mkdir -p ~/.cache/
RUN ln -s /opt/bazel-cache ~/.cache/bazel
COPY build_inside_docker.sh /
#COPY wait.sh /

WORKDIR /opt/proxy-wasm-cli/
CMD ["/build_inside_docker.sh"]
#CMD ["/wait.sh"]
