#pragma once
#ifndef CMAIN_H
#define CMAIN_H

#include "wx/wx.h"
#include "wx/scrolwin.h"
#include "wx/imaglist.h"
#include "wx/filefn.h"
#include "wx/textfile.h"
#include "wx/tglbtn.h"
#include "wx/valnum.h"
#include "wx/thread.h"
#include "wx/appprogress.h"
#include "wx/propgrid/propgrid.h"
#include "wx/propgrid/advprops.h"
#include "wx/fs_zip.h"
#include "wx/html/helpctrl.h"
#include "wx/progdlg.h"
#include "wx/gauge.h"
#include "wx/listctrl.h"

#include "wxMaterialDesignArtProvider.hpp"

#include <string>
#include <memory>
#include <chrono>
#include <regex>
#include <algorithm>

#include "json.hpp"

#include "cStylishComboBox.h"
#include "cHistogramPanel.h"
#include "cCamPreview.h"
#include "cSettings.h"
#include "cGenerateReportDialog.h"

#include "XimeaControl.h"
#include "MoravianInstrumentsControl.h"

#include "src/img/logo.xpm"
#include <wx/msw/window.h>

#define MAJOR_VERSION 1
#define MINOR_VERSION 20

namespace MainFrameVariables
{
	enum
	{
		/* Menu Bar */
		ID_MENUBAR_FILE_QUIT,
		ID_MENUBAR_EDIT_ENABLE_DARK_MODE,
		ID_MENUBAR_EDIT_SETTINGS,
		ID_MENUBAR_TOOLS_ENABLE_FWHM_DISPLAYING,
		ID_MENUBAR_TOOLS_ENABLE_FOCUS_CENTER_DISPLAYING,
		ID_MENUBAR_TOOLS_ENABLE_ANNULUS_DISPLAYING,
		ID_MENUBAR_TOOLS_ENABLE_GRID_MESH_DISPLAYING,
		ID_MENUBAR_TOOLS_ENABLE_CIRCLE_MESH_DISPLAYING,
		ID_MENUBAR_TOOLS_CROSSHAIR,
		ID_MENUBAR_TOOLS_VALUE_DISPLAYING,
		ID_MENUBAR_WINDOW_FULLSCREEN,
		ID_MENUBAR_HELP_ABOUT,
		/* Detector X */
		ID_RIGHT_SC_DET_X_ABS_TE_CTL,
		ID_RIGHT_SC_DET_X_SET_BTN,
		ID_RIGHT_SC_DET_X_REL_TE_CTL,
		ID_RIGHT_SC_DET_X_DEC_BTN,
		ID_RIGHT_SC_DET_X_INC_BTN,
		ID_RIGHT_SC_DET_X_CENTER_BTN,
		ID_RIGHT_SC_DET_X_HOME_BTN,
		/* Detector Y */
		ID_RIGHT_SC_DET_Y_ABS_TE_CTL,
		ID_RIGHT_SC_DET_Y_SET_BTN,
		ID_RIGHT_SC_DET_Y_REL_TE_CTL,
		ID_RIGHT_SC_DET_Y_DEC_BTN,
		ID_RIGHT_SC_DET_Y_INC_BTN,
		ID_RIGHT_SC_DET_Y_CENTER_BTN,
		ID_RIGHT_SC_DET_Y_HOME_BTN,
		/* Detector Z */
		ID_RIGHT_SC_DET_Z_ABS_TE_CTL,
		ID_RIGHT_SC_DET_Z_SET_BTN,
		ID_RIGHT_SC_DET_Z_REL_TE_CTL,
		ID_RIGHT_SC_DET_Z_DEC_BTN,
		ID_RIGHT_SC_DET_Z_INC_BTN,
		ID_RIGHT_SC_DET_Z_CENTER_BTN,
		ID_RIGHT_SC_DET_Z_HOME_BTN,
		/* Optics X */
		ID_RIGHT_SC_OPT_X_ABS_TE_CTL,
		ID_RIGHT_SC_OPT_X_SET_BTN,
		ID_RIGHT_SC_OPT_X_REL_TE_CTL,
		ID_RIGHT_SC_OPT_X_DEC_BTN,
		ID_RIGHT_SC_OPT_X_INC_BTN,
		ID_RIGHT_SC_OPT_X_CENTER_BTN,
		ID_RIGHT_SC_OPT_X_HOME_BTN,	
		/* Optics Y */
		ID_RIGHT_SC_OPT_Y_ABS_TE_CTL,
		ID_RIGHT_SC_OPT_Y_SET_BTN,
		ID_RIGHT_SC_OPT_Y_REL_TE_CTL,
		ID_RIGHT_SC_OPT_Y_DEC_BTN,
		ID_RIGHT_SC_OPT_Y_INC_BTN,
		ID_RIGHT_SC_OPT_Y_CENTER_BTN,
		ID_RIGHT_SC_OPT_Y_HOME_BTN,
		/* Optics Z */
		ID_RIGHT_SC_OPT_Z_ABS_TE_CTL,
		ID_RIGHT_SC_OPT_Z_SET_BTN,
		ID_RIGHT_SC_OPT_Z_REL_TE_CTL,
		ID_RIGHT_SC_OPT_Z_DEC_BTN,
		ID_RIGHT_SC_OPT_Z_INC_BTN,
		ID_RIGHT_SC_OPT_Z_CENTER_BTN,
		ID_RIGHT_SC_OPT_Z_HOME_BTN,
		/* Tools */
		ID_RIGHT_TOOLS_GRID_MESH_STEP_TXT_CTRL,
		ID_RIGHT_TOOLS_CIRCLE_MESH_STEP_TXT_CTRL,
		ID_RIGHT_TOOLS_ANNULUS_CENTER_X_TXT_CTRL,
		ID_RIGHT_TOOLS_ANNULUS_CENTER_Y_TXT_CTRL,
		ID_RIGHT_TOOLS_ANNULUS_R1_TXT_CTRL,
		ID_RIGHT_TOOLS_ANNULUS_R2_TXT_CTRL,
		ID_RIGHT_TOOLS_ANNULUS_LIST_CTRL,
		/* Camera */
		ID_RIGHT_CAM_TEMPERATURE_TXT_CTL,
		ID_RIGHT_CAM_EXPOSURE_TXT_CTL,
		ID_RIGHT_CAM_BINNING_CHOICE,
		ID_RIGHT_CAM_COLORMAP_COMBOBOX,
		ID_RIGHT_CAM_MANUFACTURER_CHOICE,
		ID_RIGHT_CAM_SINGLE_SHOT_BTN,
		ID_RIGHT_CAM_START_STOP_LIVE_CAPTURING_TGL_BTN,
		ID_RIGHT_CAM_CROSS_HAIR_POS_X_TXT_CTRL,
		ID_RIGHT_CAM_CROSS_HAIR_POS_Y_TXT_CTRL,
		ID_RIGHT_CAM_CROSS_HAIR_SET_POS_TGL_BTN,
		ID_RIGHT_CAM_ACTUAL_PARAMETERS_PROPERTY_GRID,
		/* Histogram */
		ID_HISTOGRAM_LEFT_BORDER_TXT_CTRL,
		ID_HISTOGRAM_RIGHT_BORDER_TXT_CTRL,
		/* Measurement */
		ID_RIGHT_MT_OUT_FLD_TEXT_CTRL,
		ID_RIGHT_MT_OUT_FLD_BTN,
		ID_RIGHT_MT_FIRST_STAGE_CHOICE,
		ID_RIGHT_MT_FIRST_STAGE_START,
		ID_RIGHT_MT_FIRST_STAGE_STEP,
		ID_RIGHT_MT_FIRST_STAGE_FINISH,
		ID_RIGHT_MT_SECOND_STAGE_CHOICE,
		ID_RIGHT_MT_SECOND_STAGE_START,
		ID_RIGHT_MT_SECOND_STAGE_STEP,
		ID_RIGHT_MT_SECOND_STAGE_FINISH,
		ID_RIGHT_MY_GENERATE_REPORT_BTN,
		ID_RIGHT_MT_START_STOP_MEASUREMENT,
		/* Live Capturing */
		ID_THREAD_LIVE_CAPTURING,
		/* Progress */
		ID_THREAD_PROGRESS_CAPTURING,
	};

