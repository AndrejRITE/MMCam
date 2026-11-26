#include "cCamPreview.h"

BEGIN_EVENT_TABLE(cCamPreview, wxPanel)
	EVT_PAINT(cCamPreview::PaintEvent)
	EVT_SIZE(cCamPreview::OnSize)
	EVT_MOTION(cCamPreview::OnMouseMoved)
	EVT_MOUSEWHEEL(cCamPreview::OnMouseWheelMoved)
	EVT_LEFT_DOWN(cCamPreview::OnPreviewMouseLeftPressed)
	EVT_LEFT_UP(cCamPreview::OnPreviewMouseLeftReleased)
	EVT_KEY_DOWN(cCamPreview::OnKeyPressed)
	EVT_KEY_UP(cCamPreview::OnKeyReleased)
	EVT_ENTER_WINDOW(cCamPreview::OnEnterPanel)
	EVT_LEAVE_WINDOW(cCamPreview::OnLeavePanel)
	EVT_MOUSE_CAPTURE_LOST(cCamPreview::OnMouseCaptureLost)
END_EVENT_TABLE()

cCamPreview::cCamPreview
(
	std::unique_ptr<CameraPreviewVariables::InputPreviewPanelArgs> previewPanelArgs
) 
	: wxPanel(previewPanelArgs->frame)
{
	wxArtProvider::Push(new wxMaterialDesignArtProvider);

	m_ParentArguments.reset(previewPanelArgs.release());

	SetDoubleBuffered(true);
#ifdef _DEBUG
	SetBackgroundColour(wxColor(210, 185, 155));
#else
	SetBackgroundColour(wxColor(255, 255, 255));
#endif // _DEBUG
	m_ParentArguments->sizer->Add(this, 1, wxEXPAND);

	InitDefaultComponents();
}

auto cCamPreview::ActivateCrossHairDisplaying(bool activate) -> void
{
	m_DisplayCrossHair = activate;
	m_CrossHairTool->ActivateToolButton(activate, activate);
	if (!activate)
	{
		EndCrossHairDrag();
		SetCrossHairHover({});
	}
	//m_CrossHairTool->SetCursorPosOnCanvas(m_CursorPosOnCanvas);
	ChangeCursorInDependenceOfCurrentParameters();
	Refresh();
}

auto cCamPreview::ActivateFWHMDisplaying(bool activate) -> void
{
	m_DisplayFWHM = activate;
	if (activate)
	{
		CalculateFWHM();
		CalculateHEW();
	}

	Refresh();
}

auto cCamPreview::ActivateFocusCenterDisplaying(bool activate) -> void
{
	m_DisplayFocusCenter = activate;
	Refresh();
}

auto cCamPreview::ActivateGridMeshDisplaying(bool activate) -> void
{
	m_DisplayGridMesh = activate;
	Refresh();
}

auto cCamPreview::ActivateCircleMeshDisplaying(bool activate) -> void
{
	m_DisplayCircleMesh = activate;
	Refresh();
}

void cCamPreview::SetXCrossHairPosFromParentWindow(const int& x_pos)
{
	int corrected_x_pos = x_pos - 1;
	if (corrected_x_pos >= 0 && corrected_x_pos < m_ImageSize.GetWidth())
	{
		m_CrossHairPos.x = corrected_x_pos;
		m_CrossHairTool->SetXPosFromParent(corrected_x_pos);
		Refresh();
	}
}

void cCamPreview::SetYCrossHairPosFromParentWindow(const int& y_pos)
{
	int corrected_y_pos = y_pos - 1;
	if (corrected_y_pos >= 0 && corrected_y_pos < m_ImageSize.GetHeight())
	{
		m_CrossHairPos.y = corrected_y_pos;
		m_CrossHairTool->SetYPosFromParent(corrected_y_pos);
		Refresh();
	}
}

auto cCamPreview::SettingCrossHairPosFromParentWindow(bool set) -> void
{
	m_CrossHairTool->ActivateSetPositionFromParentWindow(set);
	//m_SettingCrossHairPos = set;
}

//auto cCamPreview::SetImageSize(const wxSize& img_size) -> void
//{
//	m_ImageSize = img_size;
//
//	m_IsImageSet = false;
//
//	m_ImageData = std::make_unique<unsigned short[]>(img_size.GetWidth() * img_size.GetHeight());
//
//	m_HorizontalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetWidth());
//	m_VerticalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetHeight());
//
//	m_Image.Create(img_size);
//
//	UpdateImageParameters(true);
//}

auto cCamPreview::GetDataPtr() const -> unsigned short*
{
	return m_ImageData.get();
}

//auto cCamPreview::GetImagePtr() const -> wxImage*
//{
//	return &m_Image;
//}

auto cCamPreview::GetImageSize() const -> wxSize
{
	return m_ImageSize;
}

void cCamPreview::UpdateImageParameters(const wxSize oldImageSize)
{
	LOG("Started: " + wxString(__FUNCSIG__))
	/* 
	Saving previous values for correct displaying of the image in the same place, 
	where it was before capturing.
	*/
	if (oldImageSize != m_ImageSize)
	{
		auto temp_zoom = m_Zoom;
		auto temp_pan_offset = m_PanOffset;
		auto temp_start_draw_pos = m_StartDrawPos;
		m_Zoom = m_MinZoom;
		m_PanOffset = {};
		ChangeSizeOfImageInDependenceOnCanvasSize();
		if (m_IsImageSet)
		{
			m_Zoom = temp_zoom;
			m_PanOffset = temp_pan_offset;
			m_StartDrawPos = temp_start_draw_pos;
		}
	}

	/* CrossHair*/
	if (m_CrossHairTool)
	{
		auto imageDataType = m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? ToolsVariables::DATA_U12 : ToolsVariables::DATA_U16;
		m_CrossHairTool->SetImageDataType(imageDataType);
		m_CrossHairTool->SetImageDimensions(m_ImageSize);

		m_CrossHairTool->SetZoomOfOriginalSizeImage(m_ZoomOnOriginalSizeImage);
		m_CrossHairTool->UpdateZoomValue(m_Zoom);

		m_CrossHairTool->SetImageStartDrawPos(wxRealPoint
		(
			m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
			m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
		));

		if (oldImageSize != m_ImageSize)
		{
			m_CrossHairPos.x = m_ImageSize.GetWidth() / 2;
			m_CrossHairPos.y = m_ImageSize.GetHeight() / 2;
			m_CrossHairTool->SetXPosFromParent(m_CrossHairPos.x);
			m_CrossHairTool->SetYPosFromParent(m_CrossHairPos.y);
			if (m_ParentArguments && m_ParentArguments->x_pos_crosshair && m_ParentArguments->y_pos_crosshair)
			{
				m_ParentArguments->x_pos_crosshair->ChangeValue(wxString::Format("%i", m_CrossHairPos.x + 1));
				m_ParentArguments->y_pos_crosshair->ChangeValue(wxString::Format("%i", m_CrossHairPos.y + 1));
			}
		}
	}

	// FWHM
	if (m_DisplayFWHM)
	{
		CalculateFWHM();
		CalculateHEW();
	}

	m_IsImageSet = true;
	m_IsGraphicsBitmapSet = false;
	Refresh();

	LOG("Finished: " + wxString(__FUNCSIG__))
}

namespace
{
	/// Dark translucent rounded card used behind every overlay on the preview.
	void DrawOverlayCard(wxGraphicsContext* gc, double x, double y, double w, double h, double radius = 6.0)
	{
		gc->SetBrush(wxBrush(wxColour(20, 20, 24, 205)));
		gc->SetPen(wxPen(wxColour(255, 255, 255, 40), 1));
		wxGraphicsPath path = gc->CreatePath();
		path.AddRoundedRectangle(x, y, w, h, radius);
		gc->DrawPath(path);
	}

	/// 12,345 for integers that are shown to the user
	wxString FormatInt(long long v)
	{
		return CameraPreviewVariables::FormatWithThousandsDelimiter(v, ",");
	}
}

auto cCamPreview::GetImagePixelColour(int x, int y) const -> wxColour
{
	if (!HasImage() || m_ColourLut.empty()) return wxColour(0, 0, 0);
	if (x < 0 || y < 0 || x >= m_ImageSize.GetWidth() || y >= m_ImageSize.GetHeight()) return wxColour(0, 0, 0);

	const unsigned short v = std::min(m_ImageData[static_cast<size_t>(y) * m_ImageSize.GetWidth() + x], m_ColourLutMax);
	const unsigned char* c = &m_ColourLut[static_cast<size_t>(v) * 3];
	return wxColour(c[0], c[1], c[2]);
}

auto cCamPreview::GetCanvasColour(double x, double y) const -> wxColour
{
	const double k = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (!HasImage() || k <= 0.0) return GetBackgroundColour();

	const int ix = static_cast<int>(std::floor(x / k - m_StartDrawPos.x));
	const int iy = static_cast<int>(std::floor(y / k - m_StartDrawPos.y));
	if (ix < 0 || iy < 0 || ix >= m_ImageSize.GetWidth() || iy >= m_ImageSize.GetHeight())
		return GetBackgroundColour();
	return GetImagePixelColour(ix, iy);
}

auto cCamPreview::SetCameraCapturedImage
(
	unsigned short* data_ptr,
	const wxSize& imgSize,
	unsigned short minValue,
	unsigned short maxValue
) -> void
{
	if (!data_ptr) return;
	if (!imgSize.GetWidth() || !imgSize.GetHeight()) return;

	auto oldImageSize = m_ImageSize;
	
	m_ImageSize = imgSize;

	m_IsImageSet = false;

	LOG("Started: " + wxString(__FUNCSIG__));

	SetExecutionFinished(false);

	++m_FrameCount;

	const size_t readDataSize = static_cast<size_t>(m_ImageSize.GetWidth()) * static_cast<size_t>(m_ImageSize.GetHeight());

	if (!m_ImageData || oldImageSize != m_ImageSize)
	{
		m_ImageData.reset(); // free the old frame before allocating a new one (large sensors)
		m_ImageData = std::make_unique<unsigned short[]>(readDataSize);
		m_HorizontalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetWidth());
		m_VerticalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetHeight());
	}

	memcpy(m_ImageData.get(), data_ptr, sizeof(unsigned short) * readDataSize);

	CalculateImageStatistics();

	UpdateColourLut(minValue, maxValue);

	UpdateImageParameters(oldImageSize);
	
	// Update Mouse position
	{
		wxMouseEvent event(wxEVT_MOTION);
		event.SetEventObject(this);

		// Set the last known mouse position
		event.m_x = m_CursorPosOnCanvas.x / m_ZoomOnOriginalSizeImage;
		event.m_y = m_CursorPosOnCanvas.y / m_ZoomOnOriginalSizeImage;

		ProcessEvent(event);
	}

	TickFPS();

	LOG("Image was updated: " + wxString(__FUNCSIG__))
	SetExecutionFinished(true);
}

auto cCamPreview::UpdateBlackAndWhiteRange(const int black, const int white) -> std::optional<int>
{
	if (!m_ImageData) return std::optional<int>(1);

	UpdateColourLut(black, white);

	m_IsImageSet = true;
	m_IsGraphicsBitmapSet = false;

	Refresh();

	return std::optional<int>();
}

auto cCamPreview::UpdateCursorPositionOnStatusBar() -> void
{
	if (!HasImage()) return;

	int pixel_offset{ 1 };
	auto status_bar_text = wxString("");
	if (m_IsCursorInsideImage)
	{
		// X Position
		status_bar_text =
			wxString("x=") +
			wxString::Format(wxT("%i"), (int)(m_CheckedCursorPosOnImage.x + pixel_offset)) +
			wxString(", ");
		// Y Position
		status_bar_text +=
			wxString("y=") +
			wxString::Format(wxT("%i"), (int)(m_CheckedCursorPosOnImage.y + pixel_offset)) +
			wxString(", ");
		// Value
		unsigned short value = m_ImageData[(int)m_CheckedCursorPosOnImage.y * m_ImageSize.GetWidth() + (int)m_CheckedCursorPosOnImage.x];
		status_bar_text +=
			wxString("value=") +
			wxString::Format(wxT("%i"), (int)(value));
	}
	m_ParentArguments->statusBar->SetStatusText(status_bar_text);
}

void cCamPreview::CalculateJetColormapPixelRGB
(
	const unsigned short& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b,
	const int& black,
	const int& white
)
{
	// Clamp and normalize value
	float norm = 0.0f;
	if (white > black) {
		int clamped = std::clamp(static_cast<int>(value), black, white);
		norm = static_cast<float>(clamped - black) / (white - black);
	}

	// Define gradient breakpoints
	constexpr float t0 = 0.0f;
	constexpr float t1 = 0.25f;
	constexpr float t2 = 0.5f;
	constexpr float t3 = 0.75f;
	constexpr float t4 = 1.0f;

	if (norm < t1)
	{
		// Blue ramping up
		r = 0;
		g = 0;
		b = static_cast<unsigned char>(131.48f + norm / t1 * (255.0f - 131.48f)); // 255 * 0.51563 ≈ 131.48
	}
	else if (norm < t2)
	{
		// Blue to Cyan (G up)
		r = 0;
		g = static_cast<unsigned char>((norm - t1) / (t2 - t1) * 255.0f);
		b = 255;
	}
	else if (norm < t3)
	{
		// Cyan to Yellow (R up, B down)
		r = static_cast<unsigned char>((norm - t2) / (t3 - t2) * 255.0f);
		g = 255;
		b = static_cast<unsigned char>((t3 - norm) / (t3 - t2) * 255.0f);
	}
	else if (norm < t4)
	{
		// Yellow to Orange-Red (G down)
		r = 255;
		g = static_cast<unsigned char>((t4 - norm) / (t4 - t3) * 255.0f);
		b = 0;
	}
	else
	{
		// Clip: value >= white
		r = 255;
		g = 255;
		b = 255;
	}
}

void cCamPreview::Calculate16_ColorsImageJColormapPixelRGB
(
	const unsigned short& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b,
	const int& black,
	const int& white
)
{
	static const std::array<std::tuple<unsigned char, unsigned char, unsigned char>, 16> colormap = { {
		{  0,   0,   0},
		{  1,   1, 171},
		{  1,   1, 224},
		{  0, 110, 255},
		{  1, 171, 254},
		{  1, 224, 254},
		{  1, 254,   1},
		{190, 255,   0},
		{255, 255,   0},
		{255, 224,   0},
		{255, 141,   0},
		{250,  94,   0},
		{245,   0,   0},
		{245,   0, 222},
		{222, 180, 222},
		{255, 255, 255}
	} };

	// Clamp value to [black, white]
	int clamped = std::clamp(static_cast<int>(value), black, white);

	// Prevent divide-by-zero
	int range = white - black;
	int index = 0;

	if (range > 0)
	{
		// Compute which of the 16 intervals this value falls into
		index = static_cast<int>(15.0 * (clamped - black) / range);
	}

	// Assign RGB from the colormap
	std::tie(r, g, b) = colormap[index];
}

