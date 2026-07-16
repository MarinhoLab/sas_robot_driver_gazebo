FROM ghcr.io/marinholab/gazebo:jazzy
LABEL authors="murilomarinho"
SHELL ["/bin/bash", "-c"]
ENV BASH_ENV="/etc/bash_env"

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y tree
COPY --from=repo . /root/sas/
