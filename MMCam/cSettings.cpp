#include "cSettings.h"

cSettings::cSettings(wxWindow* parent_frame) 
	: wxDialog(
		parent_frame, 
		wxID_ANY, 
		"Settings", 
		wxDefaultPosition, 
		wxDefaultSize, 
		wxDEFAULT_DIALOG_STYLE)
{
	CreateMainFrame();
	InitDefaultStateWidgets();
	CenterOnScreen();
}

bool cSettings::IsCapturingFinished() const
{
	return m_Progress->is_finished;
}

void cSettings::ProvideProgressInfo(wxString* msg, int* prgrs)
{
	*msg = "Capturing " + wxString::Format(wxT("%i"), m_Progress->current_capture + 1) + " of " + wxString::Format(wxT("%i"), m_Progress->whole_captures_num) + " images";
	*prgrs = 100.f * (float)(m_Progress->current_capture + 1) / (float)m_Progress->whole_captures_num;
}

auto cSettings::ProvideProgressMessage() const -> wxString
{
	auto msg = "Capturing " + wxString::Format(wxT("%i"), m_Progress->current_capture + 1) + " of " + wxString::Format(wxT("%i"), m_Progress->whole_captures_num) + " images";
	return msg;
}

auto cSettings::ProvideProgressValue() const -> int
{
	auto prgrs = 100.f * (float)(m_Progress->current_capture + 1) / (float)m_Progress->whole_captures_num;
	return (int)prgrs;
}

void cSettings::ResetCapturing()
{
	m_Progress->is_finished = false;
}

void cSettings::SetCurrentProgress(const int& curr_capturing_num, const int& whole_capturing_num)
{
	m_Progress->current_capture = curr_capturing_num;
	m_Progress->whole_captures_num = whole_capturing_num;
	m_Progress->is_finished = curr_capturing_num >= whole_capturing_num ? true : false;
}

auto cSettings::GetSelectedCamera() const -> wxString
{
	return m_Cameras->selected_camera_str;
}

void cSettings::CreateMainFrame()
{
	InitComponents();
	ReadInitializationFile();
	LoadWorkStationFiles();
	//IterateOverConnectedCameras();
	//ReadXMLFile();
	CreateSettings();
	BindControls();
	UpdateMotorsAndCameraTXTCtrls();
	//SelectMotorsAndRangesOnWXChoice();
}

void cSettings::CreateSettings()
{
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ms_sizer = new wxBoxSizer(wxVERTICAL);
	
	CreateMotorsSelection(ms_sizer);

	CreateOtherSettings(ms_sizer);
	
	main_sizer->Add(ms_sizer, 1, wxEXPAND);
	SetSizerAndFit(main_sizer);
}

