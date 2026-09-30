#!/usr/bin/env python3
"""Capture one frame from a Gazebo camera image topic into a PNG.

Runs inside the Gazebo Docker container (base image
`ghcr.io/marinholab/gazebo:jazzy`). Subscribes to a camera image topic
with the in-image gz Python bindings (gz.transport13 / gz.msgs10) and
decodes the first `gz.msgs.Image` message with Pillow.

Usage:
    python3 snapshot_camera.py TOPIC OUT_PNG [TIMEOUT_SEC]

Example (topic name exactly as reported by `gz topic -l`):
    python3 snapshot_camera.py \
        /world/snapshot_world/model/snapshot_camera/link/link/sensor/camera/image \
        /snap/snapshot.png 20
"""
import sys
import time

from PIL import Image
from gz.msgs10 import image_pb2
from gz.msgs10.image_pb2 import Image as GzImage
from gz.transport13 import Node

# gz.msgs pixel-format enum value -> (Pillow mode, bytes per pixel)
FORMAT_PIL = {
    image_pb2.L_INT8: ("L", 1),
    image_pb2.RGB_INT8: ("RGB", 3),
    image_pb2.RGBA_INT8: ("RGBA", 4),
    image_pb2.BGRA_INT8: ("BGRA", 4),
    image_pb2.BGR_INT8: ("BGR", 3),
}


def main() -> None:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    topic, out_png = sys.argv[1], sys.argv[2]
    timeout = float(sys.argv[3]) if len(sys.argv) > 3 else 30.0

    captured: dict = {}

    def on_image(msg: GzImage, _msg_info=None) -> None:
        if "msg" not in captured:
            captured["msg"] = msg

    node = Node()
    node.subscribe(GzImage, topic, on_image)

    deadline = time.monotonic() + timeout
    while "msg" not in captured and time.monotonic() < deadline:
        time.sleep(0.1)
    if "msg" not in captured:
        sys.exit(f"timeout: no image received on {topic} within {timeout}s")

    msg = captured["msg"]
    width, height, step = msg.width, msg.height, msg.step
    fmt = msg.pixel_format_type
    if fmt not in FORMAT_PIL:
        sys.exit(f"unsupported pixel format: "
                 f"{image_pb2.PixelFormatType.Name(fmt)}")
    pil_mode, bpp = FORMAT_PIL[fmt]
    raw = bytes(msg.data)
    if len(raw) < width * height * bpp:
        sys.exit(f"payload too small: {len(raw)} < {width * height * bpp}")

    img = Image.frombytes(pil_mode, (width, height), raw, "raw", pil_mode,
                          step or -1)
    if pil_mode == "BGR":
        r, g, b = img.split()
        img = Image.merge("RGB", (b, g, r))
    elif pil_mode == "BGRA":
        b, g, r, a = img.split()
        img = Image.merge("RGB", (r, g, b))
    img.save(out_png)
    print(f"saved {width}x{height} frame to {out_png}")


if __name__ == "__main__":
    main()
