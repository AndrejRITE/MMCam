#pragma once
#ifndef CCAM_PREVIEW_H
#define CCAM_PREVIEW_H

#include "wx/wx.h"
#include "wx/dcbuffer.h"
#include "wx/graphics.h"
#include "wx/tglbtn.h"
#include "MaterialDesign/wxMaterialDesignArtProvider.hpp"

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
		IMAGEJ_16_COLORS_COLORMAP,
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
		wxWindow* frame;
		wxSizer* sizer;
		wxTextCtrl* x_pos_crosshair{}, * y_pos_crosshair{};
		wxTextCtrl* xAnnulusCenterPos{}, * yAnnulusCenterPos{}, * r1Annulus{}, * r2Annulus{};
		wxStatusBar* statusBar{};

		InputPreviewPanelArgs() {};
		InputPreviewPanelArgs
		(
			wxWindow* parentFrame,
			wxSizer* parentSizer,
			wxTextCtrl* parXPosCrosshair,
			wxTextCtrl* parYPosCrosshair,
			wxTextCtrl* parXAnnulusCenterPos,
			wxTextCtrl* parYAnnulusCenterPos,
			wxTextCtrl* parR1Annulus,
			wxTextCtrl* parR2Annulus,
			wxStatusBar* parStatusBar
		) : frame(parentFrame),
			sizer(parentSizer),
			x_pos_crosshair(parXPosCrosshair),
			y_pos_crosshair(parYPosCrosshair),
			xAnnulusCenterPos(parXAnnulusCenterPos),
			yAnnulusCenterPos(parYAnnulusCenterPos),
			r1Annulus(parR1Annulus),
			r2Annulus(parR2Annulus),
			statusBar(parStatusBar)
		{};
	};

	struct ImageStats 
	{ 
		unsigned short minV{}, maxV{}; 
		unsigned long long sum{}; 
		double mean{}, stddev{}; 
		unsigned long long blackClippedPixelsCount{}, saturatedPixelsCount{};

		bool valid{ false }; 
	};

	struct Annulus
	{
		Annulus()
		{
			static std::atomic<long> counter{ 1 }; // start at 1 to avoid confusion with 0
			m_ID = counter.fetch_add(1, std::memory_order_relaxed);
		};

		auto SetID(const long& id) -> void { m_ID = id; };
		auto GetID() const -> long { return m_ID; };

	public:
		wxPoint m_Center{};
		wxDouble m_InnerRadius{}, m_OuterRadius{ 1.0 };
		wxLongLong m_Sum{};

	private:
		long m_ID{};
	};

	static auto CreateStringWithPrecision(double value, int decimalPlaces = 0) -> wxString
	{
		std::ostringstream stream;
		if (decimalPlaces > 0)
		{
			stream.precision(decimalPlaces);
			stream << std::fixed;
		}
		stream << value;
		return wxString::FromUTF8(stream.str());
	};

	static auto CreateScientificString(wxLongLong value, int decimalPlaces = 3) -> wxString
	{
		double doubleValue = static_cast<double>(value.GetValue());
		std::ostringstream stream;
		stream.precision(decimalPlaces);
		stream << std::scientific << std::uppercase << doubleValue;
		return wxString::FromUTF8(stream.str());
	};

	static auto FormatWithThousandsDelimiter = [](long long value, const wxString& delimiter = ",") -> wxString
	{
		const bool negative = value < 0;
		const unsigned long long magnitude = negative
			? static_cast<unsigned long long>(-(value + 1)) + 1ull
			: static_cast<unsigned long long>(value);

		wxString formatted = wxString::Format("%llu", magnitude);

		for (int insertPos = static_cast<int>(formatted.Length()) - 3; insertPos > 0; insertPos -= 3)
		{
			formatted.insert(static_cast<size_t>(insertPos), delimiter);
		}

		return negative ? "-" + formatted : formatted;
	};
}

class cCamPreview final : public wxPanel
{
public:
	cCamPreview
	(
		std::unique_ptr<CameraPreviewVariables::InputPreviewPanelArgs> previewPanelArgs
	);

	auto SetValueDisplayingActive(bool activate = false) -> void { m_DisplayPixelValues = activate; };

