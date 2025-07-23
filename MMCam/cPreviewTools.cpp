#include "cPreviewTools.h"

Tool::Tool()
{

}

void Tool::SetImageDimensions(const wxSize& image_size)
{
	m_ImageSize = image_size;
}

void Tool::SetImageDataType(const int& data_type)
{
	m_DataType = data_type;
}

void Tool::SetCursorPosOnCanvas(const wxRealPoint& cursor_pos_on_canvas)
{
	m_CursorOnCanvas = { cursor_pos_on_canvas.x / m_ZoomOriginalSizeImage, cursor_pos_on_canvas.y / m_ZoomOriginalSizeImage };
}

void Tool::ActivateToolButton(bool activate)
{
	m_ToolButtonActive = activate;
}

bool Tool::IsToolButtonActive() const
{
	return m_ToolButtonActive;
}

void Tool::MouseHoverOverImage(bool moi)
{
	m_MouseHoverOverImage = moi;
}

void Tool::UpdateZoomValue(const double& zoom)
{
	m_Zoom = zoom;
}

void Tool::SetImageStartDrawPos(const wxRealPoint& start_draw_pos)
{
	m_ImageStartDraw = start_draw_pos;
}

void Tool::SetImageOnCanvasSize(const wxSize& img_size)
{
	m_ImageOnCanvasSize = img_size;
}

void Tool::SetZoomOfOriginalSizeImage(const double& original_size_image_zoom)
{
	m_ZoomOriginalSizeImage = original_size_image_zoom;
}

/* Hand Tool */
HandTool::HandTool()
{
}

void HandTool::SetCursorPositionOnImage(const wxRealPoint& cursor_position)
{
	if (!m_ToolButtonActive) return;
	else
	{
		m_HandActive = false;
		if (cursor_position.x >= 0 && cursor_position.x < m_ImageSize.GetWidth() &&
			cursor_position.y >= 0 && cursor_position.y < m_ImageSize.GetHeight())
			m_HandActive = true;
	}
}

wxStockCursor HandTool::UpdateCursor(const wxStockCursor& current_cursor) 
{
	auto edited_cursor = m_ToolButtonActive && m_HandActive && m_Zoom > 1.0 ? wxCURSOR_HAND : current_cursor;
	return edited_cursor;
}

/* CrossHair Tool */
CrossHairTool::CrossHairTool(wxTextCtrl* parent_x_pos, wxTextCtrl* parent_y_pos) 
	: m_ParentXPosTextCtrl(parent_x_pos), m_ParentYPosTextCtrl(parent_y_pos)
{
}

void CrossHairTool::UpdateZoomValue(const double& zoom)
{
	m_Zoom = zoom;
	
	m_ActualZoomedSizeOfImageOnCanvas =
	{
		(int)((double)m_ImageSize.GetWidth() * m_Zoom / m_ZoomOriginalSizeImage),
		(int)((double)m_ImageSize.GetHeight() * m_Zoom / m_ZoomOriginalSizeImage)
	};

	m_ActualHalfPixelSize =
	{
		m_Zoom / m_ZoomOriginalSizeImage / 2.0,
		m_Zoom / m_ZoomOriginalSizeImage / 2.0,
	};

	//CalculateCrossHairPositionOnCanvas();
}

void CrossHairTool::ActivateToolButton(bool vertical_line, bool horizontal_line)
{
	m_VerticalLineProfileActive = vertical_line;
	m_ShowVerticalLine = m_ShowDataOnVerticalLine = vertical_line;
	m_HorizontalLineProfileActive = m_ShowDataOnHorizontalLine = horizontal_line;
	m_ShowHorizontalLine = horizontal_line;
	m_ToolButtonActive = vertical_line || horizontal_line;
}

void CrossHairTool::ActivateValueDisplaying(bool activate)
{
}

void CrossHairTool::SetCursorPosOnCanvas(const wxRealPoint& cursor_pos_on_canvas)
{
	m_CursorOnCanvas = { cursor_pos_on_canvas.x / m_ZoomOriginalSizeImage, cursor_pos_on_canvas.y / m_ZoomOriginalSizeImage };
}

void CrossHairTool::SetCursorPosOnImage(const wxRealPoint& cursor_pos_on_image)
{
	m_CursorOnImage = cursor_pos_on_image;
	CheckIfMouseAboveCrossHair();
}

void CrossHairTool::SetXPosFromParent(const int& x_pos)
{
	m_CrossHairOnImage.x = x_pos;
	CalculateCrossHairPositionOnCanvas();
}

void CrossHairTool::SetYPosFromParent(const int& y_pos)
{
	m_CrossHairOnImage.y = y_pos;
	CalculateCrossHairPositionOnCanvas();
}

void CrossHairTool::ChangeStartPositionOfImage(const wxRealPoint& img_pos)
{
	SetImageStartDrawPos(img_pos);
}

void CrossHairTool::CheckIfMouseAboveCrossHair()
{
	if (m_MouseHoverOverImage)
	{
		m_CursorAboveHorizontalLine = m_CursorAboveVerticalLine = false;
		if ((m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_CrossHairOnCanvas.x + m_ActualHalfPixelSize.x - m_CrosshairRectangle.GetWidth() / 2.0) &&
			(m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_CrossHairOnCanvas.x + m_ActualHalfPixelSize.x + m_CrosshairRectangle.GetWidth() / 2.0) &&
			m_ShowVerticalLine)
		{
			m_CursorAboveVerticalLine = true;
		}
		if ((m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_CrossHairOnCanvas.y + m_ActualHalfPixelSize.y - m_CrosshairRectangle.GetHeight() / 2.0) && 
			(m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_CrossHairOnCanvas.y + m_ActualHalfPixelSize.y + m_CrosshairRectangle.GetHeight() / 2.0) && 
			m_ShowHorizontalLine)
		{
			m_CursorAboveHorizontalLine = true;
		}
	}
}