void cSettings::CreateMotorsSelection(wxBoxSizer* panel_sizer)
{	
	wxPanel* main_panel = new wxPanel(this);
#ifdef _DEBUG
	main_panel->SetBackgroundColour(m_BlackAppearenceColor);
#else
	main_panel->SetBackgroundColour(m_BlackAppearenceColor);
#endif // _DEBUG
	wxBoxSizer* main_panel_sizer = new wxBoxSizer(wxVERTICAL);

	/* Work Station */
	wxSizer* const work_station_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Work Station");
	{
		auto work_station_txt_ctrl_size = wxSize(120, 24);
		m_WorkStations->work_station_choice = new wxChoice
		(
			main_panel, 
			SettingsVariables::ID_WORK_STATION_CHOICE, 
			wxDefaultPosition, 
			work_station_txt_ctrl_size,
			m_WorkStations->all_work_station_array_str
		);
		m_WorkStations->work_station_choice->SetSelection(m_WorkStations->initialized_work_station_num);


		work_station_static_box_sizer->AddStretchSpacer();
		work_station_static_box_sizer->Add(m_WorkStations->work_station_choice, 0, wxEXPAND);
		work_station_static_box_sizer->AddStretchSpacer();
	}
	main_panel_sizer->Add(work_station_static_box_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);

	/* Motors */
	auto motor_txt_ctrl_size = wxSize(80, 24);
	wxSizer* const motors_static_box_sizer = new wxStaticBoxSizer(wxVERTICAL, main_panel, "&Motors");
	{
		int top_offset_static_text{ 5 };
		wxSizer* const detector_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Detector");
		{
			/* X */
			wxSizer* const det_x_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&X");
			/* Serial Number */
			{
				wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&S/N");

				m_Motors->m_Detector[0].motor = new wxTextCtrl(
					main_panel, 
					SettingsVariables::ID_MOT_DET_X_MOTOR_TXT_CTRL, 
					wxT("None"),
					wxDefaultPosition, 
					motor_txt_ctrl_size,
					wxTE_CENTRE | wxTE_READONLY
				);
				
				m_Motors->m_Detector[0].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[0]);
				//m_Motors->m_Detector[0].motors->SetSelection(0);

				sn_static_box_sizer->Add(m_Motors->m_Detector[0].motor);

				det_x_static_box_sizer->Add(sn_static_box_sizer);
			}
			/* Steps/mm */
			det_x_static_box_sizer->AddSpacer(2);
			{
				wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Steps/mm");

				m_Motors->m_Detector[0].steps_per_mm = new wxStaticText(
					main_panel,
					SettingsVariables::ID_MOT_DET_X_STEPS_PER_MM_ST_TEXT,
					wxT("None"), 
					wxDefaultPosition, 
					wxDefaultSize, 
					wxALIGN_CENTRE_HORIZONTAL);
				range_static_box_sizer->Add(m_Motors->m_Detector[0].steps_per_mm, 1, wxEXPAND | wxTOP, top_offset_static_text);

				det_x_static_box_sizer->Add(range_static_box_sizer, 1, wxEXPAND);
			}
			detector_static_box_sizer->Add(det_x_static_box_sizer, 0, wxEXPAND);

			detector_static_box_sizer->AddSpacer(2);
			detector_static_box_sizer->AddStretchSpacer();

			/* Y */
			wxSizer* const det_y_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Y");
			/* Serial Number */
			{
				wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&S/N");

				m_Motors->m_Detector[1].motor = new wxTextCtrl(
					main_panel, 
					SettingsVariables::ID_MOT_DET_Y_MOTOR_TXT_CTRL, 
					wxT("None"),
					wxDefaultPosition, 
					motor_txt_ctrl_size,
					wxTE_CENTRE | wxTE_READONLY
					//m_Motors->unique_motors[0]
				);

				m_Motors->m_Detector[1].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[1]);
				//m_Motors->m_Detector[1].motors->SetSelection(0);

				sn_static_box_sizer->Add(m_Motors->m_Detector[1].motor);

				det_y_static_box_sizer->Add(sn_static_box_sizer);
			}
			/* Steps/mm */
			det_y_static_box_sizer->AddSpacer(2);
			{
				wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Steps/mm");

				m_Motors->m_Detector[1].steps_per_mm = new wxStaticText(
					main_panel, 
					SettingsVariables::ID_MOT_DET_Y_STEPS_PER_MM_ST_TEXT, 	
					wxT("None"), 
					wxDefaultPosition, 
					wxDefaultSize,
					wxALIGN_CENTRE_HORIZONTAL);
				range_static_box_sizer->Add(m_Motors->m_Detector[1].steps_per_mm, 1, wxEXPAND | wxTOP, top_offset_static_text);

				det_y_static_box_sizer->Add(range_static_box_sizer, 1, wxEXPAND);
			}
			detector_static_box_sizer->Add(det_y_static_box_sizer, 0, wxEXPAND);
			detector_static_box_sizer->AddSpacer(2);
			detector_static_box_sizer->AddStretchSpacer();

			/* Z */
			wxSizer* const det_z_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Z");
			/* Serial Number */
			{
				wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&S/N");

				m_Motors->m_Detector[2].motor = new wxTextCtrl(
					main_panel, 
					SettingsVariables::ID_MOT_DET_Z_MOTOR_TXT_CTRL, 
					wxT("None"),
					wxDefaultPosition, 
					motor_txt_ctrl_size,
					wxTE_CENTRE | wxTE_READONLY
					//m_Motors->unique_motors[0]
				);

				m_Motors->m_Detector[2].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[2]);
				//m_Motors->m_Detector[2].motors->SetSelection(0);

				sn_static_box_sizer->Add(m_Motors->m_Detector[2].motor);

				det_z_static_box_sizer->Add(sn_static_box_sizer);
			}
			/* Steps/mm */
			det_z_static_box_sizer->AddSpacer(2);
			{
				wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Steps/mm");

				m_Motors->m_Detector[2].steps_per_mm = new wxStaticText(
					main_panel, 
					SettingsVariables::ID_MOT_DET_Z_STEPS_PER_MM_ST_TEXT, 	
					wxT("None"), 
					wxDefaultPosition, 
					wxDefaultSize, 
					wxALIGN_CENTRE_HORIZONTAL);
				range_static_box_sizer->Add(m_Motors->m_Detector[2].steps_per_mm, 1, wxEXPAND | wxTOP, top_offset_static_text);

				det_z_static_box_sizer->Add(range_static_box_sizer, 1, wxEXPAND);
			}
			detector_static_box_sizer->Add(det_z_static_box_sizer, 0, wxEXPAND);
		}
		motors_static_box_sizer->Add(detector_static_box_sizer, 0, wxEXPAND);

		wxSizer* const optics_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Optics");
		{
			/* X */
			wxSizer* const opt_x_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&X");
			/* Serial Number */
			{
				wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&S/N");

				m_Motors->m_Optics[0].motor = new wxTextCtrl(
					main_panel, 
					SettingsVariables::ID_MOT_OPT_X_MOTOR_TXT_CTRL, 
					wxT("None"),
					wxDefaultPosition, 
					motor_txt_ctrl_size,
					wxTE_CENTRE | wxTE_READONLY
					//m_Motors->unique_motors[0]
				);

				m_Motors->m_Optics[0].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[3]);
				//m_Motors->m_Optics[0].motors->SetSelection(0);

				sn_static_box_sizer->Add(m_Motors->m_Optics[0].motor);

				opt_x_static_box_sizer->Add(sn_static_box_sizer);
			}
			/* Steps/mm */
			opt_x_static_box_sizer->AddSpacer(2);
			{
				wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Steps/mm");

				m_Motors->m_Optics[0].steps_per_mm = new wxStaticText(
					main_panel, 
					SettingsVariables::ID_MOT_OPT_X_STEPS_PER_MM_ST_TEXT, 	
					wxT("None"), 
					wxDefaultPosition, 
					wxDefaultSize, 
					wxALIGN_CENTRE_HORIZONTAL);
				range_static_box_sizer->Add(m_Motors->m_Optics[0].steps_per_mm, 1, wxEXPAND | wxTOP, top_offset_static_text);

				opt_x_static_box_sizer->Add(range_static_box_sizer, 1, wxEXPAND);
			}
			optics_static_box_sizer->Add(opt_x_static_box_sizer, 0, wxEXPAND);
			optics_static_box_sizer->AddSpacer(2);
			optics_static_box_sizer->AddStretchSpacer();

			/* Y */
			wxSizer* const opt_y_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Y");
			/* Serial Number */
			{
				wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&S/N");

				m_Motors->m_Optics[1].motor = new wxTextCtrl(
					main_panel, 
					SettingsVariables::ID_MOT_OPT_Y_MOTOR_TXT_CTRL, 
					wxT("None"),
					wxDefaultPosition, 
					motor_txt_ctrl_size,
					wxTE_CENTRE | wxTE_READONLY
					//m_Motors->unique_motors[0]
				);

				m_Motors->m_Optics[1].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[4]);
				//m_Motors->m_Optics[1].motors->SetSelection(0);

				sn_static_box_sizer->Add(m_Motors->m_Optics[1].motor);

				opt_y_static_box_sizer->Add(sn_static_box_sizer);
			}
			/* Steps/mm */
			opt_y_static_box_sizer->AddSpacer(2);
			{
				wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Steps/mm");

				m_Motors->m_Optics[1].steps_per_mm = new wxStaticText(
					main_panel, 
					SettingsVariables::ID_MOT_OPT_Y_STEPS_PER_MM_ST_TEXT, 
					wxT("None"), 
					wxDefaultPosition, 
					wxDefaultSize, 
					wxALIGN_CENTRE_HORIZONTAL);
				range_static_box_sizer->Add(m_Motors->m_Optics[1].steps_per_mm, 1, wxEXPAND | wxTOP, top_offset_static_text);

				opt_y_static_box_sizer->Add(range_static_box_sizer, 1, wxEXPAND);
			}
			optics_static_box_sizer->Add(opt_y_static_box_sizer, 0, wxEXPAND);
			optics_static_box_sizer->AddSpacer(2);
			optics_static_box_sizer->AddStretchSpacer();

			/* Z */
			wxSizer* const opt_z_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Z");
			/* Serial Number */
			{
				wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&S/N");

				m_Motors->m_Optics[2].motor = new wxTextCtrl(
					main_panel, 
					SettingsVariables::ID_MOT_OPT_Z_MOTOR_TXT_CTRL, 
					wxT("None"),
					wxDefaultPosition, 
					motor_txt_ctrl_size,
					wxTE_CENTRE | wxTE_READONLY
					//m_Motors->unique_motors[0]
				);

				m_Motors->m_Optics[2].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[5]);
				//m_Motors->m_Optics[2].motors->SetSelection(0);

				sn_static_box_sizer->Add(m_Motors->m_Optics[2].motor);

				opt_z_static_box_sizer->Add(sn_static_box_sizer);
			}
			/* Steps/mm */
			opt_z_static_box_sizer->AddSpacer(2);
			{
				wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Steps/mm");

				m_Motors->m_Optics[2].steps_per_mm = new wxStaticText(
					main_panel, 
					SettingsVariables::ID_MOT_OPT_Z_STEPS_PER_MM_ST_TEXT, 
					wxT("None"), 
					wxDefaultPosition, 
					wxDefaultSize, 
					wxALIGN_CENTRE_HORIZONTAL);
				range_static_box_sizer->Add(m_Motors->m_Optics[2].steps_per_mm, 1, wxEXPAND | wxTOP, top_offset_static_text);

				opt_z_static_box_sizer->Add(range_static_box_sizer, 1, wxEXPAND);
			}
			optics_static_box_sizer->Add(opt_z_static_box_sizer, 0, wxEXPAND);
		}
		motors_static_box_sizer->Add(optics_static_box_sizer, 0, wxEXPAND);

	}
	main_panel_sizer->Add(motors_static_box_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);

	/* Camera */
	wxSizer* const cameras_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, main_panel, "&Camera");
	{
		auto cam_choice_size = wxSize(80, 24);
		m_Cameras->camera = new wxTextCtrl
		(
			main_panel, 
			SettingsVariables::ID_CAM_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			cam_choice_size,
			wxTE_CENTRE | wxTE_READONLY
		);

		{
			for (auto i{ 0 }; i < m_WorkStations->work_stations_count; ++i)
			{
				if (m_WorkStations->work_station_data[i].work_station_name == m_WorkStations->initialized_work_station)
				{
					m_Cameras->selected_camera_str = m_WorkStations->work_station_data[i].selected_camera_in_data_file;
					m_WorkStations->initialized_work_station_num = i;
					break;
				}
			}

			m_Cameras->camera->SetValue(m_Cameras->selected_camera_str);
		}

		cameras_static_box_sizer->AddStretchSpacer();
		cameras_static_box_sizer->Add(m_Cameras->camera, 0, wxEXPAND);
		cameras_static_box_sizer->AddStretchSpacer();
	}
	main_panel_sizer->Add(cameras_static_box_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);
	main_panel_sizer->AddSpacer(5);
	main_panel_sizer->AddStretchSpacer();

	UpdatePreviousStatesData();

	main_panel->SetSizer(main_panel_sizer);
	panel_sizer->Add(main_panel, 1, wxEXPAND);
}

