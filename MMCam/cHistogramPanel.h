#pragma once
#ifndef CHISTOGRAM_PANEL_H
#define CHISTOGRAM_PANEL_H

#include "wx/wx.h"
#include "wx/dcbuffer.h"
#include "wx/graphics.h"
#include "wx/timer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <algorithm>
#include <optional>
#include <cmath>
#include <vector>

// Macros
#ifdef _DEBUG
#define LOG(message) wxLogDebug(message);
#define LOGST(message, number) wxLogDebug(message + wxString::Format(wxT("%i"), number))
#define LOGF(message, number) wxLogDebug(message + wxString::Format(wxT("%.2f"), number))
#define LOGI(message, number) wxLogDebug(message + wxString::Format(wxT("%i"), number))
#define LOG2I(first_message, first_number, second_message, second_number) wxLogDebug(first_message + wxString::Format(wxT("%i"), first_number) + second_message + wxString::Format(wxT("%i"), second_number))
#define LOG2F(first_message, first_number, second_message, second_number) wxLogDebug(first_message + wxString::Format(wxT("%.2f"), first_number) + second_message + wxString::Format(wxT("%.2f"), second_number))
#define LOG6F(first_message, first_number, second_message, second_number, third_message, third_number, fourth_message, fourth_number, fifth_message, fifth_number, sixth_message, sixth_number) wxLogDebug(first_message + wxString::Format(wxT("%.2f"), first_number) + second_message + wxString::Format(wxT("%.2f"), second_number) + third_message + wxString::Format(wxT("%.2f"), third_number) + fourth_message + wxString::Format(wxT("%.2f"), fourth_number) + fifth_message + wxString::Format(wxT("%.2f"), fifth_number) + sixth_message + wxString::Format(wxT("%.2f"), sixth_number))
#else
#define LOG(message)
#define LOGST(message, number)
#define LOGF(message, number)
#define LOGI(message, number)
#define LOG2I(first_message, first_number, second_message, second_number)
#define LOG2F(first_message, first_number, second_message, second_number)
#define LOG6F(first_message, first_number, second_message, second_number, third_message, third_number, fourth_message, fourth_number, fifth_message_fifth_number, sixth_message, sixth_number)
#endif // _DEBUG

namespace HistogramPanelVariables
{
	enum ImageDataTypes
	{
		RAW_12BIT,
		RAW_16BIT
	};

	/// 1234567 -> "1,234,567"
	inline auto FormatNumber(long long value) -> wxString
	{
		const bool negative = value < 0;
		std::string digits = std::to_string(negative ? -value : value);
		std::string out;
		out.reserve(digits.size() + digits.size() / 3 + 1);
		for (size_t i = 0; i < digits.size(); ++i)
		{
			if (i && (digits.size() - i) % 3 == 0) out.push_back(',');
			out.push_back(digits[i]);
		}
		return wxString(negative ? "-" + out : out);
	}

	/// Short axis labels: 950, 12.5k, 3.2M
	inline auto FormatCompact(double value) -> wxString
	{
		if (value < 10'000.0) return FormatNumber((long long)std::llround(value));
		if (value < 1'000'000.0) return wxString::Format(value < 100'000.0 ? "%.1fk" : "%.0fk", value / 1'000.0);
		return wxString::Format(value < 100'000'000.0 ? "%.1fM" : "%.0fM", value / 1'000'000.0);
	}
}

/// Interactive intensity histogram shown under the camera preview.
///
/// The two handles set the black/white display levels of the preview (they are
/// reported to the parent through two hidden text controls). Mouse:
///  - drag a handle (or its triangle on the ramp) to move a level
///  - Shift+drag selects a new level range
///  - drag elsewhere pans, wheel zooms the value axis
///  - right-click or the LIN/LOG chip toggles the count axis scale
///  - double-click or the RESET chip restores the automatic levels and view
class cHistogramPanel final : public wxPanel
{
public:
	cHistogramPanel
	(
		wxWindow* parent_frame,
		wxSizer* parent_sizer,
		wxTextCtrl* parent_black_txt_ctrl,
		wxTextCtrl* parent_white_txt_ctrl,
		const int borderSize = 1,
		const wxString title = wxEmptyString
	);

	auto SetHistogram
	(
		const HistogramPanelVariables::ImageDataTypes data_type,
		unsigned long long* data,
		unsigned long long min_value,
		unsigned long long max_value
	) -> std::optional<int>;

	auto SetBackgroundColor(wxColour bckg_colour) -> void;
	auto SetAutoBordersPos(const int leftBorder, const int rightBorder) -> void { m_AutoLeft = leftBorder; m_AutoRight = rightBorder; };
	auto SetBordersPos(const int leftBorder, const int rightBorder) -> void;