	auto SetImageStatisticsDisplayingActive(bool activate = false) -> void { m_ShowImageStats = activate; };

	auto SetAnnulusDisplayingActive(const bool activate = false) -> void { m_DisplayAnnulus = activate; };

	auto SetImageColormapMode(const CameraPreviewVariables::Colormaps colormapMode) -> void
	{
		using Colormap = CameraPreviewVariables::Colormaps;

		switch (colormapMode)
		{
		case Colormap::GRAYSCALE_COLORMAP:
		case Colormap::INVERT_COLORMAP:
		case Colormap::JET_COLORMAP:
		case Colormap::IMAGEJ_16_COLORS_COLORMAP:
		case Colormap::COOL_COLORMAP:
		case Colormap::HOT_COLORMAP:
		case Colormap::WINTER_COLORMAP:
		case Colormap::COPPER_COLORMAP:
			m_ColormapMode = colormapMode;
			break;
		default:
			throw std::invalid_argument("SetImageColormapMode: Invalid colormap mode.");
		}
	};

	auto SetCameraSensorSize(const wxSize size) -> void { m_SensorSize = size; }
	auto SetROI(const wxRect roi) -> void { m_ROIRect = roi; Refresh(); }
	
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

	/// Displayed colour of image pixel (x, y) for the current colormap and levels.
	auto GetImagePixelColour(int x, int y) const -> wxColour;
	/// Colour shown at canvas point (x, y): an image pixel, or the background outside the image.
	auto GetCanvasColour(double x, double y) const -> wxColour;

	auto SetCameraCapturedImage
	(
		unsigned short* data_ptr,
		const wxSize& imgSize,
		unsigned short minValue,
		unsigned short maxValue
	) -> void;

	auto UpdateBlackAndWhiteRange(const int black, const int white) -> std::optional<int>;

	auto UpdateCursorPositionOnStatusBar() -> void;
	
	// Jet Colormaps
	void CalculateJetColormapPixelRGB
	(
		const unsigned short& value,
		unsigned char& r,
		unsigned char& g,
		unsigned char& b,
		const int& black,
		const int& white
	);

	void Calculate16_ColorsImageJColormapPixelRGB
	(
		const unsigned short& value,
		unsigned char& r,
		unsigned char& g,
		unsigned char& b,
		const int& black,
		const int& white
	);
	
	// Hot Colormaps
	auto CalculateHotColormapPixelRGB
	(
		const unsigned short& value,
		unsigned char& r,
		unsigned char& g,
		unsigned char& b,
		const int& black,
		const int& white
	) -> void;
	
