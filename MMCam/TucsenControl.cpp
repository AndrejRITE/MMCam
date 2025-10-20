#include "TucsenControl.h"
#include <cstring>
#include <cstdlib>

using std::size_t;

static inline bool ok(TUCAMRET r) { return r == TUCAMRET_SUCCESS; }

// ---------- Ctor / Dtor ----------
TucsenControl::TucsenControl(std::string cameraSNOrIndex)
    : m_CameraSN(std::move(cameraSNOrIndex))
{
    m_ImageDataType = CameraControlVariables::ImageDataTypes::RAW_16BIT;
}

TucsenControl::~TucsenControl()
{
    Close();
}

// ---------- Private helpers ----------
auto TucsenControl::parseIndexOrDefault() const -> unsigned int
{
    try { return static_cast<unsigned int>(std::stoul(m_CameraSN)); }
    catch (...) { return 0U; }
}

void TucsenControl::ensureFrameBufferAllocated()
{
    if (!m_CameraOpened) return;
    if (m_frame.pBuffer) return;

	m_frame.pBuffer = NULL;
	m_frame.ucFormatGet = TUFRM_FMT_USUAl;
	m_frame.uiRsdSize = 1;

	TUCAM_Buf_Alloc(m_opCam.hIdxTUCam, &m_frame);

	m_Width = m_frame.usWidth;
	m_Height = m_frame.usHeight;
}

auto TucsenControl::toMsFromUs(int us) -> double
{
    return static_cast<double>(us) / 1000.0;
}

TUCAMRET TucsenControl::InitApi()
{
    /* Get the current directory */
    m_itApi.uiCamCount = 0;
    m_itApi.pstrConfigPath = const_cast<char*>("./");

    TUCAM_Api_Init(&m_itApi);

    if (0 == m_itApi.uiCamCount) return TUCAMRET_NO_CAMERA;

	m_ApiInitialized = true;

    return TUCAMRET_SUCCESS;
}

TUCAMRET TucsenControl::UnInitApi()
{
    if (m_opCam.hIdxTUCam) TUCAM_Buf_Release(m_opCam.hIdxTUCam);

	m_ApiInitialized = false;

    return TUCAM_Api_Uninit();
}

TUCAMRET TucsenControl::OpenCamera(UINT uiIdx)
{
    if (uiIdx >= m_itApi.uiCamCount) return TUCAMRET_OUT_OF_RANGE;

    m_opCam.uiIdxOpen = uiIdx;

    return TUCAM_Dev_Open(&m_opCam);
}

TUCAMRET TucsenControl::CloseCamera()
{
    if (NULL != m_opCam.hIdxTUCam) TUCAM_Dev_Close(m_opCam.hIdxTUCam);

	m_CameraOpened = false;

    return TUCAMRET_SUCCESS;
}

std::string TucsenControl::GetCameraName()
{
    TUCAM_VALUE_INFO m_viCam; // Value info object
    m_viCam.nID = (int)TUIDI_CAMERA_MODEL;

    if (TUCAMRET_SUCCESS == TUCAM_Dev_GetInfo(m_opCam.hIdxTUCam, &m_viCam)) return m_viCam.pText;

    return "";
}

int TucsenControl::GetVID()
{
    TUCAM_VALUE_INFO m_viCam; // Value info object
    m_viCam.nID = (int)TUIDI_VENDOR;

    if (TUCAMRET_SUCCESS == TUCAM_Dev_GetInfo(m_opCam.hIdxTUCam, &m_viCam)) return m_viCam.nValue;

    return 0;
}

int TucsenControl::GetPID()
{
    TUCAM_VALUE_INFO m_viCam; // Value info object
    m_viCam.nID = (int)TUIDI_PRODUCT;

    if (TUCAMRET_SUCCESS == TUCAM_Dev_GetInfo(m_opCam.hIdxTUCam, &m_viCam)) return m_viCam.nValue;

    return 0;
}

int TucsenControl::GetCameraChannels()
{
    TUCAM_VALUE_INFO m_viCam; // Value info object
    m_viCam.nID = (int)TUIDI_CAMERA_CHANNELS;

    if (TUCAMRET_SUCCESS == TUCAM_Dev_GetInfo(m_opCam.hIdxTUCam, &m_viCam)) return m_viCam.nValue;

    return 0;
}

// ---------- Lifecycle ----------
auto TucsenControl::Initialize() -> bool
{
    if (m_ApiInitialized) return true;

    if (TUCAMRET_SUCCESS != InitApi()) return false;

    if (TUCAMRET_SUCCESS != OpenCamera(0))
    {
        UnInitApi();
        return false;
    }

	m_CameraOpened = true;
    m_CameraSN = GetCameraName();

    // Get Sensor Parameters
    {
        m_frame.pBuffer = NULL;
        m_frame.ucFormatGet = TUFRM_FMT_USUAl;
        m_frame.uiRsdSize = 1;

        TUCAM_Buf_Alloc(m_opCam.hIdxTUCam, &m_frame);

        m_Width = m_frame.usWidth;
		m_Height = m_frame.usHeight;
    }

    return true;
}

