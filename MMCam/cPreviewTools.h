#pragma once
#ifndef CPREVIEWTOOLS_H
#define CPREVIEWTOOLS_H

#include "wx/wx.h"
#include "wx/graphics.h"

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

namespace ToolsVariables
{
	enum DataTypes
	{
		DATA_U8,
		DATA_U12,
		DATA_U16,
	};
}

class Tool
{
public:
	Tool();
	void SetImageDimensions(const wxSize& image_size);
	void SetImageDataType(const int& data_type);
	void SetCursorPosOnCanvas(const wxRealPoint& cursor_pos_on_canvas);
	void ActivateToolButton(bool activate = false);
	bool IsToolButtonActive() const;
	void MouseHoverOverImage(bool moi = false);
	virtual void UpdateZoomValue(const double& zoom);
	void SetImageStartDrawPos(const wxRealPoint& start_draw_pos);
	void SetImageOnCanvasSize(const wxSize& img_size);
	void SetZoomOfOriginalSizeImage(const double& original_size_image_zoom);
	virtual wxStockCursor UpdateCursor(const wxStockCursor& current_cursor) = 0;
	virtual ~Tool() = default;

protected:
	wxSize m_ImageSize{};
	double m_Zoom{}, m_ZoomOriginalSizeImage{};
	bool m_ToolButtonActive{};
	bool m_MouseHoverOverImage{};
	wxRealPoint m_ImageStartDraw{};
	wxRealPoint m_CursorOnCanvas{};
	wxSize m_ImageOnCanvasSize{};

	int m_DataType{};
};

class HandTool final : public Tool
{
public:
	HandTool();
	void SetCursorPositionOnImage(const wxRealPoint& cursor_position);
	wxStockCursor UpdateCursor(const wxStockCursor& current_cursor) override;
private:
	bool m_HandActive{};
};

class CrossHairTool final : public Tool
{
public:
	CrossHairTool(wxTextCtrl* parent_x_pos, wxTextCtrl* parent_y_pos);
	void UpdateZoomValue(const double& zoom) override;
	void ActivateToolButton(bool vertical_line = false, bool horizontal_line = false);
	void ActivateValueDisplaying(bool activate = false);
	void SetCursorPosOnCanvas(const wxRealPoint& cursor_pos_on_canvas);
	void SetCursorPosOnImage(const wxRealPoint& cursor_pos_on_image);
	void SetXPosFromParent(const int& x_pos);
	void SetYPosFromParent(const int& y_pos);
	void ChangeStartPositionOfImage(const wxRealPoint& img_pos);
	void CalculateCrossHairPositionOnCanvas();
	void UpdateParentCrossHairTextCtrls();
	wxStockCursor UpdateCursor(const wxStockCursor& current_cursor) override;
	void DrawCrossHair(wxGraphicsContext* graphics_context_, unsigned short* data_);
	bool PositionCanBeChanged() const;
	bool CanProcessPanning() const;
	void ProcessChangingCrossHairPos();
	void LoopChangingCrossHairPos();
	void StopChangingCrossHairPos();
	bool GetShowVerticalLine() const;
	int GetXPos() const;
	bool GetShowHorizontalLine() const;
	int GetYPos() const;
	auto ActivateSetPositionFromParentWindow(bool activate) -> void;

	auto SetAveragingWidthPX(const int& avgWidth) -> void;
	auto ActivateAdaptiveScaling(const bool isActive = false) -> void { m_AdaptiveScaling = isActive; };

private:
	void CheckIfMouseAboveCrossHair();
	void CalculateCrossHairOnImage();
	void DrawData(wxGraphicsContext* graphics_context, uint16_t* data);

	auto DrawDataOnHorizontalLine
	(
		wxGraphicsContext* gc,
		uint16_t* image_data,
		const int& curve_height_offset,
		const int& curve_max_height,
		const uint16_t& fixed_max_value
	) -> void;

	auto DrawDataOnVerticalLine
	(
		wxGraphicsContext* gc,
		uint16_t* image_data,
		const int& curve_width_px,
		const int& curve_max_width,
		const uint16_t& fixed_max_value
	) -> void;

	void UpdateParentCrossHairTextCtrlsWithRefresh();

	auto CheckIfPixelValueIsInsideTheImage(const int& x, const int& y) -> bool;

private:
	wxTextCtrl* m_ParentXPosTextCtrl{}, *m_ParentYPosTextCtrl{};
	wxRealPoint m_CursorOnImage{};
	wxRealPoint m_CrossHairOnCanvas{}, m_CrossHairOnImage{};
	bool m_HorizontalLineProfileActive{}, m_VerticalLineProfileActive{};
	bool m_CursorAboveHorizontalLine{}, m_CursorAboveVerticalLine{};
	bool m_ChangingHorizontalLine{}, m_ChangingVerticalLine{};
	bool m_ShowHorizontalLine{}, m_ShowDataOnHorizontalLine{};
	bool m_ShowVerticalLine{}, m_ShowDataOnVerticalLine{};
	bool m_SelectingPositionFromParentWindow{};
	wxSize m_ActualZoomedSizeOfImageOnCanvas{};

