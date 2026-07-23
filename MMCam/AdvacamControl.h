#pragma once
#ifndef ADVACAM_CONTROL_H
#define ADVACAM_CONTROL_H

#include "CameraControl.h"
#include "pxcapi.h"
#include <string>
#include <memory>
#include <mutex>

/*
	Talks to an Advacam MiniPiX (classic Timepix / TPX chip, 256x256 px) through
	the PIXet Core "pxc*" C API.

	Notes on how this maps onto CameraControl, since the PIXet API is shaped
	quite differently from XIMEA's:

	- pxcInitialize()/pxcExit() are process-wide, not per-device. A static
	  reference count is used so multiple AdvacamControl instances (or
	  re-initialization on camera reselect) don't double-init or exit the
	  library out from under another instance.
	- Devices are addressed by a 0-based index that PIXet assigns internally;
	  there's no "open by serial" call like XIMEA's xiOpenDeviceBy. So the
	  constructor's cameraSN is used to search pxcGetDeviceSerial() across all
	  enumerated devices and resolve to an index, which is then cached.
	- pxcMeasureSingleFrame() is a single blocking call that both exposes and
	  reads out the frame directly into an unsigned short* buffer (65536 px
	  for a single Timepix chip) -- this lines up with GetImage()'s return
	  type with no conversion needed. It only applies to single-output chips
	  (TPX/TPX2); MiniPIX TPX3 or Medipix3 devices need
	  pxcMeasureSingleFrameTpx3/Mpx3 instead, which return different buffer
	  layouts and are NOT handled by this class.
	- There's no persistent "set exposure" register like XIMEA's XI_PRM_EXPOSURE.
	  Exposure time is passed directly to pxcMeasureSingleFrame per-shot, so
	  SetExposureTime() just caches the value for the next GetImage() call.
	- The classic MiniPiX has no active cooling element or temperature
	  setpoint in the API -- "Temperature" is a passive chip-temperature
	  readout only. SetSensorTemperature() is therefore a no-op.
	- Supply voltage / power utilization telemetry (PowerSupplyVoltage,
	  BiasSenseVoltage, etc.) are only documented for Tpx2/Tpx3/Widepix
	  devices, not for the Mpx2/Timepix family MiniPiX belongs to. HasTelemetry()
	  still returns true because real temperature is available, but
	  GetSupplyVoltage()/GetPowerUtilization() are left at the CameraControl
	  base-class stub (0.0 / 0) -- callers showing those values for this
	  camera should expect them to be meaningless.
*/
class AdvacamControl final : public CameraControl
{
public:
	explicit AdvacamControl(std::string cameraSN) : m_CameraSN(std::move(cameraSN))
	{
		m_WantedSerial = std::stoi(m_CameraSN);
		// Timepix counting modes can exceed 12-bit range (0-4095), so treat frames as 16-bit.
		m_ImageDataType = CameraControlVariables::ImageDataTypes::RAW_16BIT;
	};
	~AdvacamControl() override { Close(); };

	auto Initialize() -> bool override;
	auto Close() -> bool override;
	auto StartAcquisition() -> bool override;
	auto StopAcquisition() -> bool override;
	auto GetImage() -> unsigned short* override;
	auto SetExposureTime(int exposure_us) -> void override;
	auto SetSensorTemperature(const double requiredTemperature) -> void override;
	auto GetSensorTemperature() -> double override;

	auto GetWidth() const -> unsigned long override;
	auto GetHeight() const -> unsigned long override;

	auto IsConnected() const -> bool override;

	auto GetFirmwareVersion() -> std::string override;

	auto GetHardwareROI() -> CameraControlVariables::HardwareROI override
	{
		return CameraControlVariables::HardwareROI(0, 0, static_cast<int>(GetWidth()), static_cast<int>(GetHeight()));
	};

	bool HasTelemetry() const override { return true; }

protected:
	std::string m_CameraSN{};
	int m_WantedSerial{ -1 };
	bool m_IsCameraOpen{ false };
	int m_DeviceIndex{ -1 };
	unsigned long m_Width{};
	unsigned long m_Height{};
	unsigned m_Exposure_us{};
	std::unique_ptr<unsigned short[]> m_FrameBuffer{};

	auto FindDeviceIndexBySerial() const -> int;

	// Guards process-wide pxc* calls, since m_TemperatureThread polls telemetry
	// concurrently with capture/UI threads calling other pxc* functions.
	static std::mutex s_ApiMutex;
	static int s_RefCount;
};

#endif // !ADVACAM_CONTROL_H
