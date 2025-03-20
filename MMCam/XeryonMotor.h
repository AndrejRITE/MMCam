#pragma once
#ifndef XERYONMOTOR_H
#define XERYONMOTOR_H

#include <memory>
#include <algorithm>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <filesystem>

#include "SerialPortCommunication.h"

#include "IMotor.h"
#include "IMotorArray.h"

#include "Xeryon.h"

namespace XeryonMotorVariables
{
	enum Command
	{
		HOME,
		CENTER,
		ABS_POSITION,
		OFFSET
	};
}

class XeryonMotor final : public IMotor
{
public:
	XeryonMotor();

    // Common motor operations
	bool GoCenter() override;
	bool GoHomeAndZero() override;
	bool GoToAbsolutePosition(float stagePosition) override;

	// Getters
	std::string GetDeviceSerNum() const override;
	float GetDeviceRange() const override;
	float GetDeviceActualStagePos() const override;

	// Setters
	void SetDeviceName(const std::string& deviceName) override;
	void SetMotorSerialNumber(std::string serialNumber) override;
	void SetStepsPerMMRatio(const int stepsPerMMRatio) override;
	void SetRange(const float minMotorDeg, const float maxMotorDeg) override;
	void SetCurrentMotorPosition(const int motorPosition) override;

	void UpdateCurrentPosition() override;

private:
	std::string m_MotorCOMPort{};
	std::string m_MotorSerialNumber{};
	const long long m_WaitAfterMovementMilliseconds{ 500 };

};

class XeryonMotorArray final : public IMotorArray
{
public:
	XeryonMotorArray();

	// Getters
	std::map<std::string, float> GetSerialNumbersWithRanges() const override { return m_NamesOfMotorsWithRanges; };
	float GetActualStagePos(const std::string& motor_sn) const override;
	bool IsMotorConnected(const std::string& motor_sn) const override;

	// Setters
	float GoMotorHome(const std::string& motor_sn) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::HOME); };
	float GoMotorCenter(const std::string& motor_sn) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::CENTER); };
	float GoMotorToAbsolutePosition(const std::string& motor_sn, float abs_pos) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::ABS_POSITION, abs_pos); };
	float GoMotorOffset(const std::string& motor_sn, float offset) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::OFFSET, offset); };

	void SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM) override;

private:
	auto InitAllMotors() -> bool;
	auto FillNames() -> void;

	auto GoMotor(const std::string& motor_sn, XeryonMotorVariables::Command command, float pos = 0.f) -> float;

private:
	std::vector<XeryonMotor> m_MotorsArray;
	std::vector<std::string> m_UninitializedMotors{};

	std::map<std::string, float> m_NamesOfMotorsWithRanges{};
	std::map<std::string, std::string> m_AllAvailableCOMPortsWithSerialNumbers{};

	const float error_position = 0.0f;
};
#endif