auto cCamPreview::CalculateHotColormapPixelRGB
(
	const unsigned short& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b,
	const int& black,
	const int& white
) -> void
{
	// Clamp and normalize the value to [0.0, 1.0]
	float norm = 0.0f;
	if (white > black)
	{
		int clamped = std::clamp(static_cast<int>(value), black, white);
		norm = static_cast<float>(clamped - black) / (white - black);
	}

	// Define normalized thresholds
	constexpr float t1 = 1.0f / 3.0f;
	constexpr float t2 = 2.0f / 3.0f;

	// Initialize RGB
	float red = 0.0f;
	float green = 0.0f;
	float blue = 0.0f;

	if (norm < t1)
	{
		// Stage 1: red increases
		red = norm / t1;
	}
	else if (norm < t2)
	{
		// Stage 2: red full, green increases
		red = 1.0f;
		green = (norm - t1) / (t2 - t1);
	}
	else if (norm <= 1.0f)
	{
		// Stage 3: red and green full, blue increases
		red = 1.0f;
		green = 1.0f;
		blue = (norm - t2) / (1.0f - t2);
	}

	// Scale to 0-255
	r = static_cast<unsigned char>(std::round(red * 255.0f));
	g = static_cast<unsigned char>(std::round(green * 255.0f));
	b = static_cast<unsigned char>(std::round(blue * 255.0f));
}

auto cCamPreview::CalculateCopperColormapPixelRGB
(
	const unsigned short& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b,
	const int& black,
	const int& white
) -> void
{
	// Normalize to [0.0, 1.0] based on black and white
	float norm = 0.0f;
	if (white > black)
	{
		int clamped = std::clamp(static_cast<int>(value), black, white);
		norm = static_cast<float>(clamped - black) / (white - black);
	}

	// Copper colormap coefficients
	constexpr float kRed = 1.0f;
	constexpr float kGreen = 0.7812f;
	constexpr float kBlue = 0.4975f;

	// Apply coefficients and scale to [0, 255]
	r = static_cast<unsigned char>(std::round(std::min(norm * kRed, 1.0f) * 255.0f));
	g = static_cast<unsigned char>(std::round(std::min(norm * kGreen, 1.0f) * 255.0f));
	b = static_cast<unsigned char>(std::round(std::min(norm * kBlue, 1.0f) * 255.0f));
}

void cCamPreview::OnMouseMoved(wxMouseEvent& evt)
{
	if (!m_IsImageSet) return;

	m_CursorPosOnCanvas.x = m_ZoomOnOriginalSizeImage * evt.GetPosition().x;
	m_CursorPosOnCanvas.y = m_ZoomOnOriginalSizeImage * evt.GetPosition().y;

	/* Mouse position on Image */
	CalculatePositionOnImage();
	CheckIfMouseAboveImage();

	UpdateCursorPositionOnStatusBar();

	ChangeCursorInDependenceOfCurrentParameters();

	if (m_CrossHairDrag.Any())
	{
		if (evt.LeftIsDown())
			MoveCrossHairTo(evt.GetPosition());
		else
			EndCrossHairDrag();
		const auto& d = m_CrossHairDrag;
		SetCursor(d.row && d.column ? wxCURSOR_SIZING : d.row ? wxCURSOR_SIZENS : wxCURSOR_SIZEWE);
		return;
	}

	if (!m_Panning && !m_MinimapDragging)
	{
		const auto hit = MinimapHitTest(evt.GetPosition()) ? CrossHairHit{} : HitTestCrossHair(evt.GetPosition());
		SetCrossHairHover(hit);
		if (hit.Any())
			SetCursor(hit.row && hit.column ? wxCURSOR_SIZING : hit.row ? wxCURSOR_SIZENS : wxCURSOR_SIZEWE);
	}

	if (m_MinimapDragging)
	{
		if (evt.LeftIsDown())
			CenterViewOnImagePoint(MinimapToImage(evt.GetPosition()));
		else
			m_MinimapDragging = false;
		SetCursor(wxCURSOR_HAND);
		return;
	}

	if (MinimapHitTest(evt.GetPosition()) && !m_Panning)
		SetCursor(wxCURSOR_HAND);

	if (m_Panning)
	{
		ProcessPan(m_CursorPosOnCanvas, true);
		m_CrossHairTool->SetImageStartDrawPos(wxRealPoint
		(
			m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
			m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
		));
	}
}

auto cCamPreview::OnEnterPanel(wxMouseEvent& evt) -> void
{
	SetFocus();
}

auto cCamPreview::OnLeavePanel(wxMouseEvent& evt) -> void
{
	// A line being dragged keeps the mouse captured, so it may leave the panel
	if (m_CrossHairDrag.Any()) return;

	m_CTRLPressed = false;
	SetCrossHairHover({});
	OnPreviewMouseLeftReleased(evt);
}

void cCamPreview::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
	EndCrossHairDrag();
}

auto cCamPreview::HitTestCrossHair(const wxPoint& canvasPos) const -> CrossHairHit
{
	CrossHairHit hit{};
	if (!m_DisplayCrossHair || !HasImage() || !m_CrossHairTool) return hit;

	const double k = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (k <= 0.0) return hit;

	const double ox = m_StartDrawPos.x * k, oy = m_StartDrawPos.y * k;
	const double imgRight = ox + m_ImageSize.GetWidth() * k, imgBottom = oy + m_ImageSize.GetHeight() * k;
	const double grab = 5.0; // px either side of a line

	const double lineX = ox + (m_CrossHairPos.x + 0.5) * k;
	const double lineY = oy + (m_CrossHairPos.y + 0.5) * k;
	const bool insideX = canvasPos.x >= ox - grab && canvasPos.x <= imgRight + grab;
	const bool insideY = canvasPos.y >= oy - grab && canvasPos.y <= imgBottom + grab;

	hit.column = m_CrossHairTool->GetShowVerticalLine() && insideY && std::abs(canvasPos.x - lineX) <= grab;
	hit.row = m_CrossHairTool->GetShowHorizontalLine() && insideX && std::abs(canvasPos.y - lineY) <= grab;
	return hit;
}

auto cCamPreview::MoveCrossHairTo(const wxPoint& canvasPos) -> void
{
	const double k = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (k <= 0.0 || !HasImage()) return;

	bool changed = false;
	if (m_CrossHairDrag.column)
	{
		const int x = std::clamp((int)std::floor(canvasPos.x / k - m_StartDrawPos.x), 0, m_ImageSize.GetWidth() - 1);
		if (x != m_CrossHairPos.x)
		{
			m_CrossHairPos.x = x;
			m_CrossHairTool->SetXPosFromParent(x);
			if (m_ParentArguments && m_ParentArguments->x_pos_crosshair)
				m_ParentArguments->x_pos_crosshair->ChangeValue(wxString::Format("%i", x + 1));
			changed = true;
		}
	}
	if (m_CrossHairDrag.row)
	{
		const int y = std::clamp((int)std::floor(canvasPos.y / k - m_StartDrawPos.y), 0, m_ImageSize.GetHeight() - 1);
		if (y != m_CrossHairPos.y)
		{
			m_CrossHairPos.y = y;
			m_CrossHairTool->SetYPosFromParent(y);
			if (m_ParentArguments && m_ParentArguments->y_pos_crosshair)
				m_ParentArguments->y_pos_crosshair->ChangeValue(wxString::Format("%i", y + 1));
			changed = true;
		}
	}

	if (changed)
	{
		// Repaint now so the line and its profile follow the mouse without lag
		Refresh(false);
		Update();
	}
}

auto cCamPreview::SetCrossHairHover(const CrossHairHit& hit) -> void
{
	if (hit.row == m_CrossHairHover.row && hit.column == m_CrossHairHover.column) return;
	m_CrossHairHover = hit;
	if (m_CrossHairTool) m_CrossHairTool->SetHighlight(hit.row, hit.column);
	Refresh(false);
}

auto cCamPreview::EndCrossHairDrag() -> void
{
	if (!m_CrossHairDrag.Any()) return;
	m_CrossHairDrag = {};
	if (HasCapture()) ReleaseMouse();
	Refresh(false);
}

void cCamPreview::OnMouseWheelMoved(wxMouseEvent& evt)
{
	if (m_Zoom <= m_MinZoom && evt.GetWheelRotation() < 0)
		return;

	if (evt.GetWheelRotation() > 0 && m_Zoom / m_ZoomOnOriginalSizeImage < m_MaxZoom)
	{
		AddZoom(m_ZoomStep);
	}
	else if (evt.GetWheelRotation() < 0)
	{
		if (m_Zoom > m_MinZoom)
			AddZoom(1 / m_ZoomStep);
	}

	/* CrossHair Tool */
	m_CrossHairTool->UpdateZoomValue(m_Zoom);
	m_CrossHairTool->CalculateCrossHairPositionOnCanvas();
	m_CrossHairTool->SetImageStartDrawPos(wxRealPoint
	(
		m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
		m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
	));
}

void cCamPreview::AddZoom(const double& zoom, bool zoom_in_center_of_window)
{
	wxRealPoint center = zoom_in_center_of_window ?
		wxRealPoint(GetSize().x / 2.0, GetSize().y / 2.0) :
		wxRealPoint(
			m_CursorPosOnCanvas.x,
			m_CursorPosOnCanvas.y);

	SetZoom(m_Zoom * zoom, center);
}

void cCamPreview::SetZoom(const double& zoom, const wxRealPoint& center_)
{
	wxRealPoint position_on_image{};
	position_on_image.x = (center_.x - m_PanOffset.x) / m_Zoom;
	position_on_image.y = (center_.y - m_PanOffset.y) / m_Zoom;

	auto isDefault = abs(zoom - m_MinZoom) < 1e-5 ? true : false;
	m_Zoom = isDefault ? m_MinZoom : zoom;

	m_PanOffset.x = m_Zoom > m_MinZoom ? center_.x - position_on_image.x * m_Zoom : 0.0;
	m_PanOffset.y = m_Zoom > m_MinZoom ? center_.y - position_on_image.y * m_Zoom : 0.0;

	m_StartDrawPos.x = m_PanOffset.x / m_Zoom + m_NotZoomedGraphicsBitmapOffset.x;
	m_StartDrawPos.y = m_PanOffset.y / m_Zoom + m_NotZoomedGraphicsBitmapOffset.y;
	Refresh();
}

void cCamPreview::ProcessPan(const wxRealPoint& point_, bool refresh_)
{
	m_PanDeltaPoints = m_PanStartPoint - point_;
	//LOG2F("PanStartPoint x: ", m_PanStartPoint.x, " y: ", m_PanStartPoint.y);
	m_StartDrawPos.x = m_PanOffset.x / m_Zoom - m_PanDeltaPoints.x / m_Zoom + m_NotZoomedGraphicsBitmapOffset.x;
	m_StartDrawPos.y = m_PanOffset.y / m_Zoom - m_PanDeltaPoints.y / m_Zoom + m_NotZoomedGraphicsBitmapOffset.y;
	if (refresh_) Refresh();
}

void cCamPreview::FinishPan(bool refresh)
{
	if (m_Panning)
	{
		m_PanOffset.x -= m_PanDeltaPoints.x;
		m_PanOffset.y -= m_PanDeltaPoints.y;

		m_PanDeltaPoints = { 0.0, 0.0 };

		m_Panning = false;

		if (refresh) Refresh();
	}
}

void cCamPreview::CheckIfMouseAboveImage()
{
	m_IsCursorInsideImage = false;
	wxRealPoint cursor_pos_on_image =
	{
		m_CursorPosOnCanvas.x / m_Zoom - m_StartDrawPos.x,
		m_CursorPosOnCanvas.y / m_Zoom - m_StartDrawPos.y
	};
	if ((cursor_pos_on_image.x >= 0.0 &&
		(int)cursor_pos_on_image.x < m_ImageSize.GetWidth()) &&
		(cursor_pos_on_image.y >= 0.0 &&
			(int)cursor_pos_on_image.y < m_ImageSize.GetHeight()))
		m_IsCursorInsideImage = true;
}

void cCamPreview::CalculatePositionOnImage()
{
	m_NotCheckedCursorPosOnImage.x = m_CursorPosOnCanvas.x / m_Zoom - m_StartDrawPos.x;
	m_NotCheckedCursorPosOnImage.y = m_CursorPosOnCanvas.y / m_Zoom - m_StartDrawPos.y;

	/* Checking X */
	m_CheckedCursorPosOnImage.x = m_NotCheckedCursorPosOnImage.x >= 0.0 ? m_NotCheckedCursorPosOnImage.x : 0.0;
	m_CheckedCursorPosOnImage.x = m_NotCheckedCursorPosOnImage.x < (double)m_ImageSize.GetWidth() ? m_CheckedCursorPosOnImage.x : (double)m_ImageSize.GetWidth() - m_MinZoom;
	/* Checking Y */
	m_CheckedCursorPosOnImage.y = m_NotCheckedCursorPosOnImage.y >= 0.0 ? m_NotCheckedCursorPosOnImage.y : 0.0;
	m_CheckedCursorPosOnImage.y = m_NotCheckedCursorPosOnImage.y < (double)m_ImageSize.GetHeight() ? m_CheckedCursorPosOnImage.y : (double)m_ImageSize.GetHeight() - m_MinZoom;
}

