#pragma once
#ifndef IMOTOR_H
#define IMOTOR_H

#include <memory>
#include <string>

class IMotor
{
public:
    virtual ~IMotor() = default;

    // Common motor operations
    virtual bool GoCenter() = 0;
    virtual bool GoHomeAndZero() = 0;
    virtual bool GoToAbsolutePosition(float stagePosition) = 0;

    // Getters
    virtual unsigned int GetDeviceSerNum() const = 0;
    virtual float GetDeviceRange() const = 0;
    virtual float GetDeviceActualStagePos() const = 0;

    // Setters
    virtual void SetDeviceName(const std::string& deviceName) = 0;
    virtual void SetMotorSerialNumber(unsigned int& serialNumber) = 0;
    virtual void SetStepsPerMMRatio(const int stepsPerMMRatio) = 0;
    virtual void UpdateCurrentPosition() = 0;
};

#endif // IMOTOR_H

