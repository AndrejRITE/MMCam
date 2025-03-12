#pragma once
#ifndef STANDAMOTOR_H
#define STANDAMOTOR_H

#include <memory>
#include <algorithm>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <filesystem>
#include <ximc.h>

#include "IMotor.h"
#include "IMotorArray.h"


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

namespace StandaVariables
{
	struct C_Settings
	{
		result_t result{};
		calibration_t calibration{};
		status_t state{};
		status_calb_t calb_state{};
	};
}

class StandaMotor final : public IMotor
{
public:
	StandaMotor();

    // Common motor operations
	bool GoCenter() override;
	bool GoHomeAndZero() override;
	bool GoToAbsolutePosition(float stagePosition);

	// Getters 
	unsigned int GetDeviceSerNum() const override { return m_MotorSerialNumber; };
	float GetDeviceRange() const override { return m_MotorSettings->stageRange; };
	float GetDeviceActualStagePos() const override { return m_MotorSettings->stagePos; };

	// Setters 
	void SetDeviceName(const std::string& deviceName) override 
	{
		if (deviceName.empty()) return;
		m_DeviceName = deviceName;
	};

	void SetMotorSerialNumber(unsigned int serialNumber) override { m_MotorSerialNumber = serialNumber; };

	// Other Functions
	//void SetResult(result_t result) { m_StandaSettings->result = result; };
	void SetCalibration(calibration_t calibration) { m_StandaSettings->calibration = calibration; };
	//void SetState(status_t state) { m_StandaSettings->state = state; };
	//void SetCalbState(status_calb_t calb_state) { m_StandaSettings->calb_state = calb_state; };

	void SetRange(const float minMotorDeg, const float maxMotorDeg) override;

	void SetStepsPerMMRatio(const int stepsPerMMRatio) override 
	{
		m_MotorSettings->stepsPerMMRatio = stepsPerMMRatio;
		UpdateStageRange();
		UpdateCurrentPosition();
	};

	void UpdateCurrentPosition() override
	{
		//m_MotorSettings->motorPos = m_CurrentMotorDeg;
		m_MotorSettings->motorPos = m_StandaSettings->state.CurPosition;
		m_MotorSettings->stagePos = m_MotorSettings->motorPos / m_MotorSettings->stepsPerMMRatio;
	}


	/* Move constructor */
	StandaMotor(StandaMotor&& other) noexcept
		: m_MotorSettings(std::move(other.m_MotorSettings)),
		//m_StandaSettings(std::move(other.m_StandaSettings)),
		m_DeviceName(std::move(other.m_DeviceName)),
		m_MotorSerialNumber(other.m_MotorSerialNumber)
	{
		other.m_MotorSettings = nullptr;
		//other.m_StandaSettings = nullptr;
		other.m_DeviceName = "";
		other.m_MotorSerialNumber = 0;
	};

	/* Move assignment */
	auto& operator=(StandaMotor&& other) noexcept
	{
		m_MotorSettings.reset(other.m_MotorSettings.release());
		//m_StandaSettings.reset(other.m_StandaSettings.release());
		m_DeviceName = std::move(other.m_DeviceName);
		m_MotorSerialNumber = other.m_MotorSerialNumber;
		return *this;
	};

private:
	auto UpdateStageRange() -> void
	{
		/* Min position */
		m_MotorSettings->minStagePos = m_MotorSettings->minMotorPos / m_MotorSettings->stepsPerMMRatio;

		/* Middle position */
		m_MotorSettings->middleStagePos = m_MotorSettings->middleMotorPos / m_MotorSettings->stepsPerMMRatio;

		/* Max position */
		m_MotorSettings->maxStagePos = m_MotorSettings->maxMotorPos / m_MotorSettings->stepsPerMMRatio;

		/* Set Whole Motor Range */
		m_MotorSettings->stageRange = m_MotorSettings->motorRange / m_MotorSettings->stepsPerMMRatio;
	};


private:
	std::unique_ptr<MotorVariables::Settings> m_MotorSettings{};
	std::unique_ptr<StandaVariables::C_Settings> m_StandaSettings{};
	//int m_StepsPerMM{ 800 }; 
	//std::unique_ptr<char[]> m_DeviceName{};
	std::string m_DeviceName{};
	unsigned int m_MotorSerialNumber{};
	int m_CurrentMotorDeg{};
	const long long m_WaitAfterMovementMilliseconds{ 500 };
};

class StandaMotorArray final : public IMotorArray
{
public:
	StandaMotorArray(const std::string ipAddress = "");

	/* Getter */
	std::map<unsigned int, float> GetSerialNumbersWithRanges() const override { return m_NamesOfMotorsWithRanges; };
	float GetActualStagePos(const std::string& motor_sn) const override;

	/* Setter */
	float GoMotorHome(const std::string& motor_sn) override;
	float GoMotorCenter(const std::string& motor_sn) override;
	float GoMotorToAbsolutePosition(const std::string& motor_sn, float abs_pos) override;
	float GoMotorOffset(const std::string& motor_sn, float offset) override;

	void SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM) override;

	//auto AreAllMotorsInitialized() const -> bool { return !m_UninitializedMotors.size(); };
	//auto GetUninitializedMotors() const -> std::vector<unsigned int> { return m_UninitializedMotors; };

private:
	auto FillNames();
	bool InitAllMotors(const std::string ip_address);


private:
	std::vector<IMotor> m_MotorsArray{};
	std::map<unsigned int, float> m_NamesOfMotorsWithRanges{};
	const float error_position = 0.0f;

	std::vector<unsigned int> m_UninitializedMotors{};
};

#endif
