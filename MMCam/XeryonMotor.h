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
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <fstream>

//#include <Python.h>
//#include <pybind11/embed.h> 
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

//namespace py = pybind11;

class XeryonMotor final : public IMotor
{
public:
	XeryonMotor();

    // Common motor operations
	bool GoCenter() override;
	bool GoHomeAndZero() override { return GoCenter(); };
	bool GoToAbsolutePosition(float stagePosition) override;

	// Getters
	std::string GetDeviceSerNum() const override { return m_MotorSerialNumber; };
	std::string GetDeviceCOMPort() const override { return m_MotorCOMPort; };
	float GetDeviceRange() const override { return (abs(m_MotorSettings->minMotorPos) + abs(m_MotorSettings->maxMotorPos)); };
	float GetDeviceActualStagePos() const override { return m_MotorSettings->motorPos; };

	// Setters
	void SetDeviceName(const std::string& deviceName) override {};
	void SetMotorSerialNumber(std::string serialNumber) override { m_MotorSerialNumber = serialNumber; };
	void SetStepsPerMMRatio(const int stepsPerMMRatio) override { m_MotorSettings->stepsPerMMRatio = stepsPerMMRatio; };
	void SetRange(const float minMotorDeg, const float maxMotorDeg) override { m_MotorSettings->minMotorPos = minMotorDeg; m_MotorSettings->maxMotorPos = maxMotorDeg; };
	void SetCurrentMotorPosition(const float motorPosition) override { m_MotorSettings->motorPos += motorPosition; };

	void SetMotorCOMPort(std::string comPort) { m_MotorCOMPort = comPort; };

	void UpdateCurrentPosition() override {};

	/* Move constructor */
	XeryonMotor(XeryonMotor&& other) noexcept
		: m_MotorSettings(std::move(other.m_MotorSettings)),
		m_MotorCOMPort(std::move(other.m_MotorCOMPort)),
		m_MotorSerialNumber(std::move(other.m_MotorSerialNumber))
	{
		other.m_MotorSerialNumber = "";
		other.m_MotorCOMPort = "";
	};

	/* Move assignment */
	XeryonMotor& operator=(XeryonMotor&& other) noexcept
	{
		if (this != &other)
		{
			m_MotorSettings = std::move(other.m_MotorSettings);
			m_MotorCOMPort = std::move(other.m_MotorCOMPort);
			m_MotorSerialNumber = std::move(other.m_MotorSerialNumber);
		}
		return *this;
	};


private:
	std::string executePythonScript(const std::string& comPort, double absolutePosition)
	{
		std::string command = "python path_to_xeryon_control.py " + comPort + " " + std::to_string(absolutePosition);

		// Open the process for reading
		std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
		if (!pipe) 
		{
			throw std::runtime_error("popen() failed!");
		}

		std::string result;
		char buffer[128];
		while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
			result += buffer;
		}

		return result;
	}

private:
	std::string m_MotorCOMPort{};
	std::string m_MotorSerialNumber{};
	std::unique_ptr<MotorVariables::Settings> m_MotorSettings{};
	
	std::string m_AbsolutePositionScriptName{ "xeryon_setAbsolutePosition.py" };
	std::string m_CenterScriptName{ "xeryon_goCenter.py" };

	std::string m_AxisPositionTempFileName{ "temp\\axisPosition.temp" };

	int m_MaxAttemptsToCallPythonFunction{ 5 };
	const long long m_WaitAfterMovementMilliseconds{ 500 };
};

class XeryonMotorArray final : public IMotorArray
{
public:
	XeryonMotorArray() { InitAllMotors(); };

	// Getters
	std::map<std::string, float> GetSerialNumbersWithRanges() const override { return m_NamesOfMotorsWithRanges; };
	float GetActualStagePos(const std::string& motor_sn) const override;
	std::string GetMotorCOMPort(const std::string& motor_sn) const override;
	bool IsMotorConnected(const std::string& motor_sn) const override;

	// Setters
	float GoMotorHome(const std::string& motor_sn) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::HOME); };
	float GoMotorCenter(const std::string& motor_sn) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::CENTER); };
	float GoMotorToAbsolutePosition(const std::string& motor_sn, float abs_pos) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::ABS_POSITION, abs_pos); };
	float GoMotorOffset(const std::string& motor_sn, float offset) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::OFFSET, offset); };

	void SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM) override;
	void SetCurrentPositionForTheMotor(const std::string motor_sn, const float currentPosition) override;

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
