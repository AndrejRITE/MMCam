#pragma once
#ifndef CHISTOGRAM_PANEL_H
#define CHISTOGRAM_PANEL_H

#include "wx/wx.h"
#include "wx/dcbuffer.h"
#include "wx/graphics.h"

#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <algorithm>
#include <optional>

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
}

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
	auto SetAutoBordersPos(const int leftBorder, const int rightBorder) -> void { m_AutoLeftBorder.x = leftBorder; m_AutoRightBorder.x = rightBorder; };
	auto SetBordersPos(const int leftBorder, const int rightBorder) -> void 
	{ 
		m_WasRangeChanged = m_LeftBorder.x != leftBorder || m_RightBorder.x != rightBorder ? true : m_WasRangeChanged; 
		m_LeftBorder.x = leftBorder; 
		m_RightBorder.x = rightBorder; 
		Refresh(); 
	};

	auto GetWasHistogramRangeChanged() const -> bool { return m_WasRangeChanged; };
	auto GetLeftBorderValue() const -> int { return m_LeftBorder.x; };
	auto GetRightBorderValue() const -> int { return m_RightBorder.x; };
	auto ResetHistogramRange() -> void 
	{
		wxMouseEvent event(wxEVT_LEFT_DCLICK);
		
		event.SetEventObject(this);

		wxPostEvent(this, event);
		ProcessEvent(event);
	};

private:
	auto SetWXImage() -> void;

	void InitDefaultComponents();
	void PaintEvent(wxPaintEvent& evt);
	void Render(wxBufferedPaintDC& dc);
	void DrawImage(wxGraphicsContext* gc);
	auto DrawTitle(wxGraphicsContext* gc) -> void;
	auto DrawRectangleRange(wxGraphicsContext* gc) -> void;
	void CreateGraphicsBitmapImage(wxGraphicsContext* gc_);
	void OnSize(wxSizeEvent& evt);
	void ChangeSizeOfImageInDependenceOnCanvasSize();

	void CheckIfMouseAboveImage();
	void CalculatePositionOnImage();

	auto OnMouseWheel(wxMouseEvent& evt) -> void;
	void OnMouseDown(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseUp(wxMouseEvent& evt);
	auto OnToggleLogScale(wxMouseEvent& evt) -> void;
	auto OnPreviewMouseLeftDoubleClick(wxMouseEvent& evt) -> void;
	auto OnPreviewMouseEnteredWindow(wxMouseEvent& evt) -> void;
	auto OnPreviewMouseLeftWindow(wxMouseEvent& evt) -> void;


	void InvalidateGraphicsBitmap() { m_IsGraphicsBitmapSet = false; }
	void RebuildHistogramImageForCurrentView();

	void ChangeCursorInDependenceOfCurrentParameters();

	auto MedianBlur1D(unsigned long long* dataPtr, const size_t dataSize, size_t windowSize) -> void;

private:
	// Helpers to convert between value<->canvas X using current view
	inline unsigned int DomainSpan() const { return (m_ViewMax > m_ViewMin) ? (m_ViewMax - m_ViewMin) : 1; }
	inline int ValueToCanvasX(unsigned int v) const {
		double t = (double)(std::clamp(v, m_ViewMin, m_ViewMax) - m_ViewMin) / (double)DomainSpan();
		return (int)std::round(t * (m_CanvasSize.GetWidth() - 1));
	}
	inline unsigned int CanvasXToValue(int x) const {
		x = std::clamp(x, 0, std::max(0, m_CanvasSize.GetWidth() - 1));
		double t = (m_CanvasSize.GetWidth() > 1) ? (double)x / (double)(m_CanvasSize.GetWidth() - 1) : 0.0;
		return m_ViewMin + (unsigned int)std::round(t * DomainSpan());
	}

private:
	int m_Width{}, m_Height{};
	bool m_IsGraphicsBitmapSet{}, m_IsImageSet{};
	wxGraphicsBitmap m_GraphicsBitmapImage{};

	wxImage m_Image{};

	wxSize m_ImageSize{}, m_ImageOnCanvasSize{}, m_CanvasSize{};
	wxRealPoint m_NotCheckedCursorPosOnImage{}, m_CheckedCursorPosOnImage{}, m_CursorPosOnCanvas{};
	wxRealPoint m_NotZoomedGraphicsBitmapOffset{}, m_StartDrawPos{};
	bool m_IsCursorInsideImage{};

	HistogramPanelVariables::ImageDataTypes m_DataType{ HistogramPanelVariables::ImageDataTypes::RAW_16BIT };
	unsigned long m_MaxHistogramValue{};
	std::unique_ptr<unsigned long long[]> m_HistogramData{};
	unsigned long long m_GlobalMin{}, m_GlobalMax{};

	wxRealPoint m_LeftBorderOnCanvas{}, m_RightBorderOnCanvas{};
	wxPoint m_LeftBorder{}, m_RightBorder{};
	wxPoint m_AutoLeftBorder{}, m_AutoRightBorder{};

	bool m_IsCursorAboveLeftBorder{}, m_IsCursorAboveRightBorder{};
	int m_RangeWhenMouseReactOnBordersPx{ 6 };
	bool m_ChangingLeftBorder{}, m_ChangingRightBorder{};
	bool m_WasRangeChanged{};

	wxTextCtrl* m_ParentLeftBorder{},* m_ParentRightBorder{};
	wxString m_TitleStr{};

	bool m_IsMouseInsidePanel{};

	// View on the X axis (value domain)
	unsigned int m_ViewMin{ 0 }, m_ViewMax{ USHRT_MAX };
	bool m_LogScale{ false }; // toggle for low-signal visibility

	bool m_Panning{ false };
	int  m_PanStartX{ 0 };
	unsigned int m_ViewMinAtDragStart{ 0 }, m_ViewMaxAtDragStart{ 0 };

	DECLARE_EVENT_TABLE();
};

#endif // !CHISTOGRAM_PANEL_H

