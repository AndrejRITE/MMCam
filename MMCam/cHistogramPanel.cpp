#include "cHistogramPanel.h"

BEGIN_EVENT_TABLE(cHistogramPanel, wxPanel)
	EVT_PAINT(cHistogramPanel::PaintEvent)
	EVT_SIZE(cHistogramPanel::OnSize)
	EVT_MOUSEWHEEL(cHistogramPanel::OnMouseWheel)
	EVT_LEFT_DOWN(cHistogramPanel::OnMouseDown)
	EVT_MOTION(cHistogramPanel::OnMouseMove)
	EVT_LEFT_UP(cHistogramPanel::OnMouseUp)
	EVT_RIGHT_UP(cHistogramPanel::OnToggleLogScale)
	EVT_LEFT_DCLICK(cHistogramPanel::OnPreviewMouseLeftDoubleClick)
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

	m_MaxHistogramValue = m_DataType == HistogramPanelVariables::ImageDataTypes::RAW_12BIT ? 4'095UL : 65'535UL;

	const size_t histSize = m_MaxHistogramValue + 1;
	auto firstNonZeroFromLeft = std::find_if
	(
		m_HistogramData.get(),
		m_HistogramData.get() + histSize,
		[](unsigned long long val) { return val > 0; }
	);

	auto firstNonZeroFromRight = std::find_if
	(
		std::make_reverse_iterator(m_HistogramData.get() + histSize),
		std::make_reverse_iterator(m_HistogramData.get()),
		[](unsigned long long val) { return val > 0; }
	);

	//MedianBlur1D
	//(
	//	m_HistogramData.get(),
	//	m_MaxHistogramValue + 1,
	//	5
	//);

	// Find Global Minimum and Maximum values inside the Histogram values

	auto minMaxElement = std::minmax_element(m_HistogramData.get(), m_HistogramData.get() + histSize);

	m_GlobalMin = *minMaxElement.first;
	m_GlobalMax = *minMaxElement.second;

	m_AutoLeftBorder.x = min_value;
	m_AutoRightBorder.x = max_value;

	SetWXImage();

	m_LeftBorder = m_AutoLeftBorder;
	m_RightBorder = m_AutoRightBorder;
	m_WasRangeChanged = false;

	m_ViewMin = (unsigned int)std::max<unsigned long long>(0, std::distance(m_HistogramData.get(), firstNonZeroFromLeft));
	m_ViewMax = (unsigned int)std::min<unsigned long long>(m_MaxHistogramValue, histSize - 1 - std::distance(std::make_reverse_iterator(m_HistogramData.get() + histSize), firstNonZeroFromRight));

	RebuildHistogramImageForCurrentView();
	InvalidateGraphicsBitmap();

	Refresh();

	return std::optional<int>();
}

auto cHistogramPanel::SetBackgroundColor(wxColour color) -> void
{
	SetBackgroundColour(color);
	SetWXImage();
}

