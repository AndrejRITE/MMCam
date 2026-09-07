#pragma once
#ifndef XERYONMOTOR_H
#define XERYONMOTOR_H

#include <windows.h>

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
#include <mutex>

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

class XeryonMotor final : public IMotor
{
public:
	XeryonMotor();

    // Common motor operations
	bool GoCenter() override;
	bool GoHomeAndZero() override { return GoCenter(); };
	bool GoToAbsolutePosition(float stagePosition) override;

	// Stops whatever move is currently running on this motor:
	// - TerminateProcess on the stuck helper script, which frees the app
	//   (Stop becomes clickable, the move thread unblocks) but does NOT by
	//   itself stop the physical stage - see stopMovements() below.
	// - Once the COM port is confirmed free, run xeryon_stop.py, which
	//   calls Xeryon's controller.stopMovements() - a real STOP=0 sent to
	//   the controller, in place, without homing or closing the port
	//   (unlike controller.stop(), which does both) - then reads back the
	//   stage's true position so the caller isn't left trusting a stale
	//   value. This can take up to ~2 seconds; call it off the UI thread.
	bool Stop() override;

	// Records the process handle currently running a move (as an
	// independently-owned duplicate - see the .cpp), so Stop() on another
	// thread can find and terminate it without racing the owning thread's
	// own cleanup. Pass nullptr to clear. Thread-safe.
	void SetActiveProcessHandle(HANDLE handle);

	// Getters
	std::string GetDeviceSerNum() const override { return m_MotorSerialNumber; };
	std::string GetDeviceCOMPort() const override { return m_MotorCOMPort; };
	float GetDeviceRange() const override { return (abs(m_MotorSettings->minMotorPos) + abs(m_MotorSettings->maxMotorPos)); };
	float GetDeviceActualStagePos() const override { return m_MotorSettings->motorPos; };

	// Setters
	void SetDeviceName(const std::string& deviceName) override {};
	void SetMotorSerialNumber(std::string serialNumber) override { m_MotorSerialNumber = serialNumber; };
	void SetStepsPerMMRatio(const int stepsPerMMRatio) override { m_MotorSettings->stepsPerMMRatio = static_cast<float>(stepsPerMMRatio); };
	void SetRange(const float minMotorDeg, const float maxMotorDeg) override { m_MotorSettings->minMotorPos = minMotorDeg; m_MotorSettings->maxMotorPos = maxMotorDeg; };
	void SetCurrentMotorPosition(const float motorPosition) override { m_MotorSettings->motorPos += motorPosition; };

	void SetMotorCOMPort(std::string comPort) { m_MotorCOMPort = comPort; };

	void UpdateCurrentPosition() override {};

	~XeryonMotor() override
	{
		if (m_ActiveProcessHandle)
			CloseHandle(m_ActiveProcessHandle);
	}

	/* Move constructor */
	XeryonMotor(XeryonMotor&& other) noexcept
		: m_MotorSettings(std::move(other.m_MotorSettings)),
		m_MotorCOMPort(std::move(other.m_MotorCOMPort)),
		m_MotorSerialNumber(std::move(other.m_MotorSerialNumber)),
		m_ProcessMutex(std::move(other.m_ProcessMutex)),
		m_ActiveProcessHandle(other.m_ActiveProcessHandle)
	{
		other.m_MotorSerialNumber = "";
		other.m_MotorCOMPort = "";
		other.m_ActiveProcessHandle = nullptr;
	};

	/* Move assignment */
	XeryonMotor& operator=(XeryonMotor&& other) noexcept
	{
		if (this != &other)
		{
			if (m_ActiveProcessHandle)
				CloseHandle(m_ActiveProcessHandle);

			m_MotorSettings = std::move(other.m_MotorSettings);
			m_MotorCOMPort = std::move(other.m_MotorCOMPort);
			m_MotorSerialNumber = std::move(other.m_MotorSerialNumber);
			m_ProcessMutex = std::move(other.m_ProcessMutex);
			m_ActiveProcessHandle = other.m_ActiveProcessHandle;
			other.m_ActiveProcessHandle = nullptr;
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

	bool RunPowerShellCommandSilently(const std::string& command)
	{
		std::wstring fullCommand = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"" + std::wstring(command.begin(), command.end()) + L"\"";

		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = {};
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;  // Hide the window

		// Create the process silently
		BOOL success = CreateProcessW(
			nullptr,
			&fullCommand[0],  // Command line must be writable
			nullptr,
			nullptr,
			FALSE,
			CREATE_NO_WINDOW,  // Important flag to hide window
			nullptr,
			nullptr,
			&si,
			&pi
		);

		if (!success)
			return false;

		// Gives Stop() (on another thread) an independent, safe-to-use
		// handle - see SetActiveProcessHandle in the .cpp.
		SetActiveProcessHandle(pi.hProcess);

		// Blocks here until the script exits on its own, OR until Stop()
		// (on another thread) terminates it via its own duplicated handle.
		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode{};
		GetExitCodeProcess(pi.hProcess, &exitCode);

		// No-op if Stop() already took ownership of the tracked handle
		// (see Stop() in the .cpp) - avoids a double-close race.
		SetActiveProcessHandle(nullptr);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		// If Stop() terminated us (it sets this specific exit code), the
		// script never finished writing a fresh temp position file, so the
		// caller must not trust it.
		return exitCode != m_StoppedByUserExitCode;
	}

private:
	std::string m_MotorCOMPort{};
	std::string m_MotorSerialNumber{};
	std::unique_ptr<MotorVariables::Settings> m_MotorSettings{};
	
	std::string m_AbsolutePositionScriptName{ "xeryon_setAbsolutePosition.py" };
	std::string m_CenterScriptName{ "xeryon_goCenter.py" };
	std::string m_StopScriptName{ "xeryon_stop.py" };

	std::string m_AxisPositionTempFileName{ "temp\\axisPosition.temp" };

	int m_MaxAttemptsToCallPythonFunction{ 5 };
	const long long m_WaitAfterMovementMilliseconds{ 500 };

	// Heap-allocated so XeryonMotor stays movable (std::mutex itself is not).
	// Guards m_ActiveProcessHandle, which Stop() reads from a different
	// thread than the one currently blocked in WaitForSingleObject.
	std::unique_ptr<std::mutex> m_ProcessMutex{ std::make_unique<std::mutex>() };
	HANDLE m_ActiveProcessHandle{ nullptr };

	// Arbitrary, unlikely to collide with a real script's own exit code -
	// passed to TerminateProcess() in Stop() and checked against in
	// RunPowerShellCommandSilently() above.
	static constexpr DWORD m_StoppedByUserExitCode = 0xDEAD10CCu;
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

	bool StopMotor(const std::string& motor_sn) override;
	bool StopAll() override;

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