void CrossHairTool::CalculateCrossHairOnImage()
{
	m_CrossHairOnImage.x = m_CrossHairOnCanvas.x / m_Zoom * m_ZoomOriginalSizeImage;
	m_CrossHairOnImage.y = m_CrossHairOnCanvas.y / m_Zoom * m_ZoomOriginalSizeImage;

	/* Checking X */
	m_CrossHairOnImage.x = m_CrossHairOnImage.x >= 0.0 ? m_CrossHairOnImage.x : 0.0;
	m_CrossHairOnImage.x = m_CrossHairOnImage.x <= m_ImageSize.GetWidth() - 1.0 ? m_CrossHairOnImage.x : m_ImageSize.GetWidth() - 1.0;
	/* Checking Y */
	m_CrossHairOnImage.y = m_CrossHairOnImage.y >= 0.0 ? m_CrossHairOnImage.y : 0.0;
	m_CrossHairOnImage.y = m_CrossHairOnImage.y <= m_ImageSize.GetHeight() - 1.0 ? m_CrossHairOnImage.y : m_ImageSize.GetHeight() - 1.0;
}

void CrossHairTool::CalculateCrossHairPositionOnCanvas()
{
	m_CrossHairOnCanvas.x = m_CrossHairOnImage.x * m_Zoom / m_ZoomOriginalSizeImage;
	m_CrossHairOnCanvas.y = m_CrossHairOnImage.y * m_Zoom / m_ZoomOriginalSizeImage;
}

void CrossHairTool::UpdateParentCrossHairTextCtrls()
{
	m_ParentXPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.x + m_TextCtrlPixelOffset)));
	m_ParentYPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.y + m_TextCtrlPixelOffset)));
}

wxStockCursor CrossHairTool::UpdateCursor(const wxStockCursor& current_cursor)
{
	CheckIfMouseAboveCrossHair();
	auto edited_cursor = current_cursor;
	if (m_ToolButtonActive)
	{
		edited_cursor = m_SelectingPositionFromParentWindow ? wxCURSOR_QUESTION_ARROW : edited_cursor;
	}
	//if (m_MouseHoverOverImage)
	//{
		//edited_cursor = m_CursorAboveVerticalLine && m_ShowVerticalLine ? wxCURSOR_SIZEWE : edited_cursor;
		//edited_cursor = m_CursorAboveHorizontalLine && m_ShowHorizontalLine ? wxCURSOR_SIZENS : edited_cursor;
		//edited_cursor = m_CursorAboveHorizontalLine && m_CursorAboveVerticalLine && m_ShowVerticalLine && m_ShowHorizontalLine ? wxCURSOR_SIZING : edited_cursor;
		//edited_cursor = m_ChangingHorizontalLine || m_ChangingVerticalLine ? wxCURSOR_CLOSED_HAND : edited_cursor;
	//}
	return edited_cursor;
}

void CrossHairTool::DrawCrossHair(wxGraphicsContext* graphics_context_, unsigned short* data_)
{
	wxGraphicsPath path = graphics_context_->CreatePath();	
	
	/* Vertical line */
	if (m_ShowVerticalLine)
	{
		path.MoveToPoint
		(
			m_CrossHairOnCanvas.x + m_ImageStartDraw.x + m_ActualHalfPixelSize.x, 
			m_ImageStartDraw.y
		);
		path.AddLineToPoint
		(
			m_CrossHairOnCanvas.x + m_ImageStartDraw.x + m_ActualHalfPixelSize.x, 
			m_ImageStartDraw.y + (double)m_ActualZoomedSizeOfImageOnCanvas.y
		);
	}

	/* Horizontal line */
	if (m_ShowHorizontalLine)
	{
		path.MoveToPoint
		(
			m_ImageStartDraw.x, 
			m_ImageStartDraw.y + m_CrossHairOnCanvas.y + m_ActualHalfPixelSize.y 
		);
		path.AddLineToPoint
		(
			m_ImageStartDraw.x + (double)m_ActualZoomedSizeOfImageOnCanvas.x, 
			m_ImageStartDraw.y + m_CrossHairOnCanvas.y + m_ActualHalfPixelSize.y
		);
	}

	graphics_context_->StrokePath(path);
	DrawData(graphics_context_, data_);
}

bool CrossHairTool::PositionCanBeChanged() const
{
	return (m_ShowHorizontalLine && m_CursorAboveHorizontalLine) || (m_ShowVerticalLine && m_CursorAboveVerticalLine);
}

bool CrossHairTool::CanProcessPanning() const
{
	return (m_ShowHorizontalLine || m_ShowVerticalLine) && (!m_CursorAboveHorizontalLine && !m_CursorAboveVerticalLine);
}

void CrossHairTool::StopChangingCrossHairPos()
{
	m_ChangingHorizontalLine = m_ChangingVerticalLine = false;
}

bool CrossHairTool::GetShowVerticalLine() const
{
	return m_ShowVerticalLine;
}

