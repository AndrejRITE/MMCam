#pragma once
#ifndef TUCSEN_CONTROL_H
#define TUCSEN_CONTROL_H

#include "CameraControl.h"
#include "Tucsen/TUCamApi.h"

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

class TucsenControl final : public CameraControl
{
public:
    explicit TucsenControl(std::string cameraSNOrIndex);
    ~TucsenControl() override;

    // CameraControl API
    auto Initialize() -> bool override;
    auto Close() -> bool override;

    auto StartAcquisition() -> bool override;
    auto StopAcquisition() -> bool override;

    // Blocks up to timeout_ms waiting for a frame. Returns pointer to 16-bit pixels or nullptr.
    auto GetImage() -> unsigned short* override;

    auto SetExposureTime(int exposure_us) -> void override;

    auto GetWidth()  const -> unsigned long override { return m_Width; }
    auto GetHeight() const -> unsigned long override { return m_Height; }
    auto IsConnected() const -> bool override { return m_CameraOpened; }

    // Cooling / temperature (see note in .cpp about value mapping)
    auto SetSensorTemperature(const double requiredTemperature) -> void override;
    auto GetSensorTemperature() -> double override;

private:
    // Helpers
    auto parseIndexOrDefault() const -> unsigned int;
    void ensureFrameBufferAllocated();
    static auto toMsFromUs(int us) -> double;

private:
    TUCAMRET InitApi(); /* Init the TUCAM API */
    TUCAMRET UnInitApi(); /* UnInit the TUCAM API */
    TUCAMRET OpenCamera(UINT uiIdx); /* Open the camera by index number */
    TUCAMRET CloseCamera(); /* Close the current camera */

    std::string GetCameraName();
    int GetVID();
    int GetPID();
    int GetCameraChannels();

private:

    // Configuration / state
    std::string  m_CameraSN{};
    TUCAM_INIT   m_itApi{};
    TUCAM_OPEN   m_opCam{};
    TUCAM_FRAME  m_frame{};
    TUCAM_VALUE_INFO  m_info{};

    bool m_ApiInitialized{ false };
    bool m_CameraOpened{ false };
    bool m_Capturing{ false };

    unsigned long m_Width{ 0 };
    unsigned long m_Height{ 0 };
    int           m_ExposureUS{ 0 };

    // Local 16-bit image buffer (owning)
    std::unique_ptr<unsigned short[]> m_ImageBuffer{};
    size_t        m_ImageBufferElems{ 0 };

    // Last read sensor temperature (°C)
    double        m_SensorTemperature{ 0.0 };
};

#endif // TUCSEN_CONTROL_H
