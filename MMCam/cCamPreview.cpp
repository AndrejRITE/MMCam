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
END_EVENT_TABLE()

cCamPreview::cCamPreview
(
	wxFrame* parent_frame, 
	wxSizer* parent_sizer,
	std::unique_ptr<CameraPreviewVariables::InputPreviewPanelArgs> input_preview_panel_args
) 
	: wxPanel(parent_frame)
{
	wxArtProvider::Push(new wxMaterialDesignArtProvider);

	m_ParentArguments.reset(input_preview_panel_args.release());

	//m_XimeaCameraControl = std::make_unique<XimeaControl>();
	SetDoubleBuffered(true);
#ifdef _DEBUG
	SetBackgroundColour(wxColor(210, 185, 155));
#else
	SetBackgroundColour(wxColor(255, 255, 255));
#endif // _DEBUG
	parent_sizer->Add(this, 1, wxEXPAND);

	InitDefaultComponents();
}

auto cCamPreview::SetBackgroundColor(wxColour bckg_colour) -> void
{
	SetBackgroundColour(bckg_colour);
}

auto cCamPreview::ActivateCrossHairDisplaying(bool activate) -> void
{
	m_DisplayCrossHair = activate;
	m_CrossHairTool->ActivateToolButton(activate, activate);
	//m_CrossHairTool->SetCursorPosOnCanvas(m_CursorPosOnCanvas);
	ChangeCursorInDependenceOfCurrentParameters();
	Refresh();
}

auto cCamPreview::SetValueDisplayingActive(bool activate) -> void
{
	m_DisplayPixelValues = activate;
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
		m_Zoom = 1.0;
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

auto cCamPreview::GetPixelColorFromImage(const wxImage& image, int x, int y) -> wxColour
{
	if (!image.IsOk()) return wxColour(0, 0, 0);  // Return black if invalid

	//wxImage image = bitmap.ConvertToImage();
	if (!image.IsOk()) return wxColour(0, 0, 0);

	if (x >= 0 && x < image.GetWidth() && y >= 0 && y < image.GetHeight()) 
	{
		unsigned char r = image.GetRed(x, y);
		unsigned char g = image.GetGreen(x, y);
		unsigned char b = image.GetBlue(x, y);
		return wxColour(r, g, b);
	}

	return wxColour(0, 0, 0);  // Default to black if out of bounds
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

	m_ExecutionFinished = false;

	unsigned long long readDataSize = m_ImageSize.GetWidth() * m_ImageSize.GetHeight();

	if (!m_ImageData || oldImageSize != m_ImageSize)
	{
		m_ImageData = std::make_unique<unsigned short[]>(readDataSize);
		m_Image = wxImage(m_ImageSize.GetWidth(), m_ImageSize.GetHeight());
		m_HorizontalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetWidth());
		m_VerticalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetHeight());
	}

	memcpy(m_ImageData.get(), data_ptr, sizeof(unsigned short) * readDataSize);

	UpdateWXImage(minValue, maxValue);

	UpdateImageParameters(oldImageSize);
	
	// Update Mouse position
	{
		wxMouseEvent event(wxEVT_MOTION);
		event.SetEventObject(this);

		// Set the last known mouse position
		event.m_x = m_CursorPosOnCanvas.x / m_ZoomOnOriginalSizeImage;
		event.m_y = m_CursorPosOnCanvas.y / m_ZoomOnOriginalSizeImage;

		wxPostEvent(this, event); // Queue the event
		ProcessEvent(event);
	}

	LOG("Image was updated: " + wxString(__FUNCSIG__))
	m_ExecutionFinished = true;
}

