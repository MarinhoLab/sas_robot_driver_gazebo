#!/bin/bash
set -e

# Clone vendor repositories into ~/.sas/sas_robot_driver_gazebo/vendor
# Usage: setup_vendor.sh <robot>
#   robot: ur | unitree | bota | agilex | all

VENDOR_DIR="$HOME/.sas/sas_robot_driver_gazebo/vendor"

usage() {
    echo "Usage: $0 <robot>"
    echo "  robot: ur | unitree | bota | agilex | all"
    echo ""
    echo "Clones vendor repositories into $VENDOR_DIR"
}

[ "$1" = "--help" ] || [ "$1" = "-h" ] && { usage; exit 0; }
[ -z "$1" ] && { usage; exit 1; }

clone_if_missing() {
    local repo_name="$1"
    local repo_url="$2"
    if [ -d "$repo_name" ]; then
        echo "[$repo_name] Already exists, skipping."
    else
        echo "[$repo_name] Cloning..."
        git clone "$repo_url" --depth 1
        echo "[$repo_name] Done."
    fi
}

clone_robot() {
    case "$1" in
        ur)
            clone_if_missing Universal_Robots_ROS2_Description \
                https://github.com/UniversalRobots/Universal_Robots_ROS2_Description.git
            ;;
        unitree)
            clone_if_missing unitree_ros \
                https://github.com/unitreerobotics/unitree_ros.git
            ;;
        bota)
            clone_if_missing bota_driver_ros2 \
                https://gitlab.com/botasys/drivers/bota_driver_ros2.git
            ;;
        agilex)
            clone_if_missing ugv_gazebo_sim \
                https://github.com/agilexrobotics/ugv_gazebo_sim.git
            ;;
        *)
            echo "Unknown robot: $1"
            usage
            ;;
    esac
}

mkdir -p "$VENDOR_DIR"
cd "$VENDOR_DIR"

case "$1" in
    all)
        clone_robot ur
        clone_robot unitree
        clone_robot bota
        clone_robot agilex
        ;;
    *)
        clone_robot "$1"
        ;;
esac

echo ""
echo "Done. Vendor repos are in $VENDOR_DIR"