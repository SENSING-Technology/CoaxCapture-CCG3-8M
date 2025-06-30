#ifndef __CAMERAGATHER_H
#define __CAMERAGATHER_H

#include <iostream>
#include <vector>
#include "camera_common.h"

class CameraImpl
{
public:
  virtual ~CameraImpl(){}

  virtual int openCamera() = 0;

  virtual int pollCamera(int *cameraFlags) = 0;
  virtual int readCamera(int channel, frameInfo *info) = 0;

  virtual int closeCamera() = 0;
};

#endif // CAMERAGATHER_H