auto cCamPreview::UpdateCursorPositionOnStatusBar() -> void
{
	if (!m_Image.IsOk()) return;

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
	unsigned char& b
)
{
	unsigned short 
		x0{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 498U : 7'967U },
		x1{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 1'526U : 24'415U },
		x2{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 2'553U : 40'863U },
		x3{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 3'581U : 57'311U },
		x4{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4'095U : USHRT_MAX };

	if (value < x0)
	{
		r = 0;
		g = 0;
		b = 255 * 0.51563f + (float)value * (255.0f - 255 * 0.51563f) / (float)x0;
	}
	else if (value >= x0 && value <= x1)
	{
		r = 0;
		g = (float)(value - x0) * 255.0f / (float)(x1 - x0);
		b = 255;
	}
	else if (value > x1 && value < x2)
	{
		r = (float)(value - x1) * 255.0f / (float)(x2 - x1);
		g = 255;
		b = (float)(x2 - value) * 255.0f / (float)(x2 - x1);
	}
	else if (value >= x2 && value <= x3)
	{
		r = 255;
		g = (float)(x3 - value) * 255.0f / (float)(x3 - x2);
		b = 0;
	}
	else if (value > x3 && value < x4)
	{
		r = 255.0f * 0.5f + (float)(x4 - value) * (255.0f - 255.0f * 0.5f) / (float)(x4 - x3);
		g = 0;
		b = 0;
	}
	else
	{
		r = 255;
		g = 255;
		b = 255;
	}
}

auto cCamPreview::CalculateHotColormapPixelRGB
(
	const unsigned short& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b
) -> void
{
	unsigned short max_ushort{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4'095U : USHRT_MAX };
	unsigned short
		x1{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 1'536U : 24'575U },
		x2{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 3'071U : 49'151U },
		x3{ max_ushort };

	unsigned short max_uchar{ UCHAR_MAX };
	unsigned short t_red{}, t_green{}, t_blue{};

	if (value < x1)
	{
		t_red = (float)value * (float)max_ushort / (float)x1;
		t_green = 0;
		t_blue = 0;
	}
	else if (value >= x1 && value < x2)
	{
		t_red = max_ushort;
		t_green = (float)(value - x1) * (float)max_ushort / (float)(x2 - x1);
		t_blue = 0;
	}
	else if (value >= x2 && value < x3)
	{
		t_red = max_ushort;
		t_green = max_ushort;
		t_blue = (float)(value - x2) * (float)max_ushort / (float)(x3 - x2);
	}
	else if (value == x3)
	{
		t_red = max_ushort;
		t_green = max_ushort;
		t_blue = max_ushort;
	}

	r = (unsigned char)(max_uchar * t_red / max_ushort);
	g = (unsigned char)(max_uchar * t_green / max_ushort);
	b = (unsigned char)(max_uchar * t_blue / max_ushort);
}

auto cCamPreview::CalculateCopperColormapPixelRGB
(
	const unsigned short& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b
) -> void
{
	unsigned short max_ushort{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4'095U : USHRT_MAX };
	unsigned short
		x1{ m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 3'276U : 52'428U },
		x2{ max_ushort };

	unsigned short max_uchar{ UCHAR_MAX };
	unsigned short t_red{}, t_green{}, t_blue{};

	if (value < x1)
		t_red = (float)value * (float)max_ushort / (float)x1;
	else
		t_red = max_ushort;
	t_green = (float)value * .7812f * (float)max_ushort / (float)x2;
	t_blue = (float)value * .4975f * (float)max_ushort / (float)x2;

	r = (unsigned char)(max_uchar * t_red / max_ushort);
	g = (unsigned char)(max_uchar * t_green / max_ushort);
	b = (unsigned char)(max_uchar * t_blue / max_ushort);
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

void cCamPreview::OnMouseWheelMoved(wxMouseEvent& evt)
{
	if (m_Zoom <= 1.0 && evt.GetWheelRotation() < 0)
	{

	}
	else
	{
		//m_CursorPosOnCanvas = evt.GetPosition();
		if (evt.GetWheelRotation() > 0 && m_Zoom / m_ZoomOnOriginalSizeImage < 64.0)
		{
			AddZoom(m_ZoomStep);
		}
		else if (evt.GetWheelRotation() < 0)
		{
			if (m_Zoom > 1.0)
			{
				AddZoom(1 / m_ZoomStep);
			}
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

	m_Zoom = zoom;

	m_PanOffset.x = m_Zoom > 1.0 ? center_.x - position_on_image.x * m_Zoom : 0.0;
	m_PanOffset.y = m_Zoom > 1.0 ? center_.y - position_on_image.y * m_Zoom : 0.0;

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
	m_CheckedCursorPosOnImage.x = m_NotCheckedCursorPosOnImage.x < (double)m_ImageSize.GetWidth() ? m_CheckedCursorPosOnImage.x : (double)m_ImageSize.GetWidth() - 1.0;
	/* Checking Y */
	m_CheckedCursorPosOnImage.y = m_NotCheckedCursorPosOnImage.y >= 0.0 ? m_NotCheckedCursorPosOnImage.y : 0.0;
	m_CheckedCursorPosOnImage.y = m_NotCheckedCursorPosOnImage.y < (double)m_ImageSize.GetHeight() ? m_CheckedCursorPosOnImage.y : (double)m_ImageSize.GetHeight() - 1.0;
}

void cCamPreview::OnPreviewMouseLeftPressed(wxMouseEvent& evt)
{
	if (m_Zoom > 1.0 && m_IsCursorInsideImage)
	{
		m_Panning = true;
		m_PanStartPoint = m_CursorPosOnCanvas;
		ChangeCursorInDependenceOfCurrentParameters();
	}

	if (m_CTRLPressed && m_CrossHairTool->IsToolButtonActive())
	{
		m_ParentArguments->x_pos_crosshair->SetValue(wxString::Format(wxT("%i"), (int)(m_CheckedCursorPosOnImage.x + 1)));
		m_ParentArguments->y_pos_crosshair->SetValue(wxString::Format(wxT("%i"), (int)(m_CheckedCursorPosOnImage.y + 1)));
		//m_CrossHairTool->SetXPosFromParent(m_CheckedCursorPosOnImage.x);
		//m_CrossHairTool->SetYPosFromParent(m_CheckedCursorPosOnImage.y);
		//LOG2I("X: ", m_CheckedCursorPosOnImage.x, " Y: ", m_CheckedCursorPosOnImage.y);
		Refresh();
	}
}

void cCamPreview::OnPreviewMouseLeftReleased(wxMouseEvent& evt)
{
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
	graphics_context->SetPen(*wxRED_PEN);
	m_CrossHairTool->DrawCrossHair(graphics_context, m_ImageData.get());
}

auto cCamPreview::DrawPixelValues(wxGraphicsContext* gc) -> void
{
	if (!m_DisplayPixelValues) return;

	if (!m_ImageData) return;

	if (m_Zoom / m_ZoomOnOriginalSizeImage < 64) return;

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
				
				auto bgColor = GetPixelColorFromImage(m_Image, x, y);
				wxColour textColor(255 - bgColor.Red(), 255 - bgColor.Green(), 255 - bgColor.Blue());
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

void cCamPreview::InitDefaultComponents()
{
	//m_GraphicsBitmapImage = std::make_unique<wxGraphicsBitmap>();
	/* Tools */
	m_CrossHairTool = std::make_unique<CrossHairTool>
		(
			m_ParentArguments->x_pos_crosshair, 
			m_ParentArguments->y_pos_crosshair
		);
	//m_XimeaCameraControl = std::make_unique<XimeaControl>();
}

void cCamPreview::PaintEvent(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	Render(dc);
}

void cCamPreview::Render(wxBufferedPaintDC& dc)
{
	m_ExecutionFinished = false;

	dc.Clear();

	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc) return;

		DrawCameraCapturedImage(gc);
		delete gc;
	}

	if (!m_IsImageSet)
	{
		m_ExecutionFinished = true;
		return;
	}

	{
		auto gc = wxGraphicsContext::Create(dc);
		if (!gc)
		{
			m_ExecutionFinished = true;
			return;
		}

		// Retrieve Canvas
		{
			wxBitmap buffer(m_CanvasSize.GetWidth(), m_CanvasSize.GetHeight());
			wxMemoryDC memDC;
			memDC.SelectObject(buffer);
			memDC.Blit(0, 0, m_CanvasSize.GetWidth(), m_CanvasSize.GetHeight(), &dc, 0, 0);
			m_LastBufferImage = buffer.ConvertToImage();
		}

		DrawActualZoomedPositionOverImage(gc);
		DrawScaleBar(gc);

		DrawCrossHair(gc);
		DrawPixelValues(gc);

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
			m_ExecutionFinished = true;
			return;
		}

		DrawActualImageSize(gc);
		delete gc;
	}
	m_ExecutionFinished = true;
}

auto cCamPreview::UpdateWXImage(int black, int white) -> void
{
	LOG("Started: " + wxString(__FUNCSIG__));

	// Check number of threads on the current machine
	auto numThreads = std::thread::hardware_concurrency();

#ifdef _DEBUG
	//numThreads = 1;
#endif // _DEBUG

	std::vector<std::thread> threads;
	threads.reserve(numThreads);

	unsigned int tileSize{};
	tileSize = m_ImageSize.GetHeight() % numThreads > 0 ? m_ImageSize.GetHeight() / numThreads + 1 : m_ImageSize.GetHeight() / numThreads;

	for (auto i{ 0 }; i < (int)numThreads; ++i)
	{
		wxPoint start{}, finish{};
		start.x = 0;
		start.y = i * tileSize;

		finish.x = m_ImageSize.GetWidth();
		finish.y = (i + 1) * (int)tileSize > m_ImageSize.GetHeight() ? m_ImageSize.GetHeight() : (i + 1) * tileSize;

		threads.emplace_back
		(
			std::thread
			(
				&cCamPreview::AdjustImageParts,
				this,
				&m_ImageData[start.y * m_ImageSize.GetWidth() + start.x],
				start.x, start.y, finish.x, finish.y,
				black, white
			)
		);
	}
	for (auto& thread : threads)
	{
		thread.join();
	}

	LOG("Finished: " + wxString(__FUNCSIG__));
}

auto cCamPreview::AdjustImageParts
(
	const unsigned short* data_ptr, 
	const unsigned int start_x, 
	const unsigned int start_y, 
	const unsigned int finish_x, 
	const unsigned int finish_y,
	const int black,
	const int white
) -> void
{
	if (!data_ptr) return;
	auto current_value{ data_ptr[0] };
	unsigned char red{}, green{}, blue{};

	//white = m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4095 : USHRT_MAX;
	unsigned short max_value = m_ImageDataType == CameraPreviewVariables::ImageDataTypes::RAW_12BIT ? 4095 : USHRT_MAX;

	const double black_d = static_cast<double>(black == max_value ? max_value - 1 : black);
	const double white_d = static_cast<double>(white > max_value ? max_value : white);
	const double range_d = white_d - black_d;

	const double max_value_d = static_cast<double>(max_value);
	const double max_minus_white = max_value_d - white_d;
	const double max_minus_black = max_value_d - black_d;
	const double max_range_d = max_minus_black - max_minus_white;
	const unsigned char uchar_max = UCHAR_MAX;

	double adjustedValue{};

	for (auto y{ start_y }; y < finish_y; ++y)
	{
		auto row_offset = (y - start_y) * (finish_x - start_x);  // Precompute row offset
		for (auto x{ start_x }; x < finish_x; ++x)
		{
			int index = row_offset + (x - start_x);  // Calculate index
			current_value = data_ptr[index];
			adjustedValue = static_cast<double>(current_value);

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
				CalculateJetColormapPixelRGB(current_value, red, green, blue);
				break;

			case CameraPreviewVariables::Colormaps::COOL_COLORMAP:
			{
				red = (unsigned char)(uchar_max * adjustedValue / max_value_d);
				green = (unsigned char)(uchar_max * (max_value_d - adjustedValue) / max_value_d);
				blue = (unsigned char)uchar_max;
				break;
			}

			case CameraPreviewVariables::Colormaps::HOT_COLORMAP:
				CalculateHotColormapPixelRGB(current_value, red, green, blue);
				break;

			case CameraPreviewVariables::Colormaps::WINTER_COLORMAP:
			{
				red = 0;
				green = (unsigned char)(uchar_max * adjustedValue / max_value_d);
				auto t_blue = (float)max_value_d * (1.f - .5f * (float)adjustedValue / (float)max_value_d);
				blue = (unsigned char)(uchar_max * t_blue / max_value_d);
				break;
			}

			case CameraPreviewVariables::Colormaps::COPPER_COLORMAP:
				CalculateCopperColormapPixelRGB(current_value, red, green, blue);
				break;

			default:
				break;
			}

			m_Image.SetRGB(x, y, red, green, blue);
		}
	}

}

void cCamPreview::CreateGraphicsBitmapImage(wxGraphicsContext* gc_)
{
	if (!m_IsGraphicsBitmapSet && m_IsImageSet)
	{
		m_GraphicsBitmapImage = wxGraphicsBitmap(gc_->CreateBitmapFromImage(m_Image));;
		m_IsGraphicsBitmapSet = true;
	}
}

void cCamPreview::DrawCameraCapturedImage(wxGraphicsContext* gc_)
{
	CreateGraphicsBitmapImage(gc_);
	
	if (m_IsGraphicsBitmapSet)
	{
		auto interpolation_quality = m_Zoom / m_ZoomOnOriginalSizeImage >= 1.0 ? wxINTERPOLATION_NONE : wxINTERPOLATION_DEFAULT;

		gc_->SetInterpolationQuality(interpolation_quality);
		gc_->Scale(m_Zoom / m_ZoomOnOriginalSizeImage, m_Zoom / m_ZoomOnOriginalSizeImage);
		gc_->DrawBitmap(m_GraphicsBitmapImage,
			m_StartDrawPos.x, m_StartDrawPos.y,
			m_ImageSize.GetWidth(), m_ImageSize.GetHeight());
	}
}

auto cCamPreview::DrawFWHMValues(wxGraphicsContext* gc_) -> void
{
	if (!m_Image.IsOk() || !m_DisplayFWHM || !m_HorizontalSumArray || !m_VerticalSumArray) return;

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
	if (!m_Image.IsOk() || !m_DisplayFocusCenter || !m_HorizontalSumArray || !m_VerticalSumArray) return;
	if (m_HEWDiameter <= 0.0) return;

	wxColour fontColour(234, 54, 128, 200);
	wxFont font = wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc_->SetFont(font, fontColour);

	wxDouble widthText{}, heightText{};

	wxDouble offsetX{ 5.0 }, offsetY{ 5.0 };
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
		drawPoint.x = offsetX;
		drawPoint.y = offsetY;

		gc_->DrawText(curr_value, drawPoint.x, drawPoint.y);
	}

	LOG("Finished: " + wxString(__FUNCSIG__))
}

auto cCamPreview::DrawGridMesh(wxGraphicsContext* gc_) -> void
{	
	if (!m_Image.IsOk() || !m_DisplayGridMesh) return;

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
	if (!m_Image.IsOk() || !m_DisplayCircleMesh) return;

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
	if (!m_ImageData || !m_Image.IsOk() || m_PixelSizeUM == 0.0) return;

	auto scale_bar_start_draw = wxRealPoint
	(
		m_CanvasSize.GetWidth() - 70,
		m_CanvasSize.GetHeight() - 20
	);
	// Set line color and thickness
	auto pen_thickness = 4;

	auto scale_bar_scale_size{ 10 };
	auto scale_bar_um_array = std::make_unique<unsigned int[]>(scale_bar_scale_size);
	for (auto i{ 0 }; i < scale_bar_scale_size; ++i)
		scale_bar_um_array[i] = pow(10, i);

	int selected_scale{ scale_bar_scale_size - 1 };
	double length_horizontal_line = m_Zoom / m_ZoomOnOriginalSizeImage * scale_bar_um_array[selected_scale] / m_PixelSizeUM;
	while (length_horizontal_line > m_CanvasSize.GetWidth() / 3.0 && selected_scale > 0)
	{
		--selected_scale;
		length_horizontal_line = m_Zoom / m_ZoomOnOriginalSizeImage * scale_bar_um_array[selected_scale] / m_PixelSizeUM;
	}

	auto bgColor = GetPixelColorFromImage(m_LastBufferImage, scale_bar_start_draw.x - length_horizontal_line / 2, scale_bar_start_draw.y);
	wxColour widgetColour(255 - bgColor.Red(), 255 - bgColor.Green(), 255 - bgColor.Blue());
	
	gc_->SetPen(wxPen(widgetColour, pen_thickness));

	// Draw the first horizntal line from (x1, y1) to (x2, y2)
	gc_->StrokeLine
	(
		scale_bar_start_draw.x,
		scale_bar_start_draw.y,
		scale_bar_start_draw.x - length_horizontal_line,
		scale_bar_start_draw.y
	);

	// Draw the text
	{
		wxRealPoint drawPoint{};
		wxFont font = wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
		gc_->SetFont(font, widgetColour);

		wxDouble widthText{}, heightText{};
		wxString curr_value{};
		auto currentScaleValue = scale_bar_um_array[selected_scale];
		int thousandsCount = 0;
		while (currentScaleValue % 1'000 == 0)
		{
			currentScaleValue /= 1'000;
			++thousandsCount;
		}

		//curr_value = wxString::Format(wxT("%i"), scale_bar_um_array[selected_scale]);
		curr_value = wxString::Format(wxT("%i"), currentScaleValue);
		for (auto i{ 0 }; i < thousandsCount; ++i)
			curr_value += wxString("'000");

		curr_value += wxString(" [micron");
		curr_value += scale_bar_um_array[selected_scale] == 1 ? wxString("]") : wxString("s]");
		gc_->GetTextExtent(curr_value, &widthText, &heightText);
		drawPoint.x = scale_bar_start_draw.x - length_horizontal_line / 2.0;
		drawPoint.x -= widthText / 2.0;
		drawPoint.y = scale_bar_start_draw.y - 25;
		drawPoint.y -= heightText / 2.0;
		gc_->DrawText(curr_value, drawPoint.x, drawPoint.y);
	}
}

auto cCamPreview::DrawHEWCircle(wxGraphicsContext* gc_) -> void
{
	if (!m_Image.IsOk() || !m_HorizontalSumArray || !m_VerticalSumArray) return;
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
	if (!m_Image.IsOk() || !m_HorizontalSumArray || !m_VerticalSumArray) return;
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
		(m_StartDrawPos.y + m_ImageSize.GetHeight()) * m_Zoom / m_ZoomOnOriginalSizeImage 
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
	if (m_Zoom > 1.0) return;
	if (!m_Image.IsOk()) return;
	if (m_DisplayFWHM) return;

	wxDouble offset_x{ 10.0 }, offset_y{ 10.0 };
	// Drawing image size below it's lower and left sides
	{
		// Setting up the current font
		wxColour fontColour(255, 87, 51, 100);
		wxFont font = wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
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
			curr_value += " [px]; ";

			curr_value += wxString::Format(wxT("%.1f"), m_PixelSizeUM * m_ImageSize.GetWidth());
			curr_value += " [um]";

			gc_->GetTextExtent(curr_value, &widthText, &heightText);
			wxRealPoint draw_point =
			{
				image_start_draw.x + m_ImageOnCanvasSize.GetWidth() / 2.0 - widthText / 2.0,
				image_start_draw.y + m_ImageOnCanvasSize.GetHeight() + offset_y
				//image_start_draw.y - offset_y - heightText
			};

			if (GetSize().GetHeight() < m_ImageOnCanvasSize.GetHeight() + 2 * offset_y + 2 * heightText)
				draw_point.y = image_start_draw.y + m_ImageOnCanvasSize.GetHeight() - offset_y - heightText;

			gc_->DrawText(curr_value, draw_point.x, draw_point.y);

			// Draw the Icon
			{
				auto bmpWidth = 24;
				auto bmp = wxMaterialDesignArtProvider::GetBitmap
				(
					wxART_AUTO_FIT_WIDTH,
					wxART_CLIENT_FLUENTUI_FILLED,
					wxSize(bmpWidth, bmpWidth),
					fontColour
				);
				draw_point =
				{
					draw_point.x - bmpWidth - offset_x,
					draw_point.y + (heightText - bmpWidth) / 2.0
				};

				gc_->DrawBitmap(bmp, draw_point.x, draw_point.y, bmp.GetWidth(), bmp.GetHeight());
			}
		}

		// Draw Value on the Left side of the image
		{
			curr_value = wxString::Format(wxT("%i"), m_ImageSize.GetHeight());
			curr_value += " [px]; ";

			curr_value += wxString::Format(wxT("%.1f"), m_PixelSizeUM * m_ImageSize.GetHeight());
			curr_value += " [um]";

			gc_->GetTextExtent(curr_value, &widthText, &heightText);
			wxRealPoint draw_point =
			{
				image_start_draw.x - offset_x - heightText,
				image_start_draw.y + m_ImageOnCanvasSize.GetHeight() / 2 + widthText / 2.0
			};

			if (GetSize().GetWidth() < m_ImageOnCanvasSize.GetWidth() + 2 * offset_x + 2 * heightText)
				draw_point.x = image_start_draw.x + offset_x;

			// Draw the Icon
			{
				auto bmpWidth = 24;
				wxRealPoint icon_draw_point =
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

				gc_->DrawBitmap(bmp, icon_draw_point.x, icon_draw_point.y, bmp.GetWidth(), bmp.GetHeight());
			}


			// Set up the transformation matrix for a 90-degree counterclockwise rotation
			gc_->Translate(draw_point.x, draw_point.y);
			gc_->Rotate(-M_PI / 2.0); // Rotate 90 degrees counterclockwise (pi/2 radians)
			gc_->Translate(-draw_point.x, -draw_point.y);
			gc_->DrawText(curr_value, draw_point.x, draw_point.y);
		}
	}
}

auto cCamPreview::DrawActualZoomedPositionOverImage(wxGraphicsContext* gc_) -> void
{
	if (m_Zoom <= 1.0) return;
	if (!m_Image.IsOk()) return;

	wxDouble offset_x{ 50.0 }, offset_y{ 50.0 };
	wxDouble max_width{ 100.0 }, max_height{ 100.0 };
	auto image_minuature = wxRect2DDouble
	(
		GetSize().GetWidth() - max_width - offset_x,
		offset_y,
		m_Image.GetWidth() >= m_Image.GetHeight() ? max_width : max_height * m_Image.GetWidth() / (wxDouble)m_Image.GetHeight(),
		m_Image.GetHeight() >= m_Image.GetWidth() ? max_height : max_width * m_Image.GetHeight() / (wxDouble)m_Image.GetWidth()
	);

	auto bgColor = GetPixelColorFromImage(m_LastBufferImage, image_minuature.GetCentre().m_x, image_minuature.GetCentre().m_y);
	wxColour widgetColour(255 - bgColor.Red(), 255 - bgColor.Green(), 255 - bgColor.Blue());

	gc_->SetPen(wxPen(widgetColour, 1, wxPENSTYLE_SOLID));
	gc_->DrawRectangle
	(
		image_minuature.m_x,
		image_minuature.m_y,
		image_minuature.m_width,
		image_minuature.m_height
	);

	auto actual_view = wxRect2DDouble
	(
		//image_minuature.m_x - m_StartDrawPos.x * max_width / (double)m_Image.GetWidth(),
		//image_minuature.m_y - m_StartDrawPos.y * max_height / (double)m_Image.GetHeight(),
		image_minuature.m_x - m_StartDrawPos.x * image_minuature.m_width / (double)m_Image.GetWidth(),
		image_minuature.m_y - m_StartDrawPos.y * image_minuature.m_height / (double)m_Image.GetHeight(),
		GetSize().GetWidth() * m_ZoomOnOriginalSizeImage / m_Zoom / m_Image.GetWidth() * image_minuature.m_width,
		GetSize().GetHeight() * m_ZoomOnOriginalSizeImage / m_Zoom / m_Image.GetHeight() * image_minuature.m_height
	);
	//LOG2F("Width: ", actual_view.m_width, " Height: ", actual_view.m_height);


	gc_->SetPen(wxPen(widgetColour, 2, wxPENSTYLE_SOLID));
	if (actual_view.m_width < 7.0 || actual_view.m_height < 7.0)
	{
		wxPoint2DDouble start_draw_cross =
		{
			actual_view.m_x + actual_view.m_width / 2.0,
			actual_view.m_y + actual_view.m_height / 2.0
		};

		// Length of the line
		wxDouble leg_length{ 10.0 };
		auto rotate_angle = M_PI / 4.0; // Rotation in [rad]
		// Draw the cross
		gc_->StrokeLine
		(
			start_draw_cross.m_x - leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y - leg_length / 2.0 * sin(rotate_angle),
			start_draw_cross.m_x + leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y + leg_length / 2.0 * sin(rotate_angle)
		);  // Horizontal line
		gc_->StrokeLine
		(
			start_draw_cross.m_x - leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y + leg_length / 2.0 * sin(rotate_angle),
			start_draw_cross.m_x + leg_length / 2.0 * cos(rotate_angle),
			start_draw_cross.m_y - leg_length / 2.0 * sin(rotate_angle)
		);  // Vertical line
	}
	else
	{
		gc_->DrawRectangle
		(
			actual_view.m_x,
			actual_view.m_y,
			actual_view.m_width,
			actual_view.m_height
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
		m_Zoom = 1.0;
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
	m_ZoomOnOriginalSizeImage = 1.0;

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

