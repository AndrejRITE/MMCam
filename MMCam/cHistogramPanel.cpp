#include "cHistogramPanel.h"

BEGIN_EVENT_TABLE(cHistogramPanel, wxPanel)
	EVT_PAINT(cHistogramPanel::PaintEvent)
	EVT_SIZE(cHistogramPanel::OnSize)
	EVT_MOTION(cHistogramPanel::OnMouseMoved)
	EVT_LEFT_DOWN(cHistogramPanel::OnPreviewMouseLeftPressed)
	EVT_LEFT_DCLICK(cHistogramPanel::OnPreviewMouseLeftDoubleClick)
	EVT_LEFT_UP(cHistogramPanel::OnPreviewMouseLeftReleased)
	EVT_ENTER_WINDOW(cHistogramPanel::OnPreviewMouseEnteredWindow)
	EVT_LEAVE_WINDOW(cHistogramPanel::OnPreviewMouseLeftWindow)
END_EVENT_TABLE()

cHistogramPanel::cHistogramPanel
(
	wxWindow* parent_frame,
	wxSizer* parent_sizer,
	wxTextCtrl* parent_black_txt_ctrl,
	wxTextCtrl* parent_white_txt_ctrl,
	const int borderSize,
	const wxString title
) : 
	wxPanel(parent_frame),
	m_ParentLeftBorder(parent_black_txt_ctrl), 
	m_ParentRightBorder(parent_white_txt_ctrl),
	m_TitleStr(title)
{
	SetDoubleBuffered(true);

	SetBackgroundColour(m_BackgroundColour);

	parent_sizer->Add(this, 1, wxEXPAND | wxLEFT | wxRIGHT, borderSize);

	SetMinSize(wxSize(200, 100));

	InitDefaultComponents();
}

auto cHistogramPanel::SetHistogram
(
	const HistogramPanelVariables::ImageDataTypes data_type, 
	unsigned long long* data, 
	unsigned long long min_value, 
	unsigned long long max_value
) -> std::optional<int>
{
	if (!m_CanvasSize.GetWidth() || !m_CanvasSize.GetHeight()) return std::optional<int>(1);

	m_DataType = data_type;
	m_HistogramData.reset(data);
	MedianBlur1D
	(
		m_HistogramData.get(), 
		m_DataType == HistogramPanelVariables::ImageDataTypes::RAW_12BIT ? 4'095 : USHRT_MAX, 
		5
	);

	// Find Global Minimum and Maximum values inside the Histogram values
	auto minMaxElement = std::minmax_element(m_HistogramData.get(), m_HistogramData.get() + USHRT_MAX + 1);

	m_GlobalMin = *minMaxElement.first;
	m_GlobalMax = *minMaxElement.second;

	m_AutoLeftBorder.x = min_value;
	m_AutoRightBorder.x = max_value;

	m_MaxHistogramValue = m_DataType == HistogramPanelVariables::ImageDataTypes::RAW_12BIT ? 4'095UL : 65'535UL;

	SetWXImage();

	m_LeftBorder = m_AutoLeftBorder;
	m_RightBorder = m_AutoRightBorder;
	m_WasRangeChanged = false;

	return std::optional<int>();
}

auto cHistogramPanel::SetBackgroundColor(wxColour bckg_colour) -> void
{
	SetBackgroundColour(bckg_colour);
	m_BackgroundColour = bckg_colour;
	SetWXImage();
}

auto cHistogramPanel::SetWXImage() -> void
{
	if (!m_HistogramData) return;

	m_Image = wxImage(m_CanvasSize);
	m_ImageSize = m_CanvasSize;
	
	//m_Image.InitAlpha();
	
	m_Image.Clear(m_BackgroundColour.GetRed());

	auto histColour = wxColour(180, 29, 47);
	int curr_x{}, curr_y{};

	auto maxHistogramValue = static_cast<double>(m_MaxHistogramValue);
	auto globalMax = static_cast<double>(m_GlobalMax);

	for (auto i{ 0 }; i <= (int)m_MaxHistogramValue; ++i)
	{
		curr_y = (m_CanvasSize.GetHeight() - 1) * m_HistogramData[i] / globalMax;
		if (curr_y <= 0) continue;
		
		curr_x = (m_CanvasSize.GetWidth() - 1) * i / maxHistogramValue;
		
		m_Image.SetRGB
		(
			wxRect
			(
				wxPoint(curr_x, m_CanvasSize.GetHeight() - curr_y),
				wxSize(1, curr_y)
			),
			histColour.GetRed(),
			histColour.GetGreen(),
			histColour.GetBlue()
		);
	}

	m_IsImageSet = true;
	m_IsGraphicsBitmapSet = false;

	Refresh();
}

void cHistogramPanel::InitDefaultComponents()
{
}

void cHistogramPanel::PaintEvent(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	Render(dc);
}

