#pragma once
#ifndef CSETTINGS_H
#define CSETTINGS_H

#include "wx/wx.h"
#include "wx/valnum.h"
#include "wx/textfile.h"
#include "wx/notebook.h"
#include "MaterialDesign/wxMaterialDesignArtProvider.hpp"

#include <memory>
#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include <nlohmann/json.hpp>

#include "StandaMotor.h"
#include "XeryonMotor.h"
#include "XimeaControl.h"
#include <wx/event.h>


namespace SettingsVariables
{
	enum ID
	{
		/* Work Station */
		WORK_STATION_CHOICE,

		/* Detector X */
		MOT_DET_X_MOTOR_TXT_CTRL,
		MOT_DET_X_STEPS_PER_MM_ST_TEXT,
		/* Detector Y */
		MOT_DET_Y_MOTOR_TXT_CTRL,
		MOT_DET_Y_STEPS_PER_MM_ST_TEXT,
		/* Detector Z */
		MOT_DET_Z_MOTOR_TXT_CTRL,
		MOT_DET_Z_STEPS_PER_MM_ST_TEXT,

		/* Optics X */
		MOT_OPT_X_MOTOR_TXT_CTRL,
		MOT_OPT_X_STEPS_PER_MM_ST_TEXT,
		/* Optics Y */
		MOT_OPT_Y_MOTOR_TXT_CTRL,
		MOT_OPT_Y_STEPS_PER_MM_ST_TEXT,
		/* Optics Z */
		MOT_OPT_Z_MOTOR_TXT_CTRL,
		MOT_OPT_Z_STEPS_PER_MM_ST_TEXT,

		/* AUX X */
		MOT_AUX_X_MOTOR_TXT_CTRL,
		MOT_AUX_X_STEPS_PER_MM_ST_TEXT,
		/* AUX Y */
		MOT_AUX_Y_MOTOR_TXT_CTRL,
		MOT_AUX_Y_STEPS_PER_MM_ST_TEXT,
		/* AUX Z */
		MOT_AUX_Z_MOTOR_TXT_CTRL,
		MOT_AUX_Z_STEPS_PER_MM_ST_TEXT,

		/* Cameras */
		CAM_ID_TXT_CTRL,
		CAM_TEMPERATURE_TXT_CTRL,
		CAM_BINNING_CHOICE,
		/* Other Parameters */
		GRID_MESH_STEP_TXT_CTRL,
		CIRCLE_MESH_STEP_TXT_CTRL
	};

	enum CameraManufacturers
	{
		XIMEA,
		MORAVIAN_INSTRUMENTS,
		TUCSEN
	};

	enum MotorsNames 
	{
		DETECTOR_X,
		DETECTOR_Y,
		DETECTOR_Z,
		OPTICS_X,
		OPTICS_Y,
		OPTICS_Z,
		AUX_X,
		AUX_Y,
		AUX_Z
	};

	enum MotorManufacturers
	{
		STANDA,
		XERYON
	};

	struct MotorSettings
	{
		wxTextCtrl* motor{}; 
		wxStaticText* steps_per_mm{};
		wxString motor_sn{};
	};

	struct MotorSettingsArray
	{
		std::unique_ptr<MotorSettings[]> m_Detector{}, m_Optics{}, m_Aux{};

		wxArrayString xml_all_motors[2];
		wxArrayString xml_selected_motors[2];

		std::map<std::string, float> unique_motors_map;
		//std::set<float> unique_motors_set[2];
		wxArrayString unique_motors[2];

		MotorSettingsArray()
		{
			m_Detector = std::make_unique<MotorSettings[]>(3);
			m_Optics = std::make_unique<MotorSettings[]>(3);
			m_Aux = std::make_unique<MotorSettings[]>(3);
		}
	};

	struct Camera
	{
		wxTextCtrl* idTxtCtrl{};
		wxString selectedCameraIDStr{};
	};

	struct WorkStationData
	{
		wxArrayString selected_motors_in_data_file{};

		std::map<wxString, int> motors_steps_per_mm{};
		std::map<wxString, SettingsVariables::MotorManufacturers> motor_vendor_by_sn{};

		wxString selected_camera_in_data_file{};
		CameraManufacturers camera_manufacturer{};
		wxString work_station_name{};
		double pixelSizeUM{};
	};

	struct WorkStations
	{
		wxChoice* work_station_choice{};
		unsigned short work_stations_count{};
		std::unique_ptr<WorkStationData[]> work_station_data{};
		wxArrayString all_work_station_array_str{};
		wxString initialized_work_station{};
		unsigned short initialized_work_station_num{};
	};

	struct ProgressValues
	{
		int current_capture{}, whole_captures_num{};
		bool is_finished{};
	};

	struct Stage 
	{
		std::string SerialNumber = "";
		std::string COMPort = "";
		double LastKnownPosition = 0.0;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE
		(
			Stage, 
			SerialNumber, 
			COMPort, 
			LastKnownPosition
		)
	};

	struct StageSettings
	{
		std::string SerialNumber{};

		int StepsPerMM{};

		std::string Manufacturer{}; // "STANDA" | "XERYON"

		NLOHMANN_DEFINE_TYPE_INTRUSIVE
		(
			StageSettings, 
			SerialNumber, 
			StepsPerMM,
			Manufacturer
		)
	};

	struct SelectedMotors 
	{
		std::string camera{};
		std::string camera_manufacturer{};
		std::string motor_manufacturer{};
		std::string station{};

		double pixel_size_um{};
		int steps_per_mm{};

		std::vector<StageSettings> detector{}, optics{}, aux{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE
		(
			SelectedMotors, 
			camera, 
			camera_manufacturer, 
			motor_manufacturer, 
			station, 

			pixel_size_um, 
			steps_per_mm,

			detector, 
			optics,
			aux
		)
	};

	static auto FindNode(rapidxml::xml_node<>* xmlNode, std::string nodeName) -> rapidxml::xml_node<>*
	{
		for (auto node = xmlNode->first_node(); node; node = node->next_sibling())
		{
			if (node->name() == nodeName)
				return node;
		}
	};

	static MotorManufacturers ParseVendor(const std::string& s) 
	{
		auto low = wxString(s).Lower();
		if (low == "xeryon") return SettingsVariables::MotorManufacturers::XERYON;
		return SettingsVariables::MotorManufacturers::STANDA; // default
	}

}

class cSettings final : public wxDialog
{
public:
	cSettings(wxWindow* parent_frame);

	bool MotorHasSerialNumber(const SettingsVariables::MotorsNames& motorName)
	{
		const auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];

		if (motorName < 0 || motorName >= static_cast<int>(ws.selected_motors_in_data_file.size()))
			return false;

		const wxString sn = ws.selected_motors_in_data_file[motorName];
		if (sn.IsEmpty() || sn == "None")
			return false;

		if (IMotorArray* arr = WhichArrayFor(sn))
			return arr->IsMotorConnected(sn.ToStdString());

		return false;
	}

	float GetActualMotorPosition(const SettingsVariables::MotorsNames& motorName)
	{
		const auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];

		if (motorName < 0 || motorName >= static_cast<int>(ws.selected_motors_in_data_file.size()))
			return 0.0f;

		const wxString sn = ws.selected_motors_in_data_file[motorName];
		if (sn.IsEmpty() || sn == "None")
			return 0.0f;

		if (IMotorArray* arr = WhichArrayFor(sn))
			return arr->GetActualStagePos(sn.ToStdString());