	enum BinningModes
	{
		BINNING_SUM,
		BINNING_AVERAGE
	};
	
	struct InitializationFileStructure 
	{
		bool dark_mode_on{};
		bool display_histogram{ true };
		bool disable_report_generator{};

		double crop_size_mm = 0.5;
		double crop_size_circle_mm = 1.5;
		double default_sensor_temperature_degC = 25.0;
		double default_cooled_sensor_temperature_degC = 15.0;

		double default_motors_step_first_tab = 1.0;
		double default_motors_step_second_tab = 0.1;

		int circle_mesh_step_px = 100;
		int grid_mesh_step_px = 150;
		int default_colormap = 0;
		int default_binning = 1;
		int default_exposure_ms = 100;
		
		std::string default_motors_name_first_tab{ "Detector" };
		std::string default_motors_name_second_tab{ "Optics" };

		std::string upload_report_folder{};
		std::string work_station{};
		std::vector<std::string> xrayImagesCaptions{};

		// Serialize/Deserialize using NLOHMANN_DEFINE_TYPE_INTRUSIVE
		NLOHMANN_DEFINE_TYPE_INTRUSIVE
		(
			InitializationFileStructure, 
			dark_mode_on,
			display_histogram,
			disable_report_generator,

			crop_size_mm, 
			crop_size_circle_mm, 
			default_sensor_temperature_degC,
			default_cooled_sensor_temperature_degC,

			default_motors_step_first_tab,
			default_motors_step_second_tab,

			circle_mesh_step_px,
			grid_mesh_step_px,
			default_colormap,
			default_binning,
			default_exposure_ms,

			default_motors_name_first_tab,
			default_motors_name_second_tab,

			upload_report_folder,
			work_station, 
			xrayImagesCaptions
		)
	};

	struct MenuBar
	{
		wxMenuBar* menu_bar{};
		wxMenu* menu_file{};
		wxMenu* menu_edit{};
		wxMenu* menu_tools{};
		wxMenu* submenu_intensity_profile{};
		wxMenu* menu_window{};
		wxMenu* menu_help{};
		MenuBar() :
			menu_bar(new wxMenuBar()),
			menu_file(new wxMenu()),
			menu_edit(new wxMenu()),
			menu_tools(new wxMenu()),
			submenu_intensity_profile(new wxMenu()),
			menu_window(new wxMenu()),
			menu_help(new wxMenu()) {};
		~MenuBar()
		{
			menu_file->~wxMenu();
			menu_edit->~wxMenu();
			submenu_intensity_profile->~wxMenu();
			menu_tools->~wxMenu();
			menu_window->~wxMenu();
			menu_help->~wxMenu();
			menu_bar->~wxMenuBar();
		}
	};
	
	struct ToolBar
	{
		wxToolBar* tool_bar{};
		ToolBar() {};
	};

	struct ImageColormapComboBox
	{
		std::unique_ptr<cStylishComboBox> stylish_combo_box{};
		wxArrayString colormap_names{};
		ImageColormapComboBox()
		{
			colormap_names.Add("Grayscale");
			colormap_names.Add("Invert");
			colormap_names.Add("Jet");
			colormap_names.Add("16 Colors");
			colormap_names.Add("Cool");
			colormap_names.Add("Hot");
			colormap_names.Add("Winter");
			colormap_names.Add("Copper");
		};
	};

	struct CameraTabControls
	{
		std::unique_ptr<wxTextCtrl> camSensorTemperature{}, camExposure{};
		std::unique_ptr<wxChoice> camBinning{};
		std::unique_ptr<wxButton> singleShotBtn{};
		std::unique_ptr<wxToggleButton> startStopLiveCapturingTglBtn{};
		std::unique_ptr<wxTextCtrl> crossHairPosXTxtCtrl{}, crossHairPosYTxtCtrl{};

		auto EnableAllControls() -> void
		{
			camSensorTemperature->Enable();
			camExposure->Enable();

			camBinning->Enable();

			singleShotBtn->Enable();

			startStopLiveCapturingTglBtn->Enable();

			crossHairPosXTxtCtrl->Enable();
			crossHairPosYTxtCtrl->Enable();
		};

		auto DisableAllControls(const bool liveCapturing = false) -> void
		{
			camSensorTemperature->Disable();
			camExposure->Disable();

			camBinning->Disable();

			singleShotBtn->Disable();
			
			if (!liveCapturing)
				startStopLiveCapturingTglBtn->Disable();
			
			crossHairPosXTxtCtrl->Disable();
			crossHairPosYTxtCtrl->Disable();
		}

	};

	struct ToolsTabControls
	{
		std::unique_ptr<wxTextCtrl> gridMeshStepTxtCtrl{}, circleMeshStepTxtCtrl{};
		std::unique_ptr<wxTextCtrl> annulusCenterXTxtCtrl{}, annulusCenterYTxtCtrl{}, annulusR1TxtCtrl{}, annulusR2TxtCtrl{};
		std::unique_ptr<wxListCtrl> annulusListCtrl{};
	};

	struct StepperControl
	{
		wxTextCtrl* absolute_text_ctrl{}, *relative_text_ctrl{};
		wxButton* set_btn{}, * increment_btn{}, * decrement_btn{};
		wxBitmapButton* center_btn{}, * home_btn{};
		void DisableAllControls()
		{
			absolute_text_ctrl->Disable();
			relative_text_ctrl->Disable();
			set_btn->Disable();
			increment_btn->Disable();
			decrement_btn->Disable();
			center_btn->Disable();
			home_btn->Disable();
		}
		void EnableAllControls()
		{
			absolute_text_ctrl->Enable();
			relative_text_ctrl->Enable();
			set_btn->Enable();
			increment_btn->Enable();
			decrement_btn->Enable();
			center_btn->Enable();
			home_btn->Enable();
		}
	};

	struct MeasurementStage
	{
		wxChoice* stage{};
		wxTextCtrl* start{}, * step{}, * finish{};
		wxArrayString motors;

		MeasurementStage()
		{
			motors.Add("None");
			motors.Add("Detector X");
			motors.Add("Detector Y");
			motors.Add("Detector Z");
			motors.Add("Optics X");
			motors.Add("Optics Y");
			motors.Add("Optics Z");
		};

		void DisableAllControls()
		{
			stage->Disable();
			start->Disable();
			step->Disable();
			finish->Disable();
		}
		void EnableAllControls()
		{
			stage->Enable();
			start->Enable();
			step->Enable();
			finish->Enable();
		}
	};
	/* Struct to transfer axis data to WorkerThread */
	struct AxisMeasurement
	{
		int axis_number{ -1 };
		float start{}, step{}, finish{};
		int step_number{};
	};

	struct ImagesFilePaths
	{
		wxString opticsScheme{}, logoPath{}, fwhmPath{}, gainPath{};
		wxArrayString optimalPositionArray{};
		wxString best2DImage{}, best3DImage{}, bestHorizontalProfile{}, bestVerticalProfile{}, bestGreyscaleImage{};
		wxArrayString circleArray{};
		wxArrayString xRayArray{};
	};

	struct PropertiesNames
	{
		wxString id{ "ID" };
		wxString temperature{ "Sensor Temperature [degC]" };
		wxString depth{ "Depth [bit]" };
		wxString sensor_width_px{ "Sensor Width [px]" };
		wxString sensor_height_px{ "Sensor Height [px]" };
		wxString sensor_width_um{ "Sensor Width [um]" };
		wxString sensor_height_um{ "Sensor Height [um]" };
	};

	static auto CreateStringWithPrecision(double value, int decimalPlaces = 0) -> wxString
	{
		// Create a wxString with the specified number of decimal places
		wxString formattedString{}
		;
		if (decimalPlaces)
			formattedString = wxString::Format(wxT("%.") + std::to_string(decimalPlaces) + wxT("f"), value);
		else
			formattedString = wxString::Format(wxT("%i"), (int)value);

		return formattedString;
	};

