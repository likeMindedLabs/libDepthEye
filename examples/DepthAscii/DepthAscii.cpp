//
//  DepthAscii.cpp
//  DepthAscii
//
//  Created by Lucas on 2018/10/31.
//  Copyright © 2018 PointCloud.AI. All rights reserved.
//
 
#include <iomanip>
#include <fstream>
#include <ostream>
#include "CameraSystem.h"
#include <thread>
#include <thread>                // std::thread
#include <mutex>                // std::mutex, std::unique_lock
#include <condition_variable>    // std::condition_variable
#include <queue>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
using namespace PointCloud;
using namespace std;

CameraSystem depthEyeSys;
int col_factor = 1;
int row_factor = 2;

int printOutFrameInfo(const DepthFrame *depthFrame, int col_factor, int row_factor) {
    if (!depthFrame) return -1;

    FrameSize _size = depthFrame->size;
    
    // Ensure buffer is large enough
    size_t buffer_size = (_size.width / col_factor + 2) * (_size.height / row_factor + 1);
    char buffer[buffer_size];
    char *out = buffer;

    short coverage[_size.width / col_factor + 1];

    int k = 0;
    for (int y = 0; y < _size.height; ++y) {
        for (int x = 0; x < _size.width; ++x) {
            if (x % col_factor == 0) { // Check every 'col_factor' pixel starting from 0
                int coverage_idx = x / col_factor;
                if (coverage_idx < (_size.width / col_factor + 1)) {
                    float depth = depthFrame->depth[k];
                    // Scale float depth (meters) to an integer for the ASCII mapping logic.
                    // Map a 0-2.4 meter range to an integer range of 0-11.
                    short depth_int = (short)(depth * 5);
                    if (depth > 0 && depth_int > 0 && depth_int < 12) {
                        coverage[coverage_idx] += depth_int;
                    }
                }
            }
            k++;
        }

        if (y % row_factor == 1) { // Render a line of ASCII
            for (int i = 0; i < _size.width / col_factor; ++i) {
                short &c = coverage[i];
                // Use the accumulated coverage value to select a character.
                // The character map is darkest to brightest.
                *out++ = " M@#&$%*o!;."[c / row_factor];
                c = 0; // Reset coverage for the next line
            }
            *out++ = '\n';
        }
    }
    *out = '\0'; // Null-terminate the string
    printf("\n%s", buffer);
    return 0;
}


void rawdataCallback(DepthCamera &dc, const Frame &frame, DepthCamera::FrameType c) 
{
    // The callback now expects a DepthFrame
	const DepthFrame *d = dynamic_cast<const DepthFrame *>(&frame);

	if(!d)
	{
		std::cout << "Null frame captured? or not of type DepthFrame" << std::endl;
		return;
	}
    
	printOutFrameInfo(d, col_factor, row_factor);
}


int main(int argc, char const *argv[])
{
	
	DevicePtr     device;
  const Vector<DevicePtr> &devices = depthEyeSys.scan();
  bool found = false;
  for (auto &d: devices){
      std::cout <<"||| Detected devices: "  << d->id() << std::endl;
      device = d;
      found = true;
  }
  if (!found){
      std::cout <<"||| No device found "  << std::endl;
      return 1;
  }
  DepthCameraPtr depthCamera;
  depthCamera = depthEyeSys.connect(device);
  if (!depthCamera) {
  	  std::cout <<"||| Could not load depth camera for device "<< device->id() << std::endl;
      return 1;
  }

  if(!depthCamera->Init()) {
    std::cout << "Could not init Depth Camera " << device->id() << std::endl;
  }

	if (!depthCamera->isInitialized()) {
        std::cout <<"||| Depth camera not initialized for device " << std::endl;
        return 1;
  }
  FrameSize s;
  if(depthCamera->getFrameSize(s))
  logger(LOG_INFO) << " ||| Frame size :  " << s.width << " * "<< s.height << std::endl;
  if (s.width >= 320){
    col_factor = (s.width/128) + 1;
    row_factor = s.height/48;
  }
  std::cout << " ||| Successfully loaded depth camera for device " << std::endl;

	depthCamera->registerCallback(DepthCamera::FRAME_DEPTH_FRAME,rawdataCallback);
	
	if(depthCamera->start()){
          logger(LOG_INFO) <<  " ||| start camera pass" << std::endl;
          
    }else{
        logger(LOG_INFO) <<  " ||| start camera fail" << std::endl;  
    }
	
	std::cout << "Press any key to quit" << std::endl;
	getchar();
  std::cout << "Stoping System" << std::endl;
  depthCamera->stop();
	depthEyeSys.disconnect(depthCamera,true);
	return 0;
}