void cCamPreview::OnPreviewMouseLeftPressed(wxMouseEvent& evt)
{
	if (MinimapHitTest(evt.GetPosition()))
	{
		m_MinimapDragging = true;
		CenterViewOnImagePoint(MinimapToImage(evt.GetPosition()));
		return;
	}

	if (!m_CTRLPressed)
	{
		const auto hit = HitTestCrossHair(evt.GetPosition());
		if (hit.Any())
		{
			m_CrossHairDrag = hit;
			SetCrossHairHover(hit);
			if (!HasCapture()) CaptureMouse();
			MoveCrossHairTo(evt.GetPosition());
			return;
		}
	}

	if (m_Zoom > m_MinZoom && m_IsCursorInsideImage && !m_CTRLPressed)
	{
		m_Panning = true;
		m_PanStartPoint = m_CursorPosOnCanvas;
		ChangeCursorInDependenceOfCurrentParameters();
	}

	if (!m_CTRLPressed) return;

	// CTRL is Active

	if (m_CrossHairTool->IsToolButtonActive())
	{
		m_ParentArguments->x_pos_crosshair->SetValue(CameraPreviewVariables::CreateStringWithPrecision(m_CheckedCursorPosOnImage.x + 1));
		m_ParentArguments->y_pos_crosshair->SetValue(CameraPreviewVariables::CreateStringWithPrecision(m_CheckedCursorPosOnImage.y + 1));

		Refresh();
	}

	if (m_DisplayAnnulus)
	{
		m_ParentArguments->xAnnulusCenterPos->ChangeValue(CameraPreviewVariables::CreateStringWithPrecision((int)m_CheckedCursorPosOnImage.x + 1));
		m_ParentArguments->yAnnulusCenterPos->SetValue(CameraPreviewVariables::CreateStringWithPrecision((int)m_CheckedCursorPosOnImage.y + 1));

		Refresh();
	}
}

void cCamPreview::OnPreviewMouseLeftReleased(wxMouseEvent& evt)
{
	m_MinimapDragging = false;
	EndCrossHairDrag();
	if (m_Panning)
	{
		FinishPan(true);
		m_Panning = false;
	}
}

void cCamPreview::ChangeCursorInDependenceOfCurrentParameters()
{
	auto current_cursor = wxCURSOR_DEFAULT;

	/* CrossHair Tool */
	//current_cursor = m_SettingCrossHairPos ? wxCURSOR_QUESTION_ARROW : current_cursor;
	current_cursor = m_CrossHairTool->UpdateCursor(current_cursor);

	SetCursor(current_cursor);
}

void cCamPreview::DrawCrossHair(wxGraphicsContext* graphics_context)
{
	if (!m_DisplayCrossHair || !HasImage()) return;

	const double k = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (k <= 0.0) return;

	ToolsVariables::ProfileView view{};
	view.data = m_ImageData.get();
	view.imageSize = m_ImageSize;
	view.scale = k;
	view.origin = wxRealPoint(m_StartDrawPos.x * k, m_StartDrawPos.y * k);
	view.canvas = GetClientSize();
	view.position = wxPoint(m_CrossHairPos.x, m_CrossHairPos.y);
	view.revision = m_ImageRevision;
	m_CrossHairTool->DrawCrossHair(graphics_context, view);
}

auto cCamPreview::DrawPixelValues(wxGraphicsContext* gc) -> void
{
	if (!m_DisplayPixelValues) return;

	if (!m_ImageData) return;

	if (m_Zoom / m_ZoomOnOriginalSizeImage < m_MaxZoom) return;

	auto CheckIfPixelValueIsInsideTheImage = [&](const int& x, const int& y)
		{
			if (x < 0 || x >= m_ImageSize.GetWidth()) return false;
			if (y < 0 || y >= m_ImageSize.GetHeight()) return false;
			return true;
		};

	wxRealPoint m_ActualHalfPixelSize =
	{
		m_Zoom / m_ZoomOnOriginalSizeImage / 2.0,
		m_Zoom / m_ZoomOnOriginalSizeImage / 2.0,
	};
	if (m_ActualHalfPixelSize.x < 32.0 || m_ActualHalfPixelSize.y < 32.0) return;

	// Setting up the current font
	wxColour fontColour(180, 20, 30, 200);
	wxFont font = wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc->SetFont(font, fontColour);

	wxRealPoint drawPoint{};
	wxSize window_disp_size
	{
		(int)(m_CanvasSize.GetWidth() / m_Zoom * m_ZoomOnOriginalSizeImage),
		(int)(m_CanvasSize.GetHeight() / m_Zoom * m_ZoomOnOriginalSizeImage)
		//(int)(m_ImageSize.GetWidth() / (m_Zoom * m_MinZoom) * m_ZoomOnOriginalSizeImage),
		//(int)(m_ImageSize.GetHeight() / (m_Zoom * m_MinZoom) * m_ZoomOnOriginalSizeImage)
	};
	auto image_start_draw = wxRealPoint
	(
		m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
		m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
	);
	wxDouble widthText{}, heightText{};
	wxString curr_value{};
	wxPoint left_upper_pixel{};
	/* Calculation of currently displayed window */
	{
		/* Checking X */
		if (image_start_draw.x >= 0)
			left_upper_pixel.x = 0;
		else
			left_upper_pixel.x = floor(fabs(image_start_draw.x / (m_ActualHalfPixelSize.x * 2.0)));

		/* Checking Y */
		if (image_start_draw.y >= 0)
			left_upper_pixel.y = 0;
		else
			left_upper_pixel.y = floor(fabs(image_start_draw.y / (m_ActualHalfPixelSize.y * 2.0)));

		/* Actual drawing */
		for (auto y{ left_upper_pixel.y }; y < left_upper_pixel.y + window_disp_size.GetHeight() + 1; ++y)
		{
			for (auto x{ left_upper_pixel.x }; x < left_upper_pixel.x + window_disp_size.GetWidth() + 1; ++x)
			{
				if (!CheckIfPixelValueIsInsideTheImage(x, y)) continue;
				
				auto bgColor = GetImagePixelColour(x, y);
				wxColour textColor(255 - bgColor.Red(), 255 - bgColor.Green(), 255 - bgColor.Blue());
				textColor = bgColor.GetRed() == bgColor.GetGreen() && bgColor.GetGreen() == bgColor.GetBlue() && bgColor.GetRed() == bgColor.GetBlue() ? m_ContrastDefaultColor : textColor;
				gc->SetFont(font, textColor);

				curr_value = wxString::Format(wxT("%i"), m_ImageData[y * m_ImageSize.GetWidth() + x]);
				gc->GetTextExtent(curr_value, &widthText, &heightText);
				drawPoint.x = image_start_draw.x + x * m_ActualHalfPixelSize.x * 2.0;
				drawPoint.x += m_ActualHalfPixelSize.x - widthText / 2.0;
				drawPoint.y = image_start_draw.y + y * m_ActualHalfPixelSize.y * 2.0;
				drawPoint.y += m_ActualHalfPixelSize.y - heightText / 2.0;
				gc->DrawText(curr_value, drawPoint.x, drawPoint.y);
			}
		}
	}
}

auto cCamPreview::DrawAnnulus(wxGraphicsContext* gc) -> void
{
	if (!m_DisplayAnnulus) return;

	if (m_AnnulusVec.empty() || m_ActivatedAnnulusNum == -1) return;

	if (!m_ImageData) return;

	auto CheckIfPixelValueIsInsideTheImage = [&](const int& x, const int& y)
		{
			if (x < 0 || x >= m_ImageSize.GetWidth()) return false;
			if (y < 0 || y >= m_ImageSize.GetHeight()) return false;
			return true;
		};

	wxRealPoint actualHalfPixelSize =
	{
		m_Zoom / m_ZoomOnOriginalSizeImage / 2.0,
		m_Zoom / m_ZoomOnOriginalSizeImage / 2.0,
	};

	wxRealPoint drawPoint{};
	wxSize canvasSize
	{
		(int)(m_CanvasSize.GetWidth() / m_Zoom * m_ZoomOnOriginalSizeImage),
		(int)(m_CanvasSize.GetHeight() / m_Zoom * m_ZoomOnOriginalSizeImage)
	};

	auto imageStartDrawPoint = wxRealPoint
	(
		m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
		m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
	);

	auto currAnnulus = m_AnnulusVec[m_ActivatedAnnulusNum];

	gc->SetPen(wxPen(m_ContrastDefaultColor, 4));

	// Compute actual center on canvas
	const double centerX = currAnnulus.m_Center.x * actualHalfPixelSize.x * 2 + imageStartDrawPoint.x + actualHalfPixelSize.x;
	const double centerY = currAnnulus.m_Center.y * actualHalfPixelSize.y * 2 + imageStartDrawPoint.y + actualHalfPixelSize.y;

	// Draw Center Cross Rotated 45 Degrees (an 'X')
	{
		const double crossSize = 10.0; // in pixels, adjust based on zoom if needed

		// Compute 45° rotated cross points
		const double offset = crossSize / std::sqrt(2.0); // diagonal offset for 45° lines

		wxPoint2DDouble p1(centerX - offset, centerY - offset);
		wxPoint2DDouble p2(centerX + offset, centerY + offset);
		wxPoint2DDouble p3(centerX - offset, centerY + offset);
		wxPoint2DDouble p4(centerX + offset, centerY - offset);

		wxGraphicsPath crossPath = gc->CreatePath();
		crossPath.MoveToPoint(p1);
		crossPath.AddLineToPoint(p2);
		crossPath.MoveToPoint(p3);
		crossPath.AddLineToPoint(p4);
		gc->StrokePath(crossPath);
	}

	const bool isFullyZoomed = !((m_Zoom / m_ZoomOnOriginalSizeImage) < m_MaxZoom);

	if (isFullyZoomed)
	{
		const int r1Sq = currAnnulus.m_InnerRadius * currAnnulus.m_InnerRadius;
		const int r2Sq = currAnnulus.m_OuterRadius * currAnnulus.m_OuterRadius;

		const int xStart = std::max(0, currAnnulus.m_Center.x - (int)currAnnulus.m_OuterRadius);
		const int xEnd = std::min(m_ImageSize.GetWidth() - 1, currAnnulus.m_Center.x + (int)currAnnulus.m_OuterRadius);
		const int yStart = std::max(0, currAnnulus.m_Center.y - (int)currAnnulus.m_OuterRadius);
		const int yEnd = std::min(m_ImageSize.GetHeight() - 1, currAnnulus.m_Center.y + (int)currAnnulus.m_OuterRadius);

		wxColour pixelOverlayColor = m_ContrastDefaultColor;
		pixelOverlayColor.Set(pixelOverlayColor.Red(), pixelOverlayColor.Green(), pixelOverlayColor.Blue(), 100); // alpha

		wxBrush pixelBrush(pixelOverlayColor);
		gc->SetBrush(pixelBrush);
		gc->SetPen(*wxTRANSPARENT_PEN);

		const double pixelDrawWidth = actualHalfPixelSize.x * 2.0;
		const double pixelDrawHeight = actualHalfPixelSize.y * 2.0;

		ForEachPixelInAnnulus(currAnnulus, [&](int x, int y, uint16_t)
			{
				// Compute canvas position
				const double canvasX = x * pixelDrawWidth + imageStartDrawPoint.x;
				const double canvasY = y * pixelDrawHeight + imageStartDrawPoint.y;

				// Check if within canvas boundaries (after pan/zoom)
				if (canvasX + pixelDrawWidth < 0 || canvasX > m_CanvasSize.GetWidth()) return;
				if (canvasY + pixelDrawHeight < 0 || canvasY > m_CanvasSize.GetHeight()) return;

				gc->DrawRectangle(canvasX, canvasY, pixelDrawWidth, pixelDrawHeight);
			});
	}
	else
	{
		// fallback to drawing semi-transparent ring as before
		const double rInner = currAnnulus.m_InnerRadius * actualHalfPixelSize.y * 2;
		const double rOuter = currAnnulus.m_OuterRadius * actualHalfPixelSize.y * 2;

		if (rOuter > 0.0)
		{
			wxGraphicsPath ringPath = gc->CreatePath();
			ringPath.AddCircle(centerX, centerY, rOuter);
			ringPath.AddCircle(centerX, centerY, rInner);

			wxColour fillColor = m_ContrastDefaultColor;
			fillColor.Set(fillColor.Red(), fillColor.Green(), fillColor.Blue(), 80);

			gc->SetBrush(gc->CreateBrush(wxBrush(fillColor, wxBRUSHSTYLE_SOLID)));
			gc->FillPath(ringPath);
		}
	}

	gc->SetPen(wxPen(m_ContrastDefaultColor, 4));

	// Draw Inner Radius
	if (currAnnulus.m_InnerRadius)
	{
		wxGraphicsPath path = gc->CreatePath();
		path.AddCircle
		(
			centerX,
			centerY,
			currAnnulus.m_InnerRadius * actualHalfPixelSize.y * 2
		);

		gc->StrokePath(path);
	}

	// Draw Outer Radius
	if (currAnnulus.m_OuterRadius)
	{
		wxGraphicsPath path = gc->CreatePath();
		path.AddCircle
		(
			centerX,
			centerY,
			currAnnulus.m_OuterRadius * actualHalfPixelSize.y * 2
		);

		gc->StrokePath(path);
	}
}

auto cCamPreview::DrawImageStatistics(wxGraphicsContext* gc) -> void
{
	if (!m_ShowImageStats || !m_LastStats.valid) return;

	const auto& s = m_LastStats;

	// Integers use a comma as the thousands separator (formatted manually so
	// we do not depend on an OS locale being installed, and do not construct
	// a std::locale on every repaint).
	auto format_number = [](auto value) -> wxString
		{
			return CameraPreviewVariables::FormatWithThousandsDelimiter(static_cast<long long>(value), ",");
		};

	auto format_scientific10 = [](unsigned long long v) -> wxString
		{
			if (v == 0ull) return "0";
			std::ostringstream os;
			os.setf(std::ios::scientific | std::ios::uppercase);
			os << std::setprecision(3) << static_cast<long double>(v);
			return wxString::FromUTF8(os.str());
		};

	const std::pair<wxString, wxString> rows[] =
	{
		{ "Min",           format_number(s.minV) },
		{ "Max",           format_number(s.maxV) },
		{ "Sum",           format_scientific10(s.sum) },
		{ "Mean",          wxString::Format("%.2f", s.mean) },
		{ "StdDev",        wxString::Format("%.2f", s.stddev) },
		{ "Black-clipped", format_number(s.blackClippedPixelsCount) },
		{ "Saturated",     format_number(s.saturatedPixelsCount) },
	};

	const wxFont labelFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	const wxFont valueFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	const wxColour labelColour(200, 200, 200);
	const wxColour valueColour(255, 255, 255);
	const wxColour alertColour(255, 110, 90);

	// Measure both columns
	wxDouble labelColW{}, valueColW{}, rowH{};
	for (const auto& [label, value] : rows)
	{
		wxDouble w{}, h{};
		gc->SetFont(labelFont, labelColour);
		gc->GetTextExtent(label, &w, &h);
		labelColW = std::max(labelColW, w);
		rowH = std::max(rowH, h);

		gc->SetFont(valueFont, valueColour);
		gc->GetTextExtent(value, &w, &h);
		valueColW = std::max(valueColW, w);
		rowH = std::max(rowH, h);
	}

	const wxDouble pad = 8.0;
	const wxDouble colGap = 14.0;
	const wxDouble lineGap = 2.0;
	const wxDouble rx = 6.0;
	const auto rowCount = static_cast<int>(std::size(rows));

	const wxDouble w = pad + labelColW + colGap + valueColW + pad;
	const wxDouble h = pad + rowCount * rowH + (rowCount - 1) * lineGap + pad;

	// Badge sits in the bottom-left corner, so it never collides with the FPS
	// badge (top-left) or the image-size annotations (left / bottom sides).
	const wxDouble margin = 6.0;
	const wxDouble x = margin;
	const wxDouble y = std::max(margin, GetSize().GetHeight() - margin - h);

	gc->SetBrush(wxBrush(wxColour(20, 20, 24, 170)));
	gc->SetPen(wxPen(wxColour(255, 255, 255, 40), 1));
	wxGraphicsPath path = gc->CreatePath();
	path.AddRoundedRectangle(x, y, w, h, rx);
	gc->DrawPath(path);

	for (int i = 0; i < rowCount; ++i)
	{
		const auto& [label, value] = rows[i];
		const wxDouble rowY = y + pad + i * (rowH + lineGap);

		gc->SetFont(labelFont, labelColour);
		gc->DrawText(label, x + pad, rowY);

		// Highlight non-zero clipping counts — they usually mean the exposure needs attention
		const bool isClippingRow = (i >= rowCount - 2);
		const bool alert = isClippingRow &&
			((i == rowCount - 2 && s.blackClippedPixelsCount > 0) || (i == rowCount - 1 && s.saturatedPixelsCount > 0));

		wxDouble vw{}, vh{};
		gc->SetFont(valueFont, alert ? alertColour : valueColour);
		gc->GetTextExtent(value, &vw, &vh);
		gc->DrawText(value, x + w - pad - vw, rowY); // right-aligned numbers
	}
}

