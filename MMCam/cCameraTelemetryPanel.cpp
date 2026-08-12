#include "cCameraTelemetryPanel.h"

cCameraTelemetryPanel::cCameraTelemetryPanel(wxWindow* parent) 
	: wxPanel
		(
			parent,
			wxID_ANY,
			wxDefaultPosition,
			wxDefaultSize,
			wxBORDER_NONE
		)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(wxSize(-1, FromDIP(200)));

	Bind(wxEVT_PAINT, &cCameraTelemetryPanel::OnPaint, this);
	Bind(wxEVT_SIZE, &cCameraTelemetryPanel::OnSize, this);
	Bind(wxEVT_MOTION, &cCameraTelemetryPanel::OnMouseMove, this);
	Bind(wxEVT_LEAVE_WINDOW, &cCameraTelemetryPanel::OnMouseLeave, this);
}

void cCameraTelemetryPanel::AddTelemetrySample(double temperatureDegC, int powerUtilizationPct)
{
	const auto now = std::chrono::steady_clock::now();

	m_HasReceivedTelemetry = true;
	m_LastTelemetryTime = now;

	const bool temperatureAvailable =
		std::isfinite(temperatureDegC);

	const bool powerAvailable =
		powerUtilizationPct >= 0 &&
		powerUtilizationPct <= 100;

	const double storedTemperature = temperatureAvailable
		? temperatureDegC
		: std::numeric_limits<double>::quiet_NaN();

	const double storedPower = powerAvailable
		? static_cast<double>(powerUtilizationPct)
		: std::numeric_limits<double>::quiet_NaN();

	/*
		Keep the most recent telemetry state even when the camera provides
		neither supported metric. We do not add an empty point because there is
		nothing useful to plot, but the panel can now distinguish unsupported
		telemetry from telemetry that has not arrived yet.
	*/
	if (!temperatureAvailable && !powerAvailable)
	{
		RemoveExpiredSamples(now);
		Refresh(false);
		return;
	}

	if
		(
			!m_Samples.empty() &&
			now - m_LastAcceptedSample < MinimumSampleInterval
			)
	{
		auto& latest = m_Samples.back();

		latest.timestamp = now;
		latest.temperatureDegC = storedTemperature;
		latest.powerUtilizationPct = storedPower;

		RemoveExpiredSamples(now);
		Refresh(false);
		return;
	}

	Telemetry::Sample sample;
	sample.timestamp = now;
	sample.temperatureDegC = storedTemperature;
	sample.powerUtilizationPct = storedPower;

	m_Samples.push_back(sample);
	m_LastAcceptedSample = now;

	RemoveExpiredSamples(now);
	Refresh(false);
}

void cCameraTelemetryPanel::ClearTelemetry()
{
	m_Samples.clear();

	m_LastAcceptedSample = {};
	m_LastTelemetryTime = {};

	m_MousePosition = wxPoint(-1, -1);
	m_LastPlotRect = wxRect();

	m_MouseInsidePlot = false;
	m_HasReceivedTelemetry = false;

	Refresh(false);
}

void cCameraTelemetryPanel::SetBackgroundColor(wxColour color)
{
	if (m_BackgroundColor == color)
		return;

	m_BackgroundColor = color;

	// Determine whether the background color is dark or light.
	m_DarkAppearance = (m_BackgroundColor.Red() * 0.299 + m_BackgroundColor.Green() * 0.587 + m_BackgroundColor.Blue() * 0.114) < 128;

	Refresh(false);
}

