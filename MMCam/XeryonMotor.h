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
#include <stdexcept>

#include <Python.h>
#include <pybind11/embed.h> 
#include <cstdlib> 
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

	//enum Axis
	//{
	//	A,
	//	B,
	//	C,
	//	D,
	//	E,
	//	G
	//};
}

namespace py = pybind11;


class XeryonMotor final : public IMotor
{
public:
	XeryonMotor();

    // Common motor operations
	bool GoCenter() override;
	bool GoHomeAndZero() override;
	bool GoToAbsolutePosition(float stagePosition) override;

	// Getters
	std::string GetDeviceSerNum() const override { return m_MotorSerialNumber; };
	std::string GetDeviceCOMPort() const { return m_MotorCOMPort; };
	float GetDeviceRange() const override { return (abs(m_MotorSettings->minMotorPos) + abs(m_MotorSettings->maxMotorPos)); };
	float GetDeviceActualStagePos() const override;

	auto GetAxisLetter() const -> char { return m_AxisLetter; };

	// Setters
	void SetDeviceName(const std::string& deviceName) override {};
	void SetMotorSerialNumber(std::string serialNumber) override { m_MotorSerialNumber = serialNumber; };
	void SetStepsPerMMRatio(const int stepsPerMMRatio) override { m_MotorSettings->stepsPerMMRatio = stepsPerMMRatio; };
	void SetRange(const float minMotorDeg, const float maxMotorDeg) override 
	{ m_MotorSettings->minMotorPos = minMotorDeg; m_MotorSettings->maxMotorPos = maxMotorDeg; };
	void SetCurrentMotorPosition(const int motorPosition) override { m_MotorSettings->motorPos = m_MotorSettings->motorPos + motorPosition; };

	//void /*SetAxis*/(Axis* axis) { m_Axis = axis; };

	void SetMotorCOMPort(std::string comPort) { m_MotorCOMPort = comPort; };
	//auto SetAxisLetter(const char letter) -> void { m_AxisLetter = letter; };

	void UpdateCurrentPosition() override 
	{
		//auto dpos = (long double)m_Axis->getData(commandForPosition);
		//m_MotorSettings->motorPos = dpos;
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

	int m_MaxAttemptsToCallPythonFunction{ 5 };

	//Axis* m_Axis{};

	char m_AxisLetter{};

	const LinearStage m_Stage = XLS_1250;
	int m_Baudrate = 9'600;
	const long long m_WaitAfterMovementMilliseconds{ 500 };
	std::string commandForPosition = "DPOS";
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
	float GoMotorHome(const std::string& motor_sn) override 
	{ 

		return GoMotor(motor_sn, XeryonMotorVariables::Command::HOME); 
	};
	float GoMotorCenter(const std::string& motor_sn) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::CENTER); };
	float GoMotorToAbsolutePosition(const std::string& motor_sn, float abs_pos) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::ABS_POSITION, abs_pos); };
	float GoMotorOffset(const std::string& motor_sn, float offset) override { return GoMotor(motor_sn, XeryonMotorVariables::Command::OFFSET, offset); };

	void SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM) override;

private:
	auto InitAllMotors() -> bool;
	auto FillNames() -> void;

	auto GoMotor(const std::string& motor_sn, XeryonMotorVariables::Command command, float pos = 0.f) -> float;

private:
	//std::vector<std::unique_ptr<Xeryon>> m_XeryonControllerArray{};
	//std::vector<std::unique_ptr<Axis>> m_XeryonAxisArray{};
	std::vector<XeryonMotor> m_MotorsArray;
	std::vector<std::string> m_UninitializedMotors{};

	std::map<std::string, float> m_NamesOfMotorsWithRanges{};
	std::map<std::string, std::string> m_AllAvailableCOMPortsWithSerialNumbers{};

	//py::module script_setAbsolutePosition, script_goCenter;
	//const LinearStage m_Stage = XLS_1250;
	//std::string m_CommandToGetPosition = "DPOS";

	const float error_position = 0.0f;
};
#endif
