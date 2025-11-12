#pragma once
#ifndef MORAVIANINSTRUMENTS_CONTROL_H
#define MORAVIANINSTRUMENTS_CONTROL_H

#define WIN32

#include "CameraControl.h"

#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

#include <gxeth.h>

namespace MoravianInstrumentsVariables
{
	/**
	 * @brief Global variable representing the camera ID.
	 *
	 */
	static gxetha::CARDINAL CAMERA_ID{};

	/**
	 * @brief Message indicating exposure abortion.
	 *
	 */
	const std::string EXPOSURE_ABORT_MSG{ "Exposure was aborted" };

	/**
	 * @brief Callback function for handling camera events.
	 *
	 * @param c_id Camera ID associated with the event.
	 */
	static void __cdecl CallbackProc(gxetha::CARDINAL c_id)
	{
		CAMERA_ID = c_id;
	};

	/**
	 * @struct ActualCameraParameters
	 * @brief Stores parameters and properties of a connected camera.
	 */
	struct ActualCameraParameters
	{
		int is_camera_connected{}; ///< Indicates if the camera is connected.
		gxetha::CARDINAL cameraID{}; ///< Camera ID.
		gxetha::REAL sensor_temperature{}; ///< Sensor temperature in degrees Celsius.
		gxetha::CARDINAL sensor_width{}, sensor_height{}; ///< Dimensions of the camera sensor.
		gxetha::CARDINAL shortest_exposure{}, longest_exposure{}; ///< Supported exposure range.
		gxetha::CARDINAL maxBinningX{}, maxBinningY{}; ///< Maximum binning factors.
		gxetha::CARDINAL maxGain{}; ///< Maximum gain value.
	};

	/**
	 * @struct CapturingParameters
	 * @brief Defines parameters for capturing images using the camera.
	 */
	struct CapturingParameters
	{
		bool continuous_scan{}; ///< Flag for continuous scanning mode.
		gxetha::LONGREAL exposure_sec{}; ///< Exposure duration in seconds.
		gxetha::BOOLEAN use_shutter{}; ///< Indicates if the shutter is used.
		gxetha::INTEGER binning{}; ///< Binning factor.
		gxetha::INTEGER start_x, start_y{}; ///< Start coordinates for capturing.
		gxetha::INTEGER cameraImgWidth{}, cameraImgHeight{}; ///< Image dimensions as configured in the camera.
		gxetha::INTEGER imgWidth{}, imgHeight{}; ///< Final image dimensions.
	};
}

class MoravianInstrumentsControl final : public CameraControl
{
public:
	explicit MoravianInstrumentsControl(std::string cameraSN)
	{
		m_CameraSerialNumber = std::move(cameraSN);
		m_ImageDataType = CameraControlVariables::ImageDataTypes::RAW_16BIT;

		m_ActualCameraParameters = std::make_unique<MoravianInstrumentsVariables::ActualCameraParameters>();
		m_CapturingParameters = std::make_unique<MoravianInstrumentsVariables::CapturingParameters>();
	};

	~MoravianInstrumentsControl() override { Close(); };

	auto Initialize() -> bool override;
	auto Close() -> bool override;
	auto StartAcquisition() -> bool override;
	auto StopAcquisition() -> bool override;
	auto GetImage() -> unsigned short* override;
	auto SetExposureTime(int exposure_us) -> void override;
    auto SetSensorTemperature(const double requiredTemperature) -> void override;
	auto GetSensorTemperature() -> double override;

	auto GetSupplyVoltage() -> double override;

	auto GetPowerUtilization() -> int override;

	auto GetWidth() const -> unsigned long override { return m_CameraHandler == nullptr || m_ActualCameraParameters == nullptr ? 0 : m_ActualCameraParameters->sensor_width; };
	auto GetHeight() const -> unsigned long override { return m_CameraHandler == nullptr || m_ActualCameraParameters == nullptr ? 0 : m_ActualCameraParameters->sensor_height; };

	auto IsConnected() const -> bool override;

private:
	auto GetCameraParameters(MoravianInstrumentsVariables::ActualCameraParameters* cameraParameters) -> void;

	auto WaitAndCallReadImage
	(
		gxetha::CCamera* camPtr,
		const MoravianInstrumentsVariables::CapturingParameters* captParam,
		gxetha::INT16* dataPtr,
		const unsigned long waitingTime,
		bool* const continueWaiting = nullptr,
		const bool continuousReading = false
	) -> void;

protected:
	bool m_IsCameraOpen{ false };
	gxetha::CCamera* m_CameraHandler{};
	std::unique_ptr<MoravianInstrumentsVariables::ActualCameraParameters> m_ActualCameraParameters{};
	std::unique_ptr<MoravianInstrumentsVariables::CapturingParameters> m_CapturingParameters{};
	std::unique_ptr<gxetha::INT16[]> m_ImageData{};
	unsigned int m_Exposure{};
	bool m_WasAcquisitionStopped{};
};

#endif // !MORAVIANINSTRUMENTS_CONTROL_H