	auto GetWasHistogramRangeChanged() const -> bool { return m_WasRangeChanged; };
	auto GetLeftBorderValue() const -> int { return m_Left; };
	auto GetRightBorderValue() const -> int { return m_Right; };
	/// Restores the automatic levels and the full view (called when the camera changes).
	auto ResetHistogramRange() -> void { ResetRangeAndView(); };

private:
	enum class DragMode { None, Left, Right, Select, Pan };
	enum class Hover { None, Left, Right, ScaleChip, ResetChip };

	// Drawing
	void PaintEvent(wxPaintEvent& evt);
	void Render(wxGraphicsContext* gc);
	void DrawPlotBackground(wxGraphicsContext* gc);
	void DrawGrid(wxGraphicsContext* gc);
	void DrawHistogram(wxGraphicsContext* gc);
	void DrawOutOfRangeShade(wxGraphicsContext* gc);
	void DrawRamp(wxGraphicsContext* gc);
	void DrawHandles(wxGraphicsContext* gc);
	void DrawStats(wxGraphicsContext* gc);
	void DrawChips(wxGraphicsContext* gc);
	void DrawHoverReadout(wxGraphicsContext* gc);
	void DrawEmptyState(wxGraphicsContext* gc);
	/// Rounded "pill" with text; returns its rectangle.
	wxRect2DDouble DrawPill(wxGraphicsContext* gc, const wxString& text, double x, double y,
		const wxColour& fill, const wxColour& textColour, const wxColour& border = wxNullColour);

	// Geometry / mapping
	void UpdateLayout();
	void RebuildOutline();
	double ValueToX(double v) const;      ///< value (bin index, may be fractional) -> canvas x
	int XToValue(double x) const;         ///< canvas x -> bin index inside the current view
	double CountToY(double c) const;      ///< count -> canvas y (linear or log)
	double LevelToX(int level) const { return ValueToX(level + 0.5); } ///< handle position (bin centre)
	bool IsDark() const;

	// Interaction
	void OnSize(wxSizeEvent& evt);
	void OnMouseWheel(wxMouseEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseUp(wxMouseEvent& evt);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
	void OnRightUp(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);

	Hover HitTest(const wxPoint& pt) const;
	void UpdateHoverAndCursor(const wxPoint& pt);
	void SetLevelsFromDrag(int left, int right);
	void NotifyParent();
	void OnNotifyTimer(wxTimerEvent& evt);
	void EndDrag();
	void ToggleLogScale();
	void ResetRangeAndView();
	void FitViewToData();
	void SetView(long long viewMin, long long viewMax);

private:
	wxColour m_Accent{ 66, 146, 255 };

	// Data
	HistogramPanelVariables::ImageDataTypes m_DataType{ HistogramPanelVariables::ImageDataTypes::RAW_16BIT };
	std::unique_ptr<unsigned long long[]> m_HistogramData{};
	unsigned int m_MaxHistogramValue{};
	unsigned int m_FirstNonZero{}, m_LastNonZero{};
	double m_Mean{};
	unsigned int m_Median{}, m_Mode{};
	unsigned int m_UpperPercentile{}; ///< value below which 99.95 % of the pixels lie
	unsigned long long m_TotalCount{};

	// Levels (black / white) and the automatic ones computed by the parent
	int m_Left{}, m_Right{};
	int m_AutoLeft{}, m_AutoRight{};
	bool m_WasRangeChanged{};

	wxTextCtrl* m_ParentLeftBorder{}, * m_ParentRightBorder{};
	wxString m_TitleStr{};

	// View on the value axis: bins [m_ViewMin, m_ViewMax] fill the plot width
	unsigned int m_ViewMin{ 0 }, m_ViewMax{ USHRT_MAX };
	bool m_UserAdjustedView{};
	bool m_LogScale{};
	unsigned long long m_ViewPeak{};

	// Layout (canvas coordinates)
	wxSize m_CanvasSize{};
	wxRect2DDouble m_Plot{}, m_Ramp{};
	std::vector<wxPoint2DDouble> m_Outline{};
	wxRect2DDouble m_ScaleChipRect{}, m_ResetChipRect{};

	// Mouse state
	DragMode m_Drag{ DragMode::None };
	Hover m_Hover{ Hover::None };
	bool m_IsMouseInside{};
	wxPoint m_MousePos{};
	int m_SelectAnchor{};
	int m_PanStartX{};
	unsigned int m_ViewMinAtDragStart{}, m_ViewMaxAtDragStart{};
	wxTimer m_NotifyTimer{};   ///< merges level updates sent to the preview while dragging
	bool m_NotifyPending{};
	wxString m_CurrentTip{};

	DECLARE_EVENT_TABLE();
};

#endif // !CHISTOGRAM_PANEL_H