auto cSettings::CreateOtherSettings(wxBoxSizer* panel_sizer) -> void
{	
	wxPanel* main_panel = new wxPanel(this);
#ifdef _DEBUG
	main_panel->SetBackgroundColour(m_BlackAppearenceColor);
#else
	main_panel->SetBackgroundColour(m_BlackAppearenceColor);
#endif // _DEBUG
	wxBoxSizer* main_panel_sizer = new wxBoxSizer(wxVERTICAL);

	wxSizer* const settings_grid_sizer = new wxGridSizer(2);

	// Grid Mesh Step
	{
		wxBoxSizer* horSizer = new wxBoxSizer(wxHORIZONTAL);

		settings_grid_sizer->Add
		(
			new wxStaticText
			(
				main_panel,
				wxID_ANY,
				"&Grid Mesh Step:"
			),
			0, wxALL | wxALIGN_CENTER_VERTICAL, 5
		);

		wxIntegerValidator<unsigned int> val(NULL, wxNUM_VAL_ZERO_AS_BLANK);
		//val.SetMin(1);
		m_GridMeshStepPXTxtCtrl = std::make_unique<wxTextCtrl>
			(
				main_panel,
				SettingsVariables::ID_GRID_MESH_STEP_TXT_CTRL,
				wxString("100"),
				wxDefaultPosition,
				wxDefaultSize,
				wxTE_CENTRE
			);
		m_GridMeshStepPXTxtCtrl->SetValidator(val);

		horSizer->Add(m_GridMeshStepPXTxtCtrl.get(), 0, wxALIGN_CENTER_VERTICAL);
		horSizer->Add
		(
			new wxStaticText
			(
				main_panel,
				wxID_ANY,
				"[px]"
			),
			0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5
		);
		settings_grid_sizer->Add(horSizer, 0, wxEXPAND);
	}

	// Circle Mesh Step
	{
		wxBoxSizer* horSizer = new wxBoxSizer(wxHORIZONTAL);

		settings_grid_sizer->Add
		(
			new wxStaticText
			(
				main_panel,
				wxID_ANY,
				"&Circle Mesh Step:"
			),
			0, wxALL | wxALIGN_CENTER_VERTICAL, 5
		);

		wxIntegerValidator<unsigned int> val(NULL, wxNUM_VAL_ZERO_AS_BLANK);
		//val.SetMin(1);
		m_CircleMeshStepPXTxtCtrl = std::make_unique<wxTextCtrl>
			(
				main_panel,
				SettingsVariables::ID_CIRCLE_MESH_STEP_TXT_CTRL,
				wxString("120"),
				wxDefaultPosition,
				wxDefaultSize,
				wxTE_CENTRE
			);
		m_CircleMeshStepPXTxtCtrl->SetValidator(val);

		horSizer->Add(m_CircleMeshStepPXTxtCtrl.get(), 0, wxALIGN_CENTER_VERTICAL);
		horSizer->Add
		(
			new wxStaticText
			(
				main_panel,
				wxID_ANY,
				"[px]"
			),
			0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5
		);
		settings_grid_sizer->Add(horSizer, 0, wxEXPAND);
	}

	main_panel_sizer->Add(settings_grid_sizer, 0, wxCENTER | wxALL, 5);

	/* Control Buttons */
	{
		m_RefreshBtn = std::make_unique<wxButton>(main_panel, wxID_ANY, wxT("Refresh"));

		m_OkBtn = std::make_unique<wxButton>(main_panel, wxID_ANY, wxT("OK"));
		m_OkBtn->SetFocus();

		m_CancelBtn = std::make_unique<wxButton>(main_panel, wxID_ANY, wxT("Cancel"));

		wxBoxSizer* btns_sizer = new wxBoxSizer(wxHORIZONTAL);
		btns_sizer->Add(m_RefreshBtn.get());
		btns_sizer->AddStretchSpacer();
		btns_sizer->Add(m_OkBtn.get(), 0, wxRIGHT, 2);
		btns_sizer->Add(m_CancelBtn.get());

		main_panel_sizer->Add(btns_sizer, 0, wxEXPAND | wxALL, 2);
	}

	main_panel->SetSizer(main_panel_sizer);
	panel_sizer->Add(main_panel, 0, wxEXPAND);
}