	static auto BinImageData
	(
		unsigned short* inDataPtr, 
		unsigned short* outDataPtr, 
		const unsigned short binning, 
		const int originalImgWidth,
		const wxSize outImgSize
	) -> void
	{
		if (!inDataPtr || !outDataPtr) return;

		if (binning == 1)
		{
			memcpy
			(
				outDataPtr, 
				inDataPtr, 
				sizeof(unsigned short) * outImgSize.GetWidth() * outImgSize.GetHeight()
			);
		}

		auto binningMode = MainFrameVariables::BinningModes::BINNING_AVERAGE;

		auto calculatePixelsInsideTheBinning = [&]
		(
			unsigned short* dataStart,
			int currStartX = 0, // Remove after DUBUG
			int currStartY = 0 // Remove after DUBUG
			)
			{
				unsigned long long sum{};
				unsigned long long position{};
				for (auto y{ 0 }; y < binning; ++y)
				{
					for (auto x{ 0 }; x < binning; ++x)
					{
						position = y * originalImgWidth + x;
						sum += static_cast<unsigned long long>(dataStart[position]);
					}
				}
				auto value = binningMode == MainFrameVariables::BINNING_SUM
					? sum
					: static_cast<unsigned long long>(sum / pow(binning, 2));
				auto valueInBounds = static_cast<unsigned short>(
					std::max((unsigned long long)0,
						std::min((unsigned long long)std::numeric_limits<unsigned short>::max(), value)
					));

				return valueInBounds;
			};

		auto calculateBinningPixelsOnTile = [&]
		(
			unsigned short* origDataStart,
			unsigned short* outDataStart,
			wxPoint start,
			wxPoint finish
			)
			{
				unsigned long long inPosition{};
				unsigned long long outPosition{};

				const auto width = finish.x - start.x;
				const auto height = finish.y - start.y;
				for (auto y{ 0 }; y < height; ++y)
				{
					for (auto x{ start.x }; x < finish.x; ++x)
					{
						inPosition = (y * originalImgWidth + x) * binning;
						outPosition = y * width + x;
						outDataStart[outPosition] = calculatePixelsInsideTheBinning(&origDataStart[inPosition], x, y);
					}
				}
			};

		auto applySoftwareBinningMultithread = [&]()
			{
				// Check number of threads on the current machine
				auto numThreads = std::thread::hardware_concurrency();
	#ifdef _DEBUG
				numThreads = 1;
	#endif // _DEBUG

				std::vector<std::thread> threads;
				threads.reserve(numThreads);

				unsigned int tileSize{};
				unsigned int outImgWidth = outImgSize.GetWidth();
				unsigned int outImgHeight = outImgSize.GetHeight();

				tileSize = outImgHeight % numThreads > 0 ? outImgHeight / numThreads + 1 : outImgHeight / numThreads;
				tileSize = tileSize > 0 ? tileSize : 1;

				for (auto i{ 0 }; i < (int)numThreads; ++i)
				{
					wxPoint start{}, finish{};
					start.x = 0;
					finish.x = outImgWidth;

					start.y = i * tileSize;
					finish.y = (i + 1) * tileSize > outImgHeight ? outImgHeight : (i + 1) * tileSize;

					unsigned long long position = (start.y * originalImgWidth + start.x) * binning;
					threads.emplace_back
					(
						std::thread
						(
							calculateBinningPixelsOnTile,
							&inDataPtr[position],
							&outDataPtr[start.y * outImgWidth + start.x],
							start, finish
						)
					);
				}
				for (auto& thread : threads)
				{
					thread.join();
				}

				// Here we can deallocate input data pointer because it's not necessary to hold this huge amount of memory in buffer
				inDataPtr = nullptr;
			};

		applySoftwareBinningMultithread();
	}

}

class ProgressBar;
class ProgressPanel;
class WorkerThread;
class ProgressThread;

#define USE_MULTITHREAD

/* ___ Start cMain ___ */
class cMain final : public wxFrame
{
public:
	cMain(const wxString& title_);
	//auto StopLiveCapturing() -> bool;
	auto LiveCapturingFinishedCapturingAndDrawing(bool is_finished) -> void;
	auto WorkerThreadFinished(bool is_finished) -> void;
	auto UpdateStagePositions() -> void;

	auto GoStageToAbsPos(SettingsVariables::MotorsNames motorName, float position) -> float;

private:
	void CreateMainFrame();
	void InitComponents();
	auto InitializeAboutHTML() -> void;
	void InitDefaultStateWidgets();
	void CreateMenuBarOnFrame();
	void CreateVerticalToolBar();
	void CreateLeftAndRightSide();
	void CreateLeftSide(wxSizer* left_side_sizer);
	void CreateRightSide(wxSizer* right_side_sizer);

	auto CreateBottomPanel(wxSizer* sizer, const int borderSize) -> void;

	auto CreateStatusBarOnFrame() -> void;

	auto CreateDetectorPage
	(
		wxWindow* parent, 
		const wxSize& absoluteTxtCtrlSize, 
		const wxSize& relativeTxtCtrlSize,
		const wxSize& setBtnSize,
		const wxSize& incrementDecrementBtnSize,
		const wxBitmap& centerBitmap,
		const wxBitmap& homeBitmap
	) -> wxWindow*;

	auto CreateOpticsPage
	(
		wxWindow* parent, 
		const wxSize& absoluteTxtCtrlSize, 
		const wxSize& relativeTxtCtrlSize,
		const wxSize& setBtnSize,
		const wxSize& incrementDecrementBtnSize,
		const wxBitmap& centerBitmap,
		const wxBitmap& homeBitmap
	) -> wxWindow*;

	auto CreateCameraPage
	(
		wxWindow* parent
	) -> wxWindow*;

	auto CreateCameraParametersPage
	(
		wxWindow* parent
	) -> wxWindow*;

	auto CreateGridMeshPage
	(
		wxWindow* parent
	) -> wxWindow*;

	auto CreateCircleMeshPage
	(
		wxWindow* parent
	) -> wxWindow*;

	auto CreateAnnulusPage
	(
		wxWindow* parent
	) -> wxWindow*;

	auto CreateMeasurementPage
	(
		wxWindow* parent
	) -> wxWindow*;


	void CreateSteppersControl(wxPanel* right_side_panel, wxBoxSizer* right_side_panel_sizer);
	void CreateCameraControls(wxPanel* right_side_panel, wxBoxSizer* right_side_panel_sizer);
	void CreateTools(wxPanel* right_side_panel, wxBoxSizer* right_side_panel_sizer);
	void CreateMeasurement(wxPanel* right_side_panel, wxBoxSizer* right_side_panel_sizer);

	auto OnEnableDarkMode(wxCommandEvent& evt) -> void;
	auto OnAbout(wxCommandEvent& evt) -> void;

	auto OnFWHMButton(wxCommandEvent& evt) -> void;
	auto OnGridMeshButton(wxCommandEvent& evt) -> void;
	auto OnGridMeshTxtCtrl(wxCommandEvent& evt) -> void;
	
	auto OnCircleMeshButton(wxCommandEvent& evt) -> void;
	auto OnCircleMeshTxtCtrl(wxCommandEvent& evt) -> void;
	
	auto OnFocusCenterButton(wxCommandEvent& evt) -> void;
	auto OnCrossHairButton(wxCommandEvent& evt) -> void;

	/* Annulus */
	auto OnAnnulusButton(wxCommandEvent& evt) -> void;
	auto OnColBeginDrag(wxListEvent& evt) -> void;

	void UnCheckAllTools();
	/* ProgressBar */
	void CreateProgressBar();

	/* Live Capturing */
	void StartLiveCapturing();

	void ChangeCameraManufacturerChoice(wxCommandEvent& evt);
	void OnSingleShotCameraImage(wxCommandEvent& evt);