auto cHistogramPanel::SetWXImage() -> void
{
	if (!m_HistogramData) return;

	m_Image = wxImage(m_CanvasSize);
	m_ImageSize = m_CanvasSize;
	
	m_Image.Clear(m_backgroundColour.GetRed());

	int curr_x{}, curr_y{};

	auto maxHistogramValue = static_cast<double>(m_MaxHistogramValue);
	auto globalMax = static_cast<double>(m_GlobalMax);

	auto step = static_cast<int>(maxHistogramValue / static_cast<double>(m_CanvasSize.GetWidth() - 1));

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
				wxSize(step, curr_y)
			),
			m_HistogramDataColor.GetRed(),
			m_HistogramDataColor.GetGreen(),
			m_HistogramDataColor.GetBlue()
		);
	}

	m_IsImageSet = true;
	InvalidateGraphicsBitmap();

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
	dc.SetBackground(wxBrush(m_backgroundColour));
	dc.Clear();

	if (!m_Image.IsOk()) return;

	// Image
	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc) return;

		DrawImage(gc);
		DrawGridAndAxes(gc);

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

	// Respect current zoom:
	m_LeftBorderOnCanvas.x = ValueToCanvasX((unsigned int)m_LeftBorder.x);
	m_RightBorderOnCanvas.x = ValueToCanvasX((unsigned int)m_RightBorder.x);

	// Clamp to canvas and bail if inverted
	m_LeftBorderOnCanvas.x = std::clamp(m_LeftBorderOnCanvas.x, 0.0, (double)m_CanvasSize.GetWidth() - 1.0);
	m_RightBorderOnCanvas.x = std::clamp(m_RightBorderOnCanvas.x, 0.0, (double)m_CanvasSize.GetWidth() - 1.0);
	if (m_LeftBorderOnCanvas.x >= m_RightBorderOnCanvas.x) return;

	gc->SetPen(wxPen(wxColour(1, 45, 222), 1, wxPENSTYLE_SOLID));
	gc->SetBrush(wxBrush(wxColour(1, 157, 222, 32)));

	const wxDouble widthTransRect = (wxDouble)m_RightBorderOnCanvas.x - (wxDouble)m_LeftBorderOnCanvas.x;
	const wxDouble heightTransRect = (wxDouble)m_CanvasSize.GetHeight() - 1.0;
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
			//curr_value = wxString::Format(wxT("%i"), m_LeftBorder.x);
			curr_value = HistogramPanelVariables::FormatNumber(m_LeftBorder.x);

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
			//curr_value = wxString::Format(wxT("%i"), m_RightBorder.x);
			curr_value = HistogramPanelVariables::FormatNumber(m_RightBorder.x);

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
	InvalidateGraphicsBitmap();
	Refresh();
}

void cHistogramPanel::ChangeSizeOfImageInDependenceOnCanvasSize()
{
}

void cHistogramPanel::OnMouseMove(wxMouseEvent& evt)
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
		if (m_CursorPosOnCanvas.x >= 0 &&
			m_CursorPosOnCanvas.x < m_RightBorderOnCanvas.x - m_RangeWhenMouseReactOnBordersPx / 2)
		{
			m_LeftBorderOnCanvas.x = m_CursorPosOnCanvas.x;
			m_LeftBorder.x = (int)CanvasXToValue((int)m_LeftBorderOnCanvas.x); // <- use view mapping
			Refresh();
		}
	}
	else if (m_ChangingRightBorder)
	{
		if (m_CursorPosOnCanvas.x <= m_CanvasSize.GetWidth() - 1 &&
			m_CursorPosOnCanvas.x > m_LeftBorderOnCanvas.x + m_RangeWhenMouseReactOnBordersPx / 2)
		{
			m_RightBorderOnCanvas.x = m_CursorPosOnCanvas.x;
			m_RightBorder.x = (int)CanvasXToValue((int)m_RightBorderOnCanvas.x); // <- use view mapping
			Refresh();
		}
	}

	if (m_Panning && evt.Dragging())
	{
		const int dx = evt.GetX() - m_PanStartX;

		// bins per pixel for the *frozen-at-drag-start* span:
		const unsigned int spanAtStart = std::max(1u, m_ViewMaxAtDragStart - m_ViewMinAtDragStart);
		const double binsPerPixel = (m_CanvasSize.GetWidth() > 1)
			? double(spanAtStart) / double(m_CanvasSize.GetWidth() - 1)
			: 0.0;

		const long long deltaBins = (long long)std::llround(-dx * binsPerPixel);

		long long newMinLL = (long long)m_ViewMinAtDragStart + deltaBins;
		long long newMaxLL = newMinLL + (long long)spanAtStart;

		const long long fullMin = 0;
		const long long fullMax = (long long)m_MaxHistogramValue;

		if (newMinLL < fullMin) { newMinLL = fullMin; newMaxLL = newMinLL + (long long)spanAtStart; }
		if (newMaxLL > fullMax) { newMaxLL = fullMax; newMinLL = newMaxLL - (long long)spanAtStart; }

		m_ViewMin = (unsigned int)newMinLL;
		m_ViewMax = (unsigned int)newMaxLL;

		RebuildHistogramImageForCurrentView();
		InvalidateGraphicsBitmap();
		Refresh(false);
	}
}

void cHistogramPanel::CheckIfMouseAboveImage()
{
}

void cHistogramPanel::CalculatePositionOnImage()
{
}