void cCameraTelemetryPanel::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	const wxSize clientSize = GetClientSize();

	if (clientSize.GetWidth() <= 0 || clientSize.GetHeight() <= 0)
		return;

	const wxColour panelBackground = m_BackgroundColor;

	const wxColour chartBackground = m_BackgroundColor;

	const wxColour borderColour = m_DarkAppearance
		? wxColour(78, 84, 92)
		: wxColour(211, 216, 224);

	const wxColour gridColour = m_DarkAppearance
		? wxColour(95, 100, 110)
		: wxColour(195, 200, 208);

	const wxColour primaryText = m_DarkAppearance
		? wxColour(235, 238, 242)
		: wxColour(40, 44, 52);

	const wxColour secondaryText = m_DarkAppearance
		? wxColour(166, 173, 184)
		: wxColour(100, 108, 120);

	const wxColour temperatureColour(255, 112, 67);
	const wxColour powerColour(61, 155, 255);

	const wxColour temperatureBadgeBackground = m_DarkAppearance
		? wxColour(91, 51, 40)
		: wxColour(255, 232, 224);

	const wxColour powerBadgeBackground = m_DarkAppearance
		? wxColour(35, 67, 99)
		: wxColour(225, 240, 255);

	dc.SetBackground(wxBrush(panelBackground));
	dc.Clear();

	const int outerMargin = FromDIP(6);
	const int headerHeight = FromDIP(34);

	const wxRect outerRect
	(
		outerMargin,
		outerMargin,
		clientSize.GetWidth() - 2 * outerMargin,
		clientSize.GetHeight() - 2 * outerMargin
	);

	if (outerRect.GetWidth() <= 0 || outerRect.GetHeight() <= 0)
		return;

	auto gridPenWidth = 3;

	// Outer card.
	auto outerCardPenWidth = 3;
	dc.SetPen(wxPen(borderColour, outerCardPenWidth));
	dc.SetBrush(wxBrush(chartBackground));
	dc.DrawRoundedRectangle(outerRect, FromDIP(10));

	wxFont titleFont = GetFont();
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	titleFont.SetPointSize(std::max(8, titleFont.GetPointSize() + 1));

	dc.SetFont(titleFont);
	dc.SetTextForeground(primaryText);
	dc.DrawText
	(
		wxT("Camera telemetry"),
		outerRect.GetLeft() + FromDIP(12),
		outerRect.GetTop() + FromDIP(8)
	);

	wxFont normalFont = GetFont();
	normalFont.SetPointSize(std::max(7, normalFont.GetPointSize()));

	wxFont smallFont = normalFont;
	smallFont.SetPointSize(std::max(7, normalFont.GetPointSize() - 1));

	dc.SetFont(normalFont);

	bool latestTemperatureAvailable = false;
	bool latestPowerAvailable = false;

	double latestTemperature = 0.0;
	double latestPower = 0.0;

	for (auto it = m_Samples.rbegin(); it != m_Samples.rend(); ++it)
	{
		if
			(
				!latestTemperatureAvailable &&
				std::isfinite(it->temperatureDegC)
				)
		{
			latestTemperature = it->temperatureDegC;
			latestTemperatureAvailable = true;
		}

		if
			(
				!latestPowerAvailable &&
				std::isfinite(it->powerUtilizationPct)
				)
		{
			latestPower = it->powerUtilizationPct;
			latestPowerAvailable = true;
		}

		if (latestTemperatureAvailable && latestPowerAvailable)
			break;
	}

	const wxString temperatureText = latestTemperatureAvailable
		? wxString::Format(wxT("%.2f °C"), latestTemperature)
		: wxString(wxT("Temperature N/A"));

	const wxString powerText = latestPowerAvailable
		? wxString::Format(wxT("%.0f %%"), latestPower)
		: wxString(wxT("Power N/A"));

	wxCoord temperatureTextWidth{};
	wxCoord temperatureTextHeight{};
	wxCoord powerTextWidth{};
	wxCoord powerTextHeight{};

	dc.GetTextExtent
	(
		temperatureText,
		&temperatureTextWidth,
		&temperatureTextHeight
	);

	dc.GetTextExtent
	(
		powerText,
		&powerTextWidth,
		&powerTextHeight
	);

	const int badgeHeight = FromDIP(24);

	const wxRect powerBadge
	(
		outerRect.GetRight() - powerTextWidth - FromDIP(18),
		outerRect.GetTop() + FromDIP(6),
		powerTextWidth + FromDIP(12),
		badgeHeight
	);

	const wxRect temperatureBadge
	(
		powerBadge.GetLeft() - temperatureTextWidth - FromDIP(24),
		outerRect.GetTop() + FromDIP(6),
		temperatureTextWidth + FromDIP(12),
		badgeHeight
	);

	DrawRoundedLabel
	(
		dc,
		temperatureBadge,
		temperatureText,
		temperatureBadgeBackground,
		temperatureColour
	);

	DrawRoundedLabel
	(
		dc,
		powerBadge,
		powerText,
		powerBadgeBackground,
		powerColour
	);

	/*
		Each side has:
		- a rotated axis caption;
		- numeric tick labels;
		- spacing before the plot border.
	*/
	const int leftAxisWidth = FromDIP(56);
	const int rightAxisWidth = FromDIP(48);
	const int bottomAxisHeight = FromDIP(25);

	const wxRect plotRect
	(
		outerRect.GetLeft() + leftAxisWidth,
		outerRect.GetTop() + headerHeight,
		outerRect.GetWidth() - leftAxisWidth - rightAxisWidth,
		outerRect.GetHeight() - headerHeight - bottomAxisHeight
	);

	if (plotRect.GetWidth() < FromDIP(60) ||
		plotRect.GetHeight() < FromDIP(50))
	{
		m_LastPlotRect = wxRect();
		return;
	}

	m_LastPlotRect = plotRect;

	dc.SetPen(wxPen(borderColour));
	dc.SetBrush(wxBrush(chartBackground));
	dc.DrawRectangle(plotRect);

	const auto now = std::chrono::steady_clock::now();
	const auto historyStart = now - HistoryDuration;

	/*
		Calculate a stable, slightly padded temperature range.

		Power always uses 0–100 %, so only the left axis needs dynamic scaling.
	*/
	bool haveTemperature = false;
	double minTemperature = 0.0;
	double maxTemperature = 0.0;

	for (const auto& sample : m_Samples)
	{
		if (!std::isfinite(sample.temperatureDegC))
			continue;

		if (!haveTemperature)
		{
			minTemperature = sample.temperatureDegC;
			maxTemperature = sample.temperatureDegC;
			haveTemperature = true;
		}
		else
		{
			minTemperature =
				std::min(minTemperature, sample.temperatureDegC);

			maxTemperature =
				std::max(maxTemperature, sample.temperatureDegC);
		}
	}

	if (!haveTemperature)
	{
		minTemperature = 0.0;
		maxTemperature = 40.0;
	}
	else
	{
		const double measuredRange =
			std::max(1.0, maxTemperature - minTemperature);

		const double padding =
			std::max(1.0, measuredRange * 0.15);

		minTemperature -= padding;
		maxTemperature += padding;

		const double step =
			NiceAxisStep(maxTemperature - minTemperature, 4);

		minTemperature =
			std::floor(minTemperature / step) * step;

		maxTemperature =
			std::ceil(maxTemperature / step) * step;

		if (maxTemperature <= minTemperature)
			maxTemperature = minTemperature + 1.0;
	}

	const auto timeToX =
		[&](const std::chrono::steady_clock::time_point& timestamp)
		{
			const double elapsedSeconds =
				std::chrono::duration<double>
				(
					timestamp - historyStart
				).count();

			const double normalized =
				std::clamp(elapsedSeconds / 300.0, 0.0, 1.0);

			return plotRect.GetLeft() +
				static_cast<int>
				(
					normalized *
					static_cast<double>(plotRect.GetWidth() - 1)
					);
		};

	const auto temperatureToY =
		[&](const double value)
		{
			const double normalized =
				std::clamp
				(
					(value - minTemperature) /
					(maxTemperature - minTemperature),
					0.0,
					1.0
				);

			return plotRect.GetBottom() -
				static_cast<int>
				(
					normalized *
					static_cast<double>(plotRect.GetHeight() - 1)
					);
		};

	const auto powerToY =
		[&](const double value)
		{
			const double normalized =
				std::clamp(value / 100.0, 0.0, 1.0);

			return plotRect.GetBottom() -
				static_cast<int>
				(
					normalized *
					static_cast<double>(plotRect.GetHeight() - 1)
					);
		};

	// Horizontal grid and Y-axis labels.
	dc.SetFont(smallFont);

	constexpr int horizontalGridCount = 4;

	for (int i = 0; i <= horizontalGridCount; ++i)
	{
		const double fraction =
			static_cast<double>(i) /
			static_cast<double>(horizontalGridCount);

		const int y =
			plotRect.GetBottom() -
			static_cast<int>
			(
				fraction *
				static_cast<double>(plotRect.GetHeight() - 1)
				);

		dc.SetPen(wxPen(gridColour, gridPenWidth, wxPENSTYLE_DOT));
		dc.DrawLine
		(
			plotRect.GetLeft(),
			y,
			plotRect.GetRight(),
			y
		);

		const double temperatureValue =
			minTemperature +
			fraction * (maxTemperature - minTemperature);

		const double powerValue = fraction * 100.0;

		const wxString leftLabel =
			wxString::Format(wxT("%.1f"), temperatureValue);

		const wxString rightLabel =
			wxString::Format(wxT("%.0f"), powerValue);

		dc.SetTextForeground(secondaryText);

		wxCoord leftWidth{};
		wxCoord leftHeight{};

		dc.GetTextExtent(leftLabel, &leftWidth, &leftHeight);

		dc.DrawText
		(
			leftLabel,
			plotRect.GetLeft() - leftWidth - FromDIP(6),
			y - leftHeight / 2
		);

		wxCoord rightWidth{};
		wxCoord rightHeight{};

		dc.GetTextExtent(rightLabel, &rightWidth, &rightHeight);

		dc.DrawText
		(
			rightLabel,
			plotRect.GetRight() + FromDIP(5),
			y - rightHeight / 2
		);
	}

	// Time grid: -5, -4, -3, -2, -1, now.
	for (int minute = 0; minute <= 5; ++minute)
	{
		const double fraction =
			static_cast<double>(minute) / 5.0;

		const int x =
			plotRect.GetLeft() +
			static_cast<int>
			(
				fraction *
				static_cast<double>(plotRect.GetWidth() - 1)
				);

		dc.SetPen(wxPen(gridColour, gridPenWidth, wxPENSTYLE_DOT));
		dc.DrawLine
		(
			x,
			plotRect.GetTop(),
			x,
			plotRect.GetBottom()
		);

		const int minutesBack = 5 - minute;

		const wxString label = minutesBack == 0
			? wxString(wxT("now"))
			: wxString::Format(wxT("-%d m"), minutesBack);

		wxCoord labelWidth{};
		wxCoord labelHeight{};

		dc.GetTextExtent(label, &labelWidth, &labelHeight);
		dc.SetTextForeground(secondaryText);

		int labelX = x - labelWidth / 2;

		labelX = std::clamp
		(
			labelX,
			plotRect.GetLeft(),
			plotRect.GetRight() - labelWidth
		);

		dc.DrawText
		(
			label,
			labelX,
			plotRect.GetBottom() + FromDIP(4)
		);
	}

	/*
		Draw separate line segments so unavailable power data produces a gap
		instead of being silently interpreted as zero.
	*/
	auto drawSeries =
		[&]
		(
			const wxColour& colour,
			const auto valueExtractor,
			const auto valueToY
			)
		{
			dc.SetPen
			(
				wxPen
				(
					colour,
					FromDIP(2),
					wxPENSTYLE_SOLID
				)
			);

			bool havePreviousPoint = false;
			wxPoint previousPoint{};

			for (const auto& sample : m_Samples)
			{
				if (sample.timestamp < historyStart)
					continue;

				const double value = valueExtractor(sample);

				if (!std::isfinite(value))
				{
					havePreviousPoint = false;
					continue;
				}

				const wxPoint currentPoint
				(
					timeToX(sample.timestamp),
					valueToY(value)
				);

				if (havePreviousPoint)
					dc.DrawLine(previousPoint, currentPoint);

				previousPoint = currentPoint;
				havePreviousPoint = true;
			}
		};

	drawSeries
	(
		temperatureColour,
		[](const Telemetry::Sample& sample)
		{
			return sample.temperatureDegC;
		},
		temperatureToY
	);

	drawSeries
	(
		powerColour,
		[](const Telemetry::Sample& sample)
		{
			return sample.powerUtilizationPct;
		},
		powerToY
	);

	// Current-point markers.
	if (!m_Samples.empty())
	{
		const auto& newest = m_Samples.back();
		const int x = timeToX(newest.timestamp);

		if (std::isfinite(newest.temperatureDegC))
		{
			const int y = temperatureToY(newest.temperatureDegC);

			dc.SetPen(wxPen(chartBackground, FromDIP(1)));
			dc.SetBrush(wxBrush(temperatureColour));
			dc.DrawCircle(wxPoint(x, y), FromDIP(3));
		}

		if (std::isfinite(newest.powerUtilizationPct))
		{
			const int y = powerToY(newest.powerUtilizationPct);

			dc.SetPen(wxPen(chartBackground, FromDIP(1)));
			dc.SetBrush(wxBrush(powerColour));
			dc.DrawCircle(wxPoint(x, y), FromDIP(3));
		}
	}

	/*
	Interactive inspection cursor.

	The cursor snaps to the nearest real sample rather than interpolating.
	Interpolation could imply a value that the camera never reported.
*/
	if
		(
			m_MouseInsidePlot &&
			!m_Samples.empty() &&
			plotRect.Contains(m_MousePosition)
			)
	{
		const double normalizedMouseX =
			std::clamp
			(
				static_cast<double>
				(
					m_MousePosition.x - plotRect.GetLeft()
					) /
				static_cast<double>
				(
					std::max(1, plotRect.GetWidth() - 1)
					),
				0.0,
				1.0
			);

		const auto targetTime =
			historyStart +
			std::chrono::duration_cast
			<
			std::chrono::steady_clock::duration
			>
			(
				std::chrono::duration<double>
				(
					normalizedMouseX * 300.0
				)
			);

		const Telemetry::Sample* hoveredSample =
			FindClosestSample(targetTime, historyStart);

		if (hoveredSample)
		{
			const int inspectionX =
				timeToX(hoveredSample->timestamp);

			const wxColour inspectionLineColour =
				m_DarkAppearance
				? wxColour(195, 199, 207)
				: wxColour(90, 96, 108);

			dc.SetPen
			(
				wxPen
				(
					inspectionLineColour,
					FromDIP(1),
					wxPENSTYLE_SHORT_DASH
				)
			);

			dc.DrawLine
			(
				inspectionX,
				plotRect.GetTop(),
				inspectionX,
				plotRect.GetBottom()
			);

			const bool hoveredTemperatureAvailable =
				std::isfinite
				(
					hoveredSample->temperatureDegC
				);

			const bool hoveredPowerAvailable =
				std::isfinite
				(
					hoveredSample->powerUtilizationPct
				);

			// Draw emphasized points at the selected sample.
			if (hoveredTemperatureAvailable)
			{
				const int y =
					temperatureToY
					(
						hoveredSample->temperatureDegC
					);

				dc.SetPen
				(
					wxPen(chartBackground, FromDIP(2))
				);

				dc.SetBrush(wxBrush(temperatureColour));

				dc.DrawCircle
				(
					wxPoint(inspectionX, y),
					FromDIP(4)
				);
			}

			if (hoveredPowerAvailable)
			{
				const int y =
					powerToY
					(
						hoveredSample->powerUtilizationPct
					);

				dc.SetPen
				(
					wxPen(chartBackground, FromDIP(2))
				);

				dc.SetBrush(wxBrush(powerColour));

				dc.DrawCircle
				(
					wxPoint(inspectionX, y),
					FromDIP(4)
				);
			}

			/*
				Show time relative to now. This is more meaningful than formatting
				steady_clock directly, because steady_clock has no calendar-time
				representation.
			*/
			const double ageSeconds =
				std::max
				(
					0.0,
					std::chrono::duration<double>
					(
						now - hoveredSample->timestamp
					).count()
				);

			wxString timestampText;

			if (ageSeconds < 1.0)
			{
				timestampText = wxT("now");
			}
			else if (ageSeconds < 60.0)
			{
				timestampText =
					wxString::Format
					(
						wxT("%.1f s ago"),
						ageSeconds
					);
			}
			else
			{
				const int totalSeconds =
					static_cast<int>
					(
						std::round(ageSeconds)
						);

				const int minutes =
					totalSeconds / 60;

				const int seconds =
					totalSeconds % 60;

				timestampText =
					wxString::Format
					(
						wxT("%d:%02d ago"),
						minutes,
						seconds
					);
			}

			wxArrayString tooltipLines;
			tooltipLines.Add(timestampText);

			if (hoveredTemperatureAvailable)
			{
				tooltipLines.Add
				(
					wxString::Format
					(
						wxT("Temperature: %.2f °C"),
						hoveredSample->temperatureDegC
					)
				);
			}

			if (hoveredPowerAvailable)
			{
				tooltipLines.Add
				(
					wxString::Format
					(
						wxT("Power: %.0f %%"),
						hoveredSample->powerUtilizationPct
					)
				);
			}

			wxFont tooltipFont = smallFont;
			tooltipFont.SetWeight(wxFONTWEIGHT_NORMAL);

			wxFont tooltipHeaderFont = tooltipFont;
			tooltipHeaderFont.SetWeight(wxFONTWEIGHT_BOLD);

			const int tooltipPadding = FromDIP(8);
			const int tooltipLineGap = FromDIP(2);

			int tooltipTextWidth = 0;
			int tooltipTextHeight = 0;

			for
				(
					size_t lineIndex = 0;
					lineIndex < tooltipLines.GetCount();
					++lineIndex
					)
			{
				dc.SetFont
				(
					lineIndex == 0
					? tooltipHeaderFont
					: tooltipFont
				);

				wxCoord lineWidth{};
				wxCoord lineHeight{};

				dc.GetTextExtent
				(
					tooltipLines[lineIndex],
					&lineWidth,
					&lineHeight
				);

				tooltipTextWidth =
					std::max
					(
						tooltipTextWidth,
						static_cast<int>(lineWidth)
					);

				tooltipTextHeight +=
					static_cast<int>(lineHeight);

				if
					(
						lineIndex + 1 <
						tooltipLines.GetCount()
						)
				{
					tooltipTextHeight +=
						tooltipLineGap;
				}
			}

			const int tooltipWidth =
				tooltipTextWidth +
				2 * tooltipPadding;

			const int tooltipHeight =
				tooltipTextHeight +
				2 * tooltipPadding;

			/*
				Prefer showing the tooltip to the right of the cursor. Move it to
				the left when it would leave the plotting area.
			*/
			int tooltipX =
				inspectionX + FromDIP(10);

			if
				(
					tooltipX + tooltipWidth >
					plotRect.GetRight()
					)
			{
				tooltipX =
					inspectionX -
					tooltipWidth -
					FromDIP(10);
			}

			tooltipX = std::clamp
			(
				tooltipX,
				plotRect.GetLeft() + FromDIP(3),
				plotRect.GetRight() -
				tooltipWidth -
				FromDIP(3)
			);

			int tooltipY =
				m_MousePosition.y -
				tooltipHeight -
				FromDIP(10);

			if (tooltipY < plotRect.GetTop())
			{
				tooltipY =
					m_MousePosition.y +
					FromDIP(10);
			}

			tooltipY = std::clamp
			(
				tooltipY,
				plotRect.GetTop() + FromDIP(3),
				plotRect.GetBottom() -
				tooltipHeight -
				FromDIP(3)
			);

			wxRect tooltipRect
			(
				tooltipX,
				tooltipY,
				tooltipWidth,
				tooltipHeight
			);

			const wxColour tooltipBackground =
				m_DarkAppearance
				? wxColour(53, 57, 64)
				: wxColour(250, 251, 253);

			const wxColour tooltipBorder =
				m_DarkAppearance
				? wxColour(112, 119, 130)
				: wxColour(170, 176, 185);

			// Small shadow.
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush
			(
				wxBrush
				(
					m_DarkAppearance
					? wxColour(20, 22, 25)
					: wxColour(205, 209, 216)
				)
			);

			tooltipRect.Offset
			(
				FromDIP(2),
				FromDIP(2)
			);

			dc.DrawRoundedRectangle
			(
				tooltipRect,
				FromDIP(6)
			);

			dc.SetPen(wxPen(tooltipBorder));
			dc.SetBrush(wxBrush(tooltipBackground));

			dc.DrawRoundedRectangle
			(
				tooltipRect,
				FromDIP(6)
			);

			int textY =
				tooltipRect.GetTop() +
				tooltipPadding;

			for
				(
					size_t lineIndex = 0;
					lineIndex < tooltipLines.GetCount();
					++lineIndex
					)
			{
				dc.SetFont
				(
					lineIndex == 0
					? tooltipHeaderFont
					: tooltipFont
				);

				if (lineIndex == 1 && hoveredTemperatureAvailable)
				{
					dc.SetTextForeground
					(
						temperatureColour
					);
				}
				else if
					(
						(lineIndex == 1 &&
							!hoveredTemperatureAvailable &&
							hoveredPowerAvailable) ||
						(lineIndex == 2 &&
							hoveredPowerAvailable)
						)
				{
					dc.SetTextForeground(powerColour);
				}
				else
				{
					dc.SetTextForeground(primaryText);
				}

				wxCoord lineWidth{};
				wxCoord lineHeight{};

				dc.GetTextExtent
				(
					tooltipLines[lineIndex],
					&lineWidth,
					&lineHeight
				);

				dc.DrawText
				(
					tooltipLines[lineIndex],
					tooltipRect.GetLeft() +
					tooltipPadding,
					textY
				);

				textY +=
					static_cast<int>(lineHeight) +
					tooltipLineGap;
			}
		}
	}

	/*
		Show an explicit state instead of an unexplained empty graph.
	*/
	if (m_Samples.empty())
	{
		const wxString noDataText = m_HasReceivedTelemetry
			? wxString(wxT("Temperature and power telemetry unavailable"))
			: wxString(wxT("Waiting for camera telemetry"));

		dc.SetFont(normalFont);
		dc.SetTextForeground(secondaryText);

		wxCoord textWidth{};
		wxCoord textHeight{};

		dc.GetTextExtent
		(
			noDataText,
			&textWidth,
			&textHeight
		);

		dc.DrawText
		(
			noDataText,
			plotRect.GetLeft() +
			(plotRect.GetWidth() - textWidth) / 2,
			plotRect.GetTop() +
			(plotRect.GetHeight() - textHeight) / 2
		);
	}

	// Rotated Y-axis captions.
	dc.SetFont(smallFont);

	const wxString temperatureAxisCaption =
		wxT("Temperature [°C]");

	const wxString powerAxisCaption =
		wxT("Power utilization [%]");

	wxCoord temperatureCaptionWidth{};
	wxCoord temperatureCaptionHeight{};

	dc.GetTextExtent
	(
		temperatureAxisCaption,
		&temperatureCaptionWidth,
		&temperatureCaptionHeight
	);

	wxCoord powerCaptionWidth{};
	wxCoord powerCaptionHeight{};

	dc.GetTextExtent
	(
		powerAxisCaption,
		&powerCaptionWidth,
		&powerCaptionHeight
	);

	/*
		Positive 90 degrees rotates counter-clockwise, so the left caption reads
		from bottom to top.
	*/
	const int leftCaptionX =
		outerRect.GetLeft() + FromDIP(8);

	const int leftCaptionY =
		plotRect.GetTop() +
		(plotRect.GetHeight() + temperatureCaptionWidth) / 2;

	dc.SetTextForeground(temperatureColour);
	dc.DrawRotatedText
	(
		temperatureAxisCaption,
		leftCaptionX,
		leftCaptionY,
		90.0
	);

	/*
		Negative 90 degrees rotates clockwise, so the right caption reads from
		top to bottom.
	*/
	const int rightCaptionX =
		outerRect.GetRight() -
		powerCaptionHeight + FromDIP(8);

	const int rightCaptionY =
		plotRect.GetTop() +
		(plotRect.GetHeight() - powerCaptionWidth) / 2;

	dc.SetTextForeground(powerColour);
	dc.DrawRotatedText
	(
		powerAxisCaption,
		rightCaptionX,
		rightCaptionY,
		-90.0
	);
}