	auto DisplayAndSaveImageFromTheCamera
	(
		unsigned short* const imgPtr, 
		const wxSize& originalImgSize, 
		const int& binning,
		const CameraControlVariables::ImageDataTypes dataType,
		const std::string outFilePath = ""
	) -> void;

	void OnSetOutDirectoryBtn(wxCommandEvent& evt);

	auto ReadInitializationFile() -> void;
	auto RewriteInitializationFile() -> void;

	void OnOpenSettings(wxCommandEvent& evt);
	auto InitializeSelectedCamera() -> void;
	auto UpdateDefaultWidgetParameters() -> void;
	auto UpdateCameraParameters() -> void;
	auto CoolDownTheCamera() -> void;
	void EnableUsedAndDisableNonUsedMotors();

	void OnValueDisplayingCheck(wxCommandEvent& evt);

	void OnFullScreen(wxCommandEvent& evt);
	void OnMaximizeButton(wxMaximizeEvent& evt);

	void OnExit(wxCloseEvent& evt);
	void OnExit(wxCommandEvent& evt);

	/* Stepper Control Functions */
	void OnEnterTextCtrlAbsPos(wxCommandEvent& evt, int index, int buttonID)
	{
		wxCommandEvent enter_evt(wxEVT_BUTTON, buttonID);
		ProcessEvent(enter_evt);
	}

	void OnSetAbsPos(wxCommandEvent& evt, int index, SettingsVariables::MotorsNames motorType)
	{
		wxBusyCursor cursor;
		double absolute_position{};
		auto& controlArray = (motorType <= SettingsVariables::DETECTOR_Z) ? m_Detector : m_Optics;

		if (!controlArray[index % 3].absolute_text_ctrl->GetValue().ToDouble(&absolute_position)) return;

		auto currentPosition = m_Settings->GoToAbsPos(motorType, (float)absolute_position);

		controlArray[index % 3].absolute_text_ctrl->SetValue(MainFrameVariables::CreateStringWithPrecision(currentPosition, m_DecimalDigits));
	}

	void OnOffsetAbsPos(wxCommandEvent& evt, int index, SettingsVariables::MotorsNames motorType, bool isIncrement)
	{
		wxBusyCursor cursor;
		double delta_position{};
		auto& controlArray = (motorType <= SettingsVariables::DETECTOR_Z) ? m_Detector : m_Optics;

		if (!controlArray[index % 3].relative_text_ctrl->GetValue().ToDouble(&delta_position)) return;

		auto currentPosition = m_Settings->GoOffsetMotor(motorType, (float)(isIncrement ? delta_position : -delta_position));

		controlArray[index % 3].absolute_text_ctrl->SetValue(MainFrameVariables::CreateStringWithPrecision(currentPosition, m_DecimalDigits));
	}

	void OnCenterMotor(wxCommandEvent& evt, int index, SettingsVariables::MotorsNames motorType)
	{
		wxBusyCursor cursor;
		auto currentPosition = m_Settings->CenterMotor(motorType);
		auto& controlArray = (motorType <= SettingsVariables::DETECTOR_Z) ? m_Detector : m_Optics;

		controlArray[index % 3].absolute_text_ctrl->SetValue(MainFrameVariables::CreateStringWithPrecision(currentPosition, m_DecimalDigits));
	}

	void OnHomeMotor(wxCommandEvent& evt, int index, SettingsVariables::MotorsNames motorType)
	{
		wxBusyCursor cursor;
		auto currentPosition = m_Settings->HomeMotor(motorType);
		auto& controlArray = (motorType <= SettingsVariables::DETECTOR_Z) ? m_Detector : m_Optics;

		controlArray[index % 3].absolute_text_ctrl->ChangeValue(MainFrameVariables::CreateStringWithPrecision(currentPosition, m_DecimalDigits));
	}

