#include "XeryonMotor.h"

XeryonMotor::XeryonMotor()
{
}

bool XeryonMotor::GoCenter()
{
	return false;
}

bool XeryonMotor::GoHomeAndZero()
{
	return false;
}

bool XeryonMotor::GoToAbsolutePosition(float stagePosition)
{
	return false;
}

std::string XeryonMotor::GetDeviceSerNum() const
{
	return std::string();
}

float XeryonMotor::GetDeviceRange() const
{
	return 0.0f;
}

float XeryonMotor::GetDeviceActualStagePos() const
{
	return 0.0f;
}

void XeryonMotor::SetDeviceName(const std::string& deviceName)
{
}

void XeryonMotor::SetMotorSerialNumber(std::string serialNumber)
{
}

void XeryonMotor::SetStepsPerMMRatio(const int stepsPerMMRatio)
{
}

void XeryonMotor::SetRange(const float minMotorDeg, const float maxMotorDeg)
{
}

void XeryonMotor::SetCurrentMotorPosition(const int motorPosition)
{
}

void XeryonMotor::UpdateCurrentPosition()
{
}

// XeryonMotorArray Start
XeryonMotorArray::XeryonMotorArray()
{
	InitAllMotors();
}

std::map<std::string, float> XeryonMotorArray::GetSerialNumbersWithRanges() const
{
	return std::map<std::string, float>();
}

float XeryonMotorArray::GetActualStagePos(const std::string& motor_sn) const
{
	return 0.0f;
}

bool XeryonMotorArray::IsMotorConnected(const std::string& motor_sn) const
{
	return false;
}

float XeryonMotorArray::GoMotorHome(const std::string& motor_sn)
{
	return 0.0f;
}

float XeryonMotorArray::GoMotorCenter(const std::string& motor_sn)
{
	return 0.0f;
}

float XeryonMotorArray::GoMotorToAbsolutePosition(const std::string& motor_sn, float abs_pos)
{
	return 0.0f;
}

float XeryonMotorArray::GoMotorOffset(const std::string& motor_sn, float offset)
{
	return 0.0f;
}

void XeryonMotorArray::SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM)
{
}

auto XeryonMotorArray::InitAllMotors() -> bool
{
	auto appendUnitializedMotor = [&](const std::string motorSN, const int motorNum)
		{
			m_UninitializedMotors.push_back(motorSN);
			m_MotorsArray[motorNum].SetMotorSerialNumber("");
		};

	m_UninitializedMotors.clear();

	size_t names_count{};

	for (auto i{ 0 }; i < UCHAR_MAX; ++i)
	{
		auto comPort = "COM" + std::to_string(i);
		auto serialNumber = getSerialNumberFromCOMPort(comPort);
		if (!serialNumber.empty())
			m_AllAvailableCOMPortsWithSerialNumbers.emplace(comPort, serialNumber);
	}

	names_count = m_AllAvailableCOMPortsWithSerialNumbers.size();

	/* Here we need to clear motor list */
	m_MotorsArray.clear();
	m_MotorsArray.reserve(names_count);

	auto iterator = m_AllAvailableCOMPortsWithSerialNumbers.begin();

	auto i{ 0 };
	while (iterator != m_AllAvailableCOMPortsWithSerialNumbers.end())
	{
		m_MotorsArray.emplace_back(XeryonMotor());
		m_MotorsArray[i].SetDeviceName(iterator->second);
		m_MotorsArray[i].SetMotorSerialNumber(iterator->second);

		auto currCOMPort = "\\\\.\\" + iterator->first;
		auto baudrate = 115'200;
		auto controller = std::make_unique<Xeryon>(currCOMPort.c_str(), baudrate);
		const auto stage = XLS_1250;
		auto axis = controller->addAxis('X', &stage);
		m_MotorsArray[i].SetCurrentMotorPosition(axis->getData("DPOS"));

		m_MotorsArray[i].SetRange(-12.5f, 12.5f);
		m_MotorsArray[i].UpdateCurrentPosition();

		++i;
		++iterator;
	}

	//std::sort(m_MotorsArray.begin(), m_MotorsArray.end(),
	//	[](const XeryonMotor& left, const XeryonMotor& right)
	//	{
	//		return left.GetDeviceSerNum() < right.GetDeviceSerNum();
	//	});

	FillNames();

	return false;
}

auto XeryonMotorArray::FillNames() -> void
{
	for (const auto& motor : m_MotorsArray)
		m_NamesOfMotorsWithRanges.emplace
		(
			std::make_pair
			(
				motor.GetDeviceSerNum(), 
				motor.GetDeviceRange()
			)
		);
}

