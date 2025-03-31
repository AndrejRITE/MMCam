#pragma once
#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <string>
#include <memory>

namespace CameraControlVariables
{
    enum ImageDataTypes
    {
        RAW_12BIT,
        RAW_16BIT,
    };
}

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

    virtual auto SetSensorTemperature(const double requiredTemperature) -> void = 0;
    virtual auto GetSensorTemperature() -> double = 0;

    auto GetCameraDataType() const -> CameraControlVariables::ImageDataTypes { return m_ImageDataType; };
    auto GetSerialNumber() const -> std::string { return m_CameraSerialNumber; };

protected:
    std::string m_CameraSerialNumber{};
    CameraControlVariables::ImageDataTypes m_ImageDataType{ CameraControlVariables::ImageDataTypes::RAW_12BIT };
    double m_SensorTemperature{};
};

#endif // CAMERA_CONTROL_H