int CrossHairTool::GetXPos() const
{
	return m_CrossHairOnImage.x;
}

bool CrossHairTool::GetShowHorizontalLine() const
{
	return m_ShowHorizontalLine;
}

int CrossHairTool::GetYPos() const
{
	return m_CrossHairOnImage.y;
}

auto CrossHairTool::ActivateSetPositionFromParentWindow(bool activate) -> void
{
	m_SelectingPositionFromParentWindow = activate;
}

auto CrossHairTool::SetAveragingWidthPX(const int& avgWidth) -> void
{
	m_AveragingWidthPX = std::clamp(avgWidth, 1, 10'000);
}

void CrossHairTool::ProcessChangingCrossHairPos()
{
	m_ChangingHorizontalLine = m_ChangingVerticalLine = false;
	if (m_CursorAboveHorizontalLine && m_ShowHorizontalLine && !m_CursorAboveVerticalLine)
	{
		m_ChangingHorizontalLine = true;
	}
	else if (m_CursorAboveVerticalLine && m_ShowVerticalLine && !m_CursorAboveHorizontalLine)
	{
		m_ChangingVerticalLine = true;
	}
	else if (m_CursorAboveHorizontalLine && m_CursorAboveVerticalLine && m_ShowHorizontalLine && m_ShowVerticalLine)
	{
		m_ChangingHorizontalLine = m_ChangingVerticalLine = true;
	}

	LoopChangingCrossHairPos();
}

void CrossHairTool::DrawData(wxGraphicsContext* graphics_context, uint16_t* data)
{
	int offset_from_line{ 5 }; // start draw data curve above horizontal line from this value
	int data_preview_height{ (int)(m_ImageSize.GetHeight() / m_ZoomOriginalSizeImage * 0.2) }, data_preview_width{ data_preview_height }; // equal to max of uint16_t 65'535
	//uint16_t max_uint16_t{ 65535 };
	uint16_t max_value = m_DataType == ToolsVariables::DATA_U12 ? 4095 : 65535;

	if (m_ShowDataOnVerticalLine)
	{
		/* Creating curve for vertical line */
		DrawDataOnVerticalLine(graphics_context, data, offset_from_line, data_preview_width, max_value);
	}
	if (m_ShowDataOnHorizontalLine)
	{
		/* Creating curve for horizontal line */
		DrawDataOnHorizontalLine(graphics_context, data, offset_from_line, data_preview_height, max_value);
	}
}

void CrossHairTool::DrawDataOnHorizontalLine(wxGraphicsContext* gc, uint16_t* data_, const int& curve_y_offset, const int& max_height, const uint16_t& max_value)
{
	wxGraphicsPath path = gc->CreatePath();
	int current_y_position_on_image = m_CrossHairOnImage.y;
	int start_draw_y_position{};
	double current_x{}, delta_x{}, current_y{};

	start_draw_y_position = m_CrossHairOnCanvas.y > (double)max_height ?
		m_CrossHairOnCanvas.y + m_ImageStartDraw.y - curve_y_offset : 
		m_CrossHairOnCanvas.y + m_ImageStartDraw.y + curve_y_offset + max_height;

	delta_x = m_ActualHalfPixelSize.x * 2.0;
	current_x = m_ImageStartDraw.x + m_ActualHalfPixelSize.x;

	uint64_t y_position_in_data = (uint64_t)current_y_position_on_image * m_ImageSize.GetWidth();
	for (auto x{ 0 }; x < m_ImageSize.GetWidth() - 1; ++x)
	{
		current_y = max_height * (double)data_[y_position_in_data + x] / (double)max_value;
		path.MoveToPoint(current_x, (double)start_draw_y_position - current_y);
		current_x += delta_x;
		current_y = max_height * (double)data_[y_position_in_data + x + 1] / (double)max_value;
		path.AddLineToPoint(current_x, (double)start_draw_y_position - current_y);
	}

	gc->SetPen(*wxGREEN_PEN);
	gc->DrawPath(path);
}

auto CrossHairTool::DrawDataOnVerticalLine
(
	wxGraphicsContext* gc, 
	uint16_t* image_data, 
	const int& curve_width_px, 
	const int& curve_max_width, 
	const uint16_t& fixed_max_value
) -> void
{
	wxGraphicsPath path = gc->CreatePath();
	const int crosshair_image_x = m_CrossHairOnImage.x;
	const int image_width = m_ImageSize.GetWidth();
	const int image_height = m_ImageSize.GetHeight();

	// Calculate start position of drawing in X based on whether the curve is to the right or left of the crosshair
	const int draw_start_x = m_CrossHairOnCanvas.x > static_cast<double>(curve_max_width)
		? m_CrossHairOnCanvas.x + m_ImageStartDraw.x - static_cast<double>(curve_width_px)
		: m_CrossHairOnCanvas.x + m_ImageStartDraw.x + static_cast<double>(curve_width_px) + static_cast<double>(curve_max_width);

	const double pixel_step_y = m_ActualHalfPixelSize.y * 2.0;
	double current_draw_y = m_ImageStartDraw.y + m_ActualHalfPixelSize.y;

	// --- Averaging logic parameters ---
	const int half_avg_width = m_AveragingWidthPX / 2;
	const int avg_start_x = std::max(0, crosshair_image_x - half_avg_width);
	const int avg_end_x = std::min(image_width - 1, crosshair_image_x + half_avg_width);
	const int avg_width = avg_end_x - avg_start_x + 1;

	std::vector<double> averaged_values(image_height, 0.0);
	double adaptive_min = std::numeric_limits<double>::max();
	double adaptive_max = std::numeric_limits<double>::lowest();

	// Compute horizontal averaging for each vertical position (y)
	for (int y = 0; y < image_height; ++y)
	{
		double sum = 0.0;
		for (int x = avg_start_x; x <= avg_end_x; ++x)
		{
			sum += image_data[y * image_width + x];
		}
		double avg_value = sum / avg_width;
		averaged_values[y] = avg_value;

		if (m_AdaptiveScaling)
		{
			if (avg_value < adaptive_min) adaptive_min = avg_value;
			if (avg_value > adaptive_max) adaptive_max = avg_value;
		}
	}

	// Compute vertical line path
	double value_range = m_AdaptiveScaling ? (adaptive_max - adaptive_min) : static_cast<double>(fixed_max_value);
	if (value_range == 0) value_range = 1.0; // avoid division by zero

	for (int y = 0; y < image_height - 1; ++y)
	{
		double scaled_value_current = (averaged_values[y] - (m_AdaptiveScaling ? adaptive_min : 0.0)) / value_range;
		double x_offset_current = curve_max_width * scaled_value_current;

		path.MoveToPoint(draw_start_x - x_offset_current, current_draw_y);
		current_draw_y += pixel_step_y;

		double scaled_value_next = (averaged_values[y + 1] - (m_AdaptiveScaling ? adaptive_min : 0.0)) / value_range;
		double x_offset_next = curve_max_width * scaled_value_next;

		path.AddLineToPoint(draw_start_x - x_offset_next, current_draw_y);
	}

	gc->SetPen(wxColour(240, 134, 80));
	gc->DrawPath(path);
}

void CrossHairTool::UpdateParentCrossHairTextCtrlsWithRefresh()
{
	CalculateCrossHairOnImage();
	/* Here should be SetValue to initiate Refresh() from parent window */
	m_ParentXPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.x + m_TextCtrlPixelOffset)));
	m_ParentYPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.y + m_TextCtrlPixelOffset)));
}