	/* _____________________Detector X_____________________ */
	void OnEnterTextCtrlDetectorXAbsPos(wxCommandEvent& evt) { OnEnterTextCtrlAbsPos(evt, 0, MainFrameVariables::ID_RIGHT_SC_DET_X_SET_BTN); }
	void OnSetDetectorXAbsPos(wxCommandEvent& evt) { OnSetAbsPos(evt, 0, SettingsVariables::DETECTOR_X); }
	void OnDecrementDetectorXAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 0, SettingsVariables::DETECTOR_X, false); }
	void OnIncrementDetectorXAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 0, SettingsVariables::DETECTOR_X, true); }
	void OnCenterDetectorX(wxCommandEvent& evt) { OnCenterMotor(evt, 0, SettingsVariables::DETECTOR_X); }
	void OnHomeDetectorX(wxCommandEvent& evt) { OnHomeMotor(evt, 0, SettingsVariables::DETECTOR_X); }

	/* _____________________Detector Y_____________________ */
	void OnEnterTextCtrlDetectorYAbsPos(wxCommandEvent& evt) { OnEnterTextCtrlAbsPos(evt, 1, MainFrameVariables::ID_RIGHT_SC_DET_Y_SET_BTN); }
	void OnSetDetectorYAbsPos(wxCommandEvent& evt) { OnSetAbsPos(evt, 1, SettingsVariables::DETECTOR_Y); }
	void OnDecrementDetectorYAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 1, SettingsVariables::DETECTOR_Y, false); }
	void OnIncrementDetectorYAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 1, SettingsVariables::DETECTOR_Y, true); }
	void OnCenterDetectorY(wxCommandEvent& evt) { OnCenterMotor(evt, 1, SettingsVariables::DETECTOR_Y); }
	void OnHomeDetectorY(wxCommandEvent& evt) { OnHomeMotor(evt, 1, SettingsVariables::DETECTOR_Y); }

	/* _____________________Detector Z_____________________ */
	void OnEnterTextCtrlDetectorZAbsPos(wxCommandEvent& evt) { OnEnterTextCtrlAbsPos(evt, 2, MainFrameVariables::ID_RIGHT_SC_DET_Z_SET_BTN); }
	void OnSetDetectorZAbsPos(wxCommandEvent& evt) { OnSetAbsPos(evt, 2, SettingsVariables::DETECTOR_Z); }
	void OnDecrementDetectorZAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 2, SettingsVariables::DETECTOR_Z, false); }
	void OnIncrementDetectorZAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 2, SettingsVariables::DETECTOR_Z, true); }
	void OnCenterDetectorZ(wxCommandEvent& evt) { OnCenterMotor(evt, 2, SettingsVariables::DETECTOR_Z); }
	void OnHomeDetectorZ(wxCommandEvent& evt) { OnHomeMotor(evt, 2, SettingsVariables::DETECTOR_Z); }

	/* _____________________Optics X_____________________ */
	void OnEnterTextCtrlOpticsXAbsPos(wxCommandEvent& evt) { OnEnterTextCtrlAbsPos(evt, 3, MainFrameVariables::ID_RIGHT_SC_OPT_X_SET_BTN); }
	void OnSetOpticsXAbsPos(wxCommandEvent& evt) { OnSetAbsPos(evt, 3, SettingsVariables::OPTICS_X); }
	void OnDecrementOpticsXAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 3, SettingsVariables::OPTICS_X, false); }
	void OnIncrementOpticsXAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 3, SettingsVariables::OPTICS_X, true); }
	void OnCenterOpticsX(wxCommandEvent& evt) { OnCenterMotor(evt, 3, SettingsVariables::OPTICS_X); }
	void OnHomeOpticsX(wxCommandEvent& evt) { OnHomeMotor(evt, 3, SettingsVariables::OPTICS_X); }

	/* _____________________Optics Y_____________________ */
	void OnEnterTextCtrlOpticsYAbsPos(wxCommandEvent& evt) { OnEnterTextCtrlAbsPos(evt, 4, MainFrameVariables::ID_RIGHT_SC_OPT_Y_SET_BTN); }
	void OnSetOpticsYAbsPos(wxCommandEvent& evt) { OnSetAbsPos(evt, 4, SettingsVariables::OPTICS_Y); }
	void OnDecrementOpticsYAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 4, SettingsVariables::OPTICS_Y, false); }
	void OnIncrementOpticsYAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 4, SettingsVariables::OPTICS_Y, true); }
	void OnCenterOpticsY(wxCommandEvent& evt) { OnCenterMotor(evt, 4, SettingsVariables::OPTICS_Y); }
	void OnHomeOpticsY(wxCommandEvent& evt) { OnHomeMotor(evt, 4, SettingsVariables::OPTICS_Y); }

	/* _____________________Optics Z_____________________ */
	void OnEnterTextCtrlOpticsZAbsPos(wxCommandEvent& evt) { OnEnterTextCtrlAbsPos(evt, 5, MainFrameVariables::ID_RIGHT_SC_OPT_Z_SET_BTN); }
	void OnSetOpticsZAbsPos(wxCommandEvent& evt) { OnSetAbsPos(evt, 5, SettingsVariables::OPTICS_Z); }
	void OnDecrementOpticsZAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 5, SettingsVariables::OPTICS_Z, false); }
	void OnIncrementOpticsZAbsPos(wxCommandEvent& evt) { OnOffsetAbsPos(evt, 5, SettingsVariables::OPTICS_Z, true); }
	void OnCenterOpticsZ(wxCommandEvent& evt) { OnCenterMotor(evt, 5, SettingsVariables::OPTICS_Z); }
	void OnHomeOpticsZ(wxCommandEvent& evt) { OnHomeMotor(evt, 5, SettingsVariables::OPTICS_Z); }

	/* First Stage */
	void OnFirstStageChoice(wxCommandEvent& evt);
	
	/* Second Stage */
	void OnSecondStageChoice(wxCommandEvent& evt);
	
	/* Changed Exposure value */
	void ExposureValueChanged(wxCommandEvent& evt);
	auto OnSensorTemperatureChanged(wxCommandEvent& evt) -> void;
	auto OnBinningChoice(wxCommandEvent& evt) -> void;
	auto OnColormapComboBox(wxCommandEvent& evt) -> void;

	/* Generate Report */
	auto OnGenerateReportBtn(wxCommandEvent& evt) -> void;
	auto IsPythonInstalledOnTheCurrentMachine() -> bool;
	auto IsVirtualEnvironmentAlreadyCreated(wxString pathToVenv) -> bool;
	auto CreateVirtualEnvironment(wxString pathToVenv, wxString pathToRequirements) -> bool;

	/* Start Capturing */
	void OnStartStopCapturingTglButton(wxCommandEvent& evt);
	auto OnStartStopCapturingMenuButton(wxCommandEvent& evt) -> void;
	auto EnableControlsAfterCapturing() -> void;
	auto EnableControlsAfterSuccessfulCameraInitialization() -> void;
	auto DisableControlsAfterUnsuccessfulCameraInitialization() -> void;
	auto DisableControlsBeforeCapturing() -> void;
	void OnStartStopLiveCapturingMenu(wxCommandEvent& evt);
	void OnStartStopLiveCapturingTglBtn(wxCommandEvent& evt);

	/* Thread Live Capturing */
	auto LiveCapturingThread(wxThreadEvent& evt) -> void;
	/* Progress */
	void UpdateProgress(wxThreadEvent& evt);
	bool Cancelled();

	void UpdateAllAxisGlobalPositions();

	/* CrossHair */
	void OnXPosCrossHairTextCtrl(wxCommandEvent& evt);
	void OnYPosCrossHairTextCtrl(wxCommandEvent& evt);
	//auto OnSetPosCrossHairTglBtn(wxCommandEvent& evt) -> void;

	auto OnHistogramLeftBorderPosChanged(wxCommandEvent& evt) -> void;
	auto OnHistogramRightBorderPosChanged(wxCommandEvent& evt) -> void;
	auto UpdateUIHistogram() -> void;

	auto CalculateHistogram
	(
		unsigned short* data = nullptr,
		const int imgWidth = 0,
		const int imgHeight = 0,
		const int minimumCount = 1,
		unsigned long long* const histogramPtr = nullptr,
		unsigned short* const minValue = nullptr,
		unsigned short* const maxValue = nullptr
	) -> bool;

	auto CreateMetadataFile() -> void;

	auto FindSpotCenterCoordinates
	(
		const cv::Mat& signal, 
		int* bestX, 
		int* bestY
	) -> void;

	auto CropDataIntoArray
	(
		const cv::Mat& inData, 
		const int startX, 
		const int startY, 
		const int windowWidth,
		unsigned short* const outData
	) -> void;

	auto CropDataIntoArray
	(
		unsigned short* const inData, 
		const int origDataWidth,
		const int startX, 
		const int startY, 
		const int windowWidth,
		unsigned short* const outData
	) -> void;

	auto ApplyFFCOnData
	(
		unsigned short* const inRawData, 
		unsigned short* const inBlackData, 
		unsigned short* const inWhiteData, 
		const int imgWidth
	) -> void;

	auto WriteJSONDataToFile(const std::string& filePath, const std::string& data) -> void
	{
		std::ofstream outFile(filePath);
		if (!outFile) 
		{
			throw std::runtime_error("Failed to create temporary file for data.");
		}

		outFile << data;
		outFile.close();
	}

	auto WriteTempJSONImageDataToTXTFile
	(
		unsigned short* const dataPtr,
		int width,
		int height,
		wxString colormap,
		double pixelSizeUM,
		wxString filePath
	) -> void
	{
		using json = nlohmann::json;

		// Prepare JSON string with data
		json j;
		j["data"] = std::vector<unsigned short>(dataPtr, dataPtr + (width * height));
		j["width"] = width;
		j["height"] = height;
		j["colormap"] = colormap;
		j["pixelSizeUM"] = pixelSizeUM;
		j["filePath"] = std::string(filePath.utf8_string());

		std::string jsonString = j.dump();

		// Create wxFileName object
		wxFileName fileName(filePath);

		// Change the file extension
		fileName.SetExt("txt");

		WriteJSONDataToFile(fileName.GetFullPath().ToStdString(), jsonString);
	};

	auto WriteTempJSONDataToTXTFile
	(
		double* const firstData,
		std::string firstSmallLabel,
		std::string firstColor,
		std::string xLabel,
		double* const secondData,
		std::string secondSmallLabel,
		std::string secondColor,
		std::string yLabel,
		const int dataSize,
		double startX,
		double step,
		int bestInGain,
		wxString filePath
	) -> void
	{
		using json = nlohmann::json;

		// Prepare JSON string with data
		json j;

		j["firstData"] = std::vector<double>(firstData, firstData + dataSize);
		j["firstSmallLabel"] = firstSmallLabel;
		j["firstColor"] = firstColor;
		j["xLabel"] = xLabel;

		j["secondData"] = std::vector<double>(secondData, secondData + dataSize);
		j["secondSmallLabel"] = secondSmallLabel;
		j["secondColor"] = secondColor;
		j["yLabel"] = yLabel;

		j["dataSize"] = dataSize;
		j["startX"] = startX;
		j["step"] = step;
		j["bestInGain"] = bestInGain;
		j["filePath"] = std::string(filePath.utf8_string());

		std::string jsonString = j.dump();

		// Create wxFileName object
		wxFileName fileName(filePath);

		// Change the file extension
		fileName.SetExt("txt");

		WriteJSONDataToFile(fileName.GetFullPath().ToStdString(), jsonString);
	};

	auto WriteTempJSONLineProfileDataToTXTFile
	(
		unsigned short* const lineProfileData,
		double* const horizontalAxisData,
		int width,	
		std::string title,
		std::string xLabel,
		std::string yLabel,
		std::string lineColour,
		wxString filePath
	) -> void
	{
		using json = nlohmann::json;

		// Prepare JSON string with data
		json j;
		j["lineProfileData"] = std::vector<unsigned short>(lineProfileData, lineProfileData + width);
		j["horizontalAxisData"] = std::vector<double>(horizontalAxisData, horizontalAxisData + width);
		j["width"] = width;
		j["title"] = title;
		j["xLabel"] = xLabel;
		j["yLabel"] = yLabel;
		j["lineColour"] = lineColour;
		j["filePath"] = std::string(filePath.utf8_string());

		std::string jsonString = j.dump();

		// Create wxFileName object
		wxFileName fileName(filePath);

		// Change the file extension
		fileName.SetExt("txt");

		WriteJSONDataToFile(fileName.GetFullPath().ToStdString(), jsonString);
	};

	auto RaisePythonExceptionMSG(const std::string& command, int code) -> void
	{
		wxString title = "Python execution error";
		wxMessageBox(
			wxT
			(
				"Failed to run Python script.\n" 
				+ wxString(command)
				+ "\nError code: " + wxString::Format(wxT("%i"), code)
			),
			title,
			wxICON_ERROR);
	};

	std::string EscapeSpecialChars(const std::string& str) 
	{
		std::unordered_map<char, std::string> replacements = 
		{
			{'&', "^&"},
			{'%', "^%"},
			{'(', "^("},
			{')', "^)"}
		};

		std::string result;
		for (char ch : str) 
		{
			if (replacements.find(ch) != replacements.end()) 
			{
				result += replacements[ch];  // Replace special character
			}
			else 
			{
				result += ch;  // Keep normal character
			}
		}
		return result;
	};

	wxString CheckForDisallowedCharacters(const wxString& dirPath)
	{
		// Convert wxString to std::string for easier manipulation
		std::string path = dirPath.ToStdString();

		const std::vector<char> disallowedChars = { ' ', '&', '#', '%', '$', '^', '(', ')', '@', '!', '+', '=', ';', '<', '>', '?', '[', ']', '{', '}', '`' };

		wxString retChars{};
		// Loop through the list of disallowed characters
		for (char ch : disallowedChars)
		{
			if (path.find(ch) != std::string::npos)
				retChars += wxString(ch) + " ";
		}

		// If no disallowed characters are found
		return retChars;
	}

	auto Invoke2DPlotsCreation( wxArrayString filePaths) -> bool
	{
		std::string command = "cmd /c"; 

		command += " \"src\\ReportGenerator\\.venv\\Scripts\\activate && ";

		for (const auto& filePath : filePaths)
		{
			wxFileName fileName(filePath);
			fileName.SetExt("txt");
			//std::string fileNameString = "R(" + fileName.GetFullPath().ToStdString() + ")";
			command += "py.exe -X utf8 src\\ReportGenerator\\visualize2DPlot.py \"" 
				//+ EscapeSpecialChars(fileName.GetFullPath().ToStdString()) 
				+ fileName.GetFullPath().ToStdString()
				+ "\" && ";
		}
		command += "deactivate";

#ifdef _DEBUG
		//command += " && pause";
#endif // _DEBUG

		command += "\"";

		// Execute the Python script
		int result{};

		result = std::system(command.c_str());
		if (result != 0)
		{
			RaisePythonExceptionMSG(command, result);
			return false;
		}

		return true;
	}

	auto InvokePlotGraphCreation(wxString pythonFileName, wxString filePath) -> bool
	{
		std::string command = "cmd /c \"src\\ReportGenerator\\.venv\\Scripts\\activate && ";

		//command += "py.exe src\\ReportGenerator\\plotGraph.py \"" + filePath.ToStdString() + "\" && ";
		command += "py.exe src\\ReportGenerator\\" + pythonFileName + ".py \"" + filePath.ToStdString() + "\" && ";
		command += "deactivate\"";

		// Execute the Python script
		int result{};

		result = std::system(command.c_str());
		if (result != 0)
		{
			RaisePythonExceptionMSG(command, result);
			return false;
		}

		return true;
	}

	auto ExecuteLatex(wxString latexFilePath) -> bool
	{
		constexpr auto raise_exception_msg = [](int code) 
		{
			wxString title = "CMD execution error";
			wxMessageBox(
				wxT
				(
					"Failed to run CMD script. Error code: " + wxString::Format(wxT("%i"), code)
				),
				title,
				wxICON_ERROR);
		};


		wxFileName file(latexFilePath);
		std::string command = "cmd /c \"cd \"" + file.GetPath().ToStdString() + "\" && pdflatex \"";

		command += file.GetFullName();
		command += "\"\"";

		// Execute the CMD script
		int result{};

		result = std::system(command.c_str());
		if (result != 0)
		{
			raise_exception_msg(result);
			return false;
		}

		return true;
	}


	auto FindGainMaxInArrayData
	(
		unsigned short* const inRawData,
		unsigned short* const inWhiteData,
		const int imgWidth
	) -> double
	{
		auto maxInRawData = *std::max_element(inRawData, inRawData + imgWidth * imgWidth);
		auto sum = std::accumulate(inWhiteData, inWhiteData + imgWidth * imgWidth, 0);
		auto mean = (double)sum / imgWidth / imgWidth;

		return maxInRawData / mean;
	};

	auto FindGainFWHMInArrayData
	(
		unsigned short* const inRawData,
		unsigned short* const inWhiteData,
		const int imgWidth
	) -> double
	{
		auto dataSize = imgWidth * imgWidth;
		auto minMaxElement = std::minmax_element(inRawData, inRawData + dataSize);
		auto halfMaxValue = (*minMaxElement.second - *minMaxElement.first) / 2 + *minMaxElement.first;

		unsigned long long sumRAW{}, sumWhite{};

		for (auto i{ 0 }; i < dataSize; ++i)
		{
			auto value = inRawData[i];
			if (value > halfMaxValue)
			{
				sumRAW += value;
				sumWhite += inWhiteData[i];
			}
		}

		return (double)sumRAW / sumWhite;
	};

	auto RemoveBackgroundFromTheImage(wxString imagePath) -> void;

	auto Create2DImageInGrayscale(unsigned short* const inData, const int imgWidth) -> wxBitmap;

	auto GeneratePDFReportUsingLatex
	(
		wxString folderContainingTEXFile, 
		wxString folderWithData,
		wxString pdfFileName,
		const MainFrameVariables::ImagesFilePaths& imageFilePaths,
		const GenerateReportVariables::ReportParameters& reportParameters,
		const wxString timeStamp
	) -> wxString;

	void ReplacePlaceholderInTexFile
	(
		const wxString& texFilePath, 
		const wxString& placeholder, 
		const wxString& imagePath
	) 
	{
		wxTextFile file(texFilePath);

		if (!file.Open()) 
		{
			wxLogError("Failed to open the TEX file: %s", texFilePath);
			return;
		}

		// Store modified content
		wxArrayString lines;
		for (size_t i = 0; i < file.GetLineCount(); ++i) 
		{
			wxString line = file.GetLine(i);
			line.Replace(placeholder, imagePath);
			lines.Add(line);
		}

		// Rewrite the file
		file.Clear();  // Clears the file contents
		for (const auto& line : lines) {
			file.AddLine(line);
		}
		file.Write();
		file.Close();
	}

	void RemoveSectionFromFile
	(
		const wxString& filePath, 
		const wxString& startMarker, 
		const wxString& endMarker
	) 
	{
		wxTextFile file(filePath);

		if (!file.Open()) 
		{
			wxLogError("Failed to open file: %s", filePath);
			return;
		}

		bool insideSection = false;

		for (size_t i = 0; i < file.GetLineCount(); /* no increment here */) 
		{
			wxString line = file.GetLine(i);

			if (line.Contains(startMarker)) 
			{
				insideSection = true;
			}

			if (insideSection) 
			{
				file.RemoveLine(i);  // Remove the current line
			}
			else 
			{
				i++;  // Only increment if we don't remove a line
			}

			if (line.Contains(endMarker)) 
			{
				insideSection = false;
			}
		}

		file.Write();
		file.Close();
	}

	wxString ConvertToForwardSlashes(const wxString & path) 
	{
		wxString convertedPath = path;
		convertedPath.Replace("\\", "/");  // Replace all backslashes with forward slashes
		return convertedPath;
	}

	wxString FormatNumber(double number) 
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(5) << number;  // Convert with fixed precision
		std::string str = oss.str();

		// Remove trailing zeros and possible trailing decimal point
		str.erase(str.find_last_not_of('0') + 1, std::string::npos);
		if (str.back() == '.') {
			str.pop_back();
		}

		return wxString(str);
	}

	bool CheckLatexPresence() 
	{
		wxString latexCommand = "xelatex --version"; // You can also use "pdflatex --version"

		// Execute the command silently and capture the return code
		int exitCode = wxExecute(latexCommand, wxEXEC_NODISABLE | wxEXEC_SYNC);

		if (exitCode != 0) 
		{
			wxLogError("LaTeX is not installed on this system. Please install a LaTeX distribution (such as TeX Live or MiKTeX) and ensure it's added to the system PATH.");
			return false;
		}
		return true;
	}

	auto RemoveAllUnnecessaryFilesFromFolder(const wxString& folder, wxArrayString removeExtensions) -> void;

	wxString GenerateLatexSmallImageBlock(const wxString& imgTag, const wxString& positionTag, bool isBestPosition) 
	{
		wxString latex;

		latex << "\t\\begin{minipage}[b]{0.22\\textwidth}\n"
			<< "\t\t\\includegraphics[width=\\textwidth]{" << imgTag << "}\n"
			<< "\t\t\\centering\n"
			<< "\t\t\\text{Z " << positionTag << " [mm]}\n"
			<< "\t\\end{minipage} \\hfill";

		if (isBestPosition) 
		{
			latex << " \\\\[1ex]"; // Add line break if it's the best position
		}

		return latex;
	}

	wxString GenerateLatexBigImageBlock(const wxString& imgTag, const wxString& caption) 
	{
		wxString latex;

		latex 
			<< "\t\\begin{figure}[H]" << "\n"
			<< "\t\t\\centering" << "\n"
			<< "\t\t\\includegraphics[width=0.7\\textwidth]{" << imgTag << "}\n"
			<< "\t\t\\caption{" << caption << "}\n"
			<< "\t\\end{figure}" << "\n";

		return latex;
	}


	void InsertLatexAtMarker
	(
		const wxString& filePath, 
		const wxString& marker, 
		const wxArrayString& generatedBlocks
	) 
	{
		wxTextFile file(filePath);

		if (!file.Open()) 
		{
			wxLogError("Failed to open LaTeX file: %s", filePath);
			return;
		}

		bool markerFound = false;

		for (size_t i = 0; i < file.GetLineCount(); ++i) 
		{
			if (file[i].Contains(marker)) 
			{
				markerFound = true;

				// Insert generated LaTeX blocks after the marker
				for (auto k{0}; k < generatedBlocks.GetCount(); ++k)
					file.InsertLine(generatedBlocks[generatedBlocks.GetCount() - k - 1], i + 1);
				break;
			}
		}

		if (!markerFound) 
		{
			wxLogError("Marker not found: %s", marker);
		}
		else 
		{
			file.Write();
			//wxLogMessage("Successfully inserted LaTeX blocks into: %s", filePath);
		}

		file.Close();
	}

	bool EnsureFolderHierarchy(const wxString& basePath, wxString& targetFolder) 
	{
		wxDateTime now = wxDateTime::Now();
		wxString yearFolder = basePath + wxFileName::GetPathSeparator() + now.Format("%Y");
		wxString monthFolder = yearFolder + wxFileName::GetPathSeparator() + now.Format("%m");

		// Ensure Year Folder
		if (!wxDirExists(yearFolder)) 
		{
			if (!wxMkdir(yearFolder)) 
			{
				wxLogError("Failed to create year folder: %s", yearFolder);
				return false;
			}
		}

		// Ensure Month Folder
		if (!wxDirExists(monthFolder)) 
		{
			if (!wxMkdir(monthFolder)) 
			{
				wxLogError("Failed to create month folder: %s", monthFolder);
				return false;
			}
		}

		targetFolder = monthFolder; // Set the final target folder path
		return true;
	}

	auto UploadReportToDestinationFolder(const wxString& report, const wxString& destinationFolder) -> void
	{
		if (!wxFileExists(report)) 
		{
			wxLogError("Source file does not exist: %s", report);
			return;
		}

		wxFileName source(report);
		wxString destination = destinationFolder.EndsWith("\\") ? destinationFolder : destinationFolder + wxFileName::GetPathSeparator();
		destination += source.GetFullName();

		if (!wxCopyFile(report, destination, true)) { // true = overwrite if exists
			wxLogError("Failed to copy file to: %s", destination);
			return;
		}
		//wxLogMessage("File successfully copied to: %s", destination);
	}


