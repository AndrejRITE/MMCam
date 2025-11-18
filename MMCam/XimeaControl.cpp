#include "XimeaControl.h"

auto XimeaControl::Initialize() -> bool
{
	m_State = xiOpenDeviceBy(XI_OPEN_BY_SN, m_CameraSN.c_str(), &m_CamHandler);
	m_IsCameraOpen = (m_State == XI_OK);

	if (!m_IsCameraOpen)
		return false;

	m_CameraSerialNumber = m_CameraSN;

	memset(&m_Image, 0, sizeof(m_Image));
	m_Image.size = sizeof(XI_IMG);

	// Set the data format to 12-bit
	m_State = xiSetParamInt(m_CamHandler, XI_PRM_IMAGE_DATA_FORMAT, XI_RAW16);

	return (m_State == XI_OK);
}

auto XimeaControl::Close() -> bool
{
	if (m_IsCameraOpen && xiCloseDevice(m_CamHandler) == XI_OK)
	{
		m_IsCameraOpen = false;
		m_CamHandler = nullptr;
		return true;
	}
	return false;
}

auto XimeaControl::StartAcquisition() -> bool
{
	return (m_IsCameraOpen && xiStartAcquisition(m_CamHandler) == XI_OK);
}

auto XimeaControl::StopAcquisition() -> bool
{
	return (m_IsCameraOpen && xiStopAcquisition(m_CamHandler) == XI_OK);
}

auto XimeaControl::GetImage() -> unsigned short*
{
	if (!m_IsCameraOpen)
		return nullptr;

	if (!StartAcquisition()) return nullptr;

	DWORD timeout = (m_Exposure / 1000.0) + 5000;
	m_State = xiGetImage(m_CamHandler, timeout, &m_Image);

	if (!StopAcquisition()) return nullptr;

	return (m_State == XI_OK) ? static_cast<unsigned short*>(m_Image.bp) : nullptr;
}

auto XimeaControl::SetExposureTime(int exposure_us) -> void
{
	if (!m_IsCameraOpen) return;

	m_State = xiSetParamInt(m_CamHandler, XI_PRM_EXPOSURE, exposure_us);
	if (m_State == XI_OK)
		m_Exposure = exposure_us;
}

auto XimeaControl::GetWidth() const -> unsigned long
{
	int img_width{};
	xiGetParamInt(m_CamHandler, XI_PRM_WIDTH, &img_width);
	return static_cast<unsigned long>(img_width);
}

auto XimeaControl::GetHeight() const -> unsigned long
{
	int img_height{};
	xiGetParamInt(m_CamHandler, XI_PRM_HEIGHT, &img_height);
	return static_cast<unsigned long>(img_height);
}

auto XimeaControl::IsConnected() const -> bool
{
	DWORD numDevices{};
	if (xiGetNumberDevices(&numDevices) != XI_OK || numDevices == 0)
		return false;

	for (auto i = 0; i < numDevices; ++i)
	{
		char serial[256]{};
		if (
			xiGetDeviceInfoString(i, XI_PRM_DEVICE_SN, serial, sizeof(serial)) == XI_OK &&
			m_CameraSN == serial)
		{
			return true;
		}
	}
	return false;
}
