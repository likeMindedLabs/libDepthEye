#include <iostream>
#include <cstdint>
#include <cmath>
#include "CameraSystem.h"
#ifndef _WIN32
    #include <unistd.h>
#else
    #include <io.h>
    #include <fcntl.h>
#endif
#include <vector>
#include <thread>
#include <chrono>

using namespace PointCloud;

// -----------------------------------------------------------------------------
// Focal length (in pixels) the driver uses to back-project the depth map into a
// point cloud. This is the single knob to control here:
//
//   * Leave at 0.0 to AUTO-DERIVE it from the sensor's reported field of view
//     each run (focal = (width / 2) / tan(fovHalfAngle)).
//   * Set a positive value to OVERRIDE with a fixed / calibrated focal length.
//
// The resolved value is written into every frame header, so driver_rerun.py
// always back-projects with the same focal the camera is actually using.
// -----------------------------------------------------------------------------
static double FOCAL_LENGTH_PX = 0.0;

// Sensor field-of-view half-angle (radians), read once at startup and used to
// derive the focal length when FOCAL_LENGTH_PX is left at 0.
static float g_fovHalfAngle = 0.0f;

// Resolve the focal length (pixels) for a frame of the given width.
static float resolveFocalPx(uint32_t width) {
    if (FOCAL_LENGTH_PX > 0.0) {
        return static_cast<float>(FOCAL_LENGTH_PX);
    }
    if (g_fovHalfAngle > 0.0f) {
        return (width / 2.0f) / std::tan(g_fovHalfAngle);
    }
    return width / 2.0f; // fallback: assumes ~90° horizontal FOV
}

// -----------------------------------------------------------------------------
// We register a SINGLE callback, on FRAME_DEPTH_FRAME. A DepthFrame carries two
// pixel-aligned channels we care about:
//   - depth     : distance per pixel, in meters
//   - amplitude : amount of reflected (active IR) light per pixel, 0..1
//
// Both are emitted together each frame. driver_rerun.py back-projects the depth
// channel into a point cloud (colored by depth) and shows the amplitude channel
// as a reflection map.
//
// Wire protocol: RAW BINARY on stdout (text/ASCII serialization was the
// throughput bottleneck). One record per camera frame, little-endian:
//
//   magic   : 4 bytes  "DFRM"
//   width   : uint32
//   height  : uint32
//   focal   : float32                (back-projection focal length, pixels)
//   depth   : width*height float32   (meters)
//   reflect : width*height float32   (amplitude 0..1)
//
// The magic lets the reader resynchronize if any stray bytes ever reach stdout.
// All human-readable status goes to stderr to keep stdout a clean binary stream.
// -----------------------------------------------------------------------------

void depthCallback(DepthCamera &dc, const Frame &frame, DepthCamera::FrameType c) {
    const DepthFrame *depthFrame = dynamic_cast<const DepthFrame *>(&frame);
    if (!depthFrame) {
        return;
    }

    const uint32_t width = depthFrame->size.width;
    const uint32_t height = depthFrame->size.height;
    const size_t count = static_cast<size_t>(width) * height;

    // Guard against a short buffer (avoids reading out of bounds).
    if (depthFrame->depth.size() < count || depthFrame->amplitude.size() < count) {
        return;
    }

    const float focal = resolveFocalPx(width);

    std::cout.write("DFRM", 4);
    std::cout.write(reinterpret_cast<const char *>(&width), sizeof(width));
    std::cout.write(reinterpret_cast<const char *>(&height), sizeof(height));
    std::cout.write(reinterpret_cast<const char *>(&focal), sizeof(focal));
    std::cout.write(reinterpret_cast<const char *>(depthFrame->depth.data()),
                    count * sizeof(float));
    std::cout.write(reinterpret_cast<const char *>(depthFrame->amplitude.data()),
                    count * sizeof(float));
    std::cout.flush();
}

int main(int argc, char const *argv[]) {
    // Decouple std::cout from C stdio; we only write large binary blocks.
    std::ios::sync_with_stdio(false);

#ifdef _WIN32
    // Windows opens stdout in text mode by default, which corrupts binary data:
    // 0x0A bytes become 0x0D 0x0A and 0x1A is treated as EOF. Switch to binary
    // mode before any writes so the pipe carries raw float32 frames intact.
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    CameraSystem sys;

    // 1. Scan for devices
    const Vector<DevicePtr> &devices = sys.scan();
    if (devices.size() == 0) {
        std::cerr << "No devices found." << std::endl;
        return 1;
    }
    std::cerr << "Found " << devices.size() << " device(s)." << std::endl;

    // 2. Connect to the first device
    DevicePtr device_to_use = devices[0];
    DepthCameraPtr depth_camera = sys.connect(device_to_use);
    if (!depth_camera) {
        std::cerr << "Failed to connect to device " << device_to_use->id() << std::endl;
        return 1;
    }
    std::cerr << "Connected to " << device_to_use->id() << std::endl;

    // 3. Initialize the camera
    if (!depth_camera->Init()) {
        std::cerr << "Failed to initialize camera." << std::endl;
        sys.disconnect(depth_camera, true);
        return 1;
    }

    // 4. Register the single depth-frame callback (depth + reflection).
    depth_camera->registerCallback(DepthCamera::FRAME_DEPTH_FRAME, depthCallback);
    std::cerr << "Registered depth-frame callback (depth + reflection)." << std::endl;

    // Resolve the focal length used for back-projection. When FOCAL_LENGTH_PX
    // is left at 0, derive it from the sensor's reported field of view.
    if (FOCAL_LENGTH_PX > 0.0) {
        std::cerr << "Using fixed focal length: " << FOCAL_LENGTH_PX << " px." << std::endl;
    } else {
        float fovHalfAngle = 0.0f;
        if (depth_camera->getFieldOfView(fovHalfAngle) && fovHalfAngle > 0.0f) {
            g_fovHalfAngle = fovHalfAngle;
            std::cerr << "Field of view half-angle: " << fovHalfAngle
                      << " rad (focal derived per frame)." << std::endl;
        } else {
            std::cerr << "Could not read field of view; falling back to width/2 focal."
                      << std::endl;
        }
    }

    // 5. Set framerate
    FrameRate desired_rate;
    desired_rate.numerator = 30;
    desired_rate.denominator = 1; // 30 fps
    if (!depth_camera->setFrameRate(desired_rate)) {
        std::cerr << "Failed to set frame rate." << std::endl;
    }

    if (!depth_camera->start()) {
        std::cerr << "Failed to start camera stream." << std::endl;
        sys.disconnect(depth_camera, true);
        return 1;
    }

    std::cerr << "Camera streaming... Outputting depth + reflection data." << std::endl;
    std::cerr << "Press Ctrl+C in the controlling terminal to stop." << std::endl;

    // The callback will be fired in a background thread.
    // Keep the main thread alive until the user interrupts.
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 6. Stop and disconnect (this part is unreachable in this loop,
    // but good practice for a more complex app with a proper exit condition)
    depth_camera->stop();
    sys.disconnect(depth_camera, true);
    std::cerr << "Camera stopped." << std::endl;

    return 0;
}
