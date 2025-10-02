#include "TucsenControl.h"
// ---- Private helpers -------------------------------------------------


auto TucsenControl::parseIndexOrDefault() const -> unsigned int
{
	try {
		return static_cast<unsigned int>(std::stoul(m_CameraSN));
	}
	catch (...) {
		return 0U; // fallback
	}
}


void TucsenControl::ensureFrameBufferAllocated()
{
	if (!m_CameraOpened) return;

	// If a buffer is already allocated via TUCAM_Buf_Alloc, nothing to do here.
	if (m_frame.pBuffer != nullptr) return;

	// Set desired output format. TUFRM_FMT_USUAl is the typical raw format.
	m_frame.pBuffer = nullptr;  // let SDK allocate
	m_frame.ucFormatGet = TUFRM_FMT_USUAl;
	m_frame.uiRsdSize = 1;       // reserve one frame

	if (TUCAMRET_SUCCESS != TUCAM_Buf_Alloc(m_opCam.hIdxTUCam, &m_frame))
	{
		// Allocation failure — leave pBuffer null; caller will handle gracefully
		m_frame.pBuffer = nullptr;
	}
}

auto TucsenControl::Initialize() -> bool
{
	if (m_ApiInitialized) return true;

	// Point SDK to current working directory for config; update if you use a custom path
	m_itApi.uiCamCount = 0;
	m_itApi.pstrConfigPath = const_cast<char*>("./");

	if (TUCAMRET_SUCCESS != TUCAM_Api_Init(&m_itApi))
		return false;

	m_ApiInitialized = true;

	if (m_itApi.uiCamCount == 0)
		return false;

	const auto idx = parseIndexOrDefault();
	if (idx >= m_itApi.uiCamCount)
		return false;

	m_opCam.uiIdxOpen = idx;
	if (TUCAMRET_SUCCESS != TUCAM_Dev_Open(&m_opCam))
		return false;

	m_CameraOpened = true;

	// We don't have a direct serial here from the sample headers; set a readable label.
	m_CameraSerialNumber = std::string("TucsonCam#") + std::to_string(idx);

	// Prepare frame descriptor; actual width/height will be known after first frame.
	std::memset(&m_frame, 0, sizeof(m_frame));

	return true;
}

auto TucsenControl::Close() -> bool
{
	if (m_Capturing)
		StopAcquisition();

	if (m_CameraOpened)
	{
		// Release frame buffer first (safe to call even if not allocated)
		if (m_opCam.hIdxTUCam)
			TUCAM_Buf_Release(m_opCam.hIdxTUCam);

		TUCAM_Dev_Close(m_opCam.hIdxTUCam);
		m_CameraOpened = false;
	}

	if (m_ApiInitialized)
	{
		TUCAM_Api_Uninit();
		m_ApiInitialized = false;
	}

	m_ImageBuffer.reset();
	m_ImageBufferElems = 0;
	m_Width = m_Height = 0;

	return true;
}

auto TucsenControl::StartAcquisition() -> bool
{
	if (!m_CameraOpened) return false;

	ensureFrameBufferAllocated();
	if (!m_frame.pBuffer) return false;

	// Start in continuous sequence mode; you can switch to trigger if needed
	if (TUCAMRET_SUCCESS != TUCAM_Cap_Start(m_opCam.hIdxTUCam, static_cast<UINT32>(TUCCM_SEQUENCE)))
		return false;

	m_Capturing = true;
	return true;
}

auto TucsenControl::StopAcquisition() -> bool
{
	if (!m_CameraOpened) return false;

	if (m_Capturing)
	{
		// Abort any waiters and stop streaming
		TUCAM_Buf_AbortWait(m_opCam.hIdxTUCam);
		TUCAM_Cap_Stop(m_opCam.hIdxTUCam);
		m_Capturing = false;
	}

	// Keep buffer allocated between runs for performance; 
	// call Close() to release fully.
	return true;
}

auto TucsenControl::GetImage() -> unsigned short*
{
	if (!m_CameraOpened) return nullptr;

	// If not already capturing, start a short run just for this frame
	const bool startedHere = !m_Capturing;
	if (startedHere && !StartAcquisition())
		return nullptr;

	// Wait for a frame into m_frame
	if (TUCAMRET_SUCCESS != TUCAM_Buf_WaitForFrame(m_opCam.hIdxTUCam, &m_frame))
	{
		if (startedHere) StopAcquisition();
		return nullptr;
	}

	// Update dimensions from the actual frame
	m_Width = m_frame.usWidth;
	m_Height = m_frame.usHeight;

	// Ensure we have a local copy buffer of the right size
	const size_t elems = static_cast<size_t>(m_Width) * static_cast<size_t>(m_Height);
	if (elems != m_ImageBufferElems)
	{
		m_ImageBuffer = std::make_unique<unsigned short[]>(elems);
		m_ImageBufferElems = elems;
	}

	// Copy pixels (SDK provides a pointer in m_frame.pBuffer)
	if (m_frame.pBuffer && m_ImageBuffer)
	{
		// m_frame.ucElemBytes == 2 for 16-bit, per sample; copy as-is
		std::memcpy(m_ImageBuffer.get(), m_frame.pBuffer, elems * sizeof(unsigned short));
	}

	// If we started the capture here (single-shot), stop again
	if (startedHere)
		StopAcquisition();

	return m_ImageBuffer.get();
}


auto TucsenControl::SetExposureTime(int exposure_us) -> void
{
	m_ExposureUS = exposure_us;


	// TODO: bind to Tucson SDK exposure property when available in headers.
	// Typical approach with TUCAM is something like:
	// TUCAM_VALUE val{}; val.nID = TUIDC_INT_TIME; val.dValue = exposure_us / 1000.0; // ms
	// TUCAM_Cap_SetValue(m_opCam.hIdxTUCam, &val);
	// but the exact API names depend on your SDK version.
}


auto TucsenControl::SetSensorTemperature(const double /*requiredTemperature*/) -> void
{
	// TODO: Implement when the Tucson SDK property for sensor setpoint is available.
	// Some cameras do not expose TEC control through TUCAM; in that case this is a no-op.
}


auto TucsenControl::GetSensorTemperature() -> double
{
	// TODO: Query the current sensor temperature via Tucson API if exposed.
	// Returning cached value (if you add one) or 0.0 as a safe default for now.
	m_SensorTemperature = 0.0;
	return m_SensorTemperature;
}
