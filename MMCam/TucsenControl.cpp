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

    std::memset(&m_frame, 0, sizeof(m_frame));
    // Ask SDK for usual raw format (8/16-bit per device setting)
    m_frame.ucFormatGet = TUFRM_FMT_USUAl;     // doc: “8bit/16bit, mono/color” :contentReference[oaicite:4]{index=4}
    m_frame.uiRsdSize = 1;                   // reserve one frame (multi-frame used for trigger) :contentReference[oaicite:5]{index=5}

    if (!ok(TUCAM_Buf_Alloc(m_opCam.hIdxTUCam, &m_frame)))  // allocate before Cap_Start :contentReference[oaicite:6]{index=6}
        m_frame.pBuffer = nullptr;
}

auto TucsenControl::toMsFromUs(int us) -> double
{
    return static_cast<double>(us) / 1000.0;
}

// ---------- Lifecycle ----------
auto TucsenControl::Initialize() -> bool
{
    if (m_ApiInitialized) return true;

    m_itApi.uiCamCount = 0;
    m_itApi.pstrConfigPath = const_cast<char*>("./");

    if (!ok(TUCAM_Api_Init(&m_itApi))) return false;                       // required first :contentReference[oaicite:7]{index=7}
    m_ApiInitialized = true;
    if (m_itApi.uiCamCount == 0) return false;

    const auto idx = parseIndexOrDefault();
    if (idx >= m_itApi.uiCamCount) return false;

    m_opCam.uiIdxOpen = idx;
    if (!ok(TUCAM_Dev_Open(&m_opCam))) return false;                        // open camera :contentReference[oaicite:8]{index=8}
    m_CameraOpened = true;

    // Prepare empty frame; actual W/H known after first WaitForFrame / or via Dev_GetInfoEx
    std::memset(&m_frame, 0, sizeof(m_frame));
    return true;
}

auto TucsenControl::Close() -> bool
{
    if (m_Capturing) StopAcquisition();

    if (m_CameraOpened) {
        if (m_opCam.hIdxTUCam) TUCAM_Buf_Release(m_opCam.hIdxTUCam);       // release internal buffers :contentReference[oaicite:9]{index=9}
        TUCAM_Dev_Close(m_opCam.hIdxTUCam);                                 // close camera :contentReference[oaicite:10]{index=10}
        m_CameraOpened = false;
    }
    if (m_ApiInitialized) {
        TUCAM_Api_Uninit();                                                 // deinit API :contentReference[oaicite:11]{index=11}
        m_ApiInitialized = false;
    }

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
    if (!ok(TUCAM_Cap_Start(m_opCam.hIdxTUCam, static_cast<UINT32>(TUCCM_SEQUENCE))))
        return false;

    m_Capturing = true;
    return true;
}

auto TucsenControl::StopAcquisition() -> bool
{
    if (!m_CameraOpened) return false;

    if (m_Capturing) {
        TUCAM_Buf_AbortWait(m_opCam.hIdxTUCam);                             // break any waiters :contentReference[oaicite:13]{index=13}
        TUCAM_Cap_Stop(m_opCam.hIdxTUCam);                                  // stop capture :contentReference[oaicite:14]{index=14}
        m_Capturing = false;
    }
    return true;
}

// ---------- Frames ----------
auto TucsenControl::GetImage() -> unsigned short*
{
    if (!m_CameraOpened) return nullptr;

    const bool startedHere = !m_Capturing;
    if (startedHere && !StartAcquisition()) return nullptr;

    auto timeout_ms = 1000;

    // Wait with timeout; must be after Cap_Start, else NOT_READY (per doc) :contentReference[oaicite:15]{index=15}
    if (!ok(TUCAM_Buf_WaitForFrame(m_opCam.hIdxTUCam, &m_frame, timeout_ms))) {
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
    m_ExposureUS = exposure_us;

    // TUIDP_EXPOSURETM is in milliseconds (double). Example in manual shows Prop_Get/SetValue usage. :contentReference[oaicite:17]{index=17}
    const double ms = toMsFromUs(exposure_us);
    (void)TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_EXPOSURETM, ms);
}

auto TucsenControl::SetSensorTemperature(const double requiredTemperature) -> void
{
    // Manual states 0..100 maps to –50..+50 °C. Clamp and convert. :contentReference[oaicite:18]{index=18}
    double target = requiredTemperature;
    if (target < -50.0) target = -50.0;
    if (target > 50.0) target = 50.0;

    const int sdkVal = static_cast<int>(target + 50.0); // –50->0, +50->100
    (void)TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_TEMPERATURE, sdkVal);
}

auto TucsenControl::GetSensorTemperature() -> double
{
    double v = 50.0; // default midpoint
    (void)TUCAM_Prop_GetValue(m_opCam.hIdxTUCam, TUIDP_TEMPERATURE, &v);  // property available in table :contentReference[oaicite:19]{index=19}
    m_SensorTemperature = v - 50.0; // back to °C
    return m_SensorTemperature;
}

// ---------- Info ----------
auto TucsenControl::DeviceModelString() const -> std::string
{
    // Optional: Use TUCAM_Dev_GetInfoEx with TUIDI_CAMERA_MODEL (string). (Not critical for capture.) :contentReference[oaicite:20]{index=20}
    return {};
}