void cHistogramPanel::OnMouseDown(wxMouseEvent& evt)
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
	else if (!m_Panning && evt.LeftDown()) 
	{
		m_Panning = true;
		m_PanStartX = evt.GetX();
		m_ViewMinAtDragStart = m_ViewMin;
		m_ViewMaxAtDragStart = m_ViewMax;
		CaptureMouse();
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

	ResetViewToFull();

	Refresh();
}

void cHistogramPanel::OnMouseUp(wxMouseEvent& evt)
{
	if (m_Panning && evt.LeftUp()) 
	{
		m_Panning = false;
		if (HasCapture()) ReleaseMouse();
	}

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

void cHistogramPanel::ResetViewToFull()
{
	m_ViewMin = 0;
	m_ViewMax = (unsigned int)m_MaxHistogramValue;
	RebuildHistogramImageForCurrentView();
	InvalidateGraphicsBitmap();
	Refresh(false);
}

auto cHistogramPanel::OnMouseWheel(wxMouseEvent& evt) -> void
{
	if (m_MaxHistogramValue == 0) return;

	const int rot = evt.GetWheelRotation();
	if (evt.ControlDown()) {
		if (rot != 0) m_LogScale = !m_LogScale;
	}
	else {
		// Desired zoom factor (in/out)
		const double zoomIn = 0.80;  // 20% in
		const double zoomOut = 1.25;  // 25% out
		const double factor = (rot > 0) ? zoomIn : zoomOut;

		// Full domain [0 .. m_MaxHistogramValue] (inclusive)
		const unsigned int fullMin = 0;
		const unsigned int fullMax = (unsigned int)m_MaxHistogramValue;
		const unsigned int fullSpan = std::max(1u, fullMax - fullMin);

		// Current view and mouse anchor
		const int mouseX = evt.GetX();
		const double f = (m_CanvasSize.GetWidth() > 1)
			? std::clamp(double(mouseX) / double(m_CanvasSize.GetWidth() - 1), 0.0, 1.0)
			: 0.0;
		const unsigned int anchor = CanvasXToValue(mouseX); // in value domain

		// Target span (keep >=1, <= fullSpan)
		unsigned int oldSpan = std::max(1u, DomainSpan());
		unsigned int newSpan = (unsigned int)std::round(oldSpan * factor);
		newSpan = std::clamp(newSpan, 1u, fullSpan);

		// Keep anchor under the same canvas fraction:
		//   f = (anchor - newMin) / newSpan  => newMin = anchor - f*newSpan
		long long newMinLL = (long long)anchor - (long long)std::llround(f * newSpan);
		long long newMaxLL = newMinLL + (long long)newSpan;

		// Clamp window to [fullMin, fullMax] while preserving span
		if (newMinLL < (long long)fullMin) {
			newMinLL = fullMin;
			newMaxLL = newMinLL + (long long)newSpan;
		}
		if (newMaxLL > (long long)fullMax) {
			newMaxLL = fullMax;
			newMinLL = newMaxLL - (long long)newSpan;
			if (newMinLL < (long long)fullMin) newMinLL = fullMin; // paranoid
		}

		m_ViewMin = (unsigned int)newMinLL;
		m_ViewMax = (unsigned int)newMaxLL;
	}

	RebuildHistogramImageForCurrentView();
	InvalidateGraphicsBitmap();
	Refresh(false);
}

auto cHistogramPanel::OnToggleLogScale(wxMouseEvent& evt) -> void
{
	m_LogScale = !m_LogScale;
	RebuildHistogramImageForCurrentView();
	InvalidateGraphicsBitmap();
	Refresh(false);
}

void cHistogramPanel::RebuildHistogramImageForCurrentView()
{
	if (!m_HistogramData || m_CanvasSize.GetWidth() <= 0 || m_CanvasSize.GetHeight() <= 0) return;

	// 1) Prepare a clean canvas
	m_Image = wxImage(m_CanvasSize);
	m_ImageSize = m_CanvasSize;
	m_Image.Clear(m_backgroundColour.GetRed()); // R=G=B is fine for a gray background

	// 2) Find peak in the visible window (for vertical scaling)
	unsigned long long viewPeak = 0;
	for (unsigned int v = m_ViewMin; v <= m_ViewMax; ++v)
		viewPeak = std::max(viewPeak, m_HistogramData[v]);

	m_ViewPeak = viewPeak;

	// Avoid divide-by-zero
	const double denom = m_LogScale ? std::log1p((double)std::max(1ULL, viewPeak))
		: (double)std::max(1ULL, viewPeak);

	// 3) Draw only bins in [m_ViewMin, m_ViewMax], mapped to panel width
	const int W = m_CanvasSize.GetWidth();
	const int H = m_CanvasSize.GetHeight();

	for (unsigned int v = m_ViewMin; v <= m_ViewMax; ++v)
	{
		const auto cnt = m_HistogramData[v];
		if (cnt == 0) continue; // avoid 1px bar at baseline

		const int x0 = ValueToCanvasX(v);
		int x1 = (v < m_ViewMax) ? ValueToCanvasX(v + 1) : (W - 1);
		if (x1 <= x0) x1 = x0 + 1;

		const int yTop = CountToCanvasY(cnt);
		const int h = (H - 1) - yTop;     // note: -1 fixes the off-by-one
		if (h <= 0) continue;

		m_Image.SetRGB(wxRect(wxPoint(x0, yTop), wxSize(x1 - x0, h)),
			m_HistogramDataColor.GetRed(),
			m_HistogramDataColor.GetGreen(),
			m_HistogramDataColor.GetBlue());
	}

	// 4) Update the on-canvas positions of the left/right borders so they stay accurate in the zoomed view
	m_LeftBorderOnCanvas.x = ValueToCanvasX((unsigned int)m_LeftBorder.x);
	m_RightBorderOnCanvas.x = ValueToCanvasX((unsigned int)m_RightBorder.x);
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

int cHistogramPanel::CountToCanvasY(unsigned long long c) const
{
	const int H = m_CanvasSize.GetHeight();
	if (H <= 0) return 0;

	if (m_ViewPeak == 0) return H - 1;

	double num = m_LogScale ? std::log1p((double)c) : (double)c;
	double den = m_LogScale ? std::log1p((double)m_ViewPeak) : (double)m_ViewPeak;

	double t = (den > 0.0) ? (num / den) : 0.0;   // 0..1
	int y = H - 1 - (int)std::round(t * (H - 1)); // 0=top, H-1=bottom
	return std::clamp(y, 0, H - 1);
}

void cHistogramPanel::DrawGridAndAxes(wxGraphicsContext* gc)
{
	if (m_CanvasSize.GetWidth() <= 0 || m_CanvasSize.GetHeight() <= 0) return;

	// === helpers ===
	auto nice_step = [](double span, int targetTicks) {
		if (span <= 0.0) return 1.0;
		double raw = span / std::max(1, targetTicks);
		double pow10 = std::pow(10.0, std::floor(std::log10(raw)));
		double frac = raw / pow10;
		double step;
		if (frac < 1.5) step = 1.0;
		else if (frac < 3.5) step = 2.0;
		else if (frac < 7.5) step = 5.0;
		else step = 10.0;
		return step * pow10;
		};
	auto fmt_uint = [](double v) {
		// avoid scientific notation, clamp to integer bins
		unsigned int ui = (unsigned int)std::llround(std::max(0.0, v));
		return wxString::Format("%u", ui);
		};

	// === grid styles ===
	const wxColour gridMajor(255 - m_backgroundColour.GetRed(), 255 - m_backgroundColour.GetGreen(), 255 - m_backgroundColour.GetBlue(), 60);
	const wxColour gridMinor(255 - m_backgroundColour.GetRed(), 255 - m_backgroundColour.GetGreen(), 255 - m_backgroundColour.GetBlue(), 30);
	const auto redLabel = 255 - m_backgroundColour.GetRed() - 20 < 0 ? 0 : 255 - m_backgroundColour.GetRed() - 20;
	const wxColour labelCol(redLabel, redLabel, redLabel, 140);

	gc->SetFont(wxFontInfo(9), labelCol);

	const int W = m_CanvasSize.GetWidth();
	const int H = m_CanvasSize.GetHeight();

	// =========================
	// X ticks (value domain)
	// =========================
	const double xSpan = std::max(1u, DomainSpan());
	// Aim for ~8 vertical lines max, based on pixel width
	int targetXTicks = std::clamp(W / 120, 3, 8);
	double xStep = nice_step((double)xSpan, targetXTicks);

	// start at the first multiple within view
	double xStart = std::ceil((double)m_ViewMin / xStep) * xStep;

	for (double xv = xStart; xv <= (double)m_ViewMax + 0.5; xv += xStep) {
		int x = ValueToCanvasX((unsigned int)std::llround(xv));
		bool isMajor = std::fmod(std::round(xv / xStep), 5.0) == 0.0; // every 5th is major

		gc->SetPen(wxPen(isMajor ? gridMajor : gridMinor, isMajor ? 1.5 : 1));
		gc->StrokeLine(x + 0.5, 0, x + 0.5, H);

		// label: only for majors and if not too crowded
		if (isMajor) {
			auto s = fmt_uint(xv);
			wxDouble tw, th, d, l;
			gc->GetTextExtent(s, &tw, &th, &d, &l);
			int lx = std::clamp(x - (int)tw / 2, 0, std::max(0, W - (int)tw));
			int ly = H - (int)th - 2;
			// avoid overlapping with left/right edges too much
			if (lx < 2) lx = 2;
			if (lx + (int)tw > W - 2) lx = W - 2 - (int)tw;
			gc->SetBrush(*wxTRANSPARENT_BRUSH);
			gc->DrawText(s, lx, ly);
		}
	}

	// =========================
	// Y ticks (counts)
	// =========================
	if (m_ViewPeak > 0) {
		// Build Y ticks for linear or log scale
		std::vector<unsigned long long> yTicks;

		if (!m_LogScale) {
			// linear
			int targetYTicks = std::clamp(H / 80, 3, 8);
			double yStep = nice_step((double)m_ViewPeak, targetYTicks);
			double yStart = std::ceil(0.0 / yStep) * yStep; // zero
			for (double yv = yStart; yv <= (double)m_ViewPeak + 0.5; yv += yStep)
				yTicks.push_back((unsigned long long)std::llround(yv));
		}
		else {
			// log scale — use 1,2,5 * 10^k up to peak
			double maxVal = (double)m_ViewPeak;
			int kmin = 0;
			int kmax = (int)std::floor(std::log10(std::max(1.0, maxVal)));
			const int mults[3] = { 1, 2, 5 };
			for (int k = 0; k <= kmax; ++k) {
				for (int m : mults) {
					double v = m * std::pow(10.0, k);
					if (v <= maxVal) yTicks.push_back((unsigned long long)std::llround(v));
				}
			}
			// ensure we include the top if not exact
			if (yTicks.empty() || yTicks.back() != m_ViewPeak) yTicks.push_back(m_ViewPeak);
		}

		for (size_t i = 0; i < yTicks.size(); ++i) {
			unsigned long long cv = yTicks[i];
			int y = CountToCanvasY(cv);
			bool isMajor = (!m_LogScale)
				? (i % 2 == 0)          // every 2nd major on linear
				: (cv == 1 || cv == 5 ||
					cv == 10 || cv == 50 ||
					std::fmod(std::log10((double)std::max(1ULL, cv)), 1.0) == 0.0); // powers of 10

			gc->SetPen(wxPen(isMajor ? gridMajor : gridMinor, isMajor ? 1.5 : 1));
			gc->StrokeLine(0, y + 0.5, W, y + 0.5);

			// labels at left
			wxString s = (!m_LogScale) ? wxString::Format("%llu", cv)
				: wxString::Format("%llu", cv);
			wxDouble tw, th, d, l;
			gc->GetTextExtent(s, &tw, &th, &d, &l);
			int lx = 2;
			int ly = std::clamp(y - (int)th - 1, 0, H - (int)th);
			gc->DrawText(s, lx, ly);
		}
	}

	// Corner info (optional): show [viewMin:viewMax]
	{
		wxString s = wxString::Format("[%u : %u]", m_ViewMin, m_ViewMax);
		wxDouble tw, th, d, l; gc->GetTextExtent(s, &tw, &th, &d, &l);
		gc->DrawText(s, W - (int)tw - 6, H - (int)th - 2);
	}
}
