#include "XeryonMotor.h"

XeryonMotor::XeryonMotor()
{
	m_MotorSettings = std::make_unique<MotorVariables::Settings>();
}

bool XeryonMotor::GoCenter()
{
	auto attempt = 0;

	while (attempt < m_MaxAttemptsToCallPythonFunction)
	{
		try
		{
			py::gil_scoped_acquire gil;  // 🔹 Acquire GIL in the new thread
			py::module importlib = py::module::import("importlib");
			py::module script_goCenter = importlib.attr("reload")(py::module::import("xeryon_goCenter"));

			auto currentPosition = script_goCenter.attr("move_to_position")
				(m_MotorCOMPort.c_str()).cast<double>();

			m_MotorSettings->motorPos = currentPosition;
			return true;  // Success
		}
		catch (const py::error_already_set& e)
		{
			auto error = std::string("Python Exception on attempt ") + std::to_string(attempt + 1) 
				+ ": " + e.what();
			std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));  // Wait before retrying
			attempt++;
		}
	}

	return false;
}

bool XeryonMotor::GoToAbsolutePosition(float stagePosition)
{
	auto attempt = 0;

	static py::module script_setAbsolutePosition = py::module::import("xeryon_setAbsolutePosition");

	while (attempt < m_MaxAttemptsToCallPythonFunction)
	{
		try
		{
			// Ensure GIL is acquired only when needed
			py::gil_scoped_acquire gil;

			auto currentPosition = script_setAbsolutePosition.attr("move_to_position")
				(m_MotorCOMPort.c_str(), (double)stagePosition).cast<double>();

			SetCurrentMotorPosition(currentPosition);
			return true;  // Success
		}
		catch (const py::error_already_set& e)
		{
			auto error = std::string("Python Exception on attempt ") + std::to_string(attempt + 1) 
				+ ": " + e.what();
			std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));  // Wait before retrying
			attempt++;
		}
	}

	return false;
}

// XeryonMotorArray Start
float XeryonMotorArray::GetActualStagePos(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return error_position;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	return (it != m_MotorsArray.end()) ? it->GetDeviceActualStagePos() : error_position;

	//for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	//{
	//	if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
	//		return m_MotorsArray[motor].GetDeviceActualStagePos();
	//}
	//return error_position;
}

std::string XeryonMotorArray::GetMotorCOMPort(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return "";

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	return (it != m_MotorsArray.end()) ? it->GetDeviceCOMPort() : "";

	//for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	//{
	//	if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
	//		return m_MotorsArray[motor].GetDeviceCOMPort();
	//}
	//return "";
}

bool XeryonMotorArray::IsMotorConnected(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return error_position;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	return (it != m_MotorsArray.end()) ? true : false;

	//for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	//{
	//	if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
	//		return true;
	//}
	//return false;
}

void XeryonMotorArray::SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM)
{
	if (motor_sn.empty() || motor_sn == "None") return;

	if (stepsPerMM <= 0) return;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	if (it != m_MotorsArray.end())
		it->SetStepsPerMMRatio((float)stepsPerMM);

	//for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	//{
	//	if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
	//	{
	//		m_MotorsArray[motor].SetStepsPerMMRatio((float)stepsPerMM);
	//		break;
	//	}
	//}
}

void XeryonMotorArray::SetCurrentPositionForTheMotor(const std::string motor_sn, const float currentPosition)
{
	if (motor_sn.empty() || motor_sn == "None") return;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	if (it != m_MotorsArray.end())
		it->SetCurrentMotorPosition((float)currentPosition);

	//for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	//{
	//	if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
	//	{
	//		m_MotorsArray[motor].SetCurrentMotorPosition((float)currentPosition);
	//		break;
	//	}
	//}
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
		m_MotorsArray[i].SetMotorCOMPort(iterator->first);
		m_MotorsArray[i].SetDeviceName(iterator->second);
		m_MotorsArray[i].SetMotorSerialNumber(iterator->second);

		m_MotorsArray[i].SetCurrentMotorPosition(0.f);

		m_MotorsArray[i].SetRange(-12.5f, 12.5f);
		m_MotorsArray[i].UpdateCurrentPosition();

		++i;
		++iterator;
	}

	std::sort(m_MotorsArray.begin(), m_MotorsArray.end(),
		[](const XeryonMotor& left, const XeryonMotor& right)
		{
			return left.GetDeviceSerNum() < right.GetDeviceSerNum();
		});

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
	if (motor_sn.empty() || motor_sn == "None") return error_position;

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
				m_MotorsArray[motor].GoToAbsolutePosition
				(
					pos - m_MotorsArray[motor].GetDeviceActualStagePos()
				);
				break;
			case XeryonMotorVariables::OFFSET:
				m_MotorsArray[motor].GoToAbsolutePosition
				(
					pos
				);
				break;
			default:
				break;
			}
			return m_MotorsArray[motor].GetDeviceActualStagePos();
		}
	}
	return error_position;
}

