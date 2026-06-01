#include "MoravianInstrumentsControl.h"

auto MoravianInstrumentsControl::Initialize() -> bool 
{
	auto cameraSN = std::stoi(m_CameraSerialNumber);
	m_CameraHandler = gxetha::Initialize(cameraSN);

	auto isConnected = IsConnected();
	if (!isConnected)
	{
		m_CameraHandler = nullptr;
		return isConnected;
	}

	m_IsCameraOpen = isConnected;
	GetCameraParameters(m_ActualCameraParameters.get());

	return isConnected;
}

auto MoravianInstrumentsControl::Close() -> bool 
{
	if (!m_CameraHandler) return false;
	gxetha::Release(m_CameraHandler);
	m_CameraHandler = nullptr;
	m_IsCameraOpen = false;
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
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CameraHandler || !m_IsCameraOpen)
		return false;

	if (!m_ExposureInProgress.load(std::memory_order_acquire))
		return true;

	gxetha::BOOLEAN rescue_image{};

	const auto result = gxetha::AbortExposure(m_CameraHandler, rescue_image);

	m_ExposureInProgress.store(false, std::memory_order_release);

	return static_cast<bool>(result);
}

auto MoravianInstrumentsControl::GetImage() -> unsigned short* 
{
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CameraHandler || !m_IsCameraOpen)
		return nullptr;

	if (!m_ActualCameraParameters || !m_CapturingParameters)
		return nullptr;

	if (m_CapturingParameters->cameraImgWidth <= 0 || m_CapturingParameters->cameraImgHeight <= 0)
		ResetHardwareROIToFullFrame();

	const size_t need =
		static_cast<size_t>(m_CapturingParameters->cameraImgWidth) *
		static_cast<size_t>(m_CapturingParameters->cameraImgHeight);

	if (need == 0)
		return nullptr;

	if (!m_ImageData || m_ImageElemCount != need)
	{
		m_ImageData = std::make_unique<gxetha::INT16[]>(need);
		m_ImageElemCount = need;
	}

	m_ExposureInProgress.store(true, std::memory_order_release);

	const auto startResult = gxetha::StartExposure
	(
		m_CameraHandler,
		m_CapturingParameters->exposure_sec,
		m_CapturingParameters->use_shutter,
		m_CapturingParameters->start_x,
		m_CapturingParameters->start_y,
		m_CapturingParameters->cameraImgWidth,
		m_CapturingParameters->cameraImgHeight
	);

	if (!static_cast<bool>(startResult))
	{
		m_ExposureInProgress.store(false, std::memory_order_release);

		gxetha::CHAR err_string[100]{};
		gxetha::GetLastErrorString(m_CameraHandler, 100, err_string);

		return nullptr;
	}

	bool continueWaiting = true;

	const bool readOk = WaitAndCallReadImage
	(
		m_CameraHandler,
		m_CapturingParameters.get(),
		m_ImageData.get(),
		5,
		&continueWaiting,
		false
	);

	m_ExposureInProgress.store(false, std::memory_order_release);

	if (!readOk)
		return nullptr;

	return reinterpret_cast<unsigned short*>(m_ImageData.get());
}

auto MoravianInstrumentsControl::SetExposureTime(int exposure_us) -> void 
{
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CapturingParameters) return;

	m_CapturingParameters->exposure_sec = exposure_us / 1'000'000.0 ;
}

auto MoravianInstrumentsControl::SetSensorTemperature(const double requiredTemperature) -> void
{
	if (!m_CameraHandler) return;
	gxetha::SetTemperature(m_CameraHandler, static_cast<gxetha::REAL>(requiredTemperature));
}

auto MoravianInstrumentsControl::GetSensorTemperature() -> double 
{
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CameraHandler || !IsConnected()) return 0.0;

	gxetha::REAL value{};

	if (!gxetha::GetValue(m_CameraHandler, gvChipTemperature, &value))
		return m_SensorTemperature;

	m_SensorTemperature = static_cast<double>(value);

	return m_SensorTemperature;
}

auto MoravianInstrumentsControl::GetSupplyVoltage() -> double
{
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CameraHandler || !IsConnected()) return 0.0;

	gxetha::REAL value{};

	if (!gxetha::GetValue(m_CameraHandler, gvSupplyVoltage, &value))
		return m_SupplyVoltage;

	m_SupplyVoltage = static_cast<double>(value);
	return m_SupplyVoltage;
}

auto MoravianInstrumentsControl::GetPowerUtilization() -> int
{
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CameraHandler || !m_IsCameraOpen)
		return 0;

	gxetha::BOOLEAN state{};

	if (!gxetha::GetBooleanParameter(m_CameraHandler, gbpPowerUtilization, &state))
		return -1;

	if (!state)
		return -1;

	gxetha::REAL value{};

	if (!gxetha::GetValue(m_CameraHandler, gvPowerUtilization, &value))
		return -1;

	return static_cast<int>(value * 100.0);
}

