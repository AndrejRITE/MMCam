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
    virtual void GoCenter() = 0;
    virtual void GoHomeAndZero() = 0;
    virtual void GoToPos(float stage_position) = 0;

    // Getters
    virtual unsigned int GetDeviceSerNum() const = 0;
    virtual float GetDeviceRange() const = 0;
    virtual float GetDeviceActualStagePos() const = 0;

    // Setters
    virtual void SetDeviceName(const std::string& device_name) = 0;
    virtual void SetSerNum(unsigned int s_n) = 0;
};

#endif // IMOTOR_H