auto CrossHairTool::CheckIfPixelValueIsInsideTheImage(const int& x, const int& y) -> bool
{
	if (x < 0 || x >= m_ImageSize.GetWidth()) return false;
	if (y < 0 || y >= m_ImageSize.GetHeight()) return false;
	return true;
}

void CrossHairTool::LoopChangingCrossHairPos()
{
	if (m_ChangingHorizontalLine && !m_ChangingVerticalLine)
	{
		m_CrossHairOnCanvas = 
		{
			m_CrossHairOnCanvas.x, 
			m_CursorOnCanvas.y - m_ImageStartDraw.y
		};
	}
	else if (!m_ChangingHorizontalLine && m_ChangingVerticalLine)
	{
		m_CrossHairOnCanvas = 
		{
			m_CursorOnCanvas.x - m_ImageStartDraw.x,
			m_CrossHairOnCanvas.y
		};
	}
	else if (m_ChangingHorizontalLine && m_ChangingVerticalLine)
	{
		m_CrossHairOnCanvas = 
		{
			m_CursorOnCanvas.x - m_ImageStartDraw.x,
			m_CursorOnCanvas.y - m_ImageStartDraw.y
		};
	}
	//UpdateParentCrossHairTextCtrlsWithRefresh();
}

/* _____ Rectangle Tool _____ */

RectangleSelectionTool::RectangleSelectionTool(
	wxTextCtrl* parent_start_x_pos, 
	wxTextCtrl* parent_start_y_pos, 
	wxTextCtrl* parent_width, 
	wxTextCtrl* parent_height) 
	: m_ParentStartXPosTextCtrl(parent_start_x_pos), 
	m_ParentStartYPosTextCtrl(parent_start_y_pos), 
	m_ParentWidthTextCtrl(parent_width), 
	m_ParentHeightTextCtrl(parent_height)
{
}

wxStockCursor RectangleSelectionTool::UpdateCursor(const wxStockCursor& current_cursor)
{
	auto edited_cursor = current_cursor;
	if (m_MouseHoverOverImage && m_ToolButtonActive)
	{
		edited_cursor = m_CursorOutsideOfRectangleSelection ? wxCURSOR_CROSS : edited_cursor;
		edited_cursor = m_CursorInsideRectangleSelection ? wxCURSOR_SIZING : edited_cursor;
		edited_cursor = m_CursorAboveVerticalLeftLineMiddlePoint || m_CursorAboveVerticalRightLineMiddlePoint ? wxCURSOR_SIZEWE : edited_cursor;
		edited_cursor = m_CursorAboveHorizontalUpperLineMiddlePoint || m_CursorAboveHorizontalBottomLineMiddlePoint ? wxCURSOR_SIZENS : edited_cursor;
		edited_cursor = m_CursorAboveLeftUpperCornerPoint || m_CursorAboveRightBottomCornerPoint ? wxCURSOR_SIZENWSE : edited_cursor;
		edited_cursor = m_CursorAboveRightUpperCornerPoint || m_CursorAboveLeftBottomCornerPoint ? wxCURSOR_SIZENESW : edited_cursor;
		edited_cursor = m_UserChangingSizeSelectedRectangleSelection ? wxCURSOR_CLOSED_HAND : edited_cursor;
#if 0
		/* User Selecting or Changing Position/Size of Rectangle Selection */
		edited_cursor = m_ChangingLUPointRectangleSelection || m_ChangingRUPointRectangleSelection || m_ChangingRBPointRectangleSelection || m_ChangingLBPointRectangleSelection ? wxCURSOR_CLOSED_HAND : current_cursor;
#endif
		
		edited_cursor = m_UserSelectingAreaOfRectangle ? wxCURSOR_CROSS : edited_cursor;
		edited_cursor = m_UserPanningSelectedRectangleSelection ? wxCURSOR_CLOSED_HAND : edited_cursor;
	}
	return edited_cursor;
}