void cCameraTelemetryPanel::OnSize(wxSizeEvent& event)
{
	Refresh(false);
	event.Skip();
}

void cCameraTelemetryPanel::OnMouseMove(wxMouseEvent& event)
{
	m_MousePosition = event.GetPosition();

	const bool insidePlot =
		!m_LastPlotRect.IsEmpty() &&
		m_LastPlotRect.Contains(m_MousePosition);

	if (insidePlot != m_MouseInsidePlot)
	{
		m_MouseInsidePlot = insidePlot;
		Refresh(false);
	}
	else if (insidePlot)
	{
		/*
			The inspection line follows the mouse, so repaint while the cursor
			moves inside the plotting area.
		*/
		Refresh(false);
	}

	event.Skip();
}

void cCameraTelemetryPanel::OnMouseLeave(wxMouseEvent& event)
{
	m_MouseInsidePlot = false;
	m_MousePosition = wxPoint(-1, -1);

	Refresh(false);
	event.Skip();
}

void cCameraTelemetryPanel::RemoveExpiredSamples(const std::chrono::steady_clock::time_point& now)
{
	const auto oldestAllowed = now - HistoryDuration;

	while
		(
			!m_Samples.empty() &&
			m_Samples.front().timestamp < oldestAllowed
			)
	{
		m_Samples.pop_front();
	}
}