void cSettings::InitDefaultStateWidgets()
{
	/* Disabling X and Z Optics motors choices */
	{
		//m_Motors->m_Optics[0].motor->Disable();
		//m_Motors->m_Optics[2].motor->Disable();
	}

	m_RefreshBtn->Disable();
}

void cSettings::InitComponents()
{
	m_WorkStations = std::make_unique<SettingsVariables::WorkStations>();
	m_Motors = std::make_unique<SettingsVariables::MotorSettingsArray>();
	m_Cameras = std::make_unique<SettingsVariables::Cameras>();
	m_PhysicalMotors = std::make_unique<IMotorArray>();
}

void cSettings::BindControls()
{
	m_WorkStations->work_station_choice->Bind(wxEVT_CHOICE, &cSettings::OnWorkStationChoice, this);
	/* Binding Bottom Buttons */
	m_RefreshBtn->Bind(wxEVT_BUTTON, &cSettings::OnRefreshBtn, this);
	m_OkBtn->Bind(wxEVT_BUTTON, &cSettings::OnOkBtn, this);
	m_CancelBtn->Bind(wxEVT_BUTTON, &cSettings::OnCancelBtn, this);
}

void cSettings::UpdateRangesTextCtrls()
{	
	wxString find_string{}, current_range_text{};
	uint8_t current_index{};
	/* Set the corresponding range for selected motors */
	for (auto motor{ 0 }; motor < m_MotorsCount; ++motor)
	{
		if (motor < m_MotorsCount / 2)
		{
			//current_index = m_Motors->m_Detector[motor].current_selection[0];
			find_string = m_Motors->unique_motors[0][current_index];
			if (find_string == "None")
			{
				m_Motors->m_Detector[motor].steps_per_mm->SetLabel("None");
				//m_Motors->m_Detector[motor].current_selection[1] = 0;
				continue;
			}
			current_index = m_Motors->xml_all_motors[0].Index(find_string);
			current_range_text = m_Motors->xml_all_motors[1].Item(current_index);

			m_Motors->m_Detector[motor].steps_per_mm->SetLabel(current_range_text);
			/* Update current_selection index of range */
			current_index = m_Motors->unique_motors[1].Index(current_range_text);
			//m_Motors->m_Detector[motor].current_selection[1] = current_index;
		}
		else
		{
			//current_index = m_Motors->m_Optics[motor - m_MotorsCount / 2].current_selection[0];
			find_string = m_Motors->unique_motors[0][current_index];
			if (find_string == "None")
			{
				m_Motors->m_Optics[motor - m_MotorsCount / 2].steps_per_mm->SetLabel("None");
				//m_Motors->m_Optics[motor - m_MotorsCount / 2].current_selection[1] = 0;
				continue;
			}
			current_index = m_Motors->xml_all_motors[0].Index(find_string);
			current_range_text = m_Motors->xml_all_motors[1].Item(current_index);

			m_Motors->m_Optics[motor - m_MotorsCount / 2].steps_per_mm->SetLabel(current_range_text);
			/* Update current_selection index of range */
			current_index = m_Motors->unique_motors[1].Index(current_range_text);
			//m_Motors->m_Optics[motor - m_MotorsCount / 2].current_selection[1] = current_index;
		}
	}
}

