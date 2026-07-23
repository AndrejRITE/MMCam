#include "AdvacamControl.h"
#include <algorithm>

std::mutex AdvacamControl::s_ApiMutex{};
int AdvacamControl::s_RefCount{ 0 };

auto AdvacamControl::FindDeviceIndexBySerial() const -> int
{
	const int count = pxcGetDevicesCount();
	for (int i = 0; i < count; ++i)
	{
		auto serial = pxcGetDeviceSerial(static_cast<unsigned>(i));
		if (serial == m_WantedSerial)
			return i;
	}

	return -1;
}

auto AdvacamControl::Initialize() -> bool
{
	std::lock_guard<std::mutex> lock(s_ApiMutex);

	if (s_RefCount == 0)
	{
		auto err = pxcInitialize();
		if (err != 0)
		{
			auto message = std::make_unique<char[]>(200);
			pxcGetLastError(message.get(), 200);
			return false;
		}
	}
	++s_RefCount;

	m_DeviceIndex = FindDeviceIndexBySerial();
	if (m_DeviceIndex < 0)
	{
		// Balance the ref count / pxcExit() before bailing out.
		if (--s_RefCount == 0)
			pxcExit();
		return false;
	}

	CDevInfo devInfo{};
	pxcGetDeviceInfo(static_cast<unsigned>(m_DeviceIndex), &devInfo);

	// This class only handles the single-buffer measurement path
	// (pxcMeasureSingleFrame). TPX3/Mpx3 devices need a different
	// GetImage() implementation and are intentionally rejected here.
	if (devInfo.type != TPX && devInfo.type != TPX2)
	{
		m_DeviceIndex = -1;
		if (--s_RefCount == 0)
			pxcExit();
		return false;
	}

	unsigned w{}, h{};
	if (pxcGetDeviceDimensions(static_cast<unsigned>(m_DeviceIndex), &w, &h) != 0)
	{
		m_DeviceIndex = -1;
		if (--s_RefCount == 0)
			pxcExit();
		return false;
	}

	m_Width = w;
	m_Height = h;
	m_FrameBuffer = std::make_unique<unsigned short[]>(static_cast<size_t>(w) * h);
	m_CameraSerialNumber = m_CameraSN;
	m_IsCameraOpen = true;

	return true;
}

auto AdvacamControl::Close() -> bool
{
	std::lock_guard<std::mutex> lock(s_ApiMutex);

	if (!m_IsCameraOpen)
		return true;

	m_IsCameraOpen = false;
	m_DeviceIndex = -1;
	m_FrameBuffer.reset();

	if (s_RefCount > 0 && --s_RefCount == 0)
		return pxcExit() == 0;

	return true;
}

auto AdvacamControl::StartAcquisition() -> bool
{
	// pxcMeasureSingleFrame() exposes and reads out the frame synchronously
	// in one call -- there's no separate hardware "arm" step to trigger.
	return m_IsCameraOpen && m_DeviceIndex >= 0;
}

auto AdvacamControl::StopAcquisition() -> bool
{
	return m_IsCameraOpen && m_DeviceIndex >= 0;
}

auto AdvacamControl::GetImage() -> unsigned short*
{
	if (!StartAcquisition() || !m_FrameBuffer)
		return nullptr;

	std::lock_guard<std::mutex> lock(s_ApiMutex);

	const double frameTimeSec = m_Exposure_us / 1'000'000.0;
	unsigned size = static_cast<unsigned>(m_Width * m_Height);

	const int rc = pxcMeasureSingleFrame
	(
		static_cast<unsigned>(m_DeviceIndex),
		frameTimeSec,
		m_FrameBuffer.get(),
		&size,
		PXC_TRG_NO
	);

	if (!StopAcquisition())
		return nullptr;

	return (rc == 0) ? m_FrameBuffer.get() : nullptr;
}

auto AdvacamControl::SetExposureTime(int exposure_us) -> void
{
	m_Exposure_us = static_cast<unsigned>(std::max(0, exposure_us));
}

auto AdvacamControl::SetSensorTemperature(const double requiredTemperature) -> void
{
	// Classic MiniPiX / Timepix has no active cooling element or temperature
	// setpoint exposed by the PIXet API -- "Temperature" is read-only chip
	// telemetry. Nothing to command; kept as a no-op to satisfy the interface.
	(void)requiredTemperature;
}

auto AdvacamControl::GetSensorTemperature() -> double
{
	if (!m_IsCameraOpen || m_DeviceIndex < 0)
		return 0.0;

	std::lock_guard<std::mutex> lock(s_ApiMutex);

	double temperature_degC{};
	if (pxcGetDeviceParameterDouble(static_cast<unsigned>(m_DeviceIndex), "Temperature", &temperature_degC) != 0)
		return 0.0;

	m_SensorTemperature = temperature_degC;
	return temperature_degC;
}

auto AdvacamControl::GetWidth() const -> unsigned long
{
	return m_Width;
}

auto AdvacamControl::GetHeight() const -> unsigned long
{
	return m_Height;
}

auto AdvacamControl::IsConnected() const -> bool
{
	if (!m_IsCameraOpen || m_DeviceIndex < 0)
		return false;

	std::lock_guard<std::mutex> lock(s_ApiMutex);
	const int count = pxcGetDevicesCount();
	for (int i = 0; i < count; ++i)
	{
		if (pxcGetDeviceSerial(static_cast<unsigned>(i)) == m_WantedSerial)
			return true;
	}

	return false;
}

auto AdvacamControl::GetFirmwareVersion() -> std::string
{
	if (!m_IsCameraOpen || m_DeviceIndex < 0)
		return std::string();

	std::lock_guard<std::mutex> lock(s_ApiMutex);

	char value[200] = "";
	if (pxcGetDeviceParameterString(static_cast<unsigned>(m_DeviceIndex), "HwLibVer", value, sizeof(value)) != 0)
		return std::string();

	return std::string(value);
}