void RectangleSelectionTool::UpdateZoomValue(const double& zoom)
{
	m_Zoom = zoom;

	m_ActualPixelSize = { m_Zoom / m_ZoomOriginalSizeImage, m_Zoom / m_ZoomOriginalSizeImage };
}

void RectangleSelectionTool::SetStartXPosFromParent(const int& x_pos)
{
	m_RectangleSelectionOnImage.SetLeft(x_pos);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::SetStartYPosFromParent(const int& y_pos)
{
	m_RectangleSelectionOnImage.SetTop(y_pos);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::SetWidthFromParent(const int& width)
{
	m_RectangleSelectionOnImage.SetWidth(width);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::SetHeightFromParent(const int& height)
{
	m_RectangleSelectionOnImage.SetHeight(height);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::UpdateParentRectangleTextCtrls()
{
	m_ParentStartXPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetLeft() + m_TextCtrlPixelOffset)));
	m_ParentStartYPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetTop() + m_TextCtrlPixelOffset)));
	m_ParentWidthTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetWidth())));
	m_ParentHeightTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetHeight())));
}

void RectangleSelectionTool::DrawRectangleSelection(wxGraphicsContext* graphics_context)
{
	if (!m_ToolButtonActive) return;

	wxGraphicsPath path = graphics_context->CreatePath();

	wxRealPoint start_draw_rectangle{}, finish_draw_rectangle{};
	wxDouble rect_width{}, rect_height{};

	{
		start_draw_rectangle.x = m_RectangleSelectionOnCanvas.GetRight() > m_RectangleSelectionOnCanvas.GetLeft() ? m_RectangleSelectionOnCanvas.GetLeft() : m_RectangleSelectionOnCanvas.GetRight();
		start_draw_rectangle.y = m_RectangleSelectionOnCanvas.GetBottom() > m_RectangleSelectionOnCanvas.GetTop() ? m_RectangleSelectionOnCanvas.GetTop() : m_RectangleSelectionOnCanvas.GetBottom();
		finish_draw_rectangle.x = m_RectangleSelectionOnCanvas.GetRight() > m_RectangleSelectionOnCanvas.GetLeft() ? m_RectangleSelectionOnCanvas.GetRight() : m_RectangleSelectionOnCanvas.GetLeft();
		finish_draw_rectangle.y = m_RectangleSelectionOnCanvas.GetBottom() > m_RectangleSelectionOnCanvas.GetTop() ? m_RectangleSelectionOnCanvas.GetBottom() : m_RectangleSelectionOnCanvas.GetTop();

		start_draw_rectangle.x += m_ImageStartDraw.x;
		start_draw_rectangle.y += m_ImageStartDraw.y;
		finish_draw_rectangle.x += m_ImageStartDraw.x;
		finish_draw_rectangle.y += m_ImageStartDraw.y;
	}

	{
		rect_width = abs(finish_draw_rectangle.x - start_draw_rectangle.x); 
		rect_height = abs(finish_draw_rectangle.y - start_draw_rectangle.y);

		rect_width += m_ActualPixelSize.x;
		rect_height += m_ActualPixelSize.y;

		rect_width = m_Zoom / m_ZoomOriginalSizeImage > 1.0 ? rect_width - 1.0 : rect_width;
		rect_height = m_Zoom / m_ZoomOriginalSizeImage > 1.0 ? rect_height - 1.0 : rect_height;
	}

	if (rect_width <= 0.0 || rect_height <= 0.0) return;

	path.AddRectangle(start_draw_rectangle.x, start_draw_rectangle.y, rect_width, rect_height);

	graphics_context->DrawPath(path);

	DrawRectangleSelectionPoints(graphics_context);
}