auto cSettings::OnWorkStationChoice(wxCommandEvent& evt) -> void
{
	auto curr_selected_work_station = m_WorkStations->work_station_choice->GetSelection();
	m_WorkStations->initialized_work_station_num = curr_selected_work_station;
	UpdateMotorsAndCameraTXTCtrls(curr_selected_work_station);
}

auto cSettings::UpdateMotorsAndCameraTXTCtrls(const short selected_work_station) -> void
{
	if (selected_work_station > -1)
	{
		m_WorkStations->initialized_work_station_num = selected_work_station;
	}
	else
	{
		m_WorkStations->work_station_choice->SetSelection(m_WorkStations->initialized_work_station_num);
	}
	m_WorkStations->initialized_work_station = m_WorkStations->all_work_station_array_str[m_WorkStations->initialized_work_station_num];
	for (auto i{ 0 }; i < m_WorkStations->work_station_data[0].selected_motors_in_data_file.size(); ++i)
	{
		auto motorSN = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[i];
		auto steps_per_mm = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].motors_steps_per_mm[motorSN];
		if (i < 3)
		{
			// SN
			m_Motors->m_Detector[i].motor->SetLabel(motorSN);
			m_Motors->m_Detector[i].motor_sn = motorSN;
			// Steps/mm
			m_Motors->m_Detector[i].steps_per_mm->SetLabel(wxString::Format(wxT("%i"), steps_per_mm));
		}
		else
		{
			// SN
			m_Motors->m_Optics[i - 3].motor->SetLabel(motorSN);
			m_Motors->m_Optics[i - 3].motor_sn = motorSN;
			// Steps/mm
			m_Motors->m_Optics[i - 3].steps_per_mm->SetLabel(wxString::Format(wxT("%i"), steps_per_mm));
		}
	}
	m_Cameras->camera->SetLabel(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_camera_in_data_file);
	m_Cameras->selected_camera_str = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_camera_in_data_file;
}

void cSettings::OnRefreshBtn(wxCommandEvent& evt)
{
	wxBusyCursor busy_cursor{};
	SelectMotorsAndRangesOnWXChoice();
	for (auto motor{ 0 }; motor < m_MotorsCount; ++motor)
	{
		if (motor < m_MotorsCount / 2)
		{
			m_Motors->m_Detector[motor].steps_per_mm->SetLabel("None");
		}
		else
		{
			m_Motors->m_Optics[motor - m_MotorsCount / 2].steps_per_mm->SetLabel("None");
		}
	}
	ResetAllMotorsAndRangesInXMLFile();
}

void cSettings::OnOkBtn(wxCommandEvent& evt)
{
	if (
		!CheckIfThereIsCollisionWithMotors() && 
		CheckIfUserSelectedAllRangesForAllSelectedMotors() && 
		CheckIfUserSelectedAllMotorsForAllSelectedRanges())
	{
		Hide();
		RewriteInitializationFile();
		//UpdatePreviousStatesData();
		//WriteActualSelectedMotorsAndRangesIntoXMLFile();
	}
}