	wxSize m_CrosshairRectangle{ 6, 6 };
	int m_TextCtrlPixelOffset{ 1 };
	wxRealPoint m_MultiplicationFactor{};
	wxRealPoint m_ActualHalfPixelSize{};

	int m_AveragingWidthPX{ 1 };
	bool m_AdaptiveScaling{};
};

class RectangleSelectionTool final : public Tool
{
public:
	RectangleSelectionTool(
		wxTextCtrl* parent_start_x_pos,
		wxTextCtrl* parent_start_y_pos,
		wxTextCtrl* parent_width,
		wxTextCtrl* parent_height);
	wxStockCursor UpdateCursor(const wxStockCursor& current_cursor) override;
	void UpdateZoomValue(const double& zoom) override;
	void SetStartXPosFromParent(const int& x_pos);
	void SetStartYPosFromParent(const int& y_pos);
	void SetWidthFromParent(const int& width);
	void SetHeightFromParent(const int& height);
	void UpdateParentRectangleTextCtrls();
	void CalculateRectangleSelectionPosOnCanvas();
	void CalculateRectangleSelectionPosOnImage();
	void DrawRectangleSelection(wxGraphicsContext* graphics_context);
	void DrawRectangleSelectionPoints(wxGraphicsContext* gc);
	void SetCursorPosOnImage(const wxRealPoint& cursor_pos_on_image);
	/* Selecting region */
	bool CanSelectRegion() const;
	void ProcessSelectingRegion();
	void LoopSelectingRegion();
	void StopSelectingRegion();
	/* Panning region */
	bool CanPanSelectedRegion() const;
	void ProcessPanningSelectedRegion();
	void LoopPanningSelectedRegion();
	void StopPanningSelectedRegion();
	/* Changing Size of region */
	bool CanChangeSize() const;
	void ProcessChangingSizeSelectedRegion();
	void LoopChangingSizeSelectedRegion();
	void StopChangingSizeSelectedRegion();

private:
	void UpdateParentRectangleTextCtrlsWithRefresh();
	void CheckIfMouseAboveRectangleSelection();
	void ChangeLUorRBRectanglePoints();

private:
	/* Parent Text Controls */
	wxTextCtrl* m_ParentStartXPosTextCtrl{}, * m_ParentStartYPosTextCtrl{}, * m_ParentWidthTextCtrl{}, * m_ParentHeightTextCtrl{};

	wxRect m_RectangleSelectionOnImage{};

	wxRect2DDouble m_RectangleSelectionOnCanvas{}, m_VisualRectangleSelectionOnCanvas{};
	wxPoint2DDouble m_StartSelectingAreaPoint{};
	wxRealPoint m_StartPanSizeCursorPos{};
	wxPoint m_StartPanRectangleSelection{}, m_DeltaPoint{};
	wxRect m_StartSizingRectangleSelection{};

	bool m_IsRectangleSelectionButtonChecked{}, m_ShowRectangleSelection{};
	bool m_IsRectangleSelectionAlreadyDrawn{};
	bool m_UserSelectingAreaOfRectangle{}, m_UserPanningSelectedRectangleSelection{}, m_UserChangingSizeSelectedRectangleSelection{};
	bool m_ChangingLUPoint{}, m_ChangingRUPoint{}, m_ChangingRBPoint{}, m_ChangingLBPoint{};
	bool m_ChangingLMPoint{}, m_ChangingUMPoint{}, m_ChangingRMPoint{}, m_ChangingBMPoint{};
	/* Cursor position */
	bool m_CursorInsideRectangleSelection{}, m_CursorOutsideOfRectangleSelection{};
	bool m_CursorAboveVerticalLeftLineMiddlePoint{}, m_CursorAboveHorizontalUpperLineMiddlePoint{}, m_CursorAboveVerticalRightLineMiddlePoint{}, m_CursorAboveHorizontalBottomLineMiddlePoint{};
	bool m_CursorAboveLeftUpperCornerPoint{}, m_CursorAboveRightUpperCornerPoint{}, m_CursorAboveRightBottomCornerPoint{}, m_CursorAboveLeftBottomCornerPoint{};

	wxRealPoint m_CursorOnImage{};
	int m_TextCtrlPixelOffset{ 1 };
	wxRealPoint m_ActualPixelSize{};
	wxSize m_PointSize{ 6, 6 };
	bool m_DrawHorizontalPoints{}, m_DrawVerticalPoints{};
};

#endif // !CPREVIEWTOOLS_H

