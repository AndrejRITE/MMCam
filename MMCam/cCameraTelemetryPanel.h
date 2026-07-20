#pragma once

#include "wx/wx.h"
#include "wx/dcbuffer.h"

#include <chrono>
#include <deque>
#include <limits>

namespace Telemetry
{
	struct Sample
	{
		std::chrono::steady_clock::time_point timestamp{};
		double temperatureDegC
		{
			std::numeric_limits<double>::quiet_NaN()
		};
		double powerUtilizationPct
		{
			std::numeric_limits<double>::quiet_NaN()
		};
	};
}

class cCameraTelemetryPanel final : public wxPanel
{
public:
	explicit cCameraTelemetryPanel(wxWindow* parent);

	void AddTelemetrySample
	(
		double temperatureDegC,
		int powerUtilizationPct
	);

	void ClearTelemetry();
	void SetBackgroundColor(wxColour color);

private:
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);

	void RemoveExpiredSamples
	(
		const std::chrono::steady_clock::time_point& now
	);

	void DrawRoundedLabel
	(
		wxDC& dc,
		const wxRect& rect,
		const wxString& text,
		const wxColour& background,
		const wxColour& foreground
	) const;

	const Telemetry::Sample* FindClosestSample
	(
		const std::chrono::steady_clock::time_point& targetTime,
		const std::chrono::steady_clock::time_point& historyStart
	) const;

	static double NiceAxisStep
	(
		double range,
		int desiredIntervals
	);

private:
	std::deque<Telemetry::Sample> m_Samples;

	std::chrono::steady_clock::time_point m_LastAcceptedSample{};
	std::chrono::steady_clock::time_point m_LastTelemetryTime{};

	wxPoint m_MousePosition{ -1, -1 };
	wxRect m_LastPlotRect{};

	bool m_DarkAppearance{ false };
	wxColour m_BackgroundColor{ 0, 0, 0 };
	bool m_MouseInsidePlot{ false };

	/*
		Distinguishes these two states:

		1. No telemetry update has arrived yet.
		2. Telemetry is arriving, but this camera exposes neither temperature
		   nor power utilization.
	*/
	bool m_HasReceivedTelemetry{ false };

	static constexpr auto HistoryDuration =
		std::chrono::minutes(5);

	static constexpr auto MinimumSampleInterval =
		std::chrono::milliseconds(250);
};