		// Driver not available (e.g., vendor array not instantiated)
		return 0.0f;
	}

	int ShowModal() override;
	auto SetMotorStepsPerMM() -> void;
	/* Progress Getter */
	bool IsCapturingFinished() const;
	void ProvideProgressInfo(wxString* msg, int* prgrs);
	auto ProvideProgressMessage() const->wxString;
	auto ProvideProgressValue() const -> int;

	/* Progress Setter */
	void ResetCapturing();

	/* Setters */
	float GoToAbsPos(const int motorName, const float absolute_position)
	{
		auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
		const wxString sn = ws.selected_motors_in_data_file[motorName];
		IMotorArray* arr = WhichArrayFor(sn);
		auto position = arr->GoMotorToAbsolutePosition(sn.ToStdString(), absolute_position);
		PrepareStagesDataAndWriteThemIntoJSONFile(); // still fine; see step 5
		return position;
	};

	float GoOffsetMotor(const int motorName, const float delta)
	{
		auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
		const wxString sn = ws.selected_motors_in_data_file[motorName];
		IMotorArray* arr = WhichArrayFor(sn);
		auto position = arr->GoMotorOffset(sn.ToStdString(), delta);
		PrepareStagesDataAndWriteThemIntoJSONFile();
		return position;
	};

	float CenterMotor(const int motorName)
	{
		auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
		const wxString sn = ws.selected_motors_in_data_file[motorName];
		IMotorArray* arr = WhichArrayFor(sn);
		auto position = arr->GoMotorCenter(sn.ToStdString());
		PrepareStagesDataAndWriteThemIntoJSONFile();
		return position;
	};

	float HomeMotor(const int motorName)
	{
		auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
		const wxString sn = ws.selected_motors_in_data_file[motorName];
		IMotorArray* arr = WhichArrayFor(sn);
		auto position = arr->GoMotorHome(sn.ToStdString());
		PrepareStagesDataAndWriteThemIntoJSONFile();
		return position;
	};

	/* Progress */
	void SetCurrentProgress(const int& curr_capturing_num, const int& whole_capturing_num);

	auto SetLastInitializedWorkStation(const wxString& workStation) -> void 
	{ 
		auto workStationPosition = m_WorkStations->work_station_choice->FindString(workStation);
		if (workStationPosition == wxNOT_FOUND) return;

		m_WorkStations->work_station_choice->SetSelection(workStationPosition);
		wxCommandEvent event(wxEVT_CHOICE, SettingsVariables::ID::WORK_STATION_CHOICE);
		OnWorkStationChoice(event);
	};

	/* Camera */
	auto GetSelectedCamera() const->wxString;

	auto GetPixelSizeUM() const -> double { return m_PixelSizeUM; };
	auto GetInitializedWorkStation() const -> wxString { return m_WorkStations->initialized_work_station; };
	auto GetUploadReportFolder() const -> wxString { return m_UploadReportFolder; };
	auto GetXRayImagesDefaultCaption() const -> wxArrayString { return m_XRayImagesCaptions; };
	
	auto GetCameraManufacturer() const -> int { return m_CameraManufacturer; }