	// Copper Colormaps
	auto CalculateCopperColormapPixelRGB
	(
		const unsigned short& value,
		unsigned char& r,
		unsigned char& g,
		unsigned char& b,
		const int& black,
		const int& white
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

	bool IsExecutionFinished() const noexcept { return m_ExecutionFinished.load(std::memory_order_acquire); };

	void SetExecutionFinished(bool v) noexcept { m_ExecutionFinished.store(v, std::memory_order_release); };

	std::atomic<bool>* GetExecutionFinishedPtr() noexcept { return &m_ExecutionFinished; };

	auto SetCrossHairAveragingWidthPX(const int& avgWidth)
	{
		m_CrossHairAveragingWidthPX = std::clamp(avgWidth, 1, 10'000);
		m_CrossHairTool->SetAveragingWidthPX(m_CrossHairAveragingWidthPX);
	};

	auto ActivateCrossHairAdaptiveScaling(const bool isActive = false)
	{
		m_CrossHairTool->ActivateAdaptiveScaling(isActive);
	};

	auto SetGridMeshStepPX(const int& step)
	{
		m_GridMeshStepPX = std::clamp(step, 1, 10'000);
	};

	auto SetCircleMeshStepPX(const int& step)
	{
		m_CircleMeshStepPX = std::clamp(step, 1, 10'000);
	};

	/* Annulus */
	auto AddAnnulusOnCurrentImage() -> CameraPreviewVariables::Annulus;
	auto RemoveAnnulusByID(const long& id) -> void;;
	auto CalculateSumInsideAnnulus(CameraPreviewVariables::Annulus& annulus) -> void;
	auto SetAnnulusIDSelected(const long& id) -> CameraPreviewVariables::Annulus;
	auto UpdateAnnulusValues(CameraPreviewVariables::Annulus& annulus) -> void;

	void ActivateFPSDisplaying(bool show = true) { m_ShowFPS = show; Refresh(); }

	auto ResetFPS() -> void;

private:
	void InitDefaultComponents();
	void PaintEvent(wxPaintEvent& evt);
	void Render(wxBufferedPaintDC& dc);
	void DrawImage(wxGraphicsContext* gc);
	
	/// Rebuilds the value -> RGB lookup table for the given black/white levels.
	auto UpdateColourLut(const int black, const int white) -> void;
	auto HasImage() const -> bool { return m_ImageData && m_ImageSize.GetWidth() > 0 && m_ImageSize.GetHeight() > 0; }
	/// Colour of one pixel value for the current colormap and black/white levels.
	auto MapValueToRGB
	(
		const unsigned short current_value,
		unsigned char& red,
		unsigned char& green,
		unsigned char& blue,
		const int black,
		const int white
	) -> void;


	/// Resamples the image into m_ScreenBitmap for the given canvas rectangle and zoom.
	void CreateScreenBitmap(const wxRect& screenRect, const double k);
	void DrawCameraCapturedImage(wxDC& dc);
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
	auto DrawEmptyState(wxGraphicsContext* gc) -> void;
	/// Bottom edge of the badges stacked in the top-left corner (FPS, HEW, ...), reset on every paint
	wxDouble m_TopLeftOverlayBottom{};
	auto DrawActualZoomedPositionOverImage(wxGraphicsContext* gc_) -> void;
	/* Minimap (top right while zoomed in): click or drag on it to move the view */
	auto UpdateMinimapBitmap(const int width, const int height) -> void;
	auto CenterViewOnImagePoint(const wxRealPoint& imagePoint) -> void;
	auto MinimapHitTest(const wxPoint& canvasPos) const -> bool;
	auto MinimapToImage(const wxPoint& canvasPos) const -> wxRealPoint;
	wxBitmap m_MinimapBitmap{};
	unsigned long long m_ImageRevision{};         ///< bumped whenever the displayed colours change
	unsigned long long m_MinimapBitmapRevision{}; ///< m_ImageRevision the thumbnail was built for
	wxRect2DDouble m_MinimapThumbRect{};
	bool m_MinimapVisible{}, m_MinimapDragging{};

	auto DrawROI(wxGraphicsContext* gc_) -> void;

	void OnSize(wxSizeEvent& evt);
	void ChangeSizeOfImageInDependenceOnCanvasSize();
	auto UpdateCrossHairOnSize() -> void;
	void OnMouseMoved(wxMouseEvent& evt);
	auto OnEnterPanel(wxMouseEvent& evt) -> void;
	auto OnLeavePanel(wxMouseEvent& evt) -> void;
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

	/* Dragging the crosshair (line profile) lines */
	struct CrossHairHit { bool row{}, column{}; bool Any() const { return row || column; } };
	/// Which crosshair line(s) are within grabbing distance of a canvas point.
	auto HitTestCrossHair(const wxPoint& canvasPos) const -> CrossHairHit;
	auto MoveCrossHairTo(const wxPoint& canvasPos) -> void;
	auto SetCrossHairHover(const CrossHairHit& hit) -> void;
	auto EndCrossHairDrag() -> void;
	CrossHairHit m_CrossHairHover{}, m_CrossHairDrag{};
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

	/* Annulus */
	auto DrawAnnulus(wxGraphicsContext* gc) -> void;

	auto DrawImageStatistics(wxGraphicsContext* gc) -> void;

	/* Key Events */
	auto OnKeyPressed(wxKeyEvent& evt) -> void;
	auto OnKeyReleased(wxKeyEvent& evt) -> void;

	auto CalculateFWHM() -> void;
	auto CalculateHEW() -> void;

	template<typename Func>
	void ForEachPixelInAnnulus(const CameraPreviewVariables::Annulus& annulus, Func&& callback);

	auto CalculateImageStatistics() -> void;

	void TickFPS();
	void DrawFPS(wxGraphicsContext* gc);

private:
	/* Buttons on keyboard */
	bool m_CTRLPressed{};

	CameraPreviewVariables::Colormaps m_ColormapMode{ CameraPreviewVariables::Colormaps::GRAYSCALE_COLORMAP };

	std::atomic<bool> m_ExecutionFinished{ true };

	int m_Width{}, m_Height{};
	bool m_IsGraphicsBitmapSet{}, m_IsImageSet{};
	/// The visible part of the image at screen resolution. Memory and repaint time do not
	/// depend on the sensor size (a full-frame copy of a 100 MP image would need a 300 MB
	/// RGB image plus a 400 MB GDI+ bitmap). m_IsGraphicsBitmapSet = false forces a rebuild.
	wxBitmap m_ScreenBitmap{};
	wxRect m_ScreenBitmapRect{};      ///< canvas area covered by m_ScreenBitmap
	double m_ScreenBitmapZoom{};      ///< canvas pixels per image pixel it was built for
	wxRealPoint m_ScreenBitmapStart{};///< m_StartDrawPos it was built for

	std::vector<unsigned char> m_ColourLut{}; ///< RGB triplets indexed by pixel value
	unsigned short m_ColourLutMax{};
	std::unique_ptr<unsigned short[]> m_ImageData{};

	wxSize m_SensorSize{};

	wxRect m_ROIRect{};

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
	const double m_MinZoom{ 1.0 }, m_MaxZoom{ m_ZoomStep * 54 };

	const wxColour m_ContrastDefaultColor = wxColour(255, 127, 39);

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
	int m_CrossHairAveragingWidthPX{ 1 };

	bool m_DisplayFocusCenter{};
	bool m_DisplayCrossHair{};

	int m_HEWDiameter{};

	CameraPreviewVariables::ImageDataTypes m_ImageDataType{};

	/* Annulus */
	bool m_DisplayAnnulus{};
	std::vector<CameraPreviewVariables::Annulus> m_AnnulusVec{};
	int m_ActivatedAnnulusNum{ -1 };

	/* Scale Bar */
	bool m_DisplayScaleBar{ true };

	std::unique_ptr<CameraPreviewVariables::InputPreviewPanelArgs> m_ParentArguments{};

	CameraPreviewVariables::ImageStats m_LastStats{};
	bool m_ShowImageStats{};

	// FPS state
	bool m_ShowFPS{ true };
	double m_FPSInstant{ 0.0 };
	double m_FPSEMA{ 0.0 };
	std::chrono::steady_clock::time_point m_LastFrameTS{};
	unsigned long long m_FrameCount{ 0 };

	DECLARE_EVENT_TABLE();
};

template<typename Func>
void cCamPreview::ForEachPixelInAnnulus
(
	const CameraPreviewVariables::Annulus& annulus, 
	Func&& callback
)
{
	if (!m_ImageData) return;

	const int r1Sq = annulus.m_InnerRadius * annulus.m_InnerRadius;
	const int r2Sq = annulus.m_OuterRadius * annulus.m_OuterRadius;

	const int xStart = std::max(0, annulus.m_Center.x - (int)annulus.m_OuterRadius);
	const int xEnd = std::min(m_ImageSize.GetWidth() - 1, annulus.m_Center.x + (int)annulus.m_OuterRadius);
	const int yStart = std::max(0, annulus.m_Center.y - (int)annulus.m_OuterRadius);
	const int yEnd = std::min(m_ImageSize.GetHeight() - 1, annulus.m_Center.y + (int)annulus.m_OuterRadius);

	for (int y = yStart; y <= yEnd; ++y)
	{
		const int dy = y - annulus.m_Center.y;
		for (int x = xStart; x <= xEnd; ++x)
		{
			const int dx = x - annulus.m_Center.x;
			const int distSq = dx * dx + dy * dy;

			if (distSq >= r1Sq && distSq < r2Sq)
			{
				callback(x, y, m_ImageData[y * m_ImageSize.GetWidth() + x]);
			}
		}
	}
}
#endif // !CCAM_PREVIEW_H