auto MoravianInstrumentsControl::IsConnected() const -> bool 
{
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CameraHandler) return false;

	gxetha::BOOLEAN cam_connected{};

	if (!gxetha::GetBooleanParameter(m_CameraHandler, gbpConnected, &cam_connected))
		return false;

	return (bool)cam_connected;
}

auto MoravianInstrumentsControl::GetFirmwareVersion() -> std::string
{
	std::lock_guard<std::recursive_mutex> lock(m_ApiMutex);

	if (!m_CameraHandler || !m_IsCameraOpen) return std::string();

	gxetha::CARDINAL major{}, minor{}, build{};
	gxetha::GetIntegerParameter(m_CameraHandler, gipFirmwareMajor, &major);
	gxetha::GetIntegerParameter(m_CameraHandler, gipFirmwareMinor, &minor);
	gxetha::GetIntegerParameter(m_CameraHandler, gipFirmwareBuild, &build);

	return std::to_string(build) + "." + std::to_string(minor) + "." + std::to_string(major);
}

auto MoravianInstrumentsControl::SetHardwareROI(int startX, int startY, int width, int height) -> void
{
	if (!m_ActualCameraParameters || !m_CapturingParameters) return;

	const int sensorW = (int)m_ActualCameraParameters->sensor_width;
	const int sensorH = (int)m_ActualCameraParameters->sensor_height;

	startX = std::max(0, std::min(startX, sensorW - 1));
	startY = std::max(0, std::min(startY, sensorH - 1));
	width = std::max(1, std::min(width, sensorW));
	height = std::max(1, std::min(height, sensorH));

	if (startX + width > sensorW) width = sensorW - startX;
	if (startY + height > sensorH) height = sensorH - startY;

	m_CapturingParameters->start_x = (gxetha::INTEGER)startX;
	m_CapturingParameters->start_y = (gxetha::INTEGER)startY;
	m_CapturingParameters->cameraImgWidth = (gxetha::INTEGER)width;
	m_CapturingParameters->cameraImgHeight = (gxetha::INTEGER)height;
}

auto MoravianInstrumentsControl::ResetHardwareROIToFullFrame() -> void
{
	if (!m_ActualCameraParameters || !m_CapturingParameters) return;

	m_CapturingParameters->start_x = 0;
	m_CapturingParameters->start_y = 0;
	m_CapturingParameters->cameraImgWidth = (gxetha::INTEGER)m_ActualCameraParameters->sensor_width;
	m_CapturingParameters->cameraImgHeight = (gxetha::INTEGER)m_ActualCameraParameters->sensor_height;
}

auto MoravianInstrumentsControl::GetHardwareROI() -> CameraControlVariables::HardwareROI
{
	CameraControlVariables::HardwareROI r;
	if (!m_ActualCameraParameters || !m_CapturingParameters)
	{
		r.width = 0; r.height = 0;
		return r;
	}

	r.startX = static_cast<int>(m_CapturingParameters->start_x);
	r.startY = static_cast<int>(m_CapturingParameters->start_y);
	r.width = static_cast<int>(m_CapturingParameters->cameraImgWidth > 0 ? m_CapturingParameters->cameraImgWidth : m_ActualCameraParameters->sensor_width);
	r.height = static_cast<int>(m_CapturingParameters->cameraImgHeight > 0 ? m_CapturingParameters->cameraImgHeight : m_ActualCameraParameters->sensor_height);
	return r;

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
) -> bool
{
	if (!m_IsCameraOpen || !camPtr || !captParam || !dataPtr || !continueWaiting)
		return false;

	gxetha::BOOLEAN image_ready{};

	auto startCheckingTime = std::chrono::high_resolution_clock::now();

	const double waitingTimePlusExposureTime =
		(static_cast<double>(waitingTime) + captParam->exposure_sec) * 1000.0;

	while (!static_cast<bool>(image_ready))
	{
		const auto readyResult = gxetha::ImageReady(camPtr, &image_ready);

		if (!static_cast<bool>(readyResult))
			return false;

		const auto currentTime = std::chrono::high_resolution_clock::now();

		const double deltaTime = static_cast<double>
			(
				std::chrono::duration_cast<std::chrono::milliseconds>
				(
					currentTime - startCheckingTime
				).count()
				);

		if (!*continueWaiting || deltaTime > waitingTimePlusExposureTime)
		{
			StopAcquisition();
			return false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	gxetha::BOOLEAN result{};

	if (continuousReading)
	{
		result = gxetha::ReadImageExposure
		(
			camPtr,
			captParam->cameraImgWidth * captParam->cameraImgHeight * sizeof(gxetha::INT16),
			dataPtr
		);
	}
	else
	{
		result = gxetha::ReadImage
		(
			camPtr,
			captParam->cameraImgWidth * captParam->cameraImgHeight * sizeof(gxetha::INT16),
			dataPtr
		);
	}

	if (!static_cast<bool>(result))
	{
		gxetha::CHAR err_string[100]{};
		gxetha::GetLastErrorString(camPtr, 100, err_string);
		return false;
	}

	return true;
}
