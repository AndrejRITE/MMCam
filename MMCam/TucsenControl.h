#pragma once
#ifndef TUCSEN_CONTROL_H
#define TUCSEN_CONTROL_H

#include "CameraControl.h"

#include "Tucsen/TUCamApi.h"

#include <memory>
#include <string>
#include <vector>
#include <cstring>


/* TucsenControl — a thin adapter that implements your CameraControl interface
using the Tucson (TUCAM) C API.

Design notes:
- Mirrors the style of MoravianInstrumentsControl (Initialize/Close/Start/Stop/GetImage...)
- Keeps ownership of a single TUCAM_FRAME buffer and exposes it as unsigned short*
- Interprets the provided camera "serial" as an index ("0", "1", ...). If parsing fails,
falls back to index 0.
- Exposure setters are plumbed but you may want to hook them to the SDK property once available
in your headers (see TODOs in .cpp).
*/

class TucsenControl final : public CameraControl
{
public:
	explicit TucsenControl(std::string cameraSNOrIndex)
		: m_CameraSN(std::move(cameraSNOrIndex))
	{
		m_ImageDataType = CameraControlVariables::ImageDataTypes::RAW_16BIT;
	}


	~TucsenControl() override { Close(); }


	// CameraControl API
	auto Initialize() -> bool override;
	auto Close() -> bool override;


	auto StartAcquisition() -> bool override;
	auto StopAcquisition() -> bool override;


	auto GetImage() -> unsigned short* override;
	auto SetExposureTime(int exposure_us) -> void override;


	auto GetWidth() const -> unsigned long override { return m_Width; }
	auto GetHeight() const -> unsigned long override { return m_Height; }


	auto IsConnected() const -> bool override { return m_CameraOpened; }


	auto SetSensorTemperature(const double /*requiredTemperature*/) -> void override;
	auto GetSensorTemperature() -> double override;


private:
	// Helpers
	auto parseIndexOrDefault() const -> unsigned int;
	void ensureFrameBufferAllocated();


private:
	// Configuration / state
	std::string m_CameraSN{}; // user-provided string; interpreted as index

	// Tucson SDK objects
	TUCAM_INIT m_itApi{}; // API initialization object
	TUCAM_OPEN m_opCam{}; // open-camera object
	TUCAM_FRAME m_frame{}; // frame descriptor


	// Bookkeeping
	bool m_ApiInitialized{ false };
	bool m_CameraOpened{ false };
	bool m_Capturing{ false };


	// Derived and cached info
	unsigned long m_Width{ 0 };
	unsigned long m_Height{ 0 };
	int m_ExposureUS{ 0 }; // cached user value (see TODO in .cpp)


	// Local buffer for 16-bit image data (owning)
	std::unique_ptr<unsigned short[]> m_ImageBuffer{};
	size_t m_ImageBufferElems{ 0 };
};
#endif // TUCSEN_CONTROL_H
