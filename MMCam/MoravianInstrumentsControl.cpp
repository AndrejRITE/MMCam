#include "MoravianInstrumentsControl.h"

auto MoravianInstrumentsControl::Initialize() -> bool 
{
	auto cameraSN = std::stoi(m_CameraSN);
	m_CameraHandler = gxetha::Initialize(cameraSN);

	auto isConnected = IsConnected();
	if (!isConnected)
	{
		m_CameraHandler = nullptr;
		return isConnected;
	}

	GetCameraParameters(m_ActualCameraParameters.get());

	return isConnected;
}

auto MoravianInstrumentsControl::Close() -> bool 
{
	if (!m_CameraHandler) return false;
	gxetha::Release(m_CameraHandler);
	m_CameraHandler = nullptr;
	return m_CameraHandler == nullptr;
}

auto MoravianInstrumentsControl::StartAcquisition() -> bool 
{
	if (!m_CameraHandler) return false;

	auto result = gxetha::StartExposure
	(
		m_CameraHandler,
		m_CapturingParameters->exposure_sec,
		m_CapturingParameters->use_shutter,
		m_CapturingParameters->start_x,
		m_CapturingParameters->start_y,
		m_CapturingParameters->cameraImgWidth,
		m_CapturingParameters->cameraImgHeight
	);

	gxetha::CHAR err_string[100];
	if (!(bool)result)
	{
		gxetha::GetLastErrorString(m_CameraHandler, 100, err_string);
		return false;
	}

	return true;
}

auto MoravianInstrumentsControl::StopAcquisition() -> bool
{
	if (!m_CameraHandler) return false;

	gxetha::BOOLEAN rescue_image{};
	gxetha::AbortExposure(m_CameraHandler, rescue_image);

	return true;
}

auto MoravianInstrumentsControl::GetImage() -> unsigned short* 
{
	if (!m_CameraHandler) return nullptr;
	if (!m_ActualCameraParameters || !m_CapturingParameters) return nullptr;

	m_CapturingParameters->cameraImgWidth = m_ActualCameraParameters->sensor_width;
	m_CapturingParameters->cameraImgHeight = m_ActualCameraParameters->sensor_height;

	if (!m_ImageData)
		m_ImageData = std::make_unique<gxetha::INT16[]>
			(
				m_ActualCameraParameters->sensor_width * m_ActualCameraParameters->sensor_height
			);

	auto result = gxetha::StartExposure
	(
		m_CameraHandler,
		m_CapturingParameters->exposure_sec,
		m_CapturingParameters->use_shutter,
		m_CapturingParameters->start_x,
		m_CapturingParameters->start_y,
		m_CapturingParameters->cameraImgWidth,
		m_CapturingParameters->cameraImgHeight
	);

	auto continueWaiting = true;

	WaitAndCallReadImage
	(
		m_CameraHandler,
		m_CapturingParameters.get(),
		m_ImageData.get(),
		5,
		&continueWaiting,
		false
	);

	return reinterpret_cast<unsigned short*>(m_ImageData.get());
}

auto MoravianInstrumentsControl::SetExposureTime(int exposure_us) -> void 
{
	if (!m_CapturingParameters) return;

	m_CapturingParameters->exposure_sec = exposure_us / 1'000'000.0 ;
}

auto MoravianInstrumentsControl::SetSensorTemperature(const double requiredTemperature) -> void
{
	if (!m_CameraHandler) return;
	gxetha::SetTemperature(m_CameraHandler, requiredTemperature);
}

auto MoravianInstrumentsControl::GetSensorTemperature() -> double 
{
	if (!m_CameraHandler) return 0.0;
	gxetha::REAL sensorTemperature;
	gxetha::GetValue(m_CameraHandler, gvChipTemperature, &sensorTemperature);
	m_SensorTemperature = static_cast<double>(sensorTemperature);
	return m_SensorTemperature;
}