void cHistogramPanel::Render(wxBufferedPaintDC& dc)
{	
	dc.SetBackground(wxBrush(m_BackgroundColour));
	dc.Clear();
	//wxGraphicsContext* gc_image{};

	if (!m_Image.IsOk()) return;

	// Image
	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc) return;

		DrawImage(gc);
		delete gc;
	}

	// Title
	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc) return;

		DrawTitle(gc);
		delete gc;
	}

	// Rectangle Range
	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc) return;

		DrawRectangleRange(gc);
		delete gc;
	}
}

void cHistogramPanel::DrawImage(wxGraphicsContext* gc)
{
	CreateGraphicsBitmapImage(gc);

	if (m_IsGraphicsBitmapSet)
	{
		auto interpolation_quality = wxINTERPOLATION_BEST;

		gc->SetInterpolationQuality(interpolation_quality);
		gc->DrawBitmap
		(
			m_GraphicsBitmapImage,
			m_StartDrawPos.x,
			m_StartDrawPos.y,
			m_CanvasSize.GetWidth(),
			m_CanvasSize.GetHeight()
		);
	}

}

auto cHistogramPanel::DrawTitle(wxGraphicsContext* gc) -> void
{
	if (m_TitleStr == wxEmptyString) return;

	wxColour fontColor(128, 128, 255, 64);
	wxFont font = wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc->SetFont(font, fontColor);

	wxString curr_value{};
	wxDouble widthText{}, heightText{};

	curr_value = m_TitleStr;
	gc->GetTextExtent(curr_value, &widthText, &heightText);
	wxRealPoint draw_point =
	{
		m_CanvasSize.GetWidth() / 2.0 - widthText / 2.0,
		m_CanvasSize.GetHeight() / 2.0 - heightText / 2.0
	};
	gc->DrawText(curr_value, draw_point.x, draw_point.y);
}

auto cHistogramPanel::DrawRectangleRange(wxGraphicsContext* gc) -> void
{
	if (!m_MaxHistogramValue) return;
	m_LeftBorderOnCanvas.x = m_CanvasSize.GetWidth() * m_LeftBorder.x / (wxDouble)m_MaxHistogramValue;
	m_RightBorderOnCanvas.x = m_CanvasSize.GetWidth() * m_RightBorder.x / (wxDouble)m_MaxHistogramValue;
	m_RightBorderOnCanvas.x = m_RightBorderOnCanvas.x >= m_CanvasSize.GetWidth() ? m_CanvasSize.GetWidth() - 1 : m_RightBorderOnCanvas.x;
	if (m_LeftBorderOnCanvas.x >= m_RightBorderOnCanvas.x) return;

	gc->SetPen(wxPen(wxColour(1, 45, 222), 1, wxPENSTYLE_SOLID));
	gc->SetBrush(wxBrush(wxColour(1, 157, 222, 32)));

	wxDouble widthTransRect{ (wxDouble)m_RightBorderOnCanvas.x - (wxDouble)m_LeftBorderOnCanvas.x }, 
		heightTransRect{ (wxDouble)m_CanvasSize.GetHeight() - 1.0 };
	gc->DrawRectangle(m_LeftBorderOnCanvas.x, 0.0, widthTransRect, heightTransRect);

	// Draw asterisk if Range was changed
	if (m_WasRangeChanged)
	{
		wxColour fontColour(255, 165, 0, 160);
		wxFont font = wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
		gc->SetFont(font, fontColour);
		wxString curr_value{};
		wxDouble widthText{}, heightText{};
		curr_value = wxString("*");
		gc->GetTextExtent(curr_value, &widthText, &heightText);	
		int offset = 4; // Horizontal offset in pixels
		wxRealPoint draw_point = 
			{ 
				offset + widthText / 2, 
				(wxDouble)offset
			};
		gc->DrawText(curr_value, draw_point.x, draw_point.y);
	}

	if (!m_IsMouseInsidePanel && !m_WasRangeChanged) return;
	// Drawing Text Values above the Rectangle
	{
		// Setting up the current font
		wxColour fontColour(10, 200, 30, 128);
		wxFont font = wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
		gc->SetFont(font, fontColour);

		wxString curr_value{};
		wxDouble widthText{}, heightText{};

		int x_offset = 4; // Horizontal offset in pixels
	
		// Draw Value of the Left Border
		{
			curr_value = wxString::Format(wxT("%i"), m_LeftBorder.x);
			gc->GetTextExtent(curr_value, &widthText, &heightText);
			wxRealPoint draw_point = 
			{ 
				(double)x_offset, 
				//m_LeftBorderOnCanvas.x + x_offset, 
				m_CanvasSize.GetHeight() / 2.0 + widthText / 2.0
			};
			//draw_point.x = m_RightBorderOnCanvas.x - m_LeftBorderOnCanvas.x < 80 ? m_LeftBorderOnCanvas.x - x_offset - heightText : draw_point.x;
			// Set up the transformation matrix for a 90-degree counterclockwise rotation
			gc->Translate(draw_point.x, draw_point.y);
			gc->Rotate(-M_PI / 2.0); // Rotate 90 degrees counterclockwise (pi/2 radians)
			gc->Translate(-draw_point.x, -draw_point.y);
			gc->DrawText(curr_value, draw_point.x, draw_point.y);

			// Return the matrix to the original state
			gc->Translate(draw_point.x, draw_point.y);
			gc->Rotate(M_PI / 2.0); // Rotate 90 degrees counterclockwise (pi/2 radians)
			gc->Translate(-draw_point.x, -draw_point.y);
		}

		// Draw Value of the Right Border
		{
			curr_value = wxString::Format(wxT("%i"), m_RightBorder.x);
			gc->GetTextExtent(curr_value, &widthText, &heightText);
			wxRealPoint draw_point = 
			{ 
				GetSize().GetWidth() - x_offset - heightText,
				//m_RightBorderOnCanvas.x - x_offset - heightText, 
				m_CanvasSize.GetHeight() / 2.0 + widthText / 2.0
			};
			//draw_point.x = m_RightBorderOnCanvas.x - m_LeftBorderOnCanvas.x < 80 ? m_RightBorderOnCanvas.x + x_offset : draw_point.x;
			// Set up the transformation matrix for a 90-degree counterclockwise rotation
			gc->Translate(draw_point.x, draw_point.y);
			gc->Rotate(-M_PI / 2.0); // Rotate 90 degrees counterclockwise (pi/2 radians)
			gc->Translate(-draw_point.x, -draw_point.y);
			gc->DrawText(curr_value, draw_point.x, draw_point.y);
		}
	}
}

