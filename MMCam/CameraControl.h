#pragma once
#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <string>
#include <memory>

class CameraControl
{
public:
    virtual ~CameraControl() = default;

    virtual auto Initialize() -> bool = 0;
    virtual auto Close() -> bool = 0;
    virtual auto StartAcquisition() -> bool = 0;
    virtual auto StopAcquisition() -> bool = 0;
    virtual auto GetImage() -> unsigned short* = 0;
    virtual auto SetExposureTime(int exposure_us) -> void = 0;

    virtual auto GetWidth() const -> unsigned long = 0;
    virtual auto GetHeight() const -> unsigned long = 0;

    virtual auto IsConnected() const -> bool = 0;
};

#endif // CAMERA_CONTROL_H