private:
	/* Initialization file */
	const wxString m_InitializationFilePath = "MMCam.ini";
	std::unique_ptr<MainFrameVariables::InitializationFileStructure> m_Config{};

	/* Settings Menu */
	std::unique_ptr<cSettings> m_Settings{};

	/* Help */
	std::unique_ptr<wxHtmlHelpController> m_HelpController{};
	/* Menu Bar */
	std::unique_ptr<MainFrameVariables::MenuBar> m_MenuBar{};
	/* Tool Bar */
	std::unique_ptr<MainFrameVariables::ToolBar> m_VerticalToolBar{};
	/* Preview Panel */
	std::unique_ptr<cCamPreview> m_CamPreview{};
	/* Steppers Control */
	std::unique_ptr<MainFrameVariables::StepperControl[]> m_Detector = std::make_unique<MainFrameVariables::StepperControl[]>(3);
	std::unique_ptr<MainFrameVariables::StepperControl[]> m_Optics = std::make_unique<MainFrameVariables::StepperControl[]>(3);

	// Colormap ComboBox
	std::unique_ptr<MainFrameVariables::ImageColormapComboBox> m_ImageColormapComboBox{};

	/* Camera */
	std::unique_ptr<CameraControl> m_CameraControl{};
	std::unique_ptr<MainFrameVariables::CameraTabControls> m_CameraTabControls{};
	//std::unique_ptr<XimeaControl> m_XimeaControl{};

	wxSize m_OutputImageSize{};

	/* Measurement */

	std::unique_ptr<wxTextCtrl> m_OutDirTextCtrl{};
	std::unique_ptr<wxButton> m_OutDirBtn{};
	std::unique_ptr<MainFrameVariables::MeasurementStage> m_FirstStage{};
	std::unique_ptr<wxToggleButton> m_StartStopMeasurementTglBtn{};

	/* Report Generation */
	std::unique_ptr<wxButton> m_GenerateReportBtn{};

	/* Progress */
	bool m_Cancelled{}, m_DataCalculatedInThisApp{};
	wxCriticalSection m_CSCancelled{};
	//std::unique_ptr<ProgressBar> m_ProgressBar{};
	std::chrono::steady_clock::time_point m_StartCalculationTime;
	std::unique_ptr<wxAppProgressIndicator> m_AppProgressIndicator{};

	int m_Progress{};
	wxString m_ProgressMsg{};

	// Tools
	bool m_IsCrossHairChecked{}, m_IsFWHMChecked{}, m_IsGridMeshChecked{}, m_IsCircleMeshChecked{}, m_IsFocusCenterChecked{}, m_IsAnnulusChecked{};

	/* Value Displaying */
	bool m_IsValueDisplayingChecked{};

	/* Live Capturing */
	//bool m_StopLiveCapturing{};
	//bool m_LiveCapturingEndedDrawingOnCamPreview{ true };

	/* Appearance Colors */
	const wxColour m_DefaultAppearanceColor = wxColour(255, 255, 255);
	const wxColour m_BlackAppearanceColor = wxColour(30, 30, 30);

	std::unique_ptr<wxStatusBar> m_StatusBar{};
	std::unique_ptr<wxGauge> m_ProgressBar{};

	// Histogram Variables
	std::unique_ptr<cHistogramPanel> m_HistogramPanel{};
	std::unique_ptr<wxTextCtrl> m_LeftHistogramRange{}, m_RightHistogramRange{};

	/* wxPanels */
	wxScrolledWindow* m_RightSidePanel{};
	//wxPanel* m_RightSidePanel{};
	wxNotebook* m_DetectorControlsNotebook{}, * m_OpticsControlsNotebook{};
	
	wxNotebook* m_CameraControlNotebook{};
	
	wxNotebook* m_ToolsControlsNotebook{};
	std::unique_ptr<MainFrameVariables::ToolsTabControls> m_ToolsControls{};

	wxNotebook* m_MeasurementNotebook{};

	wxPropertyGrid* m_CurrentCameraSettingsPropertyGrid{};
	const wxColour m_DefaultCellColour = wxColour(180, 180, 180);
	std::unique_ptr<MainFrameVariables::PropertiesNames> m_PropertiesNames{};

	std::vector<std::pair<wxString, bool>> m_StartedThreads{};

	int m_DecimalDigits = 4;

	wxDECLARE_EVENT_TABLE();
};
/* ___ End cMain ___ */