auto cCamPreview::OnKeyPressed(wxKeyEvent& evt) -> void
{
	/* Ctrl */
	if (evt.ControlDown() && !evt.AltDown() && !evt.ShiftDown() && evt.GetKeyCode() == WXK_CONTROL)
	{
		LOG("CTRL DOWN");
		m_CTRLPressed = true;
		//ChangeCursorInDependenceOfCurrentParameters();
	}
	else
		evt.Skip();
}

auto cCamPreview::OnKeyReleased(wxKeyEvent& evt) -> void
{
	/* Ctrl */
	if (!evt.ControlDown() && evt.GetKeyCode() == WXK_CONTROL)
	{
		LOG("CTRL UP");
		// Ctrl key is released
		m_CTRLPressed = false;
		//ChangeCursorInDependenceOfCurrentParameters();
	}
	else
		evt.Skip(); // Allow other event handlers to process the event
}

auto cCamPreview::CalculateFWHM() -> void
{
	unsigned int minValueInHorizontalData{}, minValueInVerticalData{};
	// Create threads for horizontal and vertical calculations
	std::thread verticalThread([&]()
		{
			try 
			{
				PostprocessingAlgorithms::CalculateSumVertically
				(
					m_ImageData.get(),
					m_ImageSize.GetWidth(),
					m_ImageSize.GetHeight(),
					m_HorizontalSumArray.get()
				);

				LOG("CalculatedSumVertically: " + wxString(__FUNCSIG__));

				m_VerticalFWHM_PX = PostprocessingAlgorithms::CalculateVerticalFWHM
				(
					m_ImageData.get(),
					m_HorizontalSumArray.get(),
					m_ImageSize.GetWidth(),
					m_ImageSize.GetHeight(),
					&m_HorizontalWorstSum,
					&m_HorizonalBestSum,
					&m_VerticalMiddleFWHMPosPixel,
					&minValueInVerticalData
				);
			}
			catch (const std::exception& e) 
			{
				LOG("Exception in vertical thread: " + wxString(e.what()));
			}
		});

	std::thread horizontalThread([&]()
		{
			try 
			{
				PostprocessingAlgorithms::CalculateSumHorizontally
				(
					m_ImageData.get(),
					m_ImageSize.GetWidth(),
					m_ImageSize.GetHeight(),
					m_VerticalSumArray.get()
				);

				LOG("CalculatedSumHorizontally: " + wxString(__FUNCSIG__));

				m_HorizontalFWHM_PX = PostprocessingAlgorithms::CalculateHorizontalFWHM
				(
					m_ImageData.get(),
					m_VerticalSumArray.get(),
					m_ImageSize.GetWidth(),
					m_ImageSize.GetHeight(),
					&m_VerticalWorstSum,
					&m_VerticalBestSum,
					&m_HorizontalMiddleFWHMPosPixel,
					&minValueInHorizontalData
				);
			}
			catch (const std::exception& e) 
			{
				LOG("Exception in vertical thread: " + wxString(e.what()));
			}
		});

	// Wait for both threads to complete execution
	horizontalThread.join();
	verticalThread.join();

	m_MinValueInData = (minValueInHorizontalData + minValueInVerticalData) / 2;

#ifdef _DEBUG
	// Exporting Horizontal Sum Data
	{
		auto flags = std::ios::binary;

		const auto fileName = "horizontalSum.txt";
		std::ofstream ofs(fileName, flags);
		if (!ofs)
			throw std::runtime_error("Failed to open file for writing.");

		for (auto i{ 0 }; i < m_ImageSize.GetWidth(); ++i)
			ofs << m_HorizontalSumArray[i] << '\n';

		ofs.close();
	}

	// Exporting Vertical Sum Data
	{
		auto flags = std::ios::binary;

		const auto fileName = "verticalSum.txt";
		std::ofstream ofs(fileName, flags);
		if (!ofs)
			throw std::runtime_error("Failed to open file for writing.");

		for (auto i{ 0 }; i < m_ImageSize.GetHeight(); ++i)
			ofs << m_VerticalSumArray[i] << '\n';

		ofs.close();
	}
#endif // _DEBUG
}

auto cCamPreview::CalculateHEW() -> void
{
	m_HEWDiameter = PostprocessingAlgorithms::CalculateHEWRadius
	(
		m_ImageData.get(),
		m_ImageSize.GetWidth(),
		m_ImageSize.GetHeight(),
		m_HorizontalMiddleFWHMPosPixel,
		m_VerticalMiddleFWHMPosPixel,
		m_MinValueInData
	);

	m_HEWDiameter *= 2;
}

auto cCamPreview::CalculateImageStatistics() -> void
{
	const size_t N = static_cast<size_t>(m_ImageSize.GetWidth()) * m_ImageSize.GetHeight();
	if (!N || !m_ImageData)
	{
		m_LastStats = {};
		return;
	}

	const unsigned short maxValue = m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4'095 : USHRT_MAX;

	// One pass split over the cores; integer sums are exact (65535^2 * 2^32 fits in 64 bits).
	struct Partial { unsigned short mn{ USHRT_MAX }, mx{}; unsigned long long sum{}, sum2{}, black{}, saturated{}; };
	const unsigned int numThreads = N < 1'000'000 ? 1u : std::clamp(std::thread::hardware_concurrency(), 1u, 16u);
	std::vector<Partial> parts(numThreads);
	const size_t chunk = (N + numThreads - 1) / numThreads;
	const unsigned short* data = m_ImageData.get();

	auto work = [&, data](unsigned int t)
		{
			Partial p{};
			const size_t begin = t * chunk, end = std::min(N, begin + chunk);
			for (size_t i = begin; i < end; ++i)
			{
				const unsigned short v = data[i];
				if (v < p.mn) p.mn = v;
				if (v > p.mx) p.mx = v;
				p.sum += v;
				p.sum2 += static_cast<unsigned long long>(v) * v;
				p.black += (v == 0);
				p.saturated += (v >= maxValue);
			}
			parts[t] = p;
		};

	if (numThreads == 1) work(0);
	else
	{
		std::vector<std::thread> threads;
		threads.reserve(numThreads);
		for (unsigned int t = 0; t < numThreads; ++t) threads.emplace_back(work, t);
		for (auto& th : threads) th.join();
	}

	Partial total{};
	for (const auto& p : parts)
	{
		total.mn = std::min(total.mn, p.mn);
		total.mx = std::max(total.mx, p.mx);
		total.sum += p.sum;
		total.sum2 += p.sum2;
		total.black += p.black;
		total.saturated += p.saturated;
	}

	const double mean = static_cast<double>(total.sum) / N;
	const double var = std::max(0.0, static_cast<double>(total.sum2) / N - mean * mean);
	m_LastStats =
	{
		total.mn,
		total.mx,
		total.sum,
		mean,
		std::sqrt(var),
		total.black,
		total.saturated,
		true
	};
}

void cCamPreview::TickFPS()
{
	const auto now = std::chrono::steady_clock::now();

	if (m_LastFrameTS.time_since_epoch().count() != 0) {
		const double dt = std::chrono::duration<double>(now - m_LastFrameTS).count();
		if (dt > 0.0) {
			const double fps = 1.0 / dt;
			// warm start EMA with first value
			if (m_FPSEMA == 0.0) m_FPSEMA = fps;
			// smoother EMA: keep history (alpha ~ 0.2 feels good)
			m_FPSEMA = 0.2 * fps + 0.8 * m_FPSEMA;
			m_FPSInstant = fps;
		}
	}
	m_LastFrameTS = now;
}

void cCamPreview::DrawFPS(wxGraphicsContext* gc)
{
	if (!m_ShowFPS) return;

	if (m_FPSInstant <= 0.0) return;

	// Text
	wxString txt = wxString::Format("FPS: %.1f", m_FPSInstant);
	txt += m_FPSInstant > 0.0 ? wxString::Format(" (avg %.1f)", m_FPSEMA) : wxString("");

	m_FrameCount = m_FPSInstant == 0.0 ? 0 : m_FrameCount;

	if (m_FPSInstant > 0.0)
	{
		txt += " | Frames: ";
		txt += CameraPreviewVariables::FormatWithThousandsDelimiter(static_cast<long long>(m_FrameCount) + 1);
	}

	// Font & metrics
	wxFont font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc->SetFont(font, *wxWHITE);
	wxDouble tw{}, th{};
	gc->GetTextExtent(txt, &tw, &th);

	// Badge rect (top-left) — same look as the statistics badge
	const wxDouble pad = 8.0;
	const wxDouble rx = 6.0; // rounded corners
	wxDouble x = 6.0, y = 6.0;
	wxDouble w = tw + 2 * pad, h = th + 2 * pad;

	gc->SetBrush(wxBrush(wxColour(20, 20, 24, 170)));
	gc->SetPen(wxPen(wxColour(255, 255, 255, 40), 1));
	wxGraphicsPath path = gc->CreatePath();
	path.AddRoundedRectangle(x, y, w, h, rx);
	gc->DrawPath(path);

	// Text
	gc->DrawText(txt, x + pad, y + pad);

	m_TopLeftOverlayBottom = std::max(m_TopLeftOverlayBottom, y + h);
}

auto cCamPreview::DrawEmptyState(wxGraphicsContext* gc) -> void
{
	if (!gc) return;

	const wxSize size = GetClientSize();
	if (size.GetWidth() < 120 || size.GetHeight() < 120) return;

	// Pick foreground colours that read well on both the light and the dark background
	const wxColour bg = GetBackgroundColour();
	const bool darkBg = (0.299 * bg.Red() + 0.587 * bg.Green() + 0.114 * bg.Blue()) < 128.0;
	const wxColour primary = darkBg ? wxColour(210, 210, 210) : wxColour(70, 70, 70);
	const wxColour secondary = darkBg ? wxColour(150, 150, 150) : wxColour(120, 120, 120);
	const wxColour frame = darkBg ? wxColour(255, 255, 255, 45) : wxColour(0, 0, 0, 45);

	const int iconSide = std::clamp(std::min(size.GetWidth(), size.GetHeight()) / 6, 32, 96);

	const wxString title = "No image";
	const wxString hint = "Capture a frame with the camera controls on the right,\nor open an existing image via File → Open (Ctrl+O).";

	const wxFont titleFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	const wxFont hintFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

	wxDouble titleW{}, titleH{}, hintW{}, hintH{};
	gc->SetFont(titleFont, primary);
	gc->GetTextExtent(title, &titleW, &titleH);
	gc->SetFont(hintFont, secondary);
	gc->GetTextExtent(hint, &hintW, &hintH);

	const wxDouble gap = 12.0;
	const wxDouble blockH = iconSide + gap + titleH + gap / 2 + hintH;
	const wxDouble cx = size.GetWidth() / 2.0;
	wxDouble top = (size.GetHeight() - blockH) / 2.0;

	// Dashed frame around the message
	{
		const wxDouble boxW = std::max<wxDouble>(hintW, titleW) + 64.0;
		const wxDouble boxH = blockH + 48.0;
		wxPen pen(frame, 2, wxPENSTYLE_SHORT_DASH);
		gc->SetPen(pen);
		gc->SetBrush(*wxTRANSPARENT_BRUSH);
		gc->DrawRoundedRectangle(cx - boxW / 2.0, top - 24.0, boxW, boxH, 10.0);
	}

	auto bmp = wxMaterialDesignArtProvider::GetBitmap
	(
		wxART_CAMERA,
		wxART_CLIENT_FLUENTUI_FILLED,
		wxSize(iconSide, iconSide),
		secondary
	);
	if (bmp.IsOk())
		gc->DrawBitmap(bmp, cx - iconSide / 2.0, top, iconSide, iconSide);
	top += iconSide + gap;

	gc->SetFont(titleFont, primary);
	gc->DrawText(title, cx - titleW / 2.0, top);
	top += titleH + gap / 2;

	// Centre each hint line individually
	gc->SetFont(hintFont, secondary);
	for (const auto& line : wxSplit(hint, '\n'))
	{
		wxDouble lw{}, lh{};
		gc->GetTextExtent(line, &lw, &lh);
		gc->DrawText(line, cx - lw / 2.0, top);
		top += lh;
	}
}

