#!/bin/bash
set -euo pipefail

sudo /opt/homebrew/bin/python3 -c "
import usb.core, usb.backend.libusb1, sys
b = usb.backend.libusb1.get_backend(find_library=lambda x: '/opt/homebrew/lib/libusb-1.0.dylib')
d = usb.core.find(idVendor=0x1d6b, idProduct=0x0102, backend=b)
if d is None:
    sys.exit('DepthEye not found')
d.detach_kernel_driver(1)
print('detach OK')
"

sleep 1

sudo ./bin/DepthAscii
#sudo ./bin/DepthMeasure