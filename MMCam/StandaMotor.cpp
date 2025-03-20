#include "StandaMotor.h"
/* Motor */

StandaMotor::StandaMotor()
{
	m_MotorSettings = std::make_unique<MotorVariables::Settings>();
	m_StandaSettings = std::make_unique<StandaVariables::C_Settings>();
}

bool StandaMotor::GoCenter()
{
	device_t device_c;
	device_c = open_device(m_DeviceName.c_str());

	{
		if ((m_StandaSettings->result = command_move_calb
		(
			device_c,
			m_MotorSettings->middleMotorPos,
			&m_StandaSettings->calibration
		)
			) != result_ok)
		{
			/* Error command_move_calb */
			return false;
		}
		/* Wait to Stop */
		if ((m_StandaSettings->result = command_wait_for_stop
		(
			device_c,
			100
		)
			) != result_ok)
		{
			/* Error command_wait_for_stop */
			return false;
		}
		/* Wait for elimination of vibrations */
		std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));
		/* Get Status */
		if ((m_StandaSettings->result = get_status
		(
			device_c,
			&m_StandaSettings->state
		)
			) != result_ok)
		{
			/* Error getting status */
			return false;
		}
		/* Get Calibrated Status */
		if ((m_StandaSettings->result = get_status_calb
		(
			device_c,
			&m_StandaSettings->calb_state,
			&m_StandaSettings->calibration
		)
			) != result_ok)
		{
			/* Error getting status */
			return false;
		}
	}

	close_device(&device_c);

	UpdateCurrentPosition();

	return true;
}

bool StandaMotor::GoHomeAndZero()
{
	device_t device_c;
	device_c = open_device(m_DeviceName.c_str());

	{
		if ((m_StandaSettings->result = command_homezero(device_c) != result_ok)) return false;
		/* Wait to Stop */
		if ((m_StandaSettings->result = command_wait_for_stop
		(
			device_c,
			100
		)
			) != result_ok)
		{
			/* Error command_wait_for_stop */
			return false;
		}
		/* Wait for elimination of vibrations */
		std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));
		/* Get Status */
		if ((m_StandaSettings->result = get_status
		(
			device_c,
			&m_StandaSettings->state
		)
			) != result_ok)
		{
			/* Error getting status */
			return false;
		}
		/* Get Calibrated Status */
		if ((m_StandaSettings->result = get_status_calb
		(
			device_c,
			&m_StandaSettings->calb_state,
			&m_StandaSettings->calibration
		)
			) != result_ok)
		{
			/* Error getting status */
			return false;
		}
	}

	close_device(&device_c);

	UpdateCurrentPosition();
}

bool StandaMotor::GoToAbsolutePosition(float stagePosition)
{
	device_t device_c;
	device_c = open_device(&m_DeviceName[0]);

	/* Get Status */
	if ((m_StandaSettings->result = get_status_calb
	(
		device_c,
		&m_StandaSettings->calb_state,
		&m_StandaSettings->calibration
	)
		) != result_ok)
	{
		/* Error getting status */
		return false;
	}

	/* If stage_position is outside of motor's range -> return */
	if (stagePosition < m_MotorSettings->minStagePos ||
		stagePosition > m_MotorSettings->maxStagePos)
		return false;

	{
		//float motor_position = stage_position * deg_per_mm;
		float motor_position = stagePosition * m_MotorSettings->stepsPerMMRatio;
		if ((m_StandaSettings->result = command_move_calb
		(
			device_c,
			motor_position,
			&m_StandaSettings->calibration
		) != result_ok))
			return false;

		/* Wait to Stop */
		if ((m_StandaSettings->result = command_wait_for_stop
		(
			device_c,
			100
		)
			) != result_ok)
		{
			/* Error command_wait_for_stop */
			return false;
		}

		/* Wait for elimination of vibrations */
		std::this_thread::sleep_for(std::chrono::milliseconds(m_WaitAfterMovementMilliseconds));
		/* Get Status */
		if ((m_StandaSettings->result = get_status
		(
			device_c,
			&m_StandaSettings->state
		)
			) != result_ok)
		{
			/* Error getting status */
			return false;
		}
		/* Get Calibrated Status */
		if ((m_StandaSettings->result = get_status_calb
		(
			device_c,
			&m_StandaSettings->calb_state,
			&m_StandaSettings->calibration
		)
			) != result_ok)
		{
			/* Error getting status */
			return false;
		}
	}
	close_device(&device_c);

	UpdateCurrentPosition();
}