/* ___ Start Live Capturing Theread ___ */
class LiveCapturing : public wxThreadHelper
{
public:
	LiveCapturing
	(
		cMain* main_frame,
		CameraControl* cameraControl,
		const int& exposure_us,
		const unsigned short& binning,
		wxString* uniqueThreadKey,
		bool* aliveOrDeadThread,
		bool* isDrawExecutionFinished
	);
	~LiveCapturing();

	virtual void* Entry();

protected:
	virtual auto CaptureImage
	(
		unsigned short* dataPtr
	) -> bool;

	//auto BinImageData(unsigned short* inDataPtr, unsigned short* outDataPtr) -> void;

protected:
	cMain* m_MainFrame{};
	CameraControl* m_CameraControl{};
	unsigned short m_Binning{ 1 };
	int m_ExposureUS{};
	wxSize m_ImageSize{};

	// Thread
	wxString* m_UniqueThreadKey{};
	bool* m_AliveOrDeadThread{};

	bool* m_IsDrawExecutionFinished{};
};
/* ___ End Worker Thread ___ */

/* ___ Start Worker Theread ___ */
class WorkerThread final: public LiveCapturing
{
public:
	WorkerThread
	(
		cMain* main_frame,
		CameraControl* cameraControl,
		const int& exposure_us,
		const unsigned short& binning,
		wxString* uniqueThreadKey,
		bool* aliveOrDeadThread,
		bool* isDrawExecutionFinished,
		const wxString& path, 
		MainFrameVariables::AxisMeasurement* first_axis, 
		MainFrameVariables::AxisMeasurement* second_axis,
		const double pixelSizeUM,
		const int decimalDigits
	);
	~WorkerThread();

