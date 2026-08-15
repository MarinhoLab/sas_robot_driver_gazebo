FROM ghcr.io/marinholab/gazebo:jazzy
LABEL authors="murilomarinho"
SHELL ["/bin/bash", "-c"]
ENV BASH_ENV="/etc/bash_env"

RUN apt-get update && apt-get upgrade -y

RUN curl -s --compressed "https://smartarmstack.github.io/smart_arm_stack_ROS2/KEY.gpg" \
| gpg --dearmor \
| sudo tee /etc/apt/trusted.gpg.d/smartarmstack_lgpl.gpg >/dev/null
RUN sudo curl -s --compressed -o /etc/apt/sources.list.d/smartarmstack_lgpl.list \
"https://smartarmstack.github.io/smart_arm_stack_ROS2/smartarmstack_lgpl.list"
RUN sudo apt-get update
RUN sudo apt-get install ros-jazzy-sas-*
RUN sudo apt-get install libgz-transport13-dev
RUN mkdir -p /root/sas_robot_driver_gazebo_devel/src/
COPY . /root/sas_robot_driver_gazebo_devel/src/sas_robot_driver_gazebo