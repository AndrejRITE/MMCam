#pragma once
#ifndef CCAM_PREVIEW_H
#define CCAM_PREVIEW_H

#include "wx/wx.h"
#include "wx/dcbuffer.h"
#include "wx/graphics.h"
#include "wx/tglbtn.h"
#include "wxMaterialDesignArtProvider.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <algorithm>

#include <opencv2/opencv.hpp>

#include "PostprocessingAlgorithms.h"

#include "cPreviewTools.h"

namespace CameraPreviewVariables
{
	enum Colormaps
	{
		GRAYSCALE_COLORMAP,
		INVERT_COLORMAP,
		JET_COLORMAP,
		COOL_COLORMAP,
		HOT_COLORMAP,
		WINTER_COLORMAP,
		COPPER_COLORMAP
	};

	enum 
	{
		MORAVIAN_INSTRUMENTS_CAM,
		XIMEA_CAM,
	};

	enum ImageDataTypes
	{
		RAW_12BIT,
		RAW_16BIT,
	};


	struct InputPreviewPanelArgs
	{
		wxTextCtrl* x_pos_crosshair{}, * y_pos_crosshair{};
		wxStatusBar* statusBar{};
		//wxToggleButton* set_pos_tgl_btn{};
		InputPreviewPanelArgs() {};
		InputPreviewPanelArgs
		(
			wxTextCtrl* par_x_pos_crosshair,
			wxTextCtrl* par_y_pos_crosshair,
			wxStatusBar* par_statusBar
			//wxToggleButton* par_pos_crosshair_tgl_btn
		) : x_pos_crosshair(par_x_pos_crosshair),
			y_pos_crosshair(par_y_pos_crosshair),
			statusBar(par_statusBar)
		{};
			//set_pos_tgl_btn(par_pos_crosshair_tgl_btn) {};
	};
}

class cCamPreview final : public wxPanel
{
public:
	cCamPreview
	(
		wxFrame* parent_frame,
		wxSizer* parent_sizer,
		std::unique_ptr<CameraPreviewVariables::InputPreviewPanelArgs> input_preview_panel_args
	);
	auto SetBackgroundColor(wxColour bckg_colour) -> void;
	auto SetValueDisplayingActive(bool activate = false) -> void;

	auto SetImageColormapMode(const CameraPreviewVariables::Colormaps colormapMode) -> void
	{
		if (colormapMode < 0 || colormapMode > 6) return;
		m_ColormapMode = colormapMode;
	};
	
	auto IsImageSet() const -> bool { return m_ImageData != nullptr; }

	auto ActivateCrossHairDisplaying(bool activate = false) -> void;
	auto ActivateFWHMDisplaying(bool activate = false) -> void;
	auto ActivateFocusCenterDisplaying(bool activate = false) -> void;
	auto ActivateGridMeshDisplaying(bool activate = false) -> void;
	auto ActivateCircleMeshDisplaying(bool activate = false) -> void;

	auto SetCameraDataType(CameraPreviewVariables::ImageDataTypes dataType) -> void { m_ImageDataType = dataType; };

	void SetXCrossHairPosFromParentWindow(const int& x_pos);
	void SetYCrossHairPosFromParentWindow(const int& y_pos);
	auto SettingCrossHairPosFromParentWindow(bool set = false) -> void;
	//auto SetImageSize(const wxSize& img_size) -> void;
	auto GetDataPtr() const -> unsigned short*;
	//auto GetImagePtr() const->wxImage*;
	auto GetImageSize() const->wxSize;
	//auto InitializeSelectedCamera(const std::string& camera_sn) -> void;
	auto UpdateImageParameters(const wxSize oldImageSize) -> void;

	auto GetPixelColorFromImage(const wxImage& image, int x, int y) -> wxColour;

	auto SetCameraCapturedImage
	(
		unsigned short* data_ptr,
		const wxSize& imgSize,
		unsigned short minValue,
		unsigned short maxValue
	) -> void;

	auto UpdateCursorPositionOnStatusBar() -> void;
	