void cCameraTelemetryPanel::DrawRoundedLabel(wxDC& dc, const wxRect& rect, const wxString& text, const wxColour& background, const wxColour& foreground) const
{
	dc.SetPen(wxPen(background));
	dc.SetBrush(wxBrush(background));
	dc.DrawRoundedRectangle(rect, FromDIP(7));

	dc.SetTextForeground(foreground);

	wxCoord textWidth{};
	wxCoord textHeight{};

	dc.GetTextExtent(text, &textWidth, &textHeight);

	const int x =
		rect.GetLeft() +
		std::max(0, (rect.GetWidth() - textWidth) / 2);

	const int y =
		rect.GetTop() +
		std::max(0, (rect.GetHeight() - textHeight) / 2);

	dc.DrawText(text, x, y);
}

const Telemetry::Sample* cCameraTelemetryPanel::FindClosestSample(const std::chrono::steady_clock::time_point& targetTime, const std::chrono::steady_clock::time_point& historyStart) const
{
	const Telemetry::Sample* closestSample = nullptr;
	double closestDistanceSeconds =
		std::numeric_limits<double>::max();

	for (const auto& sample : m_Samples)
	{
		if (sample.timestamp < historyStart)
			continue;

		const bool hasTemperature =
			std::isfinite(sample.temperatureDegC);

		const bool hasPower =
			std::isfinite(sample.powerUtilizationPct);

		if (!hasTemperature && !hasPower)
			continue;

		const double distanceSeconds =
			std::fabs
			(
				std::chrono::duration<double>
				(
					sample.timestamp - targetTime
				).count()
			);

		if (distanceSeconds < closestDistanceSeconds)
		{
			closestDistanceSeconds = distanceSeconds;
			closestSample = &sample;
		}
	}

	return closestSample;
}

double cCameraTelemetryPanel::NiceAxisStep(double range, int desiredIntervals)
{
	if (!std::isfinite(range) || range <= 0.0)
		return 1.0;

	const double rawStep =
		range / static_cast<double>(std::max(1, desiredIntervals));

	const double magnitude =
		std::pow(10.0, std::floor(std::log10(rawStep)));

	const double normalized = rawStep / magnitude;

	double niceNormalized = 1.0;

	if (normalized <= 1.0)
		niceNormalized = 1.0;
	else if (normalized <= 2.0)
		niceNormalized = 2.0;
	else if (normalized <= 5.0)
		niceNormalized = 5.0;
	else
		niceNormalized = 10.0;

	return niceNormalized * magnitude;
}