//void StandaMotor::SetDeviceName(const std::string& deviceName)
//{
//	if (deviceName.empty()) return;
//	m_DeviceName = deviceName;
//
//	//size_t char_count{};
//	//while (*device_name != '\0')
//	//{
//	//	++char_count;
//	//	++device_name;
//	//}
//	//device_name -= char_count;
//
//	//m_DeviceName = std::make_unique<char[]>(char_count + 1);
//	////strncpy(m_DeviceName.get(), device_name, char_count);
//	//memcpy(m_DeviceName.get(), device_name, char_count);
//	//m_DeviceName[char_count] = '\0';
//}

void StandaMotor::SetRange(const float minMotorDeg, const float maxMotorDeg)
{
	/* Min position */
	m_MotorSettings->minMotorPos = minMotorDeg;
	//m_MotorSettings->minStagePos = min_motor_deg / deg_per_mm;
	m_MotorSettings->minStagePos = minMotorDeg / m_MotorSettings->stepsPerMMRatio;

	/* Middle position */
	m_MotorSettings->middleMotorPos = (maxMotorDeg - minMotorDeg) / 2.f;
	//m_MotorSettings->middleStagePos = m_MotorSettings->middleMotorPos / deg_per_mm;
	m_MotorSettings->middleStagePos = m_MotorSettings->middleMotorPos / m_MotorSettings->stepsPerMMRatio;

	/* Max position */
	m_MotorSettings->maxMotorPos = maxMotorDeg;
	//m_MotorSettings->maxStagePos = max_motor_deg / deg_per_mm;
	m_MotorSettings->maxStagePos = maxMotorDeg / m_MotorSettings->stepsPerMMRatio;

	/* Set Whole Motor Range */
	m_MotorSettings->motorRange = maxMotorDeg - minMotorDeg;
	//m_MotorSettings->stageRange = (max_motor_deg - min_motor_deg) / deg_per_mm;
	m_MotorSettings->stageRange = (maxMotorDeg - minMotorDeg) / m_MotorSettings->stepsPerMMRatio;
}


/* StandaMotorArray */
StandaMotorArray::StandaMotorArray(const std::string& ipAddress)
{
	InitAllMotors(ipAddress);
}

auto StandaMotorArray::InitAllMotors(const std::string ip_address) -> bool
{
	auto appendUnitializedMotor = [&](const std::string motorSN, const int motorNum)
		{
			m_UninitializedMotors.push_back(motorSN);
			m_MotorsArray[motorNum].SetMotorSerialNumber(0);
		};

	m_UninitializedMotors.clear();

	const char* correction_table = "table.txt";
	// Checking whether table.txt is inside the working directory
	{
		auto currPath = std::filesystem::current_path();
		if (!std::filesystem::exists(currPath / correction_table)) return false;
	}

	result_t result_c;
	result_c = set_bindy_key("keyfile.sqlite");
	if (result_c != result_ok) return false;

	device_enumeration_t devenum_c;

#ifdef _DEBUG
	const int probe_flags = ENUMERATE_PROBE;
	const char* enumerate_hints = "addr=";
	devenum_c = enumerate_devices(probe_flags, enumerate_hints);
#else
	const int probe_flags = ENUMERATE_PROBE | ENUMERATE_NETWORK;
	std::string eh = std::string("addr=") + ip_address;
	const char* enumerate_hints = eh.c_str();
	//const char* enumerate_hints = "addr=10.0.0.134";
	devenum_c = enumerate_devices(probe_flags, enumerate_hints);
#endif // _DEBUG
	if (!devenum_c) return false;

	int names_count = get_device_count(devenum_c);

	/* Here we need to clear motor list */
	m_MotorsArray.clear();
	m_MotorsArray.reserve(names_count);

	char device_name[256];
	device_t device_c;
	status_t state_c;
	status_calb_t state_calb_c;
	emf_settings_t emfSettings{};
	calibration_t calibration_c;
	stage_settings_t stage_settings_c;
	edges_settings_calb_t edges_settings_calb_c;
	stage_information_t stage_information_c;
	unsigned int device_sn_int{};
	std::string device_sn{};
	for (int i = 0; i < names_count; ++i)
	{
		m_MotorsArray.emplace_back(StandaMotor());

		//m_MotorsArray.emplace_back(StandaMotor());
		//m_MotorsArray[i].SetResult(result_c);

		strcpy(device_name, get_device_name(devenum_c, i));
		m_MotorsArray[i].SetDeviceName(device_name);
		device_c = open_device(device_name);
		get_serial_number(device_c, &device_sn_int);
		device_sn = std::to_string(device_sn_int);
		m_MotorsArray[i].SetMotorSerialNumber(device_sn);

		if ((result_c = get_status(device_c, &state_c)) != result_ok)
		{
			appendUnitializedMotor(device_sn, i);
			continue;
		}

		// The device_t device parameter in this function is a C pointer, unlike most library functions that use this parameter
		if ((result_c = set_correction_table(device_c, correction_table)) != result_ok)
		{
			appendUnitializedMotor(device_sn, i);
			continue;
		}

		calibration_c.A = 1;
		calibration_c.MicrostepMode = MICROSTEP_MODE_FULL;
		//calibration_c.MicrostepMode = MICROSTEP_MODE_FRAC_256;

		//m_MotorsArray[i].SetCalibration(calibration_c);
		m_MotorsArray[i].SetConversionFactor(calibration_c.A);
		m_MotorsArray[i].SetMicrostepMode(calibration_c.MicrostepMode);

		/* Get Status */
		if ((result_c = get_status_calb(device_c, &state_calb_c, &calibration_c)) != result_ok)
		{
			appendUnitializedMotor(device_sn, i);
			continue;
		}
		m_MotorsArray[i].SetCurrentMotorPosition(state_c.CurPosition);

		get_edges_settings_calb(device_c, &edges_settings_calb_c, &calibration_c);
		m_MotorsArray[i].SetRange(edges_settings_calb_c.LeftBorder, edges_settings_calb_c.RightBorder);

		m_MotorsArray[i].UpdateCurrentPosition();

		close_device(&device_c);
	}

	std::sort(m_MotorsArray.begin(), m_MotorsArray.end(),
		[](const StandaMotor& left, const StandaMotor& right)
		{
			return left.GetDeviceSerNum() < right.GetDeviceSerNum();
		});

	free_enumerate_devices(devenum_c);
	FillNames();
}