void cHistogramPanel::CreateGraphicsBitmapImage(wxGraphicsContext* gc_)
{
	if (!m_IsGraphicsBitmapSet && m_IsImageSet)
	{
		m_GraphicsBitmapImage = wxGraphicsBitmap(gc_->CreateBitmapFromImage(m_Image));
		m_IsGraphicsBitmapSet = true;
	}
}

void cHistogramPanel::OnSize(wxSizeEvent& evt)
{
	if (evt.GetSize() == m_CanvasSize) return;

	m_CanvasSize = evt.GetSize();

	ChangeSizeOfImageInDependenceOnCanvasSize();
	m_IsGraphicsBitmapSet = false;
	Refresh();
}

void cHistogramPanel::ChangeSizeOfImageInDependenceOnCanvasSize()
{
}

void cHistogramPanel::OnMouseMoved(wxMouseEvent& evt)
{
	m_CursorPosOnCanvas = evt.GetPosition();
	
	//LOGF("Cursor: ", m_CursorPosOnCanvas.x);

	if (!m_ChangingLeftBorder && !m_ChangingRightBorder)
	{
		m_IsCursorAboveLeftBorder = false;
		m_IsCursorAboveRightBorder = false;

		if (evt.GetPosition().x >= m_LeftBorderOnCanvas.x - m_RangeWhenMouseReactOnBordersPx / 2 && 
			evt.GetPosition().x <= m_LeftBorderOnCanvas.x + m_RangeWhenMouseReactOnBordersPx / 2)
		{
			SetCursor(wxCURSOR_HAND);
			m_IsCursorAboveLeftBorder = true;
		}
		else if (evt.GetPosition().x >= m_RightBorderOnCanvas.x - m_RangeWhenMouseReactOnBordersPx / 2 && 
			evt.GetPosition().x <= m_RightBorderOnCanvas.x + m_RangeWhenMouseReactOnBordersPx / 2)
		{
			SetCursor(wxCURSOR_HAND);
			m_IsCursorAboveRightBorder = true;
		}
		else
		{
			SetCursor(wxCURSOR_DEFAULT);
		}
	}
	else if (m_ChangingLeftBorder)
	{
		if 
			(
			m_CursorPosOnCanvas.x >= 0 && 
			m_CursorPosOnCanvas.x < m_RightBorderOnCanvas.x - m_RangeWhenMouseReactOnBordersPx / 2
			)
		{
			m_LeftBorderOnCanvas.x = m_CursorPosOnCanvas.x;
			m_LeftBorder.x = m_LeftBorderOnCanvas.x * (wxDouble)m_MaxHistogramValue / (wxDouble)m_CanvasSize.GetWidth();
			Refresh();
		}
	}
	else if (m_ChangingRightBorder)
	{
		if 
			(
			m_CursorPosOnCanvas.x <= m_CanvasSize.GetWidth() - 1 && 
			m_CursorPosOnCanvas.x > m_LeftBorderOnCanvas.x + m_RangeWhenMouseReactOnBordersPx / 2
			)
		{
			m_RightBorderOnCanvas.x = m_CursorPosOnCanvas.x;
			m_RightBorder.x = m_RightBorderOnCanvas.x * m_MaxHistogramValue / m_CanvasSize.GetWidth();
			Refresh();
		}
	}
}