void RectangleSelectionTool::DrawRectangleSelectionPoints(wxGraphicsContext* gc)
{
	if (!m_DrawHorizontalPoints || !m_DrawVerticalPoints) return;

	wxGraphicsPath path = gc->CreatePath();
	gc->SetPen(*wxBLACK_PEN);
	gc->SetBrush(*wxWHITE_BRUSH);

	wxRealPoint start_draw_point = {};
	/* Left Upper Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Right Upper Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetRight() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Right Bottom Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetRight() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetBottom() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}	
	/* Left Bottom Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetBottom() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Left Vertical Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Upper Horizontal Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Right Vertical Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetRight() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Bottom Horizontal Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetBottom() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	gc->DrawPath(path);

}

void RectangleSelectionTool::SetCursorPosOnImage(const wxRealPoint& cursor_pos_on_image)
{
	m_CursorOnImage = cursor_pos_on_image;
	CheckIfMouseAboveRectangleSelection();
}

bool RectangleSelectionTool::CanSelectRegion() const
{
	return m_ToolButtonActive && m_MouseHoverOverImage && m_CursorOutsideOfRectangleSelection;
}

void RectangleSelectionTool::ProcessSelectingRegion()
{
	m_UserSelectingAreaOfRectangle = true;
	m_StartSelectingAreaPoint  = 
	{ 
		m_CursorOnCanvas.x - m_ImageStartDraw.x, 
		m_CursorOnCanvas.y - m_ImageStartDraw.y 
	};
	m_RectangleSelectionOnCanvas.SetLeftTop(m_StartSelectingAreaPoint);
	m_RectangleSelectionOnCanvas.SetRightBottom(m_StartSelectingAreaPoint);
	LoopSelectingRegion();
}

void RectangleSelectionTool::LoopSelectingRegion()
{
	ChangeLUorRBRectanglePoints();
	CalculateRectangleSelectionPosOnImage();
	UpdateParentRectangleTextCtrlsWithRefresh();
}

void RectangleSelectionTool::ChangeLUorRBRectanglePoints()
{
	if (!m_MouseHoverOverImage) return;

	wxRealPoint temp_point = 
	{ 
		m_CursorOnCanvas.x - m_ImageStartDraw.x, 
		m_CursorOnCanvas.y - m_ImageStartDraw.y 
	};

	/* I quadrant (right upper) */
	if (
		temp_point.x > m_StartSelectingAreaPoint.m_x && 
		temp_point.y <= m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetRight(temp_point.x);
		m_RectangleSelectionOnCanvas.SetTop(temp_point.y);
		m_RectangleSelectionOnCanvas.SetBottom(m_StartSelectingAreaPoint.m_y);
	}
	/* II quadrant (left upper) */
	else if (
		temp_point.x <= m_StartSelectingAreaPoint.m_x && 
		temp_point.y <= m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(temp_point.x);
		m_RectangleSelectionOnCanvas.SetRight(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetTop(temp_point.y);
		m_RectangleSelectionOnCanvas.SetBottom(m_StartSelectingAreaPoint.m_y);
	}
	/* III quadrant (left bottom) */
	else if (
		temp_point.x <= m_StartSelectingAreaPoint.m_x && 
		temp_point.y > m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(temp_point.x);
		m_RectangleSelectionOnCanvas.SetRight(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetTop(m_StartSelectingAreaPoint.m_y);
		m_RectangleSelectionOnCanvas.SetBottom(temp_point.y);
	}
	else if (
		temp_point.x > m_StartSelectingAreaPoint.m_x &&
		temp_point.y > m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetRight(temp_point.x);
		m_RectangleSelectionOnCanvas.SetTop(m_StartSelectingAreaPoint.m_y);
		m_RectangleSelectionOnCanvas.SetBottom(temp_point.y);
	}
}

void RectangleSelectionTool::StopSelectingRegion()
{
	m_UserSelectingAreaOfRectangle = false;
}

bool RectangleSelectionTool::CanPanSelectedRegion() const
{
	return m_ToolButtonActive && m_MouseHoverOverImage && m_CursorInsideRectangleSelection;
}

void RectangleSelectionTool::ProcessPanningSelectedRegion()
{
	m_UserPanningSelectedRectangleSelection = true;
	m_StartPanRectangleSelection = m_RectangleSelectionOnImage.GetLeftTop();
	m_StartPanSizeCursorPos = m_CursorOnImage;
	LoopPanningSelectedRegion();
}

void RectangleSelectionTool::LoopPanningSelectedRegion()
{
	m_DeltaPoint =
	{
		(int)(m_CursorOnImage.x - m_StartPanSizeCursorPos.x),
		(int)(m_CursorOnImage.y - m_StartPanSizeCursorPos.y)
	};
	m_RectangleSelectionOnImage.SetLeft(m_StartPanRectangleSelection.x + m_DeltaPoint.x >= 0 ? m_StartPanRectangleSelection.x + m_DeltaPoint.x : 0);
	m_RectangleSelectionOnImage.SetLeft(m_StartPanRectangleSelection.x + m_DeltaPoint.x + m_RectangleSelectionOnImage.GetWidth() < m_ImageSize.GetWidth() ? m_RectangleSelectionOnImage.GetLeft() : m_ImageSize.GetWidth() - m_RectangleSelectionOnImage.GetWidth());
	m_RectangleSelectionOnImage.SetTop(m_StartPanRectangleSelection.y + m_DeltaPoint.y >= 0 ? m_StartPanRectangleSelection.y + m_DeltaPoint.y : 0);
	m_RectangleSelectionOnImage.SetTop(m_StartPanRectangleSelection.y + m_DeltaPoint.y + m_RectangleSelectionOnImage.GetHeight() < m_ImageSize.GetHeight() ? m_RectangleSelectionOnImage.GetTop() : m_ImageSize.GetHeight() - m_RectangleSelectionOnImage.GetHeight());

	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrlsWithRefresh();
}

void RectangleSelectionTool::StopPanningSelectedRegion()
{
	m_UserPanningSelectedRectangleSelection = false;
}

bool RectangleSelectionTool::CanChangeSize() const
{
	return m_MouseHoverOverImage && m_ToolButtonActive &&
		(
			m_CursorAboveVerticalLeftLineMiddlePoint || 
			m_CursorAboveLeftUpperCornerPoint || 
			m_CursorAboveHorizontalUpperLineMiddlePoint || 
			m_CursorAboveRightUpperCornerPoint || 
			m_CursorAboveVerticalRightLineMiddlePoint || 
			m_CursorAboveRightBottomCornerPoint || 
			m_CursorAboveHorizontalBottomLineMiddlePoint || 
			m_CursorAboveLeftBottomCornerPoint || 
			m_CursorAboveVerticalLeftLineMiddlePoint
			);
}

void RectangleSelectionTool::ProcessChangingSizeSelectedRegion()
{
	m_UserChangingSizeSelectedRectangleSelection = true;
	m_ChangingLMPoint = m_ChangingUMPoint = m_ChangingRMPoint = m_ChangingBMPoint = false;
	m_ChangingLUPoint = m_ChangingRUPoint = m_ChangingRBPoint = m_ChangingLBPoint = false;

	if (m_CursorAboveVerticalLeftLineMiddlePoint) m_ChangingLMPoint = true;
	else if (m_CursorAboveLeftUpperCornerPoint) m_ChangingLUPoint = true;
	else if (m_CursorAboveHorizontalUpperLineMiddlePoint) m_ChangingUMPoint = true;
	else if (m_CursorAboveRightUpperCornerPoint) m_ChangingRUPoint = true;
	else if (m_CursorAboveVerticalRightLineMiddlePoint) m_ChangingRMPoint = true;
	else if (m_CursorAboveRightBottomCornerPoint) m_ChangingRBPoint = true;
	else if (m_CursorAboveHorizontalBottomLineMiddlePoint) m_ChangingBMPoint = true;
	else if (m_CursorAboveLeftBottomCornerPoint) m_ChangingLBPoint = true;

	m_StartSizingRectangleSelection = m_RectangleSelectionOnImage;
	//m_StartPanRectangleSelection = m_RectangleSelectionOnImage.GetLeftTop();
	m_StartPanSizeCursorPos = m_CursorOnImage;
	LoopChangingSizeSelectedRegion();
}

void RectangleSelectionTool::LoopChangingSizeSelectedRegion()
{
	m_DeltaPoint =
	{
		(int)(m_CursorOnImage.x - m_StartPanSizeCursorPos.x),
		(int)(m_CursorOnImage.y - m_StartPanSizeCursorPos.y)
	};
	if (m_ChangingLMPoint)
	{
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight());
	}
	else if (m_ChangingLUPoint)
	{
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetRightBottom(m_StartSizingRectangleSelection.GetRightBottom());
	}
	else if (m_ChangingUMPoint)
	{
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom());
	}
	else if (m_ChangingRUPoint)
	{
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetLeftBottom(m_StartSizingRectangleSelection.GetLeftBottom());
	}
	else if (m_ChangingRMPoint)
	{
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft());
	}
	else if (m_ChangingRBPoint)
	{
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetLeftTop(m_StartSizingRectangleSelection.GetLeftTop());
	}
	else if (m_ChangingBMPoint)
	{
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop());
	}
	else if (m_ChangingLBPoint)
	{
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetRightTop(m_StartSizingRectangleSelection.GetRightTop());
	}

	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrlsWithRefresh();
}