	// Jet Colormaps
	void CalculateJetColormapPixelRGB
	(
		const unsigned short& value,
		unsigned char& r,
		unsigned char& g,
		unsigned char& b
	);
	
	// Hot Colormaps
	auto CalculateHotColormapPixelRGB
	(
		const unsigned short& value,
		unsigned char& r,
		unsigned char& g,
		unsigned char& b
	) -> void;
	
	// Copper Colormaps
	auto CalculateCopperColormapPixelRGB
	(
		const unsigned short& value,
		unsigned char& r,
		unsigned char& g,
		unsigned char& b
	) -> void;

	// FWHM
	auto SetPixelSizeUM(const double pixel_sizeUM) 
	{ 
		m_PixelSizeUM = pixel_sizeUM > 0.0 ? pixel_sizeUM : 1.0;
		if (m_CropSizeMM != 0.0)
			m_ROIWindowWidth = static_cast<int>(std::ceil(m_CropSizeMM / (m_PixelSizeUM / 1000.0)));
	};

	auto SetCropSizeMM(const double crop_sizeMM) 
	{ 
		m_CropSizeMM = crop_sizeMM > 0.0 ? crop_sizeMM : 0.0; 
		if (m_PixelSizeUM != 0.0)
			m_ROIWindowWidth = static_cast<int>(std::ceil(m_CropSizeMM / (m_PixelSizeUM / 1000.0)));
	};

	auto GetExecutionFinishedPtr() -> bool* { return &m_ExecutionFinished; };