auto StandaMotorArray::FillNames() -> void
{
	for (const auto& motor : m_MotorsArray)
	{
		m_NamesOfMotorsWithRanges.emplace
		(
			std::make_pair
			(
				motor.GetDeviceSerNum(), 
				motor.GetDeviceRange()
			)
		);
	}
}

float StandaMotorArray::GetActualStagePos(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
			return m_MotorsArray[motor].GetDeviceActualStagePos();
	}
	return error_position;
}

bool StandaMotorArray::IsMotorConnected(const std::string& motor_sn) const
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
			return true;
	}
	return false;
}

float StandaMotorArray::GoMotorHome(const std::string& motor_sn)
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
		{
			m_MotorsArray[motor].GoHomeAndZero();
			return m_MotorsArray[motor].GetDeviceActualStagePos();
		}
	}
	return error_position;
}

float StandaMotorArray::GoMotorCenter(const std::string& motor_sn)
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
		{
			m_MotorsArray[motor].GoCenter();
			return m_MotorsArray[motor].GetDeviceActualStagePos();
		}
	}
	return error_position;
}

float StandaMotorArray::GoMotorToAbsolutePosition(const std::string& motor_sn, float abs_pos)
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
		{
			m_MotorsArray[motor].GoToAbsolutePosition(abs_pos);
			return m_MotorsArray[motor].GetDeviceActualStagePos();
		}
	}
	return error_position;
}

float StandaMotorArray::GoMotorOffset(const std::string& motor_sn, float offset)
{
	if (motor_sn.empty() || motor_sn == "None") return 0.f;

	for (auto motor{ 0 }; motor < m_MotorsArray.size(); ++motor)
	{
		if (m_MotorsArray[motor].GetDeviceSerNum() == motor_sn)
		{
			if (offset + m_MotorsArray[motor].GetDeviceActualStagePos() < 0.f ||
				offset + m_MotorsArray[motor].GetDeviceActualStagePos() > m_MotorsArray[motor].GetDeviceRange())
				return m_MotorsArray[motor].GetDeviceActualStagePos();

			m_MotorsArray[motor].GoToAbsolutePosition(m_MotorsArray[motor].GetDeviceActualStagePos() + offset);
			return m_MotorsArray[motor].GetDeviceActualStagePos();
		}
	}
	return error_position;
}

auto StandaMotorArray::SetStepsPerMMForTheMotor(const std::string motor_sn, const int stepsPerMM) -> void
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