auto cCamPreview::AddAnnulusOnCurrentImage() -> CameraPreviewVariables::Annulus
{
	CameraPreviewVariables::Annulus annulus{};
	if (!m_ImageSize.GetWidth() || !m_ImageSize.GetHeight()) return annulus;

	CalculateFWHM();

	auto maxHorizontalElem = std::max_element(m_HorizontalSumArray.get(), m_HorizontalSumArray.get() + m_ImageSize.GetWidth());
	auto maxVerticalElem = std::max_element(m_VerticalSumArray.get(), m_VerticalSumArray.get() + m_ImageSize.GetHeight());
	auto centerPos = wxPoint
	(
		std::distance(m_HorizontalSumArray.get(), maxHorizontalElem),
		std::distance(m_VerticalSumArray.get(), maxVerticalElem)
	);

	annulus.m_Center = centerPos;

	// Compute distance to all four image edges
	const int distToLeft = centerPos.x;
	const int distToRight = m_ImageSize.GetWidth() - 1 - centerPos.x;
	const int distToTop = centerPos.y;
	const int distToBottom = m_ImageSize.GetHeight() - 1 - centerPos.y;

	// Smallest distance to ensure the annulus stays fully inside the image
	annulus.m_OuterRadius = std::min({ distToLeft, distToRight, distToTop, distToBottom });

	// Optional: set a default inner radius (e.g. 25% of outer)
	annulus.m_InnerRadius = annulus.m_OuterRadius / 4;

	CalculateSumInsideAnnulus(annulus);

	m_AnnulusVec.push_back(annulus);

	return annulus;
}

auto cCamPreview::RemoveAnnulusByID(const long& id) -> void
{
	// Remember which annulus was selected (by ID), because erasing shifts the indices
	long selectedID = -1;
	const bool hadSelection = m_ActivatedAnnulusNum >= 0 && m_ActivatedAnnulusNum < (int)m_AnnulusVec.size();
	if (hadSelection)
		selectedID = m_AnnulusVec[m_ActivatedAnnulusNum].GetID();

	std::erase_if(m_AnnulusVec, [id](const CameraPreviewVariables::Annulus& a) 
		{ return a.GetID() == id; });

	m_ActivatedAnnulusNum = -1;
	if (hadSelection && selectedID != id)
	{
		for (int i = 0; i < (int)m_AnnulusVec.size(); ++i)
		{
			if (m_AnnulusVec[i].GetID() == selectedID)
			{
				m_ActivatedAnnulusNum = i;
				break;
			}
		}
	}
}

auto cCamPreview::CalculateSumInsideAnnulus(CameraPreviewVariables::Annulus& annulus) -> void
{
	if (!m_ImageData) return;

	unsigned long long sum = 0;

	ForEachPixelInAnnulus(annulus, [&](int x, int y, uint16_t pixelValue)
		{
			sum += pixelValue;
		});

	annulus.m_Sum = sum;
}

auto cCamPreview::SetAnnulusIDSelected(const long& id) -> CameraPreviewVariables::Annulus
{
	CameraPreviewVariables::Annulus selectedAnnulus{};
	if (m_AnnulusVec.empty()) return selectedAnnulus;

	m_ActivatedAnnulusNum = -1;

	auto i = 0;
	for (const auto& annulus : m_AnnulusVec)
	{
		if (annulus.GetID() == id)
		{
			m_ActivatedAnnulusNum = i;
			selectedAnnulus = annulus;
			break;
		}
		++i;
	}

	return selectedAnnulus;
}

auto cCamPreview::UpdateAnnulusValues(CameraPreviewVariables::Annulus& annulus) -> void
{
	if (m_AnnulusVec.empty()) return;
	if (m_ActivatedAnnulusNum == -1) return;

	annulus.m_Center.x = std::clamp(annulus.m_Center.x, 0, m_ImageSize.GetWidth() - 1);
	annulus.m_Center.y = std::clamp(annulus.m_Center.y, 0, m_ImageSize.GetHeight() - 1);

	// Ensure a valid range for std::clamp (hi >= lo), e.g. when the spot sits on an image edge
	annulus.m_OuterRadius = std::max(annulus.m_OuterRadius, 1.5);
	annulus.m_InnerRadius = std::clamp(annulus.m_InnerRadius, 1.0, annulus.m_OuterRadius - 0.5);
	annulus.m_OuterRadius = std::max(annulus.m_InnerRadius + 0.5, annulus.m_OuterRadius);

	CalculateSumInsideAnnulus(annulus);
	m_AnnulusVec[m_ActivatedAnnulusNum] = annulus;
}

auto cCamPreview::ResetFPS() -> void
{
	m_FPSInstant = 0.0;
	m_FPSEMA = 0.0;
	m_LastFrameTS = {};      // important: prevents the next dt from spanning “stopped” time
	if (m_ShowFPS) Refresh(); // force redraw so the overlay immediately shows 0
}

void cCamPreview::InitDefaultComponents()
{
	/* Tools */
	m_CrossHairTool = std::make_unique<CrossHairTool>
		(
			m_ParentArguments->x_pos_crosshair, 
			m_ParentArguments->y_pos_crosshair
		);
}

void cCamPreview::PaintEvent(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	Render(dc);
}

void cCamPreview::Render(wxBufferedPaintDC& dc)
{
	SetExecutionFinished(false);

	dc.Clear();

	DrawCameraCapturedImage(dc);

	if (!m_IsImageSet)
	{
		if (auto gc = wxGraphicsContext::Create(dc))
		{
			DrawEmptyState(gc);
			delete gc;
		}
		SetExecutionFinished(true);
		return;
	}

	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc)
		{
			SetExecutionFinished(true);
			return;
		}

		m_TopLeftOverlayBottom = 0.0;

		DrawActualZoomedPositionOverImage(gc);
		DrawROI(gc);
		DrawScaleBar(gc);

		DrawCrossHair(gc);
		DrawPixelValues(gc);

		DrawImageStatistics(gc);

		DrawFPS(gc);

		DrawAnnulus(gc);

		DrawGridMesh(gc);
		DrawCircleMesh(gc);

		DrawHEWCircle(gc);
		DrawHEWValues(gc);

		DrawSpotCroppedWindow(gc);
		DrawSumLines(gc);

		DrawFWHMValues(gc);

		delete gc;
	}

	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc)
		{
			SetExecutionFinished(true);
			return;
		}

		DrawActualImageSize(gc);
		delete gc;
	}
	SetExecutionFinished(true);
}

auto cCamPreview::UpdateColourLut(int black, int white) -> void
{
	// Every pixel value maps to the same colour for given levels, so the colormap maths
	// runs once per value (at most 65,536) and the bitmap is then built by table lookups.
	// Only the visible part of the image is coloured (see DrawCameraCapturedImage), so
	// a level change costs about the same for a 1 MP and a 150 MP sensor.
	const unsigned short maxValue = m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4095 : USHRT_MAX;
	m_ColourLut.resize((static_cast<size_t>(maxValue) + 1) * 3);
	m_ColourLutMax = maxValue;
	++m_ImageRevision;
	for (unsigned int v = 0; v <= maxValue; ++v)
		MapValueToRGB(static_cast<unsigned short>(v), m_ColourLut[v * 3], m_ColourLut[v * 3 + 1], m_ColourLut[v * 3 + 2], black, white);
}

auto cCamPreview::MapValueToRGB
(
	const unsigned short current_value,
	unsigned char& red,
	unsigned char& green,
	unsigned char& blue,
	const int black,
	const int white
) -> void
{
	const unsigned short max_value = m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4095 : USHRT_MAX;

	const double black_d = static_cast<double>(black == max_value ? max_value - 1 : black);
	const double white_d = static_cast<double>(white > max_value ? max_value : white);
	// Guard against black == white (e.g. a flat image or a collapsed histogram range),
	// which would otherwise divide by zero and produce NaN -> undefined uchar conversion.
	const double range_d = std::max(white_d - black_d, 1.0);

	const double max_value_d = static_cast<double>(max_value);
	const double max_minus_white = max_value_d - white_d;
	const double max_minus_black = max_value_d - black_d;
	const double max_range_d = std::max(max_minus_black - max_minus_white, 1.0);
	const unsigned char uchar_max = UCHAR_MAX;

	double adjustedValue = static_cast<double>(current_value);
	red = green = blue = 0;

	switch (m_ColormapMode)
	{
	case CameraPreviewVariables::Colormaps::GRAYSCALE_COLORMAP:
	{
		adjustedValue = std::clamp(adjustedValue, black_d, white_d);
		adjustedValue = ((adjustedValue - black_d) / range_d) * uchar_max;
		red = green = blue = static_cast<unsigned char>(adjustedValue);
		break;
	}

	case CameraPreviewVariables::Colormaps::INVERT_COLORMAP:
	{
		adjustedValue = max_value_d - adjustedValue;
		adjustedValue = std::clamp(adjustedValue, max_minus_white, max_minus_black);
		adjustedValue = ((adjustedValue - max_minus_white) / max_range_d) * uchar_max;
		red = green = blue = static_cast<unsigned char>(adjustedValue);
		break;
	}

	case CameraPreviewVariables::Colormaps::JET_COLORMAP:
		CalculateJetColormapPixelRGB(current_value, red, green, blue, black, white);
		break;

	case CameraPreviewVariables::Colormaps::IMAGEJ_16_COLORS_COLORMAP:
		Calculate16_ColorsImageJColormapPixelRGB(current_value, red, green, blue, black, white);
		break;

	case CameraPreviewVariables::Colormaps::COOL_COLORMAP:
	{
		// Normalize adjustedValue to [0.0, 1.0] based on actual black/white
		float norm = 0.0f;
		if (white > black)
		{
			int clamped = std::clamp((int)current_value, black, white);
			norm = static_cast<float>(clamped - black) / (white - black);
		}

		// Apply cool colormap
		red = static_cast<unsigned char>(std::round(norm * 255.0f));
		green = static_cast<unsigned char>(std::round((1.0f - norm) * 255.0f));
		blue = 255;
		break;
	}

	case CameraPreviewVariables::Colormaps::HOT_COLORMAP:
		CalculateHotColormapPixelRGB(current_value, red, green, blue, black, white);
		break;

	case CameraPreviewVariables::Colormaps::WINTER_COLORMAP:
	{
		// Normalize adjustedValue to [0.0, 1.0] range based on black and white
		float norm = 0.0f;
		if (white > black)
		{
			int clamped = std::clamp((int)current_value, black, white);
			norm = static_cast<float>(clamped - black) / (white - black);
		}

		// Apply winter colormap logic
		red = 0;
		green = static_cast<unsigned char>(std::round(norm * 255.0f));
		blue = static_cast<unsigned char>(std::round((1.0f - 0.5f * norm) * 255.0f));
		break;
	}

	case CameraPreviewVariables::Colormaps::COPPER_COLORMAP:
		CalculateCopperColormapPixelRGB(current_value, red, green, blue, black, white);
		break;

	default:
		break;
	}

}

void cCamPreview::CreateScreenBitmap(const wxRect& screenRect, const double k)
{
	const int W = m_ImageSize.GetWidth(), H = m_ImageSize.GetHeight();
	const int dstW = screenRect.width, dstH = screenRect.height;

	// Image column / row shown at the centre of every screen pixel (nearest neighbour).
	// Image pixel i covers screen [(start + i) * k, (start + i + 1) * k), as the overlays assume.
	std::vector<int> cols(dstW), rows(dstH);
	for (int c = 0; c < dstW; ++c)
		cols[c] = std::clamp((int)std::floor((screenRect.x + c + 0.5) / k - m_StartDrawPos.x), 0, W - 1);
	for (int r = 0; r < dstH; ++r)
		rows[r] = std::clamp((int)std::floor((screenRect.y + r + 0.5) / k - m_StartDrawPos.y), 0, H - 1);

	wxImage part(dstW, dstH, false);
	unsigned char* dst = part.GetData();
	const unsigned short* src = m_ImageData.get();
	const unsigned char* lut = m_ColourLut.data();
	const unsigned short lutMax = m_ColourLutMax;
	const int* colIdx = cols.data();
	const int* rowIdx = rows.data();

	auto fillRows = [=](int rowBegin, int rowEnd)
		{
			for (int r = rowBegin; r < rowEnd; ++r)
			{
				const unsigned short* srcRow = src + static_cast<size_t>(rowIdx[r]) * W;
				unsigned char* d = dst + static_cast<size_t>(r) * dstW * 3;
				for (int c = 0; c < dstW; ++c, d += 3)
				{
					const unsigned char* rgb = lut + static_cast<size_t>(std::min(srcRow[colIdx[c]], lutMax)) * 3;
					d[0] = rgb[0];
					d[1] = rgb[1];
					d[2] = rgb[2];
				}
			}
		};

	const unsigned int numThreads = static_cast<size_t>(dstW) * dstH < 200'000 ? 1u : std::clamp(std::thread::hardware_concurrency(), 1u, 8u);
	if (numThreads == 1)
	{
		fillRows(0, dstH);
	}
	else
	{
		const int rowsPerThread = (dstH + (int)numThreads - 1) / (int)numThreads;
		std::vector<std::thread> threads;
		threads.reserve(numThreads);
		for (int begin = 0; begin < dstH; begin += rowsPerThread)
			threads.emplace_back(fillRows, begin, std::min(dstH, begin + rowsPerThread));
		for (auto& t : threads) t.join();
	}

	m_ScreenBitmap = wxBitmap(part);
}

void cCamPreview::DrawCameraCapturedImage(wxDC& dc)
{
	if (!m_IsImageSet || !HasImage() || m_ColourLut.empty()) return;

	// The image is resampled straight to screen resolution and blitted 1:1. Letting GDI+
	// scale a bitmap took 30-60 ms per repaint; this costs a few ms whatever the sensor
	// size, because only the pixels that are actually on screen are ever touched.
	const double k = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (k <= 0.0) return;

	const wxSize canvas = GetClientSize();
	const double sx0 = m_StartDrawPos.x * k, sy0 = m_StartDrawPos.y * k;
	const double sx1 = (m_StartDrawPos.x + m_ImageSize.GetWidth()) * k;
	const double sy1 = (m_StartDrawPos.y + m_ImageSize.GetHeight()) * k;
	const int left = std::max(0, (int)std::floor(sx0)), top = std::max(0, (int)std::floor(sy0));
	const int right = std::min(canvas.GetWidth(), (int)std::ceil(sx1));
	const int bottom = std::min(canvas.GetHeight(), (int)std::ceil(sy1));
	if (right <= left || bottom <= top) return;

	const wxRect screenRect(left, top, right - left, bottom - top);

	// Repaints that only change overlays (cursor, crosshair, ...) reuse the last bitmap
	if (!m_IsGraphicsBitmapSet || !m_ScreenBitmap.IsOk() || screenRect != m_ScreenBitmapRect ||
		k != m_ScreenBitmapZoom || m_StartDrawPos != m_ScreenBitmapStart)
	{
		CreateScreenBitmap(screenRect, k);
		m_ScreenBitmapRect = screenRect;
		m_ScreenBitmapZoom = k;
		m_ScreenBitmapStart = m_StartDrawPos;
		m_IsGraphicsBitmapSet = true;
	}

	dc.DrawBitmap(m_ScreenBitmap, m_ScreenBitmapRect.GetTopLeft());
}

