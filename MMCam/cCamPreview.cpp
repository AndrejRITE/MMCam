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

auto cCamPreview::SetImageSize(const wxSize& img_size) -> void
{
	m_ImageSize = img_size;

	m_ImageData = std::make_unique<unsigned short[]>(img_size.GetWidth() * img_size.GetHeight());

	m_HorizontalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetWidth());
	m_VerticalSumArray = std::make_unique<unsigned int[]>(m_ImageSize.GetHeight());

	m_Image.Create(img_size);
}

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

void cCamPreview::UpdateImageParameters()
{
	LOG("Started: " + wxString(__FUNCSIG__))
	/* 
	Saving previous values for correct displaying of the image in the same place, 
	where it was before capturing.
	*/
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
	{
		m_CrossHairTool->SetImageDataType(ToolsVariables::DATA_U12);
		m_CrossHairTool->SetImageDimensions(m_ImageSize);
		m_CrossHairTool->SetZoomOfOriginalSizeImage(m_ZoomOnOriginalSizeImage);
		m_CrossHairTool->UpdateZoomValue(m_Zoom);
		m_CrossHairTool->SetImageStartDrawPos(wxRealPoint
		(
			m_StartDrawPos.x * m_Zoom / m_ZoomOnOriginalSizeImage,
			m_StartDrawPos.y * m_Zoom / m_ZoomOnOriginalSizeImage
		));
		//m_CrossHairTool->SetXPosFromParent(m_ImageSize.GetWidth() / 2);
		//m_CrossHairTool->SetYPosFromParent(m_ImageSize.GetHeight() / 2);
	}

	// FWHM
	if (m_DisplayFWHM)
		CalculateFWHM();

	m_IsImageSet = true;
	m_IsGraphicsBitmapSet = false;
	Refresh();

	LOG("Finished: " + wxString(__FUNCSIG__))
}

auto cCamPreview::SetCameraCapturedImage
(
	unsigned short* data_ptr
) -> void
{
	if (!data_ptr) return;
	if (!m_ImageSize.GetWidth() || !m_ImageSize.GetHeight()) return;

	LOG("Started: " + wxString(__FUNCSIG__));

	m_ExecutionFinished = false;
	unsigned long long readDataSize = m_ImageSize.GetWidth() * m_ImageSize.GetHeight();
	if (!m_ImageData)
	{
		m_ImageData = std::make_unique<unsigned short[]>(readDataSize);
		m_Image = wxImage(m_ImageSize.GetWidth(), m_ImageSize.GetHeight());
	}

	memcpy(m_ImageData.get(), data_ptr, sizeof(unsigned short) * readDataSize);

	UpdateWXImage();

	UpdateImageParameters();
	LOG("Image was updated: " + wxString(__FUNCSIG__))
	m_ExecutionFinished = true;
}

//void cCamPreview::CaptureAndSaveDataFromCamera
//(
//	const unsigned long& exposure_time_us, 
//	const wxString& path, 
//	const std::string& start_hours,
//	const std::string& start_minutes,
//	const std::string& start_seconds,
//	const int& frame_number, 
//	const float& first_axis_position, 
//	const float& second_axis_position
//)
//{
//	auto image_ptr = m_XimeaCameraControl->GetImage();
//	if (!image_ptr) return;
//
//	/* Save Captured Image */
//	{
//		std::string first_axis_position_str = std::format("{:.3f}", first_axis_position);
//		std::replace(first_axis_position_str.begin(), first_axis_position_str.end(), '.', '_');
//
//		std::string second_axis_position_str = std::format("{:.3f}", second_axis_position);
//		std::replace(second_axis_position_str.begin(), second_axis_position_str.end(), '.', '_');
//		
//		const std::string file_name = std::string(path.mb_str()) + std::string("\\") +
//			std::string("img_") + 
//			std::to_string(frame_number) + std::string("_") + 
//			start_hours + std::string("H_") + 
//			start_minutes + std::string("M_") + 
//			start_seconds + std::string("S_") + 
//			std::to_string(exposure_time_us) + std::string("us") 
//			+ std::string("_1A_") + first_axis_position_str 
//			+ std::string("_2A_") + second_axis_position_str 
//			+ std::string(".tif");
//
//		cv::Mat cv_img
//		(
//			cv::Size(m_XimeaCameraControl->GetImageWidth(), m_XimeaCameraControl->GetImageHeight()),
//			CV_8UC1, 
//			image_ptr, 
//			cv::Mat::AUTO_STEP
//		);
//		cv::imwrite(file_name, cv_img);
//	}
//
//	//SetCameraCapturedImage(image_ptr);
//}

void cCamPreview::CalculateMatlabJetColormapPixelRGB8bit
(
	const unsigned char& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b
)
{
	unsigned char x0_8bit{ 31 }, x1_8bit{ 95 }, x2_8bit{ 159 }, x3_8bit{ 223 }, x4_8bit{ 255 };
	if (value < x0_8bit)
	{
		r = 0;
		g = 0;
		b = 255 * 0.51563f + (float)value * (255.0f - 255 * 0.51563f) / (float)x0_8bit;
	}
	else if (value >= x0_8bit && value <= x1_8bit)
	{
		r = 0;
		g = (float)(value - x0_8bit) * 255.0f / (float)(x1_8bit - x0_8bit);
		b = 255;
	}
	else if (value > x1_8bit && value < x2_8bit)
	{
		r = (float)(value - x1_8bit) * 255.0f / (float)(x2_8bit - x1_8bit);
		g = 255;
		b = (float)(x2_8bit - value) * 255.0f / (float)(x2_8bit - x1_8bit);
	}
	else if (value >= x2_8bit && value <= x3_8bit)
	{
		r = 255;
		g = (float)(x3_8bit - value) * 255.0f / (float)(x3_8bit - x2_8bit);
		b = 0;
	}
	else if (value > x3_8bit)
	{
		r = 255.0f * 0.5f + (float)(x4_8bit - value) * (255.0f - 255.0f * 0.5f) / (float)(x4_8bit - x3_8bit);
		g = 0;
		b = 0;
	}
}