auto TucsenControl::Close() -> bool
{
    if (m_Capturing) StopAcquisition();

    if (m_CameraOpened) {
		CloseCamera();
    }
	if (m_ApiInitialized) UnInitApi();

    m_ImageBuffer.reset();
    m_ImageBufferElems = 0;
    m_Width = m_Height = 0;
    return true;
}

// ---------- Capture ----------
auto TucsenControl::StartAcquisition() -> bool
{
    if (!m_CameraOpened) return false;
    ensureFrameBufferAllocated();
    if (!m_frame.pBuffer) return false;

    // Sequence (continuous) mode; continues until Cap_Stop (per SDK) :contentReference[oaicite:12]{index=12}
    auto ret = TUCAM_Cap_Start(m_opCam.hIdxTUCam, (UINT32)TUCCM_SEQUENCE);
    if (!ok(ret))
        return false;

    m_Capturing = true;
    return true;
}

auto TucsenControl::StopAcquisition() -> bool
{
    if (!m_CameraOpened) return false;

    if (m_Capturing) 
    {
        TUCAM_Buf_AbortWait(m_opCam.hIdxTUCam);
        TUCAM_Cap_Stop(m_opCam.hIdxTUCam);
        m_Capturing = false;
    }
    return true;
}

// ---------- Frames ----------
auto TucsenControl::GetImage() -> unsigned short*
{
    if (!m_CameraOpened) return nullptr;
	if (m_ExposureUS <= 0) return nullptr;

    const bool startedHere = !m_Capturing;
    if (startedHere && !StartAcquisition()) return nullptr;

    const int exp_ms = (m_ExposureUS > 0) ? (m_ExposureUS / 1000) : 0;

	auto maxTimeout = exp_ms * 2 < 2000 ? 2000 : exp_ms * 2;
    const int timeout_ms = std::clamp(int(exp_ms * 1.5), 200, maxTimeout);

    // Wait with timeout; must be after Cap_Start, else NOT_READY (per doc) :contentReference[oaicite:15]{index=15}
    if (!ok(TUCAM_Buf_WaitForFrame(m_opCam.hIdxTUCam, &m_frame, timeout_ms))) 
    {
        if (startedHere) StopAcquisition();
        return nullptr;
    }

    // Buffer layout: header (usHeader bytes) + image (uiImgSize) + reserved (uiHstSize) — use offset to image. :contentReference[oaicite:16]{index=16}
    const auto* raw = static_cast<const std::uint8_t*>(m_frame.pBuffer);
    const auto* img = raw ? (raw + m_frame.usHeader) : nullptr;

    // Dimensions (these are returned in TUCAM_FRAME upon wait)
    m_Width = m_frame.usWidth;
    m_Height = m_frame.usHeight;

    const size_t elems = static_cast<size_t>(m_Width) * static_cast<size_t>(m_Height);
    if (elems != m_ImageBufferElems) {
        m_ImageBuffer = std::make_unique<unsigned short[]>(elems);
        m_ImageBufferElems = elems;
    }

    // SDK delivers 8/16; we requested “USUAl”. For RAW16, ucElemBytes==2.
    if (img && m_ImageBuffer) {
        const size_t bytes = elems * sizeof(unsigned short);
        std::memcpy(m_ImageBuffer.get(), img, bytes);
    }

    if (startedHere) StopAcquisition();
    return m_ImageBuffer.get();
}

// ---------- Properties ----------
auto TucsenControl::SetExposureTime(int exposure_us) -> void
{
    int nVal;
    if (ok(TUCAM_Capa_GetValue(m_opCam.hIdxTUCam, TUIDC_ATEXPOSURE, &nVal)))
    {
        if (nVal != 0)
        {
            // Close auto exposure
            TUCAM_Capa_SetValue(m_opCam.hIdxTUCam, TUIDC_ATEXPOSURE, 0);
        }
    }

	auto desired_exposure_ms = toMsFromUs(exposure_us);

    TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_EXPOSURETM, desired_exposure_ms);

    m_ExposureUS = exposure_us;
}

auto TucsenControl::SetSensorTemperature(const double requiredTemperature) -> void
{
    // Manual states 0..100 maps to –50..+50 °C. Clamp and convert. :contentReference[oaicite:18]{index=18}
    double target = requiredTemperature;
    if (target < -50.0) target = -50.0;
    if (target > 50.0) target = 50.0;

    const int sdkVal = static_cast<int>(target + 50.0); // –50->0, +50->100
    //const int sdkVal = static_cast<int>(target); // –50->0, +50->100
    auto ret = TUCAM_Capa_SetValue(m_opCam.hIdxTUCam, TUIDC_ENABLETEC, 1);
    ret = TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_TEMPERATURE_TARGET, sdkVal);
}

auto TucsenControl::GetSensorTemperature() -> double
{
    double v = 1.0;
    (void)TUCAM_Prop_GetValue(m_opCam.hIdxTUCam, TUIDP_TEMPERATURE, &v);
    m_SensorTemperature = v;
    return m_SensorTemperature;
}