auto cCamPreview::DrawFWHMValues(wxGraphicsContext* gc_) -> void
{
	if (!HasImage() || !m_DisplayFWHM || !m_HorizontalSumArray || !m_VerticalSumArray) return;

	wxColour fontColour(181, 230, 29, 200);
	wxFont font = wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc_->SetFont(font, fontColour);

	wxDouble widthText{}, heightText{};

	wxDouble offsetX{ 5.0 }, offsetY{ 5.0 };
	wxRealPoint drawPoint{};
	wxString curr_value{};

	// Horizontal FWHM
	{
		if (m_HorizontalFWHM_PX != -1.0)
		{
			// um
			curr_value = "FWHM [um]: ";
			curr_value += wxString::Format(wxT("%.2f"), m_HorizontalFWHM_PX * m_PixelSizeUM);
		}

		// px
		curr_value += " FWHM [px]: ";
		curr_value += wxString::Format(wxT("%i"), (int)m_HorizontalFWHM_PX);

		gc_->GetTextExtent(curr_value, &widthText, &heightText);
		drawPoint.x = m_CanvasSize.GetWidth() / 2 - widthText / 2;
		drawPoint.y = m_CanvasSize.GetHeight() - offsetY - heightText;
		gc_->DrawText(curr_value, drawPoint.x, drawPoint.y);
	}

	curr_value = wxEmptyString;

	// Vertical FWHM
	{
		if (m_VerticalFWHM_PX != -1.0)
		{
			// um
			curr_value = "FWHM [um]: ";
			curr_value += wxString::Format(wxT("%.2f"), m_VerticalFWHM_PX * m_PixelSizeUM);
		}

		// px
		curr_value += " FWHM [px]: ";
		curr_value += wxString::Format(wxT("%i"), (int)m_VerticalFWHM_PX);

		gc_->GetTextExtent(curr_value, &widthText, &heightText);
		drawPoint.x = m_CanvasSize.GetWidth() - offsetX - heightText;
		drawPoint.y = m_CanvasSize.GetHeight() / 2 + widthText / 2;

		// Set up the transformation matrix for a 90-degree counter clockwise rotation
		gc_->Translate(drawPoint.x, drawPoint.y);
		gc_->Rotate(-M_PI / 2.0); // Rotate 90 degrees counter clockwise (pi/2 radians)
		gc_->Translate(-drawPoint.x, -drawPoint.y);
		gc_->DrawText(curr_value, drawPoint.x, drawPoint.y);
	}

	LOG("Finished: " + wxString(__FUNCSIG__))
}

auto cCamPreview::DrawHEWValues(wxGraphicsContext* gc_) -> void
{
	if (!HasImage() || !m_DisplayFocusCenter || !m_HorizontalSumArray || !m_VerticalSumArray) return;
	if (m_HEWDiameter <= 0.0) return;

	wxColour fontColour(255, 120, 175);
	wxFont font = wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc_->SetFont(font, fontColour);

	wxDouble widthText{}, heightText{};

	// Stack below the FPS badge instead of drawing on top of it
	const wxDouble pad = 8.0;
	wxDouble offsetX{ 6.0 }, offsetY{ m_TopLeftOverlayBottom > 0.0 ? m_TopLeftOverlayBottom + 4.0 : 6.0 };
	wxRealPoint drawPoint{};
	wxString curr_value{};

	curr_value = wxEmptyString;

	// HEW
	{
		// um
		curr_value = "HEWd [um]: ";
		curr_value += wxString::Format(wxT("%.2f"), m_HEWDiameter * m_PixelSizeUM);

		// px
		curr_value += " HEWd [px]: ";
		curr_value += wxString::Format(wxT("%i"), (int)m_HEWDiameter);

		gc_->GetTextExtent(curr_value, &widthText, &heightText);

		// Badge background so the values stay readable on bright images
		gc_->SetBrush(wxBrush(wxColour(20, 20, 24, 170)));
		gc_->SetPen(wxPen(wxColour(255, 255, 255, 40), 1));
		wxGraphicsPath path = gc_->CreatePath();
		path.AddRoundedRectangle(offsetX, offsetY, widthText + 2 * pad, heightText + 2 * pad, 6.0);
		gc_->DrawPath(path);

		drawPoint.x = offsetX + pad;
		drawPoint.y = offsetY + pad;

		gc_->DrawText(curr_value, drawPoint.x, drawPoint.y);
		m_TopLeftOverlayBottom = offsetY + heightText + 2 * pad;
	}

	LOG("Finished: " + wxString(__FUNCSIG__))
}

