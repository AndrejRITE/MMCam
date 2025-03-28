#pragma once
#ifndef CSETTINGS_H
#define CSETTINGS_H

#include "wx/wx.h"
#include "wx/valnum.h"
#include "wx/textfile.h"

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
#include "json.hpp"

#include "StandaMotor.h"
#include "XeryonMotor.h"
#include "XimeaControl.h"
#include <wx/event.h>


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
		ID_CAM_ID_TXT_CTRL,
		ID_CAM_TEMPERATURE_TXT_CTRL,
		ID_CAM_BINNING_CHOICE,
		/* Other Parameters */
		ID_GRID_MESH_STEP_TXT_CTRL,
		ID_CIRCLE_MESH_STEP_TXT_CTRL
	};

	enum CameraManufacturers
	{
		XIMEA,
		MORAVIAN_INSTRUMENTS
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
		std::unique_ptr<MotorSettings[]> m_Detector{}, m_Optics{};

		wxArrayString xml_all_motors[2];
		wxArrayString xml_selected_motors[2];

		std::map<std::string, float> unique_motors_map;
		//std::set<float> unique_motors_set[2];
		wxArrayString unique_motors[2];

		MotorSettingsArray()
		{
			m_Detector = std::make_unique<MotorSettings[]>(3);
			m_Optics = std::make_unique<MotorSettings[]>(3);
		}
	};

	struct Camera
	{
		wxTextCtrl* idTxtCtrl{};
		wxTextCtrl* temperatureTxtCtrl{};
		wxChoice* binningChoice{};
		wxString selectedCameraIDStr{};

		wxArrayString binningsArrayStr{};
	};

	struct WorkStationData
	{
		wxArrayString selected_motors_in_data_file{};
		MotorManufacturers motor_manufacturer{};
		std::map<wxString, int> motors_steps_per_mm{};
		wxString selected_camera_in_data_file{};
		CameraManufacturers camera_manufacturer{};
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

		// To serialize Stage to JSON
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Stage, SerialNumber, COMPort, LastKnownPosition)
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
		auto position = m_PhysicalMotors->GoMotorToAbsolutePosition
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString(),
			absolute_position
		);
		PrepareStagesDataAndWriteThemIntoJSONFile();

		return position;
	};

	float GoOffsetMotor(const int motorName, const float delta)
	{
		auto position = m_PhysicalMotors->GoMotorOffset
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString(),
			delta
		);
		PrepareStagesDataAndWriteThemIntoJSONFile();

		return position;
	};

	float CenterMotor(const int motorName)
	{
		auto position = m_PhysicalMotors->GoMotorCenter
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString()
		);
		PrepareStagesDataAndWriteThemIntoJSONFile();

		return position;
	};

	float HomeMotor(const int motorName)
	{
		auto position = m_PhysicalMotors->GoMotorHome
		(
			m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[motorName].ToStdString()
		);
		PrepareStagesDataAndWriteThemIntoJSONFile();

		return position;
	};

	/* Progress */
	void SetCurrentProgress(const int& curr_capturing_num, const int& whole_capturing_num);

	/* Camera */
	auto GetSelectedCamera() const->wxString;

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

	auto GetRequiredSensorTemperature() const -> double
	{
		double requiredTemperature{};
		m_Camera->temperatureTxtCtrl->GetValue().ToDouble(&requiredTemperature);
		return requiredTemperature;
	}

	auto GetCameraManufacturer() const -> int { return m_CameraManufacturer; }
	auto GetMotorManufacturer() const -> int { return m_MotorManufacturer; }

private:
	void CreateMainFrame();
	void CreateSettings();
	void CreateMotorsSelection(wxBoxSizer* panel_sizer);
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

private:
	const wxString m_InitializationFilePath = "MMCam.ini";
	const wxString work_stations_path = "src\\";
	const std::string m_StagesPositionsFilename = "StagesPositions.json";
	double m_CropSizeMM{}, m_CropCircleSizeMM{};
	double m_PixelSizeUM{};
	SettingsVariables::MotorManufacturers m_MotorManufacturer{};
	SettingsVariables::CameraManufacturers m_CameraManufacturer{};
	wxArrayString m_XRayImagesCaptions{};
	wxString m_UploadReportFolder{};
	//const wxString xml_file_path = "src\\old_xml\\mtrs.xml";
	std::unique_ptr<SettingsVariables::WorkStations> m_WorkStations{};
	//std::unique_ptr<wxButton> m_OkBtn{}, m_CancelBtn{}, m_RefreshBtn{};
	std::unique_ptr<SettingsVariables::MotorSettingsArray> m_Motors{};
	
	// TODO: Move to Polymorphic class IMotorFactory
	std::unique_ptr<IMotorArray> m_PhysicalMotors{};

	std::unique_ptr<SettingsVariables::Camera> m_Camera{};
	const int m_MotorsCount{ 6 };
	std::unique_ptr<SettingsVariables::ProgressValues> m_Progress = std::make_unique<SettingsVariables::ProgressValues>();
	std::unique_ptr<wxTextCtrl> m_GridMeshStepPXTxtCtrl{}, m_CircleMeshStepPXTxtCtrl{};

	wxColour m_BlackAppearenceColor = wxColour(90, 90, 90);
};

#endif // !CSETTINGS_H

