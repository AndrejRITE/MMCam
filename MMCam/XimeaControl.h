#pragma once
#ifndef XIMEA_CONTROL_H
#define XIMEA_CONTROL_H

#define WIN32

#include "CameraControl.h"
#include "xiApi.h"
#include <fstream>
#include <vector>

class XimeaControl final : public CameraControl
{
public:
	explicit XimeaControl(std::string cameraSN) : m_CameraSN(std::move(cameraSN)) 
	{
		m_ImageDataType = CameraControlVariables::ImageDataTypes::RAW_12BIT;
	};
	~XimeaControl() override { Close(); };

	auto Initialize() -> bool override;
	auto Close() -> bool override;
	auto StartAcquisition() -> bool override;
	auto StopAcquisition() -> bool override;
	auto GetImage() -> unsigned short* override;
	auto SetExposureTime(int exposure_us) -> void override;
	auto SetSensorTemperature(const double requiredTemperature) -> void override { m_SensorTemperature = requiredTemperature; };
	auto GetSensorTemperature() -> double override { return m_SensorTemperature; };

	auto GetWidth() const -> unsigned long override;
	auto GetHeight() const -> unsigned long override;

	auto IsConnected() const -> bool override;

protected:
	bool m_IsCameraOpen{ false };
	std::string m_CameraSN{};
	HANDLE m_CamHandler{};
	XI_IMG m_Image{};
	XI_RETURN m_State{};
	unsigned int m_Exposure{};
	unsigned short m_CamerasCount{};
	bool m_WasAcquisitionStopped{};
};

#endif // !XIMEA_CONTROL_H

