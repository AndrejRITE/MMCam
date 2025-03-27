#pragma once
#ifndef IMOTOR_H
#define IMOTOR_H

#include <memory>
#include <string>

namespace MotorVariables
{
	struct Settings
	{
		float motorPos{};
		float minMotorPos{}, middleMotorPos{}, maxMotorPos{};
		float stagePos{};
		float minStagePos{}, middleStagePos{}, maxStagePos{};
		float motorRange{}, stageRange{};
		float stepsPerMMRatio{ 800.f };
	};
}

class IMotor
{
public:
    virtual ~IMotor() = default;

    // Common motor operations
    virtual bool GoCenter() = 0;
    virtual bool GoHomeAndZero() = 0;
    virtual bool GoToAbsolutePosition(float stagePosition) = 0;

    // Getters
    virtual std::string GetDeviceSerNum() const = 0;
    virtual std::string GetDeviceCOMPort() const = 0;
    virtual float GetDeviceRange() const = 0;
    virtual float GetDeviceActualStagePos() const = 0;

    // Setters
    virtual void SetDeviceName(const std::string& deviceName) = 0;
    virtual void SetMotorSerialNumber(std::string serialNumber) = 0;
    virtual void SetStepsPerMMRatio(const int stepsPerMMRatio) = 0;
	virtual void SetRange(const float minMotorDeg, const float maxMotorDeg) = 0;
	virtual void SetCurrentMotorPosition(const float motorPosition) = 0;

    virtual void UpdateCurrentPosition() = 0;
};

#endif // IMOTOR_H