void RectangleSelectionTool::StopChangingSizeSelectedRegion()
{
	m_UserChangingSizeSelectedRectangleSelection = false;
}

void RectangleSelectionTool::UpdateParentRectangleTextCtrlsWithRefresh()
{
	m_ParentStartXPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetLeft() + m_TextCtrlPixelOffset)));
	m_ParentStartYPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetTop() + m_TextCtrlPixelOffset)));
	m_ParentWidthTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetWidth())));
	m_ParentHeightTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetHeight())));
}

void RectangleSelectionTool::CheckIfMouseAboveRectangleSelection()
{
	if (m_MouseHoverOverImage)
	{
		/* Reset values */
		m_CursorInsideRectangleSelection = false;
		m_CursorOutsideOfRectangleSelection = true;
		m_CursorAboveVerticalLeftLineMiddlePoint = m_CursorAboveHorizontalUpperLineMiddlePoint = m_CursorAboveVerticalRightLineMiddlePoint = m_CursorAboveHorizontalBottomLineMiddlePoint = false;
		m_CursorAboveLeftUpperCornerPoint = m_CursorAboveRightUpperCornerPoint = m_CursorAboveRightBottomCornerPoint = m_CursorAboveLeftBottomCornerPoint = false;;


		/* Left Column */
		if (
			m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() - m_PointSize.GetWidth() / 2.0 &&
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() + m_PointSize.GetWidth() / 2.0)
		{
			/* Left Upper Point */
			if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveLeftUpperCornerPoint = true;
			}
			/* Left Middle Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveVerticalLeftLineMiddlePoint = true;
			}
			/* Left Bottom Point */
			else if (m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveLeftBottomCornerPoint = true;
			}
			m_CursorOutsideOfRectangleSelection = false;
		}
		/* Middle Column */
		else if (
			m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 - m_PointSize.GetWidth() / 2.0 &&
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 + m_PointSize.GetWidth() / 2.0)
		{
			/* Upper Middle Point */
			if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveHorizontalUpperLineMiddlePoint = true;
			}
			/* Lower Middle Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveHorizontalBottomLineMiddlePoint = true;
			}
			m_CursorOutsideOfRectangleSelection = false;
		}
		/* Right Column */
		else if (
			m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetRight() - m_PointSize.GetWidth() / 2.0 &&
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetRight() + m_PointSize.GetWidth() / 2.0)
		{
			/* Upper Right Point */
			if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveRightUpperCornerPoint = true;
			}
			/* Middle Right Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveVerticalRightLineMiddlePoint = true;
			}
			/* Bottom Right Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveRightBottomCornerPoint = true;
			}
			m_CursorOutsideOfRectangleSelection = false;
		}

		if (m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() && 
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetRight() &&
			m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() && 
			m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom())
		{
			if (
				!m_CursorAboveVerticalLeftLineMiddlePoint &&
				!m_CursorAboveLeftUpperCornerPoint &&
				!m_CursorAboveHorizontalUpperLineMiddlePoint &&
				!m_CursorAboveRightUpperCornerPoint &&
				!m_CursorAboveVerticalRightLineMiddlePoint &&
				!m_CursorAboveRightBottomCornerPoint &&
				!m_CursorAboveHorizontalBottomLineMiddlePoint &&
				!m_CursorAboveLeftBottomCornerPoint)
			{
				m_CursorInsideRectangleSelection = true;
				m_CursorOutsideOfRectangleSelection = false;
			}
		}

	}
}

void RectangleSelectionTool::CalculateRectangleSelectionPosOnCanvas()
{
	double multiplication_coefficient = m_Zoom / m_ZoomOriginalSizeImage;
	m_RectangleSelectionOnCanvas.SetLeft(m_RectangleSelectionOnImage.GetLeft() * multiplication_coefficient);
	m_RectangleSelectionOnCanvas.SetTop(m_RectangleSelectionOnImage.GetTop() * multiplication_coefficient);
	m_RectangleSelectionOnCanvas.SetRight(m_RectangleSelectionOnImage.GetRight() * multiplication_coefficient);
	m_RectangleSelectionOnCanvas.SetBottom(m_RectangleSelectionOnImage.GetBottom() * multiplication_coefficient);


	{
		m_VisualRectangleSelectionOnCanvas.SetLeftTop(m_RectangleSelectionOnCanvas.GetLeftTop());
		m_VisualRectangleSelectionOnCanvas.SetRight(m_RectangleSelectionOnCanvas.GetRight() + m_ActualPixelSize.x);
		m_VisualRectangleSelectionOnCanvas.SetBottom(m_RectangleSelectionOnCanvas.GetBottom() + m_ActualPixelSize.y);

		if (m_Zoom / m_ZoomOriginalSizeImage > 1.0)
		{
			m_VisualRectangleSelectionOnCanvas.SetRight(m_VisualRectangleSelectionOnCanvas.GetRight() - 1.0);
			m_VisualRectangleSelectionOnCanvas.SetBottom(m_VisualRectangleSelectionOnCanvas.GetBottom() - 1.0);
		}

		/* Checking if it's necessary to draw points on the edges of the rectangle */
		m_DrawHorizontalPoints = m_DrawVerticalPoints = false;
		if (m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() > 2 * m_PointSize.GetWidth())
			m_DrawHorizontalPoints = true;
		if (m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() > 2 * m_PointSize.GetHeight())
			m_DrawVerticalPoints = true;
	}
}

