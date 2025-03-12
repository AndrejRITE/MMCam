#pragma once
#ifndef IMOTORARRAY_H
#define IMOTORARRAY_H

#include <memory>
#include <string>
#include <map>

class IMotorArray
{
public:
    virtual ~IMotorArray() = default;

	// Getters
	virtual std::map<unsigned int, float> GetSerialNumbersWithRanges() const = 0;
	virtual float GetActualStagePos(const std::string& motor_sn) const = 0;

	// Setters
	virtual float GoMotorHome(const std::string& motor_sn) = 0;
	virtual float GoMotorCenter(const std::string& motor_sn) = 0;
	virtual float GoMotorToAbsolutePosition(const std::string& motor_sn, float abs_pos) = 0;
	virtual float GoMotorOffset(const std::string& motor_sn, float offset) = 0;

	virtual void SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM) = 0;
};

#endif // IMOTORARRAY_H