bool cSettings::CheckIfThereIsCollisionWithMotors()
{
	auto raise_exception_msg = []() 
	{
		wxString title = "Motor selection error";
		wxMessageBox(
			wxT
			(
				"There is a collision of motors"
				"\nYou have selected minimum 2 axis on the same motor ID"
				"\nPlease, change selection and try again"
			),
			title,
			wxICON_ERROR);
	};
	return false;
}

bool cSettings::CheckIfUserSelectedAllRangesForAllSelectedMotors()
{
	return true;
}

bool cSettings::CheckIfUserSelectedAllMotorsForAllSelectedRanges()
{	
	auto raise_exception_msg = []() 
	{
		wxString title = "Motor selection error";
		wxMessageBox(
			wxT
			(
				"You didn't select motors for all selected ranges"
				"\nPlease, select motors for all selected ranges and try again"
			),
			title,
			wxICON_ERROR);
	};
	return true;
}

void cSettings::OnCancelBtn(wxCommandEvent& evt)
{
	Hide();
	//SetPreviousStatesDataAsCurrentSelection();
}

unsigned int cSettings::FindSerialNumber
(
	const uint8_t selection_number, 
	const SettingsVariables::MotorSettings* motor_settings
) const
{
	wxString find_string{};
	find_string = m_Motors->unique_motors[0][selection_number];
	return (unsigned int)wxAtoi(find_string);
}

auto cSettings::CompareXMLWithConnectedDevices()
{
	auto raise_exception_msg = []() 
	{
		wxString title = "Device enumeration error";
		wxMessageBox(
			wxT
			(
				"Data file is not correct!"
				"\nData from file don't correspond with connected devices"
			),
			title,
			wxICON_ERROR);
	};


	auto physical_motors = m_PhysicalMotors->GetSerialNumbersWithRanges();
	unsigned short serial_numbers_in_xml = m_Motors->unique_motors_map.size();
	m_Motors->unique_motors_map.clear();
	for (const auto& motor : physical_motors)
	{
		m_Motors->unique_motors_map.emplace(motor);
	}

	auto default_state_of_motors = [&]()
	{
		m_Motors->xml_all_motors[0].Clear();
		m_Motors->xml_all_motors[1].Clear();
		m_Motors->xml_selected_motors[0].Clear();
		m_Motors->xml_selected_motors[1].Clear();

		wxString motor_sn{}, motor_range{};
		std::map<unsigned int, float>::iterator phys_mot_iter = physical_motors.begin();
		for (auto motor{ 0 }; motor < m_MotorsCount; ++motor)
		{
			if (motor < physical_motors.size())
			{
				motor_sn = wxString::Format(wxT("%i"), phys_mot_iter->first);
				motor_range = wxString::Format(wxT("%.2f"), phys_mot_iter->second);
				++phys_mot_iter;
			}
			else
			{
				motor_sn = "None";
				motor_range = "None";
			}
			m_Motors->xml_all_motors[0].Add(motor_sn);
			m_Motors->xml_all_motors[1].Add(motor_range);
		}
	};

	if (serial_numbers_in_xml != m_Motors->unique_motors_map.size() || serial_numbers_in_xml == 0)
	{
		m_Motors->unique_motors_map = physical_motors;
		default_state_of_motors();
		raise_exception_msg();
	}
}

auto cSettings::LoadWorkStationFiles() -> void
{
	std::string fileNameWithPath{};
	m_WorkStations->work_stations_count = 0;
	for (const auto& entry : std::filesystem::directory_iterator(work_stations_path.ToStdString()))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".xml")
		{
			++m_WorkStations->work_stations_count;
		}
	}
	m_WorkStations->work_station_data = std::make_unique<SettingsVariables::WorkStationData[]>(m_WorkStations->work_stations_count);

	auto i{ 0 };
	for (const auto& entry : std::filesystem::directory_iterator(work_stations_path.ToStdString())) 
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".xml") continue;

		fileNameWithPath = work_stations_path.ToStdString() + entry.path().filename().string();
		ReadWorkStationFile(fileNameWithPath, i);

		++i;
	}
}

auto cSettings::ReadWorkStationFile(const std::string& fileName, const int fileNum) -> void
{
	auto xmlFile = std::make_unique<rapidxml::file<>>(fileName.c_str());
	auto document = std::make_unique<rapidxml::xml_document<>>();
	document->parse<0>(xmlFile->data());

	rapidxml::xml_node<>* selected_motors_node = document->first_node("selected_motors");
	if (!selected_motors_node) return;

	// Detector
	auto element = SettingsVariables::FindNode(selected_motors_node, "detector");
	if (element)
	{
		for (auto detector = element->first_node(); detector; detector = detector->next_sibling())
		{
			// SN
			auto node = detector->first_node();
			auto value = node->value();
			m_WorkStations->work_station_data[fileNum].selected_motors_in_data_file.Add(value);
			// Steps/mm
			auto steps_node = node->next_sibling();
			auto steps_per_mm = std::stoi(std::string(steps_node->value()));
			m_WorkStations->work_station_data[fileNum].motors_steps_per_mm.insert(std::make_pair(wxString(value), steps_per_mm));
			m_PhysicalMotors->SetStepsPerMMForTheMotor(value, steps_per_mm);
		}
	}

	// Optics
	element = SettingsVariables::FindNode(selected_motors_node, "optics");
	if (element)
	{
		for (auto optics = element->first_node(); optics; optics = optics->next_sibling())
		{
			// SN
			auto node = optics->first_node();
			auto value = node->value();
			m_WorkStations->work_station_data[fileNum].selected_motors_in_data_file.Add(value);
			// Steps/mm
			auto steps_node = node->next_sibling();
			auto steps_per_mm = std::stoi(std::string(steps_node->value()));
			m_WorkStations->work_station_data[fileNum].motors_steps_per_mm.insert(std::make_pair(wxString(value), steps_per_mm));
			m_PhysicalMotors->SetStepsPerMMForTheMotor(value, steps_per_mm);
		}
	}

	// Camera
	element = SettingsVariables::FindNode(selected_motors_node, "camera");
	if (element)
		m_WorkStations->work_station_data[fileNum].selected_camera_in_data_file = wxString(element->first_node()->value());

	// Station
	element = SettingsVariables::FindNode(selected_motors_node, "station");
	if (element)
	{
		auto stationName = wxString(element->first_node()->value());
		m_WorkStations->work_station_data[fileNum].work_station_name = stationName;
		m_WorkStations->all_work_station_array_str.Add(stationName);
	}
}