void RectangleSelectionTool::CalculateRectangleSelectionPosOnImage()
{

	double multiplication_factor = m_ZoomOriginalSizeImage / m_Zoom;

	m_RectangleSelectionOnImage.SetLeft(m_RectangleSelectionOnCanvas.GetLeft() * multiplication_factor);
	m_RectangleSelectionOnImage.SetTop(m_RectangleSelectionOnCanvas.GetTop() * multiplication_factor);
	m_RectangleSelectionOnImage.SetRight(m_RectangleSelectionOnCanvas.GetRight() * multiplication_factor);
	m_RectangleSelectionOnImage.SetBottom(m_RectangleSelectionOnCanvas.GetBottom() * multiplication_factor);

	/* Checking Left Line */
	m_RectangleSelectionOnImage.SetLeft(m_RectangleSelectionOnImage.GetLeft() >= 0 ? m_RectangleSelectionOnImage.GetLeft() : 0);
	m_RectangleSelectionOnImage.SetLeft(m_RectangleSelectionOnImage.GetLeft() < m_ImageSize.GetWidth() ? m_RectangleSelectionOnImage.GetLeft() : m_ImageSize.GetWidth() - 1);
	/* Checking Right Line */
	m_RectangleSelectionOnImage.SetRight(m_RectangleSelectionOnImage.GetRight() >= 0 ? m_RectangleSelectionOnImage.GetRight() : 0);
	m_RectangleSelectionOnImage.SetRight(m_RectangleSelectionOnImage.GetRight() < m_ImageSize.GetWidth() ? m_RectangleSelectionOnImage.GetRight() : m_ImageSize.GetWidth() - 1);
	/* Checking Upper Line */
	m_RectangleSelectionOnImage.SetTop(m_RectangleSelectionOnImage.GetTop() >= 0 ? m_RectangleSelectionOnImage.GetTop() : 0);
	m_RectangleSelectionOnImage.SetTop(m_RectangleSelectionOnImage.GetTop() < m_ImageSize.GetHeight() ? m_RectangleSelectionOnImage.GetTop() : m_ImageSize.GetHeight() - 1);
	/* Checking Bottom Line */
	m_RectangleSelectionOnImage.SetBottom(m_RectangleSelectionOnImage.GetBottom() >= 0 ? m_RectangleSelectionOnImage.GetBottom() : 0);
	m_RectangleSelectionOnImage.SetBottom(m_RectangleSelectionOnImage.GetBottom() < m_ImageSize.GetHeight() ? m_RectangleSelectionOnImage.GetBottom() : m_ImageSize.GetHeight() - 1);
}