void cHistogramPanel::CheckIfMouseAboveImage()
{
}

void cHistogramPanel::CalculatePositionOnImage()
{
}

void cHistogramPanel::OnPreviewMouseLeftPressed(wxMouseEvent& evt)
{
	if (m_IsCursorAboveLeftBorder && evt.LeftDown())
	{
		SetCursor(wxCURSOR_SIZEWE);
		m_WasRangeChanged = true;
		//wxLogDebug("Pressed above Left Border");
		m_ChangingLeftBorder = true;
	}
	else if (m_IsCursorAboveRightBorder)
	{
		SetCursor(wxCURSOR_SIZEWE);
		m_WasRangeChanged = true;
		//wxLogDebug("Pressed above Right Border");
		m_ChangingRightBorder = true;
	}
}

auto cHistogramPanel::OnPreviewMouseLeftDoubleClick(wxMouseEvent& evt) -> void
{
	if (!m_WasRangeChanged) return;

	m_LeftBorder = m_AutoLeftBorder;
	m_RightBorder = m_AutoRightBorder;
	m_LeftBorderOnCanvas.x = m_CanvasSize.GetWidth() * m_LeftBorder.x / (wxDouble)m_MaxHistogramValue;
	m_RightBorderOnCanvas.x = m_CanvasSize.GetWidth() * m_RightBorder.x / (wxDouble)m_MaxHistogramValue;
	m_ParentLeftBorder->ChangeValue(wxString::Format(wxT("%i"), m_LeftBorder.x));
	m_ParentRightBorder->SetValue(wxString::Format(wxT("%i"), m_RightBorder.x));
	m_WasRangeChanged = false;

	Refresh();
}

void cHistogramPanel::OnPreviewMouseLeftReleased(wxMouseEvent& evt)
{
	if (!m_ChangingLeftBorder && !m_ChangingRightBorder) return;

	m_ParentLeftBorder->ChangeValue(wxString::Format(wxT("%i"), m_LeftBorder.x));
	m_ParentRightBorder->SetValue(wxString::Format(wxT("%i"), m_RightBorder.x));
	m_ChangingLeftBorder = false;
	m_ChangingRightBorder = false;
	SetCursor(wxCURSOR_DEFAULT);
}

auto cHistogramPanel::OnPreviewMouseEnteredWindow(wxMouseEvent& evt) -> void
{
	m_IsMouseInsidePanel = true;
	Refresh();
}

auto cHistogramPanel::OnPreviewMouseLeftWindow(wxMouseEvent& evt) -> void
{
	m_IsMouseInsidePanel = false;

	if (m_ChangingLeftBorder || m_ChangingRightBorder)
	{
		m_ParentLeftBorder->ChangeValue(wxString::Format(wxT("%i"), m_LeftBorder.x));
		m_ParentRightBorder->SetValue(wxString::Format(wxT("%i"), m_RightBorder.x));
		m_ChangingLeftBorder = false;
		m_ChangingRightBorder = false;
		SetCursor(wxCURSOR_DEFAULT);
	}
	Refresh();
}

void cHistogramPanel::ChangeCursorInDependenceOfCurrentParameters()
{
}

auto cHistogramPanel::MedianBlur1D
(
	unsigned long long* dataPtr, 
	const size_t dataSize, 
	size_t windowSize
) -> void
{
	// Function to compute the median of a window of data
	auto median_of_window = [&](const std::vector<unsigned long long>& window)
		{
			std::vector<unsigned long long> sorted_window = window;
			std::sort(sorted_window.begin(), sorted_window.end());
			size_t size = sorted_window.size();
			if (size % 2 == 0) 
			{
				return (sorted_window[size / 2 - 1] + sorted_window[size / 2]) / 2;
			}
			else 
			{
				return sorted_window[size / 2];
			}
		};

	auto tempHistogramData = std::make_unique<unsigned long long[]>(dataSize);
	memcpy(tempHistogramData.get(), dataPtr, dataSize * sizeof(unsigned long long));

	size_t half_window = windowSize / 2;

	for (auto i = 0; i < dataSize; ++i) 
	{
		// Determine the bounds of the window
		size_t start = (i < half_window) ? 0 : i - half_window;
		size_t end = std::min(i + half_window + 1, dataSize);

		// Collect the elements within the window
		std::vector<unsigned long long> window(tempHistogramData.get() + start, tempHistogramData.get() + end);

		// Compute the median and assign it to the result
		dataPtr[i] = median_of_window(window);
	}
}