auto cCamPreview::DrawGridMesh(wxGraphicsContext* gc_) -> void
{	
	if (!HasImage() || !m_DisplayGridMesh) return;

	auto penColour = wxColour(195, 195, 195, 100);
	auto penSize = 1;
	auto penStyle = wxPENSTYLE_LONG_DASH;
	gc_->SetPen(wxPen(penColour, penSize, penStyle));

	//wxDouble offsetX{ 5.0 }, offsetY{ 5.0 };
	wxRealPoint drawPointStart{}, drawPointFinish{};

	auto centerPoint = wxRealPoint();
	centerPoint.x = (m_StartDrawPos.x + m_ImageSize.GetWidth() / 2 - 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;
	centerPoint.y = (m_StartDrawPos.y + m_ImageSize.GetHeight() / 2 - 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;

	auto rightBottomBoundary = wxRealPoint();
	rightBottomBoundary.x = (m_StartDrawPos.x + m_ImageSize.GetWidth()) * m_Zoom / m_ZoomOnOriginalSizeImage;
	rightBottomBoundary.y = (m_StartDrawPos.y + m_ImageSize.GetHeight()) * m_Zoom / m_ZoomOnOriginalSizeImage;

	{
		rightBottomBoundary.x = rightBottomBoundary.x > GetSize().GetWidth() ? GetSize().GetWidth() : rightBottomBoundary.x;
		rightBottomBoundary.y = rightBottomBoundary.y > GetSize().GetHeight() ? GetSize().GetHeight() : rightBottomBoundary.y;
	}

	auto leftUpperBoundary = m_StartDrawPos;
	{
		leftUpperBoundary.x = leftUpperBoundary.x > 0.0 ? leftUpperBoundary.x : 0.0;
		leftUpperBoundary.y = leftUpperBoundary.y > 0.0 ? leftUpperBoundary.y : 0.0;
	}
	leftUpperBoundary.x *= m_Zoom / m_ZoomOnOriginalSizeImage;
	leftUpperBoundary.y *= m_Zoom / m_ZoomOnOriginalSizeImage;

	/* Vertical Lines */
	// Draw the right side from the center
	{
		drawPointStart.x = centerPoint.x;
		drawPointStart.y = leftUpperBoundary.y;
		while (drawPointStart.x <= rightBottomBoundary.x)
		{
			gc_->StrokeLine
			(
				drawPointStart.x,
				drawPointStart.y,
				drawPointStart.x,
				rightBottomBoundary.y
			);

			drawPointStart.x += m_GridMeshStepPX / m_ZoomOnOriginalSizeImage;
		}
	}

	// Draw the left side from the center
	{
		drawPointStart.x = centerPoint.x;
		drawPointStart.x -= m_GridMeshStepPX / m_ZoomOnOriginalSizeImage;

		while (drawPointStart.x >= leftUpperBoundary.x)
		{
			gc_->StrokeLine
			(
				drawPointStart.x,
				drawPointStart.y,
				drawPointStart.x,
				rightBottomBoundary.y
			);

			drawPointStart.x -= m_GridMeshStepPX / m_ZoomOnOriginalSizeImage;
		}
	}

	/* Horizontal Lines */
	// Draw the bottom side from the center
	{
		drawPointStart.x = leftUpperBoundary.x;
		drawPointStart.y = centerPoint.y;
		while (drawPointStart.y <= rightBottomBoundary.y)
		{
			gc_->StrokeLine
			(
				drawPointStart.x,
				drawPointStart.y,
				rightBottomBoundary.x,
				drawPointStart.y
			);

			drawPointStart.y += m_GridMeshStepPX / m_ZoomOnOriginalSizeImage;
		}
	}

	// Draw the top side from the center
	{
		drawPointStart.x = leftUpperBoundary.x;
		drawPointStart.y = centerPoint.y;
		drawPointStart.y -= m_GridMeshStepPX / m_ZoomOnOriginalSizeImage;
		while (drawPointStart.y >= leftUpperBoundary.y)
		{
			gc_->StrokeLine
			(
				drawPointStart.x,
				drawPointStart.y,
				rightBottomBoundary.x,
				drawPointStart.y
			);

			drawPointStart.y -= m_GridMeshStepPX / m_ZoomOnOriginalSizeImage;
		}
	}
}

auto cCamPreview::DrawCircleMesh(wxGraphicsContext* gc_) -> void
{
	if (!HasImage() || !m_DisplayCircleMesh) return;

	auto penColour = wxColour(237, 28, 36, 100);
	auto penSize = 4;
	auto penStyle = wxPENSTYLE_LONG_DASH;
	gc_->SetPen(wxPen(penColour, penSize, penStyle));

	auto centerPoint = wxRealPoint();

	if (m_DisplayCrossHair)
	{
		centerPoint.x = (m_StartDrawPos.x + m_CrossHairPos.x + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;
		centerPoint.y = (m_StartDrawPos.y + m_CrossHairPos.y + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;
	}
	else
	{
		centerPoint.x = (m_StartDrawPos.x + m_ImageSize.GetWidth() / 2 - 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;
		centerPoint.y = (m_StartDrawPos.y + m_ImageSize.GetHeight() / 2 - 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;
	}

	auto maxRadius = std::min(m_CanvasSize.GetWidth(), m_CanvasSize.GetHeight()) / 2.0;
	auto currRadius = m_CircleMeshStepPX / m_ZoomOnOriginalSizeImage;

	wxGraphicsPath path = gc_->CreatePath();
	while ((int)currRadius <= maxRadius)
	{
		path.AddCircle
		(
			centerPoint.x, 
			centerPoint.y, 
			currRadius
		);

		currRadius += m_CircleMeshStepPX / m_ZoomOnOriginalSizeImage;
	}
	gc_->StrokePath(path);
}

auto cCamPreview::DrawScaleBar(wxGraphicsContext* gc_) -> void
{
	if (!HasImage() || m_PixelSizeUM <= 0.0) return;

	// Sensor pixels per displayed image pixel (binning). Without a connected camera the
	// sensor size is unknown; treat the image as unbinned instead of a zero-length scale.
	const double binFactor = m_SensorSize.GetWidth() > 0
		? static_cast<double>(m_SensorSize.GetWidth()) / std::max(1, m_ImageSize.GetWidth())
		: 1.0;

	const double canvasPerImagePx = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (canvasPerImagePx <= 0.0) return;
	const double umPerCanvasPx = m_PixelSizeUM * binFactor / canvasPerImagePx;
	if (umPerCanvasPx <= 0.0 || !std::isfinite(umPerCanvasPx)) return;

	// Longest 1-2-5 x 10^n length that fits in ~120 px (so the bar is 48-120 px long)
	const double maxBarPx = 120.0;
	const double maxUm = maxBarPx * umPerCanvasPx;
	const double p10 = std::pow(10.0, std::floor(std::log10(maxUm)));
	const double d = maxUm / p10;
	const double scaleUm = (d >= 5.0 ? 5.0 : d >= 2.0 ? 2.0 : 1.0) * p10;
	const double barPx = scaleUm / umPerCanvasPx;

	// Label in a unit that keeps the number short: nm, um, mm or m
	auto formatLength = [](double um) -> wxString
		{
			auto num = [](double v) { return std::fabs(v - std::round(v)) < 1e-9 ? wxString::Format("%.0f", v) : wxString::Format("%g", v); };
			if (um >= 1e6) return num(um / 1e6) + " m";
			if (um >= 1e3) return num(um / 1e3) + " mm";
			if (um >= 1.0) return num(um) + wxString::FromUTF8(" \xC2\xB5m");
			return num(um * 1e3) + " nm";
		};
	const wxString label = formatLength(scaleUm);

	const wxFont font(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc_->SetFont(font, wxColour(240, 240, 240));
	wxDouble tw{}, th{};
	gc_->GetTextExtent(label, &tw, &th);

	// Card in the bottom-right corner: label above a bar with end ticks
	const double pad = 8.0, gap = 4.0, tick = 6.0, margin = 10.0;
	const double cardW = std::max(barPx, tw) + 2 * pad;
	const double cardH = pad + th + gap + tick + pad;
	const wxSize canvas = GetClientSize();
	const double cardX = canvas.GetWidth() - margin - cardW;
	const double cardY = canvas.GetHeight() - margin - cardH;
	DrawOverlayCard(gc_, cardX, cardY, cardW, cardH);

	gc_->DrawText(label, cardX + (cardW - tw) / 2.0, cardY + pad);

	const double barX0 = cardX + (cardW - barPx) / 2.0, barX1 = barX0 + barPx;
	const double barY = cardY + pad + th + gap + tick / 2.0;
	wxPen pen(wxColour(240, 240, 240), 2);
	pen.SetCap(wxCAP_BUTT);
	gc_->SetPen(pen);
	gc_->StrokeLine(barX0, barY, barX1, barY);
	gc_->StrokeLine(barX0 + 1, barY - tick / 2.0, barX0 + 1, barY + tick / 2.0);
	gc_->StrokeLine(barX1 - 1, barY - tick / 2.0, barX1 - 1, barY + tick / 2.0);

}

auto cCamPreview::DrawHEWCircle(wxGraphicsContext* gc_) -> void
{
	if (!HasImage() || !m_HorizontalSumArray || !m_VerticalSumArray) return;
	if (m_HorizontalFWHM_PX == -1 || m_VerticalFWHM_PX == -1) return;
	if (!m_DisplayFocusCenter) return;

	if (m_ROIWindowWidth <= 0) return;

	if (m_HEWDiameter <= 0) return;

	auto penColour = wxColour(234, 54, 128);
	auto penSize = 3;
	auto penStyle = wxPENSTYLE_DOT_DASH;
	gc_->SetPen(wxPen(penColour, penSize, penStyle));

	auto centerPoint = wxRealPoint();
	{
		centerPoint.x = (m_StartDrawPos.x + m_HorizontalMiddleFWHMPosPixel + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;
		centerPoint.y = (m_StartDrawPos.y + m_VerticalMiddleFWHMPosPixel + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage;
	}

	wxGraphicsPath path = gc_->CreatePath();

	path.AddCircle
	(
		centerPoint.x, 
		centerPoint.y, 
		m_HEWDiameter / 2 * m_Zoom / m_ZoomOnOriginalSizeImage
	);

	gc_->StrokePath(path);
}

auto cCamPreview::DrawSpotCroppedWindow(wxGraphicsContext* gc_) -> void
{
	if (!HasImage() || !m_HorizontalSumArray || !m_VerticalSumArray) return;
	if (m_HorizontalFWHM_PX == -1 || m_VerticalFWHM_PX == -1) return;
	if (!m_DisplayFocusCenter) return;

	if (m_ROIWindowWidth <= 0) return;

	auto penColour = wxColour("red");
	auto penSize = 2;
	auto penStyle = wxPENSTYLE_DOT_DASH;
	gc_->SetPen(wxPen(penColour, penSize, penStyle));

	auto rectangle_start_draw = wxRealPoint
	(
		(m_StartDrawPos.x + m_HorizontalMiddleFWHMPosPixel - m_ROIWindowWidth / 2) * m_Zoom / m_ZoomOnOriginalSizeImage,
		(m_StartDrawPos.y + m_VerticalMiddleFWHMPosPixel - m_ROIWindowWidth / 2) * m_Zoom / m_ZoomOnOriginalSizeImage
	);

	auto rectangleSize = wxSize
	(
		m_ROIWindowWidth * m_Zoom / m_ZoomOnOriginalSizeImage,
		m_ROIWindowWidth * m_Zoom / m_ZoomOnOriginalSizeImage
	);

	gc_->DrawRectangle
	(
		rectangle_start_draw.x, 
		rectangle_start_draw.y, 
		rectangleSize.GetWidth(), 
		rectangleSize.GetHeight()
	);

	// Draw Cross Section (The Best Value Position)
	auto crossLineLengthPX{ 50.0 * m_Zoom / m_ZoomOnOriginalSizeImage };
	auto cross_center_start_draw = wxRealPoint
	(
		(m_StartDrawPos.x + m_HorizontalMiddleFWHMPosPixel + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage,
		(m_StartDrawPos.y + m_VerticalMiddleFWHMPosPixel + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage
	);

	// Stroke Horizontal Line
	gc_->StrokeLine
	(
		cross_center_start_draw.x - crossLineLengthPX / 2.0, 
		cross_center_start_draw.y,
		cross_center_start_draw.x + crossLineLengthPX / 2.0,
		cross_center_start_draw.y
	);

	// Stroke Vertical Line
	gc_->StrokeLine
	(
		cross_center_start_draw.x, 
		cross_center_start_draw.y - crossLineLengthPX / 2.0,
		cross_center_start_draw.x,
		cross_center_start_draw.y + crossLineLengthPX / 2.0
	);
}

auto cCamPreview::DrawSumLines(wxGraphicsContext* gc_) -> void
{
	if (!m_DisplayFWHM) return;
	if (m_HorizontalFWHM_PX == -1 || m_VerticalFWHM_PX == -1) return;
	if (!m_HorizonalBestSum || !m_VerticalBestSum) return;

	DrawHorizontalSumLine(gc_);
	DrawVerticalSumLine(gc_);
}

auto cCamPreview::DrawHorizontalSumLine(wxGraphicsContext* gc_) -> void
{
	if (!m_HorizontalSumArray) return;

	auto penColour = wxColour(163, 73, 164);
	auto penSize = 2;
	auto penStyle = wxPENSTYLE_SOLID;
	gc_->SetPen(wxPen(penColour, penSize, penStyle));

	auto start_draw = wxRealPoint
	(
		(m_StartDrawPos.x + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage,
		(m_StartDrawPos.y + m_ImageSize.GetHeight() - 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage 
	);

	wxDouble offset_x{ m_Zoom / m_ZoomOnOriginalSizeImage }, max_height{ (wxDouble)m_ImageOnCanvasSize.GetHeight() / 4 };
	wxDouble current_x{}, current_y{}, start_x{ start_draw.x }, start_y{}, current_length{ m_Zoom / m_ZoomOnOriginalSizeImage };
	auto max_value = m_HorizonalBestSum - m_HorizontalWorstSum;
	LOGI("Max Value: ", max_value);
	max_value = max_value == 0 ? 1 : max_value;
	auto multiplicator = (double)max_height / max_value;
	LOGF("H Multiplicator: ", multiplicator);

	for (auto i = 0; i < m_ImageSize.GetWidth() - 1; ++i)
	{
		start_y = start_draw.y - std::floor(((int)m_HorizontalSumArray[i] - (int)m_HorizontalWorstSum) * multiplicator);
		current_y = start_draw.y - std::floor(((int)m_HorizontalSumArray[i + 1] - (int)m_HorizontalWorstSum) * multiplicator);
		gc_->StrokeLine(start_x, start_y, start_x + current_length, current_y);
		start_x += offset_x;
	}

	LOG("Finished: " + wxString(__FUNCSIG__))
}

auto cCamPreview::DrawVerticalSumLine(wxGraphicsContext* gc_) -> void
{
	if (!m_VerticalSumArray) return;

	auto penColour = wxColour(255, 127, 39);
	auto penSize = 2;
	auto penStyle = wxPENSTYLE_SOLID;
	gc_->SetPen(wxPen(penColour, penSize, penStyle));

	auto start_draw = wxRealPoint
	(
		(m_StartDrawPos.x + m_ImageSize.GetWidth() - 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage,
		(m_StartDrawPos.y + 0.5) * m_Zoom / m_ZoomOnOriginalSizeImage 
	);

	wxDouble offset_y{ m_Zoom / m_ZoomOnOriginalSizeImage }, max_height{ (wxDouble)m_ImageOnCanvasSize.GetWidth() / 4 };
	wxDouble current_x{}, current_y{}, start_x{}, start_y{ start_draw.y }, current_length{ m_Zoom / m_ZoomOnOriginalSizeImage };
	auto max_value = m_VerticalBestSum - m_VerticalWorstSum;
	max_value = max_value == 0 ? 1 : max_value;
	auto multiplicator = (double)max_height / max_value;
	LOGF("V Multiplicator: ", multiplicator);

	for (auto i = 0; i < m_ImageSize.GetHeight() - 1; ++i)
	{
		start_x = start_draw.x - std::floor(((int)m_VerticalSumArray[i] - (int)m_VerticalWorstSum) * multiplicator);
		current_x = start_draw.x - std::floor(((int)m_VerticalSumArray[i + 1] - (int)m_VerticalWorstSum) * multiplicator);
		gc_->StrokeLine(start_x, start_y, current_x, start_y + current_length);
		start_y += offset_y;
	}

	LOG("Finished: " + wxString(__FUNCSIG__))
}

auto cCamPreview::DrawActualImageSize(wxGraphicsContext* gc_) -> void
{
	if (m_Zoom > m_MinZoom) return;
	if (!HasImage()) return;
	if (m_DisplayFWHM) return;
	if (!gc_) return;

	const wxSize panelSize = GetSize();

	if (panelSize.GetWidth() <= 0 || panelSize.GetHeight() <= 0)
		return;

	const double panelMinSide = static_cast<double>(std::min(panelSize.GetWidth(), panelSize.GetHeight()));
	const double imageMinSide = static_cast<double>(std::min(m_ImageOnCanvasSize.GetWidth(), m_ImageOnCanvasSize.GetHeight()));

	/*
		The old values were:
			font size  = 22
			icon size  = 24

		Those remain the maximum values. On smaller panels/images, the overlay
		scales down so it does not dominate the preview.
	*/
	const double referenceSide = 650.0;

	const double rawScale = std::min
	(
		panelMinSide / referenceSide,
		imageMinSide / referenceSide
	);

	const double overlayScale = std::clamp(rawScale, 0.45, 1.0);

	const int fontSize = static_cast<int>(std::round(22.0 * overlayScale));
	const int bmpWidth = static_cast<int>(std::round(24.0 * overlayScale));

	const wxDouble offset_x = std::round(10.0 * overlayScale);
	const wxDouble offset_y = std::round(10.0 * overlayScale);

	// Drawing image size below its lower and left sides.
	{
		wxColour fontColour(255, 87, 51, 100);
		wxFont font = wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
		gc_->SetFont(font, fontColour);

		wxString curr_value{};
		wxDouble widthText{}, heightText{};

		auto image_start_draw = wxRealPoint
		(
			m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
			m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
		);

		// Draw Value of the Bottom side of the image
		{
			curr_value = wxString::Format(wxT("%i"), m_ImageSize.GetWidth());
			curr_value += " [px]";

			if (m_SensorSize.GetWidth())
			{
				curr_value += "; ";
				curr_value += wxString::Format(wxT("%.1f"), m_PixelSizeUM * m_SensorSize.GetWidth());
				curr_value += " [um]";
			}

			gc_->GetTextExtent(curr_value, &widthText, &heightText);

			wxRealPoint draw_point =
			{
				image_start_draw.x + m_ImageOnCanvasSize.GetWidth() / 2.0 - widthText / 2.0,
				image_start_draw.y + m_ImageOnCanvasSize.GetHeight() + offset_y
			};

			if (GetSize().GetHeight() < m_ImageOnCanvasSize.GetHeight() + 2 * offset_y + 2 * heightText)
				draw_point.y = image_start_draw.y + m_ImageOnCanvasSize.GetHeight() - offset_y - heightText;

			gc_->DrawText(curr_value, draw_point.x, draw_point.y);

			// Draw the Icon
			{
				auto bmp = wxMaterialDesignArtProvider::GetBitmap
				(
					wxART_AUTO_FIT_WIDTH,
					wxART_CLIENT_FLUENTUI_FILLED,
					wxSize(bmpWidth, bmpWidth),
					fontColour
				);

				const wxRealPoint icon_draw_point =
				{
					draw_point.x - bmpWidth - offset_x,
					draw_point.y + (heightText - bmpWidth) / 2.0
				};

				if (bmp.IsOk())
					gc_->DrawBitmap(bmp, icon_draw_point.x, icon_draw_point.y, bmp.GetWidth(), bmp.GetHeight());
			}
		}

		// Draw Value on the Left side of the image
		{
			curr_value = wxString::Format(wxT("%i"), m_ImageSize.GetHeight());
			curr_value += " [px]";

			if (m_SensorSize.GetHeight())
			{
				curr_value += "; ";
				curr_value += wxString::Format(wxT("%.1f"), m_PixelSizeUM * m_SensorSize.GetHeight());
				curr_value += " [um]";
			}

			gc_->GetTextExtent(curr_value, &widthText, &heightText);

			wxRealPoint draw_point =
			{
				image_start_draw.x - offset_x - heightText,
				image_start_draw.y + m_ImageOnCanvasSize.GetHeight() / 2.0 + widthText / 2.0
			};

			if (GetSize().GetWidth() < m_ImageOnCanvasSize.GetWidth() + 2 * offset_x + 2 * heightText)
				draw_point.x = image_start_draw.x + offset_x;

			// Draw the Icon
			{
				const wxRealPoint icon_draw_point =
				{
					draw_point.x + (heightText - bmpWidth) / 2.0,
					draw_point.y + offset_y
				};

				auto bmp = wxMaterialDesignArtProvider::GetBitmap
				(
					wxART_AUTO_FIT_HEIGHT,
					wxART_CLIENT_FLUENTUI_FILLED,
					wxSize(bmpWidth, bmpWidth),
					fontColour
				);

				if (bmp.IsOk())
					gc_->DrawBitmap(bmp, icon_draw_point.x, icon_draw_point.y, bmp.GetWidth(), bmp.GetHeight());
			}

			gc_->PushState();

			gc_->Translate(draw_point.x, draw_point.y);
			gc_->Rotate(-M_PI / 2.0);
			gc_->Translate(-draw_point.x, -draw_point.y);

			gc_->DrawText(curr_value, draw_point.x, draw_point.y);

			gc_->PopState();
		}
	}
}

auto cCamPreview::UpdateMinimapBitmap(const int width, const int height) -> void
{
	if (m_MinimapBitmap.IsOk() && m_MinimapBitmapRevision == m_ImageRevision &&
		m_MinimapBitmap.GetWidth() == width && m_MinimapBitmap.GetHeight() == height)
		return;

	const int W = m_ImageSize.GetWidth(), H = m_ImageSize.GetHeight();
	wxImage thumb(width, height, false);
	unsigned char* dst = thumb.GetData();
	for (int r = 0; r < height; ++r)
	{
		const size_t y = std::min<size_t>(H - 1, static_cast<size_t>((r + 0.5) * H / height));
		for (int c = 0; c < width; ++c, dst += 3)
		{
			const size_t x = std::min<size_t>(W - 1, static_cast<size_t>((c + 0.5) * W / width));
			const unsigned short v = std::min(m_ImageData[y * W + x], m_ColourLutMax);
			const unsigned char* rgb = &m_ColourLut[static_cast<size_t>(v) * 3];
			dst[0] = rgb[0];
			dst[1] = rgb[1];
			dst[2] = rgb[2];
		}
	}
	m_MinimapBitmap = wxBitmap(thumb);
	m_MinimapBitmapRevision = m_ImageRevision;
}

auto cCamPreview::CenterViewOnImagePoint(const wxRealPoint& imagePoint) -> void
{
	const double k = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (k <= 0.0 || m_Zoom <= 0.0) return;

	const wxSize canvas = GetClientSize();
	// Keep the view inside the image where it is larger than the view (no empty margins)
	auto clampCentre = [](double v, double halfView, double size)
		{
			return size > 2.0 * halfView ? std::clamp(v, halfView, size - halfView) : size / 2.0;
		};
	const double ix = clampCentre(imagePoint.x, canvas.GetWidth() / (2.0 * k), (double)m_ImageSize.GetWidth());
	const double iy = clampCentre(imagePoint.y, canvas.GetHeight() / (2.0 * k), (double)m_ImageSize.GetHeight());

	// Screen x of image x is (m_StartDrawPos.x + x) * k; put the point in the canvas centre
	m_StartDrawPos.x = canvas.GetWidth() / (2.0 * k) - ix;
	m_StartDrawPos.y = canvas.GetHeight() / (2.0 * k) - iy;
	m_PanOffset.x = (m_StartDrawPos.x - m_NotZoomedGraphicsBitmapOffset.x) * m_Zoom;
	m_PanOffset.y = (m_StartDrawPos.y - m_NotZoomedGraphicsBitmapOffset.y) * m_Zoom;

	m_CrossHairTool->SetImageStartDrawPos(wxRealPoint
	(
		m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
		m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
	));
	Refresh();
}

auto cCamPreview::MinimapHitTest(const wxPoint& canvasPos) const -> bool
{
	return m_MinimapVisible && m_MinimapThumbRect.Contains(wxPoint2DDouble(canvasPos.x, canvasPos.y));
}

auto cCamPreview::MinimapToImage(const wxPoint& canvasPos) const -> wxRealPoint
{
	const auto& t = m_MinimapThumbRect;
	if (t.m_width <= 0.0 || t.m_height <= 0.0) return {};
	return wxRealPoint
	(
		(canvasPos.x - t.m_x) / t.m_width * m_ImageSize.GetWidth(),
		(canvasPos.y - t.m_y) / t.m_height * m_ImageSize.GetHeight()
	);
}

auto cCamPreview::DrawActualZoomedPositionOverImage(wxGraphicsContext* gc_) -> void
{
	m_MinimapVisible = false;
	if (m_Zoom <= m_MinZoom || !HasImage() || m_ColourLut.empty()) return;

	const double k = m_ZoomOnOriginalSizeImage > 0.0 ? m_Zoom / m_ZoomOnOriginalSizeImage : 0.0;
	if (k <= 0.0) return;

	// Thumbnail of the whole image (fitted into 168 x 120 px) in a card at the top right
	const wxSize canvas = GetClientSize();
	const double maxW = 168.0, maxH = 120.0;
	const double aspect = (double)m_ImageSize.GetWidth() / m_ImageSize.GetHeight();
	const int thumbW = std::max(16, (int)std::round(aspect >= maxW / maxH ? maxW : maxH * aspect));
	const int thumbH = std::max(16, (int)std::round(aspect >= maxW / maxH ? maxW / aspect : maxH));

	const wxFont font(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	const wxFont boldFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc_->SetFont(font, *wxWHITE);
	wxDouble lineW{}, lineH{};
	gc_->GetTextExtent("Ag", &lineW, &lineH);

	const double pad = 6.0, margin = 10.0, gap = 5.0;
	const double cardW = thumbW + 2 * pad;
	const double cardH = pad + thumbH + gap + lineH + pad;
	const double cardX = canvas.GetWidth() - margin - cardW;
	const double cardY = margin;
	if (cardX < margin || cardY + cardH > canvas.GetHeight() - margin) return; // canvas too small

	DrawOverlayCard(gc_, cardX, cardY, cardW, cardH);

	const wxRect2DDouble thumb(cardX + pad, cardY + pad, thumbW, thumbH);
	UpdateMinimapBitmap(thumbW, thumbH);
	gc_->DrawBitmap(m_MinimapBitmap, thumb.m_x, thumb.m_y, thumb.m_width, thumb.m_height);
	m_MinimapThumbRect = thumb;
	m_MinimapVisible = true;

	// Visible part of the image, in thumbnail coordinates
	const double sx = thumb.m_width / m_ImageSize.GetWidth(), sy = thumb.m_height / m_ImageSize.GetHeight();
	double vx0 = (-m_StartDrawPos.x) * sx + thumb.m_x;
	double vy0 = (-m_StartDrawPos.y) * sy + thumb.m_y;
	double vx1 = vx0 + canvas.GetWidth() / k * sx;
	double vy1 = vy0 + canvas.GetHeight() / k * sy;
	vx0 = std::clamp(vx0, thumb.m_x, thumb.GetRight());
	vx1 = std::clamp(vx1, thumb.m_x, thumb.GetRight());
	vy0 = std::clamp(vy0, thumb.m_y, thumb.GetBottom());
	vy1 = std::clamp(vy1, thumb.m_y, thumb.GetBottom());

	// Dim everything outside the visible area
	{
		wxGraphicsPath shade = gc_->CreatePath();
		shade.AddRectangle(thumb.m_x, thumb.m_y, thumb.m_width, thumb.m_height);
		if (vx1 > vx0 && vy1 > vy0) shade.AddRectangle(vx0, vy0, vx1 - vx0, vy1 - vy0);
		gc_->SetPen(*wxTRANSPARENT_PEN);
		gc_->SetBrush(wxBrush(wxColour(0, 0, 0, 140)));
		gc_->FillPath(shade, wxODDEVEN_RULE);
	}

	// Viewport outline; a very small one gets a ring around it so it stays findable
	const wxColour accent(66, 146, 255);
	gc_->SetBrush(*wxTRANSPARENT_BRUSH);
	if (vx1 - vx0 < 6.0 || vy1 - vy0 < 6.0)
	{
		const double cx = (vx0 + vx1) / 2.0, cy = (vy0 + vy1) / 2.0;
		gc_->SetPen(wxPen(wxColour(0, 0, 0, 160), 3));
		gc_->DrawEllipse(cx - 6, cy - 6, 12, 12);
		gc_->SetPen(wxPen(accent, 1.5));
		gc_->DrawEllipse(cx - 6, cy - 6, 12, 12);
		gc_->SetBrush(wxBrush(accent));
		gc_->SetPen(*wxTRANSPARENT_PEN);
		gc_->DrawEllipse(cx - 1.5, cy - 1.5, 3, 3);
	}
	else
	{
		gc_->SetPen(wxPen(wxColour(0, 0, 0, 160), 3));
		gc_->DrawRectangle(vx0, vy0, vx1 - vx0, vy1 - vy0);
		gc_->SetPen(wxPen(accent, 1.5));
		gc_->DrawRectangle(vx0, vy0, vx1 - vx0, vy1 - vy0);
	}

	// Thin frame around the thumbnail
	gc_->SetBrush(*wxTRANSPARENT_BRUSH);
	gc_->SetPen(wxPen(wxColour(255, 255, 255, 60), 1));
	gc_->DrawRectangle(thumb.m_x, thumb.m_y, thumb.m_width, thumb.m_height);

	// Footer: zoom on the left, centre of the view (1-based pixels) on the right
	const double zoomPct = k * 100.0;
	const wxString zoomTxt = zoomPct >= 100.0 ? wxString::Format("%.0f%%", zoomPct) : wxString::Format("%.1f%%", zoomPct);
	const double centreX = canvas.GetWidth() / (2.0 * k) - m_StartDrawPos.x;
	const double centreY = canvas.GetHeight() / (2.0 * k) - m_StartDrawPos.y;
	const wxString posTxt = FormatInt((long long)std::floor(centreX) + 1) + ", " + FormatInt((long long)std::floor(centreY) + 1);

	const double textY = thumb.GetBottom() + gap;
	gc_->SetFont(boldFont, wxColour(240, 240, 240));
	gc_->DrawText(zoomTxt, thumb.m_x, textY);
	gc_->SetFont(font, wxColour(180, 180, 185));
	wxDouble pw{}, ph{};
	gc_->GetTextExtent(posTxt, &pw, &ph);
	gc_->DrawText(posTxt, thumb.GetRight() - pw, textY);
}

auto cCamPreview::DrawROI(wxGraphicsContext* gc_) -> void
{
	if (m_SensorSize.GetWidth() <= 1 || m_SensorSize.GetHeight() <= 1) return;

	if (m_SensorSize == wxSize(m_ROIRect.GetWidth(), m_ROIRect.GetHeight())) return;

	wxDouble max_width{ 100.0 }, max_height{ 100.0 };
	wxDouble 
		offset_x{ GetSize().GetWidth() - max_width - 50.0 },
		offset_y{ GetSize().GetHeight() / 2 - max_height / 2};

	auto camera_chip_miniature = wxRect2DDouble
	(
		offset_x,
		offset_y,
		m_SensorSize.GetWidth() >= m_SensorSize.GetHeight() ? max_width : max_height * m_SensorSize.GetWidth() / (wxDouble)m_SensorSize.GetHeight(),
		m_SensorSize.GetHeight() >= m_SensorSize.GetWidth() ? max_height : max_width * m_SensorSize.GetHeight() / (wxDouble)m_SensorSize.GetWidth()
	);

	gc_->SetPen(wxPen(wxColour(255, 174, 201), 1, wxPENSTYLE_LONG_DASH));

	gc_->DrawRectangle
	(
		camera_chip_miniature.m_x,
		camera_chip_miniature.m_y,
		camera_chip_miniature.m_width,
		camera_chip_miniature.m_height
	);

	gc_->SetPen(wxPen(wxColour("red"), 1, wxPENSTYLE_DOT_DASH));

	// Stroke Axis Lines
	{
		// Stroke horizontal Axis
		gc_->StrokeLine
		(
			-camera_chip_miniature.m_width / 10 + camera_chip_miniature.m_x,
			camera_chip_miniature.m_y + camera_chip_miniature.m_height / 2,
			camera_chip_miniature.m_x + camera_chip_miniature.m_width + camera_chip_miniature.m_width / 10,
			camera_chip_miniature.m_y + camera_chip_miniature.m_height / 2
		);

		// Stroke vertical Axis
		gc_->StrokeLine
		(
			camera_chip_miniature.m_x + camera_chip_miniature.m_width / 2,
			-camera_chip_miniature.m_height / 10 + camera_chip_miniature.m_y,
			camera_chip_miniature.m_x + camera_chip_miniature.m_width / 2,
			camera_chip_miniature.m_y + camera_chip_miniature.m_height + camera_chip_miniature.m_height / 10
		);
	}

	/* ROI */
	auto roi = wxRect2DDouble
	(
		camera_chip_miniature.m_x + camera_chip_miniature.m_width * m_ROIRect.x / m_SensorSize.GetWidth(),
		camera_chip_miniature.m_y + camera_chip_miniature.m_height * m_ROIRect.y / m_SensorSize.GetHeight(),
		camera_chip_miniature.m_width * m_ROIRect.width / m_SensorSize.GetWidth(),
		camera_chip_miniature.m_height * m_ROIRect.height / m_SensorSize.GetHeight()
	);

	gc_->SetPen(wxPen(wxColour("green"), 2, wxPENSTYLE_SOLID));

	if (roi.m_width < 7.0 && roi.m_height < 7.0)
	{
		wxPoint2DDouble start_draw_cross =
		{
			roi.m_x + roi.m_width / 2.0,
			roi.m_y + roi.m_height / 2.0
		};

		// Length of the line
		wxDouble leg_length{ 10.0 };
		auto rotate_angle = M_PI / 4.0; // Rotation in [rad]
		// Draw the cross

		// Horizontal line
		gc_->StrokeLine
		(
			start_draw_cross.m_x - leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y - leg_length / 2.0 * sin(rotate_angle),
			start_draw_cross.m_x + leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y + leg_length / 2.0 * sin(rotate_angle)
		);

		// Vertical line
		gc_->StrokeLine
		(
			start_draw_cross.m_x - leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y + leg_length / 2.0 * sin(rotate_angle),
			start_draw_cross.m_x + leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y - leg_length / 2.0 * sin(rotate_angle)
		);
	}
	else
	{
		gc_->DrawRectangle
		(
			roi.m_x,
			roi.m_y,
			roi.m_width,
			roi.m_height
		);
	}
}

void cCamPreview::OnSize(wxSizeEvent& evt)
{
	int newWidth{ evt.GetSize().x }, newHeight{ evt.GetSize().y };
	if (newWidth != m_CanvasSize.GetWidth() || newHeight != m_CanvasSize.GetHeight())
	{
		m_CanvasSize.SetWidth(newWidth);
		m_CanvasSize.SetHeight(newHeight);
		m_Zoom = m_MinZoom;
		m_PanOffset = {};
		ChangeSizeOfImageInDependenceOnCanvasSize();
		UpdateCrossHairOnSize();
		m_IsGraphicsBitmapSet = false;
		Refresh();
	}
}

void cCamPreview::ChangeSizeOfImageInDependenceOnCanvasSize()
{
	wxSize current_image_size{ m_ImageSize };
	wxSize canvas_size{ GetSize().GetWidth(), GetSize().GetHeight() };
	m_ZoomOnOriginalSizeImage = m_MinZoom;

	if (!current_image_size.GetWidth() || !current_image_size.GetHeight()) return;

	if (current_image_size.GetWidth() > canvas_size.GetWidth() || current_image_size.GetHeight() > canvas_size.GetHeight())
	{
		// Zoom out the image if it is larger than the canvas size
		while (canvas_size.GetWidth() < current_image_size.GetWidth()
			|| canvas_size.GetHeight() < current_image_size.GetHeight())
		{
			current_image_size.SetWidth(current_image_size.GetWidth() / m_ZoomStep);
			current_image_size.SetHeight(current_image_size.GetHeight() / m_ZoomStep);
			m_ZoomOnOriginalSizeImage *= m_ZoomStep;
		}
	}
	else
	{
		// Zoom in the image if it is smaller than the canvas size
		while (canvas_size.GetWidth() > current_image_size.GetWidth() * m_ZoomStep
			&& canvas_size.GetHeight() > current_image_size.GetHeight() * m_ZoomStep)
		{
			current_image_size.SetWidth(current_image_size.GetWidth() * m_ZoomStep);
			current_image_size.SetHeight(current_image_size.GetHeight() * m_ZoomStep);
			m_ZoomOnOriginalSizeImage /= m_ZoomStep;  // Decrease the zoom factor when zooming in
		}
	}

	m_ImageOnCanvasSize.Set(current_image_size.GetWidth(), current_image_size.GetHeight());
	m_CrossHairTool->SetImageOnCanvasSize(current_image_size);

	m_NotZoomedGraphicsBitmapOffset.x = (canvas_size.GetWidth() - m_ImageOnCanvasSize.GetWidth()) / 2;
	m_NotZoomedGraphicsBitmapOffset.y = (canvas_size.GetHeight() - m_ImageOnCanvasSize.GetHeight()) / 2;
	m_NotZoomedGraphicsBitmapOffset.x *= m_ZoomOnOriginalSizeImage / m_Zoom;
	m_NotZoomedGraphicsBitmapOffset.y *= m_ZoomOnOriginalSizeImage / m_Zoom;
	m_StartDrawPos = m_NotZoomedGraphicsBitmapOffset;

}

auto cCamPreview::UpdateCrossHairOnSize() -> void
{
	m_CrossHairTool->SetImageDimensions(m_ImageSize);
	m_CrossHairTool->SetZoomOfOriginalSizeImage(m_ZoomOnOriginalSizeImage);
	m_CrossHairTool->UpdateZoomValue(m_Zoom);
	m_CrossHairTool->SetImageStartDrawPos(wxRealPoint
	(
		m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
		m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
	));
	m_CrossHairTool->SetXPosFromParent(m_CrossHairPos.x);
	m_CrossHairTool->SetYPosFromParent(m_CrossHairPos.y);
}

