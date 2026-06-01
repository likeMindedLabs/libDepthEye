#!/bin/bash
set -euo pipefail

function cleanup() {
    echo "Resuming macOS camera services..."
    sudo killall -CONT coremediaiod || true
}
trap cleanup EXIT

echo "Temporarily pausing macOS camera services to access DepthEye camera..."
# Use || true to avoid an error if the process isn't running
sudo killall -STOP coremediaiod || true
# Wait a moment for the service to release the device
sleep 1

sudo /opt/homebrew/bin/python3 -c "
import usb.core, usb.backend.libusb1, sys
b = usb.backend.libusb1.get_backend(find_library=lambda x: '/opt/homebrew/lib/libusb-1.0.dylib')
d = usb.core.find(idVendor=0x1d6b, idProduct=0x0102, backend=b)
if d is None:
    sys.exit('DepthEye not found')

# Try to detach the kernel driver. If it's not attached, this can fail.
# We can ignore the error if the driver wasn't attached to begin with.
try:
    d.detach_kernel_driver(1)
    print('detach OK')
except usb.core.USBError as e:
    # Error code for 'not found' can indicate driver was not active
    if e.backend_error_code == usb.backend.libusb1.LIBUSB_ERROR_NOT_FOUND:
        print('Kernel driver was not attached, which is fine.')
    else:
        # For other errors, we should exit
        print(f'An unexpected USB error occurred: {e}', file=sys.stderr)
        sys.exit(1)
"

sleep 1

#sudo ./bin/DepthAscii
#sudo ./bin/DepthMeasure
sudo ./integrations/pointcloud_deptheyes2/libDepthEye/build/bin/DepthCloud