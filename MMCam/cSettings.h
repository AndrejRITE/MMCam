#pragma once
#ifndef CSETTINGS_H
#define CSETTINGS_H

#include "wx/wx.h"
#include "wx/valnum.h"

#include <memory>
#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

#include "StandaMotor.h"
#include "XimeaControl.h"


namespace SettingsVariables
{
	enum
	{
		/* Work Station */
		ID_WORK_STATION_CHOICE,
		/* Detector X */
		ID_MOT_DET_X_MOTOR_TXT_CTRL,
		ID_MOT_DET_X_STEPS_PER_MM_ST_TEXT,
		/* Detector Y */
		ID_MOT_DET_Y_MOTOR_TXT_CTRL,
		ID_MOT_DET_Y_STEPS_PER_MM_ST_TEXT,
		/* Detector Z */
		ID_MOT_DET_Z_MOTOR_TXT_CTRL,
		ID_MOT_DET_Z_STEPS_PER_MM_ST_TEXT,
		/* Optics X */
		ID_MOT_OPT_X_MOTOR_TXT_CTRL,
		ID_MOT_OPT_X_STEPS_PER_MM_ST_TEXT,
		/* Optics Y */
		ID_MOT_OPT_Y_MOTOR_TXT_CTRL,
		ID_MOT_OPT_Y_STEPS_PER_MM_ST_TEXT,
		/* Optics Z */
		ID_MOT_OPT_Z_MOTOR_TXT_CTRL,
		ID_MOT_OPT_Z_STEPS_PER_MM_ST_TEXT,
		/* Cameras */
		ID_CAM_TXT_CTRL,
		/* Other Parameters */
		ID_GRID_MESH_STEP_TXT_CTRL,
		ID_CIRCLE_MESH_STEP_TXT_CTRL
	};

	enum MotorsNames 
	{
		DETECTOR_X,
		DETECTOR_Y,
		DETECTOR_Z,
		OPTICS_X,
		OPTICS_Y,
		OPTICS_Z
	};

	struct MotorSettings
	{
		wxTextCtrl* motor{}; 
		wxStaticText* steps_per_mm{};
		wxString motor_sn{};
		//uint8_t current_selection[2], prev_selection[2];
		~MotorSettings()
		{
			motor->~wxTextCtrl();
		}
	};

	struct MotorSettingsArray
	{
		std::unique_ptr<MotorSettings[]> m_Detector{}, m_Optics{};

		wxArrayString xml_all_motors[2];
		wxArrayString xml_selected_motors[2];

		std::map<unsigned int, float> unique_motors_map;
		//std::set<float> unique_motors_set[2];
		wxArrayString unique_motors[2];

		MotorSettingsArray()
		{
			m_Detector = std::make_unique<MotorSettings[]>(3);
			m_Optics = std::make_unique<MotorSettings[]>(3);
		}
	};

	struct Cameras
	{
		wxTextCtrl* camera{};
		wxString selected_camera_str{};

		~Cameras()
		{
			camera->~wxTextCtrl();
		}
	};

	struct WorkStationData
	{
		wxArrayString selected_motors_in_data_file{};
		std::map<wxString, int> motors_steps_per_mm{};
		wxString selected_camera_in_data_file{};
		wxString work_station_name{};
	};

	struct WorkStations
	{
		wxChoice* work_station_choice{};
		unsigned short work_stations_count{};
		std::unique_ptr<WorkStationData[]> work_station_data{};
		wxArrayString all_work_station_array_str{};
		wxString initialized_work_station{};
		unsigned short initialized_work_station_num{};

		~WorkStations()
		{
			work_station_choice->~wxChoice();
		}
	};

	struct ProgressValues
	{
		int current_capture{}, whole_captures_num{};
		bool is_finished{};
	};

	static auto FindNode(rapidxml::xml_node<>* xmlNode, std::string nodeName) -> rapidxml::xml_node<>*
	{
		for (auto node = xmlNode->first_node(); node; node = node->next_sibling())
		{
			if (node->name() == nodeName)
				return node;
		}
	};

}

class cSettings final : public wxDialog
{
public:
	cSettings(wxWindow* parent_frame);