auto cSettings::ReadInitializationFile() -> void
{
	auto xmlFile = std::make_unique<rapidxml::file<>>(initialization_file_path.c_str());
	auto document = std::make_unique<rapidxml::xml_document<>>();
	document->parse<0>(xmlFile->data());
	rapidxml::xml_node<>* app_node = document->first_node("MMCam");

	auto element = SettingsVariables::FindNode(app_node, "crop_size_mm");
	if (element && element->first_node())
		m_CropSizeMM = std::atof(element->first_node()->value());

	element = SettingsVariables::FindNode(app_node, "crop_size_circle_mm");
	if (element && element->first_node())
		m_CropCircleSizeMM = std::atof(element->first_node()->value());

	element = SettingsVariables::FindNode(app_node, "pixel_size_um");
	if (element && element->first_node())
		m_PixelSizeUM = std::atof(element->first_node()->value());

	element = SettingsVariables::FindNode(app_node, "upload_report_folder");
	if (element && element->first_node())
		m_UploadReportFolder = element->first_node()->value();

	m_XRayImagesCaptions.Clear();

	for (auto i{ 0 }; i < 5; ++i)
	{
		m_XRayImagesCaptions.Add("");
		std::string tag_name = "xrayImage" + std::to_string(i + 1) + "Caption";
		element = SettingsVariables::FindNode(app_node, tag_name);
		if (element && element->first_node())
			m_XRayImagesCaptions[i] = element->first_node()->value();
	}

	element = SettingsVariables::FindNode(app_node, "work_station");
	if (element && element->first_node())
		m_WorkStations->initialized_work_station = wxString(element->first_node()->value());
}

void cSettings::ReadXMLFile()
{
}

void cSettings::UpdateUniqueArray()
{
	m_Motors->unique_motors[0].Clear();
	m_Motors->unique_motors[1].Clear();
	m_Motors->unique_motors[0].Add("None");
	m_Motors->unique_motors[1].Add("None");

	for (const auto& motor : m_Motors->unique_motors_map)
	{
		m_Motors->unique_motors[0].Add(wxString::Format(wxT("%i"), motor.first));
		m_Motors->unique_motors[1].Add(wxString::Format(wxT("%.2f"), motor.second));
	}
}

void cSettings::SelectMotorsAndRangesFromXMLFile()
{
	auto raise_exception_msg = []() 
	{
		wxString title = "Data file error";
		wxMessageBox(
			wxT
			(
				"Data file is not correct!"
				"\nSelected motors data don't correspond to all motors data"
			),
			title,
			wxICON_ERROR);
	};
}

void cSettings::SelectMotorsAndRangesOnWXChoice()
{
}

void cSettings::UpdatePreviousStatesData()
{
}

void cSettings::SetPreviousStatesDataAsCurrentSelection()
{
}