	auto SetGridMeshStepPX(const unsigned int step)
	{
		if (step < 1 || step > 10'000) return;
		m_GridMeshStepPX = step;
	}

	auto SetCircleMeshStepPX(const unsigned int step)
	{
		if (step < 1 || step > 10'000) return;
		m_CircleMeshStepPX = step;
	}


private:
	void InitDefaultComponents();
	void PaintEvent(wxPaintEvent& evt);
	void Render(wxBufferedPaintDC& dc);
	void DrawImage(wxGraphicsContext* gc);
	
	auto UpdateWXImage(const int black, const int white) -> void;
	auto AdjustImageParts
	(
		const unsigned short* data_ptr,
		const unsigned int start_x,
		const unsigned int start_y,
		const unsigned int finish_x,
		const unsigned int finish_y,
		const int black,
		const int white
	) -> void;


	void CreateGraphicsBitmapImage(wxGraphicsContext* gc_);
	void DrawCameraCapturedImage(wxGraphicsContext* gc_);
	auto DrawFWHMValues(wxGraphicsContext* gc_) -> void;
	auto DrawHEWValues(wxGraphicsContext* gc_) -> void;

	auto DrawGridMesh(wxGraphicsContext* gc_) -> void;
	auto DrawCircleMesh(wxGraphicsContext* gc_) -> void;

	auto DrawScaleBar(wxGraphicsContext* gc_) -> void;
	
	auto DrawHEWCircle(wxGraphicsContext* gc_) -> void;
	auto DrawSpotCroppedWindow(wxGraphicsContext* gc_) -> void;
	auto DrawSumLines(wxGraphicsContext* gc_) -> void;
	auto DrawHorizontalSumLine(wxGraphicsContext* gc_) -> void;
	auto DrawVerticalSumLine(wxGraphicsContext* gc_) -> void;
	auto DrawActualImageSize(wxGraphicsContext* gc_) -> void;
	auto DrawActualZoomedPositionOverImage(wxGraphicsContext* gc_) -> void;

	void OnSize(wxSizeEvent& evt);
	void ChangeSizeOfImageInDependenceOnCanvasSize();
	auto UpdateCrossHairOnSize() -> void;
	void OnMouseMoved(wxMouseEvent& evt);
	auto OnEnterPanel(wxMouseEvent& evt) -> void;
	/* Zooming */
	void OnMouseWheelMoved(wxMouseEvent& evt);
	void AddZoom(const double& zoom, bool zoom_in_center_of_window = false);
	void SetZoom(const double& zoom, const wxRealPoint& center_);
	/* Panning */
	void ProcessPan(const wxRealPoint& point_, bool refresh_);
	void FinishPan(bool refresh);

	void CheckIfMouseAboveImage();
	void CalculatePositionOnImage();

	void OnPreviewMouseLeftPressed(wxMouseEvent& evt);
	void OnPreviewMouseLeftReleased(wxMouseEvent& evt);

	void ChangeCursorInDependenceOfCurrentParameters();

	/* CrossHair */
	void DrawCrossHair(wxGraphicsContext* graphics_context);
	auto DrawPixelValues(wxGraphicsContext* gc) -> void;

	/* Key Events */
	auto OnKeyPressed(wxKeyEvent& evt) -> void;
	auto OnKeyReleased(wxKeyEvent& evt) -> void;

	auto CalculateFWHM() -> void;
	auto CalculateHEW() -> void;

private:
	/* Buttons on keyboard */
	bool m_CTRLPressed{};

	CameraPreviewVariables::Colormaps m_ColormapMode{ CameraPreviewVariables::Colormaps::GRAYSCALE_COLORMAP };

	bool m_ExecutionFinished{ true };
	int m_Width{}, m_Height{};
	bool m_IsGraphicsBitmapSet{}, m_IsImageSet{};
	wxGraphicsBitmap m_GraphicsBitmapImage{};

	wxImage m_Image{};
	std::unique_ptr<unsigned short[]> m_ImageData{};
	
	wxImage m_LastBufferImage{};

	wxSize m_ImageSize{}, m_ImageOnCanvasSize{}, m_CanvasSize{};
	wxRealPoint m_NotCheckedCursorPosOnImage{}, m_CheckedCursorPosOnImage{}, m_CursorPosOnCanvas{};
	wxRealPoint m_NotZoomedGraphicsBitmapOffset{}, m_StartDrawPos{};
	bool m_IsCursorInsideImage{};
	/* Panning */
	bool m_Panning{};
	wxRealPoint m_PanOffset{}, m_PanStartPoint{}, m_PanDeltaPoints{};

	/* Zoom */
	double m_Zoom{}, m_ZoomOnOriginalSizeImage{};
	const double m_ZoomStep{ 1.4 }; // Should be bigger than 1.0, otherwise infinity loop

	//std::unique_ptr<XimeaControl> m_XimeaCameraControl{};
	std::string m_SelectedCameraSN{};

	/* CrossHair Tool */
	std::unique_ptr<CrossHairTool> m_CrossHairTool{};
	wxPoint m_CrossHairPos{};
	bool m_ChangingCrossHairPosition{};
	//bool m_SettingCrossHairPos{};

	bool m_DisplayPixelValues{};

	// FWHM
	bool m_DisplayFWHM{};
	std::unique_ptr<unsigned int[]> m_HorizontalSumArray{}, m_VerticalSumArray{};
	int m_HorizontalFWHM_PX{ -1 }, m_VerticalFWHM_PX{ -1 };
	unsigned int m_HorizontalWorstSum{}, m_HorizonalBestSum{};
	unsigned int m_VerticalWorstSum{}, m_VerticalBestSum{};
	unsigned int m_MinValueInData{};
	int m_HorizontalMiddleFWHMPosPixel{}, m_VerticalMiddleFWHMPosPixel{};
	double m_PixelSizeUM{}, m_CropSizeMM{};
	int m_ROIWindowWidth{};

	bool m_DisplayGridMesh{}, m_DisplayCircleMesh{};
	int m_GridMeshStepPX{ 100 };
	int m_CircleMeshStepPX{ 200 };

	bool m_DisplayFocusCenter{};
	bool m_DisplayCrossHair{};

	int m_HEWDiameter{};

	int m_ImageDataType{};

	/* Scale Bar */
	bool m_DisplayScaleBar{ true };

	std::unique_ptr<CameraPreviewVariables::InputPreviewPanelArgs> m_ParentArguments{};

	DECLARE_EVENT_TABLE();
};

#endif // !CCAM_PREVIEW_H