	bool MotorHasSerialNumber(const int motorName)
	{
		return m_PhysicalMotors->IsMotorConnected
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString()
		);
	}

	float GetActualMotorPosition(const int motorName)
	{
		return m_PhysicalMotors->GetActualStagePos
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString()
		);
	}

	/* Progress Getter */
	bool IsCapturingFinished() const;
	void ProvideProgressInfo(wxString* msg, int* prgrs);
	auto ProvideProgressMessage() const -> wxString;
	auto ProvideProgressValue() const -> int;

	/* Progress Setter */
	void ResetCapturing();

	/* Setters */
	float GoToAbsPos(const int motorName, const float absolute_position)
	{
		return m_PhysicalMotors->GoMotorToAbsolutePosition
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString(),
			absolute_position
		);
	};

	float GoOffsetMotor(const int motorName, const float delta)
	{
		return m_PhysicalMotors->GoMotorOffset
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString(),
			delta
		);
	};

	float CenterMotor(const int motorName)
	{
		return m_PhysicalMotors->GoMotorCenter
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString()
		);

	};
	float HomeMotor(const int motorName)
	{
		return m_PhysicalMotors->GoMotorHome
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString()
		);
	};

	/* Progress */
	void SetCurrentProgress(const int& curr_capturing_num, const int& whole_capturing_num);

	/* Camera */
	auto GetSelectedCamera() const -> wxString;

	auto GetPixelSizeUM() const -> double { return m_PixelSizeUM; };
	auto GetCropSizeMM() const -> double { return m_CropSizeMM; };
	auto GetCropCircleSizeMM() const -> double { return m_CropCircleSizeMM; };
	auto GetUploadReportFolder() const -> wxString { return m_UploadReportFolder; };
	auto GetXRayImagesDefaultCaption() const -> wxArrayString { return m_XRayImagesCaptions; };

	auto GetGridMeshStep() const -> unsigned int 
	{ 
		int step = 1;
		m_GridMeshStepPXTxtCtrl->GetValue().ToInt(&step);
		return (unsigned int)step; 
	};
	auto GetCircleMeshStep() const -> unsigned int 
	{ 
		int step = 1;
		m_CircleMeshStepPXTxtCtrl->GetValue().ToInt(&step);
		return (unsigned int)step; 
	};

	auto GetOkBtnState() -> bool
	{
		if (m_OkBtnPressed)
		{
			m_OkBtnPressed = !m_OkBtnPressed;
			return !m_OkBtnPressed;
		}
		return m_OkBtnPressed;
	}

private:
	void CreateMainFrame();
	void CreateSettings();
	void CreateMotorsSelection(wxBoxSizer* panel_sizer);
	auto CreateOtherSettings(wxBoxSizer* panel_sizer) -> void;
	void InitDefaultStateWidgets();
	void InitComponents();

	void BindControls();
	void UpdateRangesTextCtrls();

	auto OnWorkStationChoice(wxCommandEvent& evt) -> void;
	auto UpdateMotorsAndCameraTXTCtrls(const short selected_work_station = -1) -> void;
	void OnRefreshBtn(wxCommandEvent& evt);
	void OnOkBtn(wxCommandEvent& evt);
	bool CheckIfThereIsCollisionWithMotors();
	bool CheckIfUserSelectedAllRangesForAllSelectedMotors();
	bool CheckIfUserSelectedAllMotorsForAllSelectedRanges();
	void OnCancelBtn(wxCommandEvent& evt);

	unsigned int FindSerialNumber(const uint8_t selection_number, const SettingsVariables::MotorSettings* motor_settings) const;

	/* Working with XML data and operating with m_Motors variables */
	auto CompareXMLWithConnectedDevices();
	auto ReadInitializationFile() -> void;
	auto LoadWorkStationFiles() -> void;
	auto ReadWorkStationFile(const std::string& fileName, const int fileNum) -> void;
	void ReadXMLFile();
	void UpdateUniqueArray();
	void SelectMotorsAndRangesFromXMLFile();
	void SelectMotorsAndRangesOnWXChoice();
	void UpdatePreviousStatesData();
	void SetPreviousStatesDataAsCurrentSelection();
	void WriteActualSelectedMotorsAndRangesIntoXMLFile();
	void ResetAllMotorsAndRangesInXMLFile();

	auto RewriteInitializationFile() -> void;

private:
	const wxString initialization_file_path = "src\\init.ini";
	const wxString work_stations_path = "src\\";
	double m_PixelSizeUM{}, m_CropSizeMM{}, m_CropCircleSizeMM{};
	wxArrayString m_XRayImagesCaptions{};
	wxString m_UploadReportFolder{};
	//const wxString xml_file_path = "src\\old_xml\\mtrs.xml";
	std::unique_ptr<SettingsVariables::WorkStations> m_WorkStations{};
	std::unique_ptr<wxButton> m_OkBtn{}, m_CancelBtn{}, m_RefreshBtn{};
	bool m_OkBtnPressed{};
	std::unique_ptr<SettingsVariables::MotorSettingsArray> m_Motors{};
	
	// TODO: Move to Polymorphic class IMotorFactory
	std::unique_ptr<IMotorArray> m_PhysicalMotors{};

	std::unique_ptr<SettingsVariables::Cameras> m_Cameras{};
	const int m_MotorsCount{ 6 };
	std::unique_ptr<SettingsVariables::ProgressValues> m_Progress = std::make_unique<SettingsVariables::ProgressValues>();
	std::unique_ptr<wxTextCtrl> m_GridMeshStepPXTxtCtrl{}, m_CircleMeshStepPXTxtCtrl{};

	wxColour m_BlackAppearenceColor = wxColour(90, 90, 90);
};

#endif // !CSETTINGS_H