void cSettings::WriteActualSelectedMotorsAndRangesIntoXMLFile()
{
	//auto document = std::make_unique<rapidxml::xml_document<>>();
	//// Open *.xml file
	//std::ifstream xml_file(xml_file_path.mb_str());
	//// Preparing buffer
	//std::stringstream file_buffer;
	//file_buffer << xml_file.rdbuf();
	//xml_file.close();

	//std::string content(file_buffer.str());
	//document->parse<0 | rapidxml::parse_no_data_nodes>(&content[0]);
	//rapidxml::xml_node<>* motors_node = document->first_node("motors");

	//if (!motors_node)
	//	return;

	//int count{};
	//for (
	//	rapidxml::xml_node<>* motor = motors_node->first_node()->first_node(); 
	//	motor; 
	//	motor = motor->next_sibling()
	//	)
	//{
	//	motor->first_node()->value(m_Motors->xml_all_motors[0][count]);
	//	motor->first_node()->next_sibling()->value(m_Motors->xml_all_motors[1][count]);
	//	++count;
	//}


	//uint8_t position_in_unique_array{};
	//auto xml_parser = [&](rapidxml::xml_node<>* motor_array, const int& i) 
	//{
	//	if (i < m_MotorsCount / 2)
	//	{
	//		//position_in_unique_array = m_Motors->m_Detector[i].prev_selection[0];
	//		motor_array->first_node()->value(m_Motors->unique_motors[0][position_in_unique_array]);

	//		//position_in_unique_array = m_Motors->m_Detector[i].prev_selection[1];
	//		motor_array->first_node()->next_sibling()->value(m_Motors->unique_motors[1][position_in_unique_array]);
	//	}
	//	else
	//	{
	//		//position_in_unique_array = m_Motors->m_Optics[i - m_MotorsCount / 2].prev_selection[0];
	//		motor_array->first_node()->value(m_Motors->unique_motors[0][position_in_unique_array]);

	//		//position_in_unique_array = m_Motors->m_Optics[i - m_MotorsCount / 2].prev_selection[1];
	//		motor_array->first_node()->next_sibling()->value(m_Motors->unique_motors[1][position_in_unique_array]);
	//	}
	//};
	///* Changing values of selected motors data in XML file */
	//count = 0;
	//for (
	//	rapidxml::xml_node<>* detector = motors_node->first_node()->next_sibling()->first_node()->first_node(); 
	//	detector; 
	//	detector = detector->next_sibling()
	//	)
	//{
	//	xml_parser(detector, count);
	//	++count;
	//}
	//for (
	//	rapidxml::xml_node<>* optics = motors_node->first_node()->next_sibling()->first_node()->next_sibling()->first_node(); 
	//	optics; 
	//	optics = optics->next_sibling()
	//	)
	//{
	//	xml_parser(optics, count);
	//	++count;
	//}

	//// Save to file
	//std::ofstream out_file(xml_file_path.mb_str());
	//if (out_file.is_open())
	//{
	//	out_file << "<?xml version=\"1.0\"?>\n";
	//	out_file << *document;
	//	out_file.close();
	//}
	//document->clear();
}

void cSettings::ResetAllMotorsAndRangesInXMLFile()
{
	//auto document = std::make_unique<rapidxml::xml_document<>>();
	//// Open *.xml file
	//std::ifstream xml_file(xml_file_path.mb_str());
	//// Preparing buffer
	//std::stringstream file_buffer;
	//file_buffer << xml_file.rdbuf();
	//xml_file.close();

	//std::string content(file_buffer.str());
	//document->parse<0 | rapidxml::parse_no_data_nodes>(&content[0]);
	//rapidxml::xml_node<>* motors_node = document->first_node("motors");

	//if (!motors_node)
	//	return;

	//int count{};
	///* Changing values in all_motors node */
	//for (
	//	rapidxml::xml_node<>* motor = motors_node->first_node()->first_node(); 
	//	motor; 
	//	motor = motor->next_sibling()
	//	)
	//{
	//	motor->first_node()->value("None");
	//	motor->first_node()->next_sibling()->value("None");
	//	++count;
	//}


	//auto xml_parser = [&](rapidxml::xml_node<>* motor_array, const int& i) 
	//{
	//	if (i < m_MotorsCount / 2)
	//	{
	//		motor_array->first_node()->value("None");
	//		motor_array->first_node()->next_sibling()->value("None");
	//	}
	//	else
	//	{
	//		motor_array->first_node()->value("None");
	//		motor_array->first_node()->next_sibling()->value("None");
	//	}
	//};
	///* Changing values in selected_motors node */
	//count = 0;
	//for (
	//	rapidxml::xml_node<>* detector = motors_node->first_node()->next_sibling()->first_node()->first_node(); 
	//	detector; 
	//	detector = detector->next_sibling()
	//	)
	//{
	//	xml_parser(detector, count);
	//	++count;
	//}
	//for (
	//	rapidxml::xml_node<>* optics = motors_node->first_node()->next_sibling()->first_node()->next_sibling()->first_node(); 
	//	optics; 
	//	optics = optics->next_sibling()
	//	)
	//{
	//	xml_parser(optics, count);
	//	++count;
	//}

	//// Save to file
	//std::ofstream out_file(xml_file_path.mb_str());
	//if (out_file.is_open())
	//{
	//	out_file << "<?xml version=\"1.0\"?>\n";
	//	out_file << *document;
	//	out_file.close();
	//}
	//document->clear();
}

auto cSettings::RewriteInitializationFile() -> void
{
	auto xmlFile = std::make_unique<rapidxml::file<>>(initialization_file_path.c_str());
	auto document = std::make_unique<rapidxml::xml_document<>>();
	// Open *.xml file
	//std::ifstream ini_file(initialization_file_path.mb_str());
	// Preparing buffer
	//std::stringstream file_buffer;
	//file_buffer << ini_file.rdbuf();
	//ini_file.close();

	//std::string content(file_buffer.str());
	//document->parse<0 | rapidxml::parse_no_data_nodes>(&content[0]);

	document->parse<0>(xmlFile->data());
	rapidxml::xml_node<>* app_node = document->first_node("MMCam");

	auto element = SettingsVariables::FindNode(app_node, "work_station");

	rapidxml::xml_node<>* work_station_node{};
	if (element && element->first_node())
		work_station_node = element;
	else
		return;

#ifndef _DEBUG
	work_station_node->first_node()->value(m_WorkStations->initialized_work_station.c_str());
#endif // !_DEBUG

	// Save to file
	std::ofstream out_file(initialization_file_path.mb_str());
	if (out_file.is_open())
	{
		out_file << "<?xml version=\"1.0\"?>\n";
		out_file << *document;
		out_file.close();
	}
	document->clear();
}
