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
	explicit XimeaControl(std::string cameraSN);
	~XimeaControl() override;

	auto Initialize() -> bool override;
	auto Close() -> bool override;
	auto StartAcquisition() -> bool override;
	auto StopAcquisition() -> bool override;
	auto GetImage() -> unsigned short* override;
	auto SetExposureTime(int exposure_us) -> void override;

	auto GetWidth() const -> unsigned long override;
	auto GetHeight() const -> unsigned long override;

	auto IsConnected() const -> bool override;

	//XimeaControl();
	//auto InitializeAllCameras() -> void;
	//auto GetCamerasCount() -> unsigned short;
	//auto GetCamerasSN() -> std::string*;
	//auto SetExposureTime(const int exposure_us) -> void;
	//auto InitializeCameraBySN(std::string cam_sn) -> bool;
	//auto GetImage() -> unsigned short*;
	//auto GetImageWidth() const -> unsigned long;
	//auto GetImageHeight() const -> unsigned long;
	//auto IsCameraInitialized() const -> bool;
	//auto CloseCamera() -> bool;
	//auto StopAcquisition() -> bool;
	//auto WasAcquisitionStopped() -> bool;
	//auto IsCameraConnected() -> bool;
	//auto TryToReconnectLastSelectedCamera() -> bool;

	//auto AppendThread() -> int; // Returns the ID
	//auto GetThreadState(int id) -> bool;
	//auto TurnOffLastThread() -> bool;
	//auto ClearAllThreads() -> void;
	//~XimeaControl();

protected:
	bool m_IsCameraOpen{ false };
	std::unique_ptr<std::string[]> m_CamerasSN{};
	std::string m_CameraSN{};
	HANDLE m_CamHandler{};
	XI_IMG m_Image;
	XI_RETURN m_State{};
	unsigned int m_Exposure{};
	unsigned short m_CamerasCount{};
	bool m_WasAcquisitionStopped{};

	// Threads
	//std::vector<bool> m_Threads{};
};

#endif // !XIMEA_CONTROL_H

