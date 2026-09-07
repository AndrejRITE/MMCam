#include "XeryonMotor.h"

XeryonMotor::XeryonMotor()
{
	m_MotorSettings = std::make_unique<MotorVariables::Settings>();
}

void XeryonMotor::SetActiveProcessHandle(HANDLE handle)
{
	if (!m_ProcessMutex) return;

	std::lock_guard<std::mutex> lock(*m_ProcessMutex);

	// Close whatever we were previously tracking - either the caller is
	// replacing it, or clearing it (Stop() already took ownership of the
	// old one directly, in which case this is a no-op since it already set
	// m_ActiveProcessHandle to nullptr itself before we get here).
	if (m_ActiveProcessHandle)
	{
		CloseHandle(m_ActiveProcessHandle);
		m_ActiveProcessHandle = nullptr;
	}

	if (!handle) return;

	// Duplicate rather than store the raw handle: RunPowerShellCommandSilently()
	// closes its own pi.hProcess once the child exits, and Stop() runs on a
	// different thread that may still be using this value at that exact
	// moment. Giving Stop() its own independently-owned reference avoids a
	// close/reuse race on the shared handle value.
	HANDLE duplicated{ nullptr };
	if (DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &duplicated, 0, FALSE, DUPLICATE_SAME_ACCESS))
		m_ActiveProcessHandle = duplicated;
}

bool XeryonMotor::Stop()
{
	HANDLE handleToKill{ nullptr };
	{
		std::lock_guard<std::mutex> lock(*m_ProcessMutex);
		// Take exclusive ownership of the tracked handle so
		// RunPowerShellCommandSilently()'s later SetActiveProcessHandle(nullptr)
		// call becomes a safe no-op instead of double-closing it.
		handleToKill = m_ActiveProcessHandle;
		m_ActiveProcessHandle = nullptr;
	}

	// Nothing running for this motor right now.
	if (!handleToKill) return false;

	TerminateProcess(handleToKill, m_StoppedByUserExitCode);

	// Give the killed process (and the COM port it held) a moment to fully
	// release before we try to reopen the same port below. TerminateProcess
	// only stops our helper script - it does not, by itself, stop the
	// physical stage.
	WaitForSingleObject(handleToKill, 2000);
	CloseHandle(handleToKill);

	// Send a real, immediate halt to the controller firmware itself
	// (STOP=0, via Xeryon's stopMovements() - see xeryon_stop.py) and read
	// back the stage's true position now that it has actually stopped.
	std::string command = "py.exe \""
		+ m_StopScriptName + "\" \""
		+ m_MotorCOMPort + "\" \""
		+ m_AxisPositionTempFileName + "\"";

	if (!RunPowerShellCommandSilently(command))
		return false;

	double currentPosition{};
	std::ifstream file(m_AxisPositionTempFileName);
	file >> currentPosition;
	file.close();

	m_MotorSettings->motorPos = static_cast<float>(currentPosition);

	return true;
}

bool XeryonMotor::GoCenter()
{
	auto attempt = 0;

	while (attempt < m_MaxAttemptsToCallPythonFunction)
	{
		try
		{
			std::string command = "py.exe \"" 
				+ m_CenterScriptName + "\" \"" 
				+ m_MotorCOMPort + "\" \"" 
				+ m_AxisPositionTempFileName + "\"";

			if (!RunPowerShellCommandSilently(command))
			{
				// Interrupted by Stop(): the script may not have finished
				// writing the temp file, so don't trust it - and don't
				// retry, since that would re-issue a move the user just
				// asked to cancel. The stage's true position is now
				// unknown until the next successful move/position query.
				return false;
			}

			double currentPosition{};
			std::ifstream file(m_AxisPositionTempFileName);  // Open the file
			file >> currentPosition;  // Read the double value
			file.close();

			m_MotorSettings->motorPos = static_cast<float>(currentPosition);

			return true;  // Success
		}
		catch (...)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));  // Wait before retrying
			attempt++;
		}
	}

	return false;
}

bool XeryonMotor::GoToAbsolutePosition(float stagePosition)
{
	auto attempt = 0;

	while (attempt < m_MaxAttemptsToCallPythonFunction)
	{
		try
		{
			std::string command = "py.exe \"" 
				+ m_AbsolutePositionScriptName + "\" \"" 
				+ m_MotorCOMPort + "\" \"" 
				+ std::to_string(stagePosition) + "\" \"" 
				+ m_AxisPositionTempFileName + "\"";

			if (!RunPowerShellCommandSilently(command))
			{
				// See GoCenter() - interrupted by Stop(), don't trust the
				// temp file and don't retry.
				return false;
			}

			double currentPosition{};
			std::ifstream file(m_AxisPositionTempFileName);  // Open the file
			file >> currentPosition;  // Read the double value
			file.close();

			SetCurrentMotorPosition(static_cast<float>(currentPosition));
			
			return true;  // Success
		}
		catch (...)
		{
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
}

std::string XeryonMotorArray::GetMotorCOMPort(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return "";

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	return (it != m_MotorsArray.end()) ? it->GetDeviceCOMPort() : "";
}

bool XeryonMotorArray::IsMotorConnected(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return false;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	return (it != m_MotorsArray.end()) ? true : false;
}

void XeryonMotorArray::SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM)
{
	if (motor_sn.empty() || motor_sn == "None") return;

	if (stepsPerMM <= 0) return;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	if (it != m_MotorsArray.end())
		it->SetStepsPerMMRatio(stepsPerMM);
}

void XeryonMotorArray::SetCurrentPositionForTheMotor(const std::string motor_sn, const float currentPosition)
{
	if (motor_sn.empty() || motor_sn == "None") return;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](const XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	if (it != m_MotorsArray.end())
		it->SetCurrentMotorPosition((float)currentPosition);
}

bool XeryonMotorArray::StopMotor(const std::string& motor_sn)
{
	if (motor_sn.empty() || motor_sn == "None") return false;

	auto it = std::find_if(m_MotorsArray.begin(), m_MotorsArray.end(),
		[&](XeryonMotor& motor) { return motor.GetDeviceSerNum() == motor_sn; });

	return (it != m_MotorsArray.end()) ? it->Stop() : false;
}

bool XeryonMotorArray::StopAll()
{
	bool stoppedAny = false;
	for (auto& motor : m_MotorsArray)
		stoppedAny |= motor.Stop();

	return stoppedAny;
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

