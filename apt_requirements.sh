#!/bin/bash
set -e

sudo apt-get install -y python3-gz-transport13

# The below was needed in macOS.
python3 -m pip install protobuf --user --break-system-packages
