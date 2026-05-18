FROM ghcr.io/uommscrobotics/sfr_gazebo_nav2:latest
LABEL authors="murilomarinho"
SHELL ["/bin/bash", "-c"]
ENV BASH_ENV="/etc/bash_env"

COPY --from=repo . /root/sas/
RUN source /opt/ros/jazzy/setup.bash && cd /root/sas/vendor && colcon build