auto MoravianInstrumentsControl::IsConnected() const -> bool 
{
	if (!m_CameraHandler) return false;

	gxetha::BOOLEAN cam_connected{};
	gxetha::GetBooleanParameter(m_CameraHandler, gbpConnected, &cam_connected);
	return (bool)cam_connected;
}

auto MoravianInstrumentsControl::GetCameraParameters
(
	MoravianInstrumentsVariables::ActualCameraParameters* cameraParameters
) -> void
{
	if (!m_CameraHandler) return;

	auto camParameters = MoravianInstrumentsVariables::ActualCameraParameters{};

	/* Checking whether the camera is connected */
	{
		gxetha::BOOLEAN checker{};
		gxetha::GetBooleanParameter(m_CameraHandler, gbpConnected, &checker);
		camParameters.is_camera_connected = (int)checker;
	}

	if (!camParameters.is_camera_connected) return;

	/* Camera ID */
	gxetha::GetIntegerParameter(m_CameraHandler, gipCameraId, &camParameters.cameraID);
	/* Sensor Width and Height*/
	gxetha::GetIntegerParameter(m_CameraHandler, gipChipW, &camParameters.sensor_width);
	gxetha::GetIntegerParameter(m_CameraHandler, gipChipD, &camParameters.sensor_height);
	/* Binning */
	gxetha::GetIntegerParameter(m_CameraHandler, gipMaxBinningX, &camParameters.maxBinningX);
	gxetha::GetIntegerParameter(m_CameraHandler, gipMaxBinningY, &camParameters.maxBinningY);
	/* Exposure */
	gxetha::GetIntegerParameter(m_CameraHandler, gipMinimalExposure, &camParameters.shortest_exposure);
	gxetha::GetIntegerParameter(m_CameraHandler, gipMaximalExposure, &camParameters.longest_exposure);
	/* Gain */
	gxetha::GetIntegerParameter(m_CameraHandler, gipMaxGain, &camParameters.maxGain);

	/* Chip Temperature */
	gxetha::GetValue(m_CameraHandler, gvChipTemperature, &camParameters.sensor_temperature);

	if (!cameraParameters) return;
		*cameraParameters = std::move(camParameters);
}

auto MoravianInstrumentsControl::WaitAndCallReadImage
(
	gxetha::CCamera* camPtr, 
	const MoravianInstrumentsVariables::CapturingParameters* captParam, 
	gxetha::INT16* dataPtr, 
	const unsigned long waitingTime, 
	bool* const continueWaiting, 
	const bool continuousReading
) -> void
{
	gxetha::BOOLEAN image_ready{};

	auto startCheckingTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();

	auto waitingTimePlusExposureTime = (waitingTime + captParam->exposure_sec) * 1000.0; // Milliseconds
	double deltaTime{};
	/* 1. Wait till the Hardware Trigger signal or till the end of Waiting Time */
	do
	{
		gxetha::ImageReady(camPtr, &image_ready);
		currentTime = std::chrono::high_resolution_clock::now();
		deltaTime = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>
			(currentTime - startCheckingTime).count());
		if (!*continueWaiting || deltaTime > waitingTimePlusExposureTime)
		{
			StopAcquisition();
			return;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	} while (!(bool)image_ready);

	gxetha::BOOLEAN result{};
	/* 2. Reading Captured Image */
	if (continuousReading)
		result = gxetha::ReadImageExposure
		(
			camPtr,
			captParam->cameraImgWidth * captParam->cameraImgHeight * sizeof(gxetha::INT16),
			dataPtr
		);
	else
		result = gxetha::ReadImage
		(
			camPtr,
			captParam->cameraImgWidth * captParam->cameraImgHeight * sizeof(gxetha::INT16),
			dataPtr
		);


	/* 3. Reading Error message if it exists */
	gxetha::CHAR err_string[100];
	if (!(bool)result)
	{
		gxetha::GetLastErrorString(camPtr, 100, err_string);
		return;
	}
}
