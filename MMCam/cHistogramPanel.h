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
		wxFrame* parent_frame,
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

private:
	void InitDefaultComponents();
	void PaintEvent(wxPaintEvent& evt);
	void Render(wxBufferedPaintDC& dc);
	void DrawImage(wxGraphicsContext* gc);
	auto DrawTitle(wxGraphicsContext* gc) -> void;
	auto DrawRectangleRange(wxGraphicsContext* gc) -> void;
	void CreateGraphicsBitmapImage(wxGraphicsContext* gc_);
	void OnSize(wxSizeEvent& evt);
	void ChangeSizeOfImageInDependenceOnCanvasSize();
	void OnMouseMoved(wxMouseEvent& evt);

	void CheckIfMouseAboveImage();
	void CalculatePositionOnImage();

	void OnPreviewMouseLeftPressed(wxMouseEvent& evt);
	auto OnPreviewMouseLeftDoubleClick(wxMouseEvent& evt) -> void;
	void OnPreviewMouseLeftReleased(wxMouseEvent& evt);
	auto OnPreviewMouseEnteredWindow(wxMouseEvent& evt) -> void;
	auto OnPreviewMouseLeftWindow(wxMouseEvent& evt) -> void;

	void ChangeCursorInDependenceOfCurrentParameters();

	auto MedianBlur1D(unsigned long long* dataPtr, const size_t dataSize, size_t windowSize) -> void;

private:
	int m_Width{}, m_Height{};
	bool m_IsGraphicsBitmapSet{}, m_IsImageSet{};
	wxGraphicsBitmap m_GraphicsBitmapImage{};

	wxImage m_Image{};
	//std::shared_ptr<unsigned short[]> m_ImageData{};

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

	DECLARE_EVENT_TABLE();
};

#endif // !CHISTOGRAM_PANEL_H

