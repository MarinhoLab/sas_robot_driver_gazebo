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
RUN sudo apt-get install -y ros-jazzy-sas-*
# ROS-free SmartArmStack core (MarinhoLab/sas_cpp) that provides the
# marinholab::sas::core::modeling::SerialManipulatorSimulatorFriendly kinematic
# model used by the SDF serial-manipulator loader.
RUN sudo apt-get install -y libmarinholab-sas-core
RUN sudo apt-get install -y libgz-transport13-dev
# The apt `sas_core` package is a thin compatibility shim that re-exports the
# Python bindings from the `marinholab-sas-core` PyPI package (MarinhoLab/sas_py).
# Install it so Python nodes that `from sas_core import ...` can import it.
# (PEP 668: the image's Python is system-managed, hence --break-system-packages.)
RUN pip install --break-system-packages marinholab-sas-core
RUN mkdir -p /root/sas_robot_driver_gazebo_devel/src/
COPY . /root/sas_robot_driver_gazebo_devel/src/sas_robot_driver_gazebo