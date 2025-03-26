#include "XeryonMotor.h"

XeryonMotor::XeryonMotor()
{
	m_MotorSettings = std::make_unique<MotorVariables::Settings>();

}

bool XeryonMotor::GoCenter()
{

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
	//try 
	//{
	//	m_MotorSettings->motorPos = script_setAbsolutePosition.attr("move_to_position")
	//		(m_MotorCOMPort.c_str(), stagePosition).cast<double>();
	//}
	//catch (const py::error_already_set& e) 
	//{
	//	//std::cerr << "Python Error: " << e.what() << std::endl;
	//	return false;
	//}

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

	return true;
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
	static py::scoped_interpreter guard{};  // Start Python interpreter
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
	auto goCenter = [&](XeryonMotor* motor)
		{
			try
			{
				py::module importlib = py::module::import("importlib");
				py::module script_goCenter = importlib.attr("reload")(py::module::import("xeryon_goCenter"));

				motor->SetCurrentMotorPosition(script_goCenter.attr("move_to_position")
					(motor->GetDeviceCOMPort().c_str()).cast<double>());
			}
			catch (const py::error_already_set& e)
			{
				auto err = std::string(e.what());
				return false;
			}
		};

	auto setAbsolutePosition = [&](XeryonMotor* motor, float position)
		{
			try
			{
				py::module importlib = py::module::import("importlib");
				py::module script_setAbsolutePosition = importlib.attr("reload")(py::module::import("xeryon_setAbsolutePosition"));

				motor->SetCurrentMotorPosition(script_setAbsolutePosition.attr("move_to_position")
					(motor->GetDeviceCOMPort().c_str(), position).cast<double>());
			}
			catch (const py::error_already_set& e)
			{
				auto err = std::string(e.what());
				return false;
			}
		};


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
				goCenter(&m_MotorsArray[motor]);
				//m_MotorsArray[motor].GoCenter();
				break;
			case XeryonMotorVariables::ABS_POSITION:
				setAbsolutePosition(&m_MotorsArray[motor], pos);
				//m_MotorsArray[motor].GoToAbsolutePosition(pos);
				break;
			case XeryonMotorVariables::OFFSET:
				setAbsolutePosition(&m_MotorsArray[motor], m_MotorsArray[motor].GetDeviceActualStagePos() + pos);
				//m_MotorsArray[motor].GoToAbsolutePosition(m_MotorsArray[motor].GetDeviceActualStagePos() + pos);
				break;
			default:
				break;
			}
			return m_MotorsArray[motor].GetDeviceActualStagePos();
		}
	}
	return error_position;
}

