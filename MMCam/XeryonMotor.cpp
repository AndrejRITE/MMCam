#include "XeryonMotor.h"

XeryonMotor::XeryonMotor()
{
	static py::scoped_interpreter guard{};  // Start Python interpreter
	m_MotorSettings = std::make_unique<MotorVariables::Settings>();
}

bool XeryonMotor::GoCenter()
{
	auto attempt = 0;

	while (attempt < m_MaxAttemptsToCallPythonFunction)
	{
		try
		{
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

#ifdef CPP
	auto currCOMPort = "\\\\.\\" + m_MotorCOMPort;
	auto controller = std::make_unique<Xeryon>(currCOMPort.c_str(), m_Baudrate);
	auto axis = controller->addAxis('X', &m_Stage);

	controller->start();
	//if (!m_Axis) return false;

	axis->findIndex();

	std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));
	m_MotorSettings->motorPos = axis->getData(commandForPosition);
	//UpdateCurrentPosition();

	controller->stop();
#endif // CPP
	return true;
}

bool XeryonMotor::GoHomeAndZero()
{
	return GoCenter();
}

bool XeryonMotor::GoToAbsolutePosition(float stagePosition)
{
	auto attempt = 0;

	while (attempt < m_MaxAttemptsToCallPythonFunction)
	{
		try
		{
			py::module importlib = py::module::import("importlib");
			py::module script_setAbsolutePosition = importlib.attr("reload")(py::module::import("xeryon_setAbsolutePosition"));

			auto currentPosition = script_setAbsolutePosition.attr("move_to_position")
				(m_MotorCOMPort.c_str(), stagePosition).cast<double>();

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

#ifdef CPP
	auto currCOMPort = "\\\\.\\" + m_MotorCOMPort;
	auto controller = std::make_unique<Xeryon>(currCOMPort.c_str(), m_Baudrate);
	auto axis = controller->addAxis('X', &m_Stage);

	controller->start();

	auto intPart = (long double)(stagePosition - m_MotorSettings->motorPos) * 1'000'000'000.0;
	Distance disNM(intPart, Distance::NM);
	axis->step(disNM);

	std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));
	m_MotorSettings->motorPos = axis->getDPOS();
	auto dpos = (double)axis->getDPOS();
	auto epos = (double)axis->getEPOS();

	//UpdateCurrentPosition();
	controller->stop();
#endif // CPP
}

float XeryonMotor::GetDeviceActualStagePos() const
{
	//if (!m_Axis) return 0.f;
	//auto currCOMPort = "\\\\.\\" + m_MotorCOMPort;
	//auto controller = std::make_unique<Xeryon>(currCOMPort.c_str(), m_Baudrate);
	//auto axis = controller->addAxis('X', &m_Stage);

	//auto position = (long double)m_Axis->getData(commandForPosition);
	//m_MotorSettings->motorPos = position;

	return m_MotorSettings->motorPos;
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

	//m_XeryonControllerArray.reserve(names_count);
	//m_XeryonAxisArray.reserve(names_count);

	auto iterator = m_AllAvailableCOMPortsWithSerialNumbers.begin();

	auto i{ 0 };
	while (iterator != m_AllAvailableCOMPortsWithSerialNumbers.end())
	{
		m_MotorsArray.emplace_back(XeryonMotor());
		m_MotorsArray[i].SetMotorCOMPort(iterator->first);
		m_MotorsArray[i].SetDeviceName(iterator->second);
		m_MotorsArray[i].SetMotorSerialNumber(iterator->second);
		//m_MotorsArray[i].SetAxisLetter(m_AxisLetters[i]);

		//auto currCOMPort = "\\\\.\\" + iterator->first;
		//auto baudrate = 115'200;
		//m_XeryonControllerArray.emplace_back
		//(
		//	std::make_unique<Xeryon>(currCOMPort.c_str(), baudrate)
		//);
		//auto axis = m_XeryonControllerArray[i]->addAxis(m_AxisLetters[i], &m_Stage);

		m_MotorsArray[i].SetCurrentMotorPosition(0.f);
		//m_MotorsArray[i].SetAxis(axis);

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