void cCamPreview::CalculateMatlabJetColormapPixelRGB12bit(const unsigned short& value, unsigned char& r, unsigned char& g, unsigned char& b)
{
	unsigned short x0_12bit{ 498 }, x1_12bit{ 1526 }, x2_12bit{ 2553 }, x3_12bit{ 3581 }, x4_12bit{ 4095 };
	if (value < x0_12bit)
	{
		r = 0;
		g = 0;
		b = 255 * 0.51563f + (float)value * (255.0f - 255 * 0.51563f) / (float)x0_12bit;
	}
	else if (value >= x0_12bit && value <= x1_12bit)
	{
		r = 0;
		g = (float)(value - x0_12bit) * 255.0f / (float)(x1_12bit - x0_12bit);
		b = 255;
	}
	else if (value > x1_12bit && value < x2_12bit)
	{
		r = (float)(value - x1_12bit) * 255.0f / (float)(x2_12bit - x1_12bit);
		g = 255;
		b = (float)(x2_12bit - value) * 255.0f / (float)(x2_12bit - x1_12bit);
	}
	else if (value >= x2_12bit && value <= x3_12bit)
	{
		r = 255;
		g = (float)(x3_12bit - value) * 255.0f / (float)(x3_12bit - x2_12bit);
		b = 0;
	}
	else if (value > x3_12bit && value < x4_12bit)
	{
		r = 255.0f * 0.5f + (float)(x4_12bit - value) * (255.0f - 255.0f * 0.5f) / (float)(x4_12bit - x3_12bit);
		g = 0;
		b = 0;
	}
	else if (value == x4_12bit)
	{
		// Saturation (white)
		r = 255.f;
		g = 255.f;
		b = 255.f;
	}
}

void cCamPreview::CalculateMatlabJetColormapPixelRGB16bit
(
	const uint16_t& value, 
	unsigned char& r, 
	unsigned char& g, 
	unsigned char& b
)
{
	uint16_t x0{ 7967 }, x1{ 24415 }, x2{ 40863 }, x3{ 57311 }, x4{ 65535 };
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
	else if (value > x3)
	{
		r = 255.0f * 0.5f + (float)(x4 - value) * (255.0f - 255.0f * 0.5f) / (float)(x4 - x3);
		g = 0;
		b = 0;
	}
}

void cCamPreview::OnMouseMoved(wxMouseEvent& evt)
{
	if (m_IsImageSet)
	{
		m_CursorPosOnCanvas.x = m_ZoomOnOriginalSizeImage * evt.GetPosition().x;
		m_CursorPosOnCanvas.y = m_ZoomOnOriginalSizeImage * evt.GetPosition().y;

		/* Mouse position on Image */
		CalculatePositionOnImage();
		CheckIfMouseAboveImage();


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
	if (m_DisplayPixelValues)
		m_CrossHairTool->DrawPixelValues(graphics_context, m_ImageData.get());
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

		DrawActualZoomedPositionOverImage(gc);
		DrawScaleBar(gc);

		DrawCrossHair(gc);

		DrawGridMesh(gc);
		DrawCircleMesh(gc);

		DrawHEWCircle(gc);
		DrawSpotCroppedWindow(gc);
		DrawSumLines(gc);

		/* FWHM */
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

auto cCamPreview::UpdateWXImage() -> void
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
				start.x, start.y, finish.x, finish.y
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
	const unsigned int finish_y
) -> void
{
	if (!data_ptr) return;
	unsigned short current_value{};
	unsigned char red{}, green{}, blue{};
	unsigned long long position_in_data_pointer{};

	for (auto y{ start_y }; y < finish_y; ++y)
	{
		for (auto x{ start_x }; x < finish_x; ++x)
		{
			current_value = data_ptr[position_in_data_pointer];
			/* Matlab implementation of JetColormap */
			/* Because XIMEA camera can produce 12-bit per pixel maximum, we use RGB12bit converter */
			CalculateMatlabJetColormapPixelRGB12bit(current_value, red, green, blue);
			m_Image.SetRGB(x, y, red, green, blue);
			++position_in_data_pointer;
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
	wxColour fontColour(255, 255, 0, 180);
	gc_->SetPen(wxPen(fontColour, pen_thickness));

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
		gc_->SetFont(font, fontColour);
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

	auto penColour = wxColour("green");
	auto penSize = 2;
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
	gc_->SetPen(wxPen(wxColour("gold"), 1, wxPENSTYLE_SOLID));
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
	gc_->SetPen(wxPen(wxColour("gold"), 2, wxPENSTYLE_SOLID));
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
	while (canvas_size.GetWidth() < current_image_size.GetWidth() || canvas_size.GetHeight() < current_image_size.GetHeight())
	{
		current_image_size.SetWidth(current_image_size.GetWidth() / 2);
		current_image_size.SetHeight(current_image_size.GetHeight() / 2);
		m_ZoomOnOriginalSizeImage *= 2.0;
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

