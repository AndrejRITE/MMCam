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

float XeryonMotorArray::GetActualStagePos(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
			return m_MotorsArray[motor].GetDeviceActualStagePos();
	}
	return error_position;
}

bool XeryonMotorArray::IsMotorConnected(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
			return true;
	}
	return false;
}

void XeryonMotorArray::SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM)
{
	if (motor_sn.empty() || motor_sn == "None") return;

	if (stepsPerMM <= 0) return;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
		{
			m_MotorsArray[motor].SetStepsPerMMRatio((float)stepsPerMM);
			break;
		}
	}
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

auto XeryonMotorArray::GoMotor
(
	const std::string& motor_sn, 
	XeryonMotorVariables::Command command, 
	float pos
) -> float
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
		{
			switch (command)
			{
			case XeryonMotorVariables::HOME:
				m_MotorsArray[motor].GoHomeAndZero();
				break;
			case XeryonMotorVariables::CENTER:
				m_MotorsArray[motor].GoCenter();
				break;
			case XeryonMotorVariables::ABS_POSITION:
				m_MotorsArray[motor].GoToAbsolutePosition(pos);
				break;
			case XeryonMotorVariables::OFFSET:
				m_MotorsArray[motor].GoToAbsolutePosition(m_MotorsArray[motor].GetDeviceActualStagePos() + pos);
				break;
			default:
				break;
			}
			return m_MotorsArray[motor].GetDeviceActualStagePos();
		}
	}
	return error_position;
}