	virtual void* Entry() override;

private:
	auto MoveFirstStage(const float position) -> float;

	auto SaveImage
	(
		unsigned short* dataPtr, 
		const int& imgWidth,
		const int& imgHeight,
		const std::string& fileName
	) -> bool;

	auto CalculateFWHM
	(
		unsigned short* dataPtr, 
		const int& imgWidth,
		const int& imgHeight,
		const int& stepNumber
	) -> bool;

	wxBitmap CreateGraph
	(
		const double* const horizontalFWHMData,
		const double* const verticalFWHMData,
		const float* const positionsData,
		unsigned int dataSize,
		int width,
		int height,
		const wxString& xAxisLabel,
		const wxString& leftYAxisLabel,
		const wxString& timestamp
	);

	auto SaveGraph
	(
		const wxBitmap& bitmap, 
		const wxString filePath
	) -> void;

	auto SaveFWHMTXTData
	(
		const wxString& filePath,
		const size_t dataSize,
		const double* horizontalFWHMData, 
		const double* verticalFWHMData, 
		const float* firstAxisData, 
		const float* secondAxisData = nullptr
	) -> void;


	auto PrepareFileName
	(
		const int imageNumber,
		const float firstStagePosition,
		const float secondStagePosition,
		const std::string hours,
		const std::string minutes,
		const std::string seconds
	) -> std::string
	{
		std::string fileName{};
		{
			std::string first_axis_position_str = std::format("{:.3f}", firstStagePosition);
			std::replace(first_axis_position_str.begin(), first_axis_position_str.end(), '.', '_');

			std::string second_axis_position_str = std::format("{:.3f}", secondStagePosition);
			std::replace(second_axis_position_str.begin(), second_axis_position_str.end(), '.', '_');

			fileName = std::string(m_ImagePath.mb_str()) + std::string("\\") +
				std::string("img_");
			fileName += imageNumber < 10 ? std::string("0") : std::string("");
			fileName += std::to_string(imageNumber) + std::string("_") +
				hours + std::string("H_") +
				minutes + std::string("M_") +
				seconds + std::string("S_") +
				std::to_string(m_ExposureUS) + std::string("us")
				+ std::string("_1A_") + first_axis_position_str;
			fileName += secondStagePosition != 0.f ? std::string("_2A_") + second_axis_position_str : "";

			fileName += std::string(".tif");
		}

		return fileName;
	};

	auto AxisNameToString(const int axis) -> std::string
	{
		switch (axis) {
		case SettingsVariables::DETECTOR_X:   return "DETECTOR X";
		case SettingsVariables::DETECTOR_Y:   return "DETECTOR Y";
		case SettingsVariables::DETECTOR_Z:   return "DETECTOR Z";
		case SettingsVariables::OPTICS_X:   return "OPTICS X";
		case SettingsVariables::OPTICS_Y:   return "OPTICS Y";
		case SettingsVariables::OPTICS_Z:   return "OPTICS Z";
		default:           return "Unknown";
		}
	};

private:
	//cSettings* m_Settings{};
	wxString m_ImagePath{};
	MainFrameVariables::AxisMeasurement* m_FirstAxis{}, * m_SecondAxis{};
	int m_DecimalDigits{ 3 };

	// FWHM
	std::unique_ptr<double[]> m_HorizontalFWHMData{}, m_VerticalFWHMData{};
	std::unique_ptr<float[]> m_FirstAxisPositionsData{};
	double m_PixelSizeUM{};
};
/* ___ End Worker Thread ___ */

/* ___ Start Progress Thread ___ */
class ProgressThread final : public wxThreadHelper
{
public:
	ProgressThread(cSettings* settings, cMain* main_frame);
	virtual void* Entry();
	~ProgressThread();

private:
	cSettings* m_Settings{};
	cMain* m_Frame{};
	int m_Progress{};
	wxString m_ProgressMsg{};
};
/* ___ End  Progress Thread ___ */

/* ___ Start ProgressBar ___ */
class ProgressBar final : public wxFrame
{
public:
	ProgressBar(wxWindow* parent, const wxPoint& pos, const wxSize& size);
	void UpdateProgressWithMessage(const wxString& msg, const int& progress);
	void UpdateElapsedTime(const uint64_t& elapsed_seconds);
	void UpdateEstimatedTime(const int& prgs, const uint64_t& estimated_seconds);
	~ProgressBar();
private:
	void CreateProgressBar();

private:
	ProgressPanel* m_ProgressPanel{};
	wxSize m_MainSize{};

	DECLARE_EVENT_TABLE()
};
/* ___ End ProgressBar ___ */

/* ___ Start ProgressPanel ___ */
class ProgressPanel final : public wxPanel
{
public:
	ProgressPanel(wxFrame* parent, const wxSize& size);
	void SetSize(const wxSize& new_size);
	void SetProgress(const int& progress);
	void SetElapsedTime(const int& elapsed_time);
	void SetEstimatedTime(const int& progress, const int& estimated_time);
	void SetComment(const wxString& progress_comment);

private:
	void PaintEvent(wxPaintEvent& evt);
	void Render(wxBufferedPaintDC& dc);
	void OnSize(wxSizeEvent& evt);

private:
	int m_Width{}, m_Height{};
	int m_Progress{}, m_PreviousEstimateProgress{};
	int m_ElapsedTime{}, m_EstimatedTime{}; // Time in seconds
	int m_ElapsedHours{}, m_ElapsedMinutes{}, m_ElapsedSeconds{};
	wxString m_ProgressComment{ "Moving to start position" };

	DECLARE_EVENT_TABLE()
};
/* ___ End ProgressPanel ___ */

#endif // !CMAIN_H