private:
	void CreateMainFrame();
	void CreateSettings();
	void CreateMotorsSelection(wxBoxSizer* panel_sizer);

	auto CreateDetectorPage(wxWindow* parent, const wxSize& txtCtrlSize, const int& topOffset) -> wxWindow*;
	auto CreateOpticsPage(wxWindow* parent, const wxSize& txtCtrlSize, const int& topOffset) -> wxWindow*;
	auto CreateAuxPage(wxWindow* parent, const wxSize& txtCtrlSize, const int& topOffset) -> wxWindow*;

	auto CreateCameraSection(wxPanel* panel, wxBoxSizer* panel_sizer) -> void;
	auto CreateOtherSettings(wxBoxSizer* panel_sizer) -> void;
	void InitDefaultStateWidgets();
	void InitComponents();

	void BindControls();
	void UpdateRangesTextCtrls();

	auto OnWorkStationChoice(wxCommandEvent& evt) -> void;
	auto UpdateMotorsAndCameraTXTCtrls(const short selected_work_station = -1) -> void;
	void OnRefreshBtn(wxCommandEvent& evt);
	bool CheckIfThereIsCollisionWithMotors();
	bool CheckIfUserSelectedAllRangesForAllSelectedMotors();
	bool CheckIfUserSelectedAllMotorsForAllSelectedRanges();
	void OnCancelBtn(wxCommandEvent& evt);

	unsigned int FindSerialNumber(const uint8_t selection_number, const SettingsVariables::MotorSettings* motor_settings) const;

	/* Working with XML data and operating with m_Motors variables */
	auto LoadWorkStationFiles() -> void;
	auto ReadWorkStationFile(const std::string& fileName, const int fileNum) -> void;
	void UpdateUniqueArray();
	void SelectMotorsAndRangesFromXMLFile();

	auto InitializeXeryonAndCheckPython() -> void;

	// Python
	bool IsPythonInstalled()
	{
		wxArrayString output;
		wxArrayString errors;
		int exitCode = wxExecute("python --version", output, errors);
		if (exitCode != 0)
		{
			exitCode = wxExecute("python3 --version", output, errors);
		}
		return (exitCode == 0);
	}

	bool IsPythonModuleInstalled(const wxString& moduleName)
	{
		wxArrayString output;
		wxArrayString errors;
		wxString command = "python -m pip show " + moduleName;
		int exitCode = wxExecute(command, output, errors);
		return (exitCode == 0);
	}

	std::vector<wxString> GetMissingPythonModules(const wxString& requirementsPath)
	{
		std::vector<wxString> missingModules;
		wxTextFile file(requirementsPath);

		if (!file.Open())
		{
			wxLogError("Could not open requirements.txt");
			return missingModules;
		}

		for (size_t i = 0; i < file.GetLineCount(); ++i)
		{
			wxString module = file.GetLine(i).BeforeFirst('='); // Extract module name before '='
			if (!IsPythonModuleInstalled(module))
			{
				missingModules.push_back(module);
			}
		}

		file.Close();
		return missingModules;
	}

	std::vector<SettingsVariables::Stage> ReadJson(const std::string& filename) 
	{
		namespace fs = std::filesystem;
		using json = nlohmann::json;

		std::vector<SettingsVariables::Stage> stages;

		if (fs::exists(filename)) {
			std::ifstream file(filename);
			json j;
			file >> j;

			// Deserialize JSON into stages
			for (const auto& item : j) 
			{
				SettingsVariables::Stage stage;
				stage.SerialNumber = item.at("SerialNumber").get<std::string>();
				stage.COMPort = item.at("COMPort").get<std::string>();
				stage.LastKnownPosition = item.at("LastKnownPosition").get<double>();
				stages.push_back(stage);
			}
		}

		return stages;
	}

	void WriteJson(const std::string& filename, const std::vector<SettingsVariables::Stage>& stages) 
	{
		using json = nlohmann::json;

		namespace fs = std::filesystem;

		// Ensure the parent directory exists
		fs::path filePath(filename);
		fs::path dirPath = filePath.parent_path();

		if (!dirPath.empty() && !fs::exists(dirPath)) 
			fs::create_directories(dirPath);  // Create the directory if it doesn't exist

		json j;

		// Serialize stages to JSON
		for (const auto& stage : stages) 
		{
			j.push_back(stage);
		}

		std::ofstream file(filename);
		file << std::setw(4) << j << std::endl;
	}

	auto ReadStagePositionsFromJSONFile() -> void;
	auto PrepareStagesDataAndWriteThemIntoJSONFile() -> void;

	IMotorArray* WhichArrayFor(const wxString& sn) const 
	{
		const auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
		auto it = ws.motor_vendor_by_sn.find(sn);
		auto v = (it != ws.motor_vendor_by_sn.end()) ? it->second : SettingsVariables::STANDA;
		return (v == SettingsVariables::XERYON) ? m_XeryonMotors.get() : m_StandaMotors.get();
	}

private:
	const wxString work_stations_path = "src\\";
	const std::string m_StagesPositionsFilename = "temp\\StagesPositions.json";

	double m_PixelSizeUM{};

	std::unique_ptr<wxPanel> m_MainPanel{};

	wxNotebook* m_MotorsNotebook{};

	SettingsVariables::CameraManufacturers m_CameraManufacturer{};
	wxArrayString m_XRayImagesCaptions{};
	wxString m_UploadReportFolder{};
	std::unique_ptr<SettingsVariables::WorkStations> m_WorkStations{};
	std::unique_ptr<SettingsVariables::MotorSettingsArray> m_Motors{};
	
	std::unique_ptr<IMotorArray> m_StandaMotors{};
	std::unique_ptr<IMotorArray> m_XeryonMotors{};

	std::unique_ptr<SettingsVariables::Camera> m_Camera{};
	const int m_MotorsCount{ 6 };
	std::unique_ptr<SettingsVariables::ProgressValues> m_Progress = std::make_unique<SettingsVariables::ProgressValues>();

	wxColour m_BackgroundColour = wxColour(90, 90, 90);
};

#endif // !CSETTINGS_H

