#include "cSettings.h"

cSettings::cSettings(wxWindow* parent_frame) 
	: wxDialog
	(
		parent_frame, 
		wxID_ANY, 
		"Settings", 
		wxDefaultPosition, 
		wxDefaultSize 
	)
		//wxDEFAULT_DIALOG_STYLE)
{
	CreateMainFrame();
	InitDefaultStateWidgets();
	CenterOnScreen();
}

int cSettings::ShowModal()
{
	int result = wxID_OK;

	// Choosing a work station in the combo box changes the active station immediately.
	// Remember the current one so that Cancel really discards the change; otherwise the
	// motors were rebuilt for the new station while the camera/pixel size stayed on the old one.
	const auto previousWorkStation = m_WorkStations->initialized_work_station_num;

	// Start with the OK button focused: Enter confirms and Esc cancels. With the work
	// station drop-down focused, the native combo box swallowed both keys.
	CallAfter([this]()
		{
			if (auto okBtn = FindWindow(wxID_OK))
				okBtn->SetFocus();
		});

	result = wxDialog::ShowModal();  // Call the base class method

	if (result != wxID_OK && m_WorkStations->initialized_work_station_num != previousWorkStation)
	{
		m_WorkStations->initialized_work_station_num = previousWorkStation;
		m_WorkStations->work_station_choice->SetSelection(previousWorkStation);
		UpdateMotorsAndCameraTXTCtrls(previousWorkStation);
	}

	wxBusyCursor busy;

	// Instantiate drivers only if needed (at least one motor of that vendor exists)
	const auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
	bool needStanda = false, needXeryon = false;

	for (auto& sn : ws.selected_motors_in_data_file) {
		auto it = ws.motor_vendor_by_sn.find(sn);
		auto vendor = (it != ws.motor_vendor_by_sn.end()) ? it->second : SettingsVariables::STANDA;
		needStanda |= (vendor == SettingsVariables::STANDA);
		needXeryon |= (vendor == SettingsVariables::XERYON);
	}

	if (needStanda)
	{
		// Empty address => StandaMotorArray probes USB/COM only, same as before.
		// Non-empty address => it also enumerates Standa devices over the network.
		m_StandaMotors = std::make_unique<StandaMotorArray>(ws.standaIPAddress.ToStdString());
	}
	if (needXeryon)
	{
		m_XeryonMotors = std::make_unique<XeryonMotorArray>();
		InitializeXeryonAndCheckPython();
	}

	SetMotorStepsPerMM();

	return result;
}

auto cSettings::SetMotorStepsPerMM() -> void
{
	auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];

	for (size_t i = 0; i < ws.selected_motors_in_data_file.size(); ++i) 
	{
		const wxString sn = ws.selected_motors_in_data_file[i];
		const int steps = ws.motors_steps_per_mm[sn];

		if (sn != "None") 
		{
			if (auto* arr = WhichArrayFor(sn)) {
				arr->SetStepsPerMMForTheMotor(sn.ToStdString(), steps);
			}
		}
	}
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

bool cSettings::StopAllMotors()
{
	bool stoppedAny = false;

	if (m_StandaMotors)
		stoppedAny |= m_StandaMotors->StopAll();

	if (m_XeryonMotors)
		stoppedAny |= m_XeryonMotors->StopAll();

	// XeryonMotor::Stop() reads back the true post-stop position; persist it so
	// the next start does not restore the stale value the move thread wrote.
	if (stoppedAny)
		PrepareStagesDataAndWriteThemIntoJSONFile();

	return stoppedAny;
}

auto cSettings::GetSelectedCamera() const -> wxString
{
	return m_Camera->selectedCameraIDStr;
}

void cSettings::CreateMainFrame()
{
	InitComponents();
	LoadWorkStationFiles();

	CreateSettings();
	BindControls();
	UpdateMotorsAndCameraTXTCtrls();
}

void cSettings::CreateSettings()
{
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ms_sizer = new wxBoxSizer(wxVERTICAL);
	
	CreateMotorsSelection(ms_sizer);

	//CreateOtherSettings(ms_sizer);

	main_sizer->Add(ms_sizer, 1, wxEXPAND);

	// Standard OK/Cancel row, separated from the content by a thin line
	auto btnSizer = CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxOK_DEFAULT);
	if (btnSizer)
		main_sizer->Add(btnSizer, 0, wxEXPAND | wxALL, FromDIP(12));

	if (auto okBtn = FindWindow(wxID_OK))
		okBtn->SetToolTip("Apply the selected work station (motors and camera are re-initialized)");
	if (auto cancelBtn = FindWindow(wxID_CANCEL))
		cancelBtn->SetToolTip("Close without changing the work station");

	SetSizerAndFit(main_sizer);
	SetMinSize(GetSize());
}

void cSettings::CreateMotorsSelection(wxBoxSizer* panel_sizer)
{	
	m_MainPanel = std::make_unique<wxPanel>(this);

	wxBoxSizer* main_panel_sizer = new wxBoxSizer(wxVERTICAL);

	/* Header: title + short explanation */
	{
		auto title = new wxStaticText(m_MainPanel.get(), wxID_ANY, "Work station");
		title->SetFont(title->GetFont().Larger().Larger().Bold());
		main_panel_sizer->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(12));

		auto hint = new wxStaticText
		(
			m_MainPanel.get(), 
			wxID_ANY, 
			"Choose the instrument configuration. Motors and the camera listed below\n"
			"are taken from the selected work station file."
		);
		main_panel_sizer->Add(hint, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
	}

	/* Work Station */
	{
		m_WorkStations->work_station_choice = new wxChoice
		(
			m_MainPanel.get(), 
			SettingsVariables::ID::WORK_STATION_CHOICE, 
			wxDefaultPosition, 
			wxDefaultSize,
			m_WorkStations->all_work_station_array_str
		);
		m_WorkStations->work_station_choice->SetSelection(m_WorkStations->initialized_work_station_num);
		m_WorkStations->work_station_choice->SetToolTip("Work station configuration file (.json files in the src folder)");

		main_panel_sizer->Add(m_WorkStations->work_station_choice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
	}

	/* Section caption for motors */
	{
		auto motorsCaption = new wxStaticText(m_MainPanel.get(), wxID_ANY, "Motors");
		motorsCaption->SetFont(motorsCaption->GetFont().Bold());
		main_panel_sizer->Add(motorsCaption, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
	}

	/* Motors */
	auto motorTxtCtrlSize = wxSize(150, 24);
	int topOffsetStaticText{ 5 };

	auto size = wxSize(16, 16);
	auto imgList = new wxImageList(size.GetWidth(), size.GetHeight());

	int detectorImgIndex{}, opticsImgIndex{}, auxImgIndex{};

	// Detector
	{
		auto bitmap = wxART_CAMERA;
		auto client = wxART_CLIENT_MATERIAL_FILLED;
		auto color = wxColour(128, 0, 255);

		auto bmp = wxMaterialDesignArtProvider::GetBitmap(bitmap, client, size, color);
		detectorImgIndex = imgList->Add(bmp);
	}

	// Optics
	{
		auto bitmap = wxART_CIRCLE_HINT;
		auto client = wxART_CLIENT_FLUENTUI_FILLED;
		auto color = wxColour(255, 128, 128);

		auto bmp = wxMaterialDesignArtProvider::GetBitmap(bitmap, client, size, color);
		opticsImgIndex = imgList->Add(bmp);
	}

	// Aux
	{
		auto bitmap = wxART_AUTO_AWESOME_MOTION;
		auto client = wxART_CLIENT_MATERIAL_ROUND;
		auto color = wxColour(181, 230, 29);

		auto bmp = wxMaterialDesignArtProvider::GetBitmap(bitmap, client, size, color);
		auxImgIndex = imgList->Add(bmp);
	}

	m_MotorsNotebook = new wxNotebook(m_MainPanel.get(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);

	m_MotorsNotebook->AssignImageList(imgList);

	m_MotorsNotebook->AddPage
	(
		CreateDetectorPage(m_MotorsNotebook, motorTxtCtrlSize, topOffsetStaticText), 
		"Detector", 
		true, 
		detectorImgIndex
	);

	m_MotorsNotebook->AddPage
	(
		CreateOpticsPage(m_MotorsNotebook, motorTxtCtrlSize, topOffsetStaticText), 
		"Optics", 
		false, 
		opticsImgIndex
	);

	m_MotorsNotebook->AddPage
	(
		CreateAuxPage(m_MotorsNotebook, motorTxtCtrlSize, topOffsetStaticText), 
		"Aux", 
		false, 
		auxImgIndex
	);

	main_panel_sizer->Add(m_MotorsNotebook, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));

	//wxSizer* const motors_static_box_sizer = new wxStaticBoxSizer(wxVERTICAL, m_MainPanel.get(), "&Motors");
	{
		//CreateDetectorGroup(m_MainPanel.get(), motors_static_box_sizer, motorTxtCtrlSize, topOffsetStaticText);

		//CreateOpticsGroup(m_MainPanel.get(), motors_static_box_sizer, motorTxtCtrlSize, topOffsetStaticText);

		//CreateAuxGroup(m_MainPanel.get(), motors_static_box_sizer, motorTxtCtrlSize, topOffsetStaticText);
	}
	//main_panel_sizer->Add(motors_static_box_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);

	/* Camera */
	CreateCameraSection(m_MainPanel.get(), main_panel_sizer);

	main_panel_sizer->AddStretchSpacer();

	m_MainPanel->SetSizer(main_panel_sizer);
	panel_sizer->Add(m_MainPanel.get(), 1, wxEXPAND);
}

auto cSettings::CreateMotorsTablePage
(
	wxWindow* parent,
	SettingsVariables::MotorSettings* motors,
	const int firstID
) -> wxWindow*
{
	auto page = new wxPanel(parent);
	auto sizerPage = new wxBoxSizer(wxVERTICAL);

	// Three rows (X, Y, Z) laid out as a simple table: Axis | Serial number | Steps/mm
	auto grid = new wxFlexGridSizer(3, FromDIP(wxSize(12, 8)));
	grid->AddGrowableCol(1, 1);

	auto makeHeader = [page](const wxString& text)
		{
			auto st = new wxStaticText(page, wxID_ANY, text);
			st->SetFont(st->GetFont().Smaller().Bold());
			return st;
		};

	grid->Add(makeHeader("Axis"), 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(makeHeader("Serial number"), 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(makeHeader("Steps / mm"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);

	const wxString axisNames[3] = { "X", "Y", "Z" };
	for (int i = 0; i < 3; ++i)
	{
		auto axisLabel = new wxStaticText(page, wxID_ANY, axisNames[i]);
		axisLabel->SetFont(axisLabel->GetFont().Larger().Bold());
		grid->Add(axisLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(6));

		motors[i].motor = new wxTextCtrl
		(
			page,
			firstID + 2 * i,
			wxT("None"),
			wxDefaultPosition,
			FromDIP(wxSize(170, -1)),
			wxTE_CENTRE | wxTE_READONLY
		);
		motors[i].motor->SetToolTip("Serial number of the motor assigned to this axis (from the work station file)");
		grid->Add(motors[i].motor, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

		motors[i].steps_per_mm = new wxStaticText
		(
			page,
			firstID + 2 * i + 1,
			wxT("None"),
			wxDefaultPosition,
			FromDIP(wxSize(70, -1)),
			wxALIGN_RIGHT | wxST_NO_AUTORESIZE
		);
		grid->Add(motors[i].steps_per_mm, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxRIGHT, FromDIP(6));
	}

	sizerPage->Add(grid, 0, wxEXPAND | wxALL, FromDIP(12));

	page->SetSizer(sizerPage);
	return page;
}

auto cSettings::CreateDetectorPage(wxWindow* parent, const wxSize&, const int&) -> wxWindow*
{
	return CreateMotorsTablePage(parent, m_Motors->m_Detector.get(), SettingsVariables::ID::MOT_DET_X_MOTOR_TXT_CTRL);
}

auto cSettings::CreateOpticsPage(wxWindow* parent, const wxSize&, const int&) -> wxWindow*
{
	return CreateMotorsTablePage(parent, m_Motors->m_Optics.get(), SettingsVariables::ID::MOT_OPT_X_MOTOR_TXT_CTRL);
}

auto cSettings::CreateAuxPage(wxWindow* parent, const wxSize&, const int&) -> wxWindow*
{
	return CreateMotorsTablePage(parent, m_Motors->m_Aux.get(), SettingsVariables::ID::MOT_AUX_X_MOTOR_TXT_CTRL);
}

auto cSettings::CreateCameraSection(wxPanel* panel, wxBoxSizer* panel_sizer) -> void
{
	auto cameraStaticBoxSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Camera");
	auto box = cameraStaticBoxSizer->GetStaticBox();

	auto label = new wxStaticText(box, wxID_ANY, "Serial number / ID:");
	cameraStaticBoxSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(6));

	m_Camera->idTxtCtrl = new wxTextCtrl
	(
		box,
		SettingsVariables::ID::CAM_ID_TXT_CTRL,
		wxT("None"),
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_CENTRE | wxTE_READONLY
	);
	m_Camera->idTxtCtrl->SetToolTip("Camera assigned to the selected work station");

	for (auto i{ 0 }; i < m_WorkStations->work_stations_count; ++i)
	{
		if (m_WorkStations->work_station_data[i].work_station_name == m_WorkStations->initialized_work_station)
		{
			m_Camera->selectedCameraIDStr = m_WorkStations->work_station_data[i].selected_camera_in_data_file;
			m_WorkStations->initialized_work_station_num = i;
			break;
		}
	}
	m_Camera->idTxtCtrl->SetValue(m_Camera->selectedCameraIDStr);

	cameraStaticBoxSizer->Add(m_Camera->idTxtCtrl, 1, wxEXPAND | wxALL, FromDIP(6));

	panel_sizer->Add(cameraStaticBoxSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
}

auto cSettings::CreateOtherSettings(wxBoxSizer* panel_sizer) -> void
{	
}

void cSettings::InitDefaultStateWidgets()
{
}

void cSettings::InitComponents()
{
	m_WorkStations = std::make_unique<SettingsVariables::WorkStations>();
	m_Motors = std::make_unique<SettingsVariables::MotorSettingsArray>();
	m_Camera = std::make_unique<SettingsVariables::Camera>();
}

void cSettings::BindControls()
{
	m_WorkStations->work_station_choice->Bind(wxEVT_CHOICE, &cSettings::OnWorkStationChoice, this);
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
	const auto stationCount = static_cast<int>(m_WorkStations->work_stations_count);
	if (stationCount == 0)
	{
		wxMessageBox(
			"No workstation JSON configuration files were found in \"" + work_stations_path + "\".",
			"Workstation configuration error",
			wxOK | wxICON_ERROR,
			this);
		return;
	}

	int stationIndex = selected_work_station;
	if (stationIndex < 0)
		stationIndex = static_cast<int>(m_WorkStations->initialized_work_station_num);

	if (stationIndex < 0 || stationIndex >= stationCount)
	{
		wxMessageBox(
			wxString::Format("Invalid workstation index %d. Valid range: 0..%d.", stationIndex, stationCount - 1),
			"Workstation selection error",
			wxOK | wxICON_ERROR,
			this);
		stationIndex = 0;
	}

	m_WorkStations->initialized_work_station_num = static_cast<unsigned short>(stationIndex);
	m_WorkStations->work_station_choice->SetSelection(stationIndex);

	auto& workStation = m_WorkStations->work_station_data[stationIndex];
	m_WorkStations->initialized_work_station = workStation.work_station_name;

	SettingsVariables::MotorSettings* const controlGroups[] =
	{
		m_Motors->m_Detector.get(),
		m_Motors->m_Optics.get(),
		m_Motors->m_Aux.get()
	};

	for (size_t group = 0; group < 3; ++group)
	{
		for (size_t axis = 0; axis < 3; ++axis)
		{
			const size_t motorIndex = group * 3 + axis;
			auto& control = controlGroups[group][axis];

			const wxString motorSN = motorIndex < workStation.selected_motors_in_data_file.size()
				? workStation.selected_motors_in_data_file[motorIndex]
				: wxString("None");

			wxString stepsLabel = "None";
			if (!motorSN.IsEmpty() && motorSN != "None")
			{
				const auto stepsIt = workStation.motors_steps_per_mm.find(motorSN);
				if (stepsIt != workStation.motors_steps_per_mm.end())
					stepsLabel = wxString::Format("%d", stepsIt->second);
			}

			if (control.motor)
				control.motor->SetValue(motorSN.IsEmpty() ? wxString("None") : motorSN);
			if (control.steps_per_mm)
				control.steps_per_mm->SetLabel(stepsLabel);
			control.motor_sn = motorSN.IsEmpty() ? wxString("None") : motorSN;
		}
	}

	m_Camera->idTxtCtrl->SetValue(workStation.selected_camera_in_data_file);
	m_Camera->selectedCameraIDStr = workStation.selected_camera_in_data_file;
	m_PixelSizeUM = workStation.pixelSizeUM;
	m_CameraManufacturer = workStation.camera_manufacturer;

	if (!workStation.configuration_errors.IsEmpty() && !workStation.configuration_error_was_shown)
	{
		wxString message = "The application loaded the workstation with safe placeholder values, but the configuration must be corrected.\n\nFile: "
			+ workStation.config_file_path + "\n\n";

		for (const auto& error : workStation.configuration_errors)
			message += "- " + error + "\n";

		message += "\nEach motor section must contain exactly three entries in X, Y, Z order. Missing or invalid entries are shown as None; extra entries are ignored.";

		wxMessageBox(message, "Workstation configuration error", wxOK | wxICON_ERROR, this);
		workStation.configuration_error_was_shown = true;
	}

	m_MotorsNotebook->Layout();
}

void cSettings::OnRefreshBtn(wxCommandEvent& evt)
{
	wxBusyCursor busy_cursor{};

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

auto cSettings::LoadWorkStationFiles() -> void
{
	std::string fileNameWithPath{};
	m_WorkStations->work_stations_count = 0;

	// Collect the file names once; the error_code overload does not throw when
	// the src folder is missing (previously an uncaught filesystem_error on startup).
	std::vector<std::filesystem::path> jsonFiles;
	{
		std::error_code ec;
		for (std::filesystem::directory_iterator it(work_stations_path.ToStdString(), ec), end; !ec && it != end; it.increment(ec))
		{
			std::error_code fileEc;
			if (it->is_regular_file(fileEc) && it->path().extension() == ".json")
				jsonFiles.push_back(it->path());
		}
	}

	if (jsonFiles.empty())
	{
		// Every accessor indexes work_station_data[initialized_work_station_num];
		// provide one placeholder station so nothing reads out of bounds.
		m_WorkStations->work_stations_count = 1;
		m_WorkStations->work_station_data = std::make_unique<SettingsVariables::WorkStationData[]>(1);

		auto& placeholder = m_WorkStations->work_station_data[0];
		placeholder.work_station_name = "None";
		placeholder.selected_camera_in_data_file = "None";
		for (int m = 0; m < 9; ++m)
			placeholder.selected_motors_in_data_file.Add("None");
		placeholder.configuration_errors.Add("No work station files (*.json) were found in \"" + work_stations_path + "\".");

		m_WorkStations->all_work_station_array_str.Add(placeholder.work_station_name);
		m_WorkStations->initialized_work_station_num = 0;
		return;
	}

	m_WorkStations->work_stations_count = static_cast<unsigned short>(jsonFiles.size());
	m_WorkStations->work_station_data = std::make_unique<SettingsVariables::WorkStationData[]>(m_WorkStations->work_stations_count);

	auto i{ 0 };
	for (const auto& path : jsonFiles)
	{
		fileNameWithPath = work_stations_path.ToStdString() + path.filename().string();
		ReadWorkStationFile(fileNameWithPath, i);

		++i;
	}

	if (m_WorkStations->initialized_work_station_num >= m_WorkStations->work_stations_count)
		m_WorkStations->initialized_work_station_num = 0;
}

auto cSettings::ReadWorkStationFile(const std::string& fileName, const int fileNum) -> void
{
	auto& workStation = m_WorkStations->work_station_data[fileNum];
	workStation.config_file_path = wxString::FromUTF8(fileName);
	workStation.work_station_name = wxString::FromUTF8(std::filesystem::path(fileName).stem().string());

	auto addEmptyMotorSlots = [&workStation]()
	{
		while (workStation.selected_motors_in_data_file.size() < 9)
			workStation.selected_motors_in_data_file.Add("None");
	};

	std::ifstream fileStream(fileName);
	if (!fileStream.is_open())
	{
		workStation.configuration_errors.Add("The file could not be opened.");
		addEmptyMotorSlots();
		m_WorkStations->all_work_station_array_str.Add(workStation.work_station_name);
		return;
	}

	nlohmann::json j;
	try
	{
		fileStream >> j;
	}
	catch (const nlohmann::json::exception& exception)
	{
		workStation.configuration_errors.Add(
			"Malformed JSON: " + wxString::FromUTF8(exception.what()));
		addEmptyMotorSlots();
		m_WorkStations->all_work_station_array_str.Add(workStation.work_station_name);
		return;
	}

	SettingsVariables::MotorManufacturers fallbackVendor = SettingsVariables::MotorManufacturers::STANDA;
	if (j.contains("motor_manufacturer"))
	{
		if (j["motor_manufacturer"].is_string())
			fallbackVendor = SettingsVariables::ParseVendor(j["motor_manufacturer"].get<std::string>());
		else
			workStation.configuration_errors.Add("\"motor_manufacturer\" must be a string.");
	}

	auto readMotorSection = [&](const char* sectionName)
	{
		constexpr size_t expectedMotorCount = 3;
		const wxString section = wxString::FromUTF8(sectionName);

		if (!j.contains(sectionName) || !j[sectionName].is_array())
		{
			workStation.configuration_errors.Add(
				"\"" + section + "\" must be an array containing exactly 3 entries; found 0.");
			for (size_t i = 0; i < expectedMotorCount; ++i)
				workStation.selected_motors_in_data_file.Add("None");
			return;
		}

		const auto& motors = j[sectionName];
		if (motors.size() != expectedMotorCount)
		{
			const wxString actualCount = wxString::Format(
				"%llu",
				static_cast<unsigned long long>(motors.size()));

			if (motors.size() < expectedMotorCount)
			{
				static const char* axisNames[] = { "X", "Y", "Z" };
				wxString missingAxes;
				for (size_t missing = motors.size(); missing < expectedMotorCount; ++missing)
				{
					if (!missingAxes.IsEmpty())
						missingAxes += ", ";
					missingAxes += wxString::FromUTF8(axisNames[missing]) + " (index "
						+ wxString::Format("%llu", static_cast<unsigned long long>(missing)) + ")";
				}

				workStation.configuration_errors.Add(
					"\"" + section + "\" contains " + actualCount
					+ " entries; missing " + missingAxes + ". Expected X, Y and Z.");
			}
			else
			{
				workStation.configuration_errors.Add(
					"\"" + section + "\" contains " + actualCount
					+ " entries; extras begin at " + section + "[3] and are ignored. Expected exactly X, Y and Z.");
			}
		}

		for (size_t position = 0; position < expectedMotorCount; ++position)
		{
			if (position >= motors.size())
			{
				workStation.selected_motors_in_data_file.Add("None");
				continue;
			}

			const auto& motor = motors[position];
			const wxString itemPath = section + "["
				+ wxString::Format("%llu", static_cast<unsigned long long>(position)) + "]";
			if (!motor.is_object())
			{
				workStation.configuration_errors.Add(itemPath + " must be a JSON object.");
				workStation.selected_motors_in_data_file.Add("None");
				continue;
			}

			if (!motor.contains("SerialNumber") || !motor["SerialNumber"].is_string())
			{
				workStation.configuration_errors.Add(itemPath + ".SerialNumber is missing or is not a string.");
				workStation.selected_motors_in_data_file.Add("None");
				continue;
			}

			if (!motor.contains("StepsPerMM") || !motor["StepsPerMM"].is_number_integer())
			{
				workStation.configuration_errors.Add(itemPath + ".StepsPerMM is missing or is not an integer.");
				workStation.selected_motors_in_data_file.Add("None");
				continue;
			}

			const wxString serialNumber = wxString::FromUTF8(motor["SerialNumber"].get<std::string>());
			const int stepsPerMM = motor["StepsPerMM"].get<int>();
			auto vendor = fallbackVendor;

			if (motor.contains("Manufacturer"))
			{
				if (motor["Manufacturer"].is_string())
					vendor = SettingsVariables::ParseVendor(motor["Manufacturer"].get<std::string>());
				else
					workStation.configuration_errors.Add(itemPath + ".Manufacturer must be a string.");
			}

			workStation.selected_motors_in_data_file.Add(serialNumber.IsEmpty() ? wxString("None") : serialNumber);
			if (!serialNumber.IsEmpty() && serialNumber != "None")
			{
				workStation.motor_vendor_by_sn.insert_or_assign(serialNumber, vendor);
				workStation.motors_steps_per_mm.insert_or_assign(serialNumber, stepsPerMM);
			}
		}
	};

	readMotorSection("detector");
	readMotorSection("optics");
	readMotorSection("aux");

	// Standa IP address (optional; absent/empty => USB-only for this station)
	if (j.contains("standa_ip")) {
		if (!j["standa_ip"].is_string()) {
			workStation.configuration_errors.Add("\"standa_ip\" must be a string.");
		}
		else {
		const std::string ip = j["standa_ip"].get<std::string>();
		if (!ip.empty()) {
			if (SettingsVariables::IsValidIPAddress(ip))
				workStation.standaIPAddress = wxString(ip);
			else
				workStation.configuration_errors.Add("\"standa_ip\" is not a valid IPv4 address.");
		}
		}
	}

	// Camera
	if (j.contains("camera") && j["camera"].is_string())
		workStation.selected_camera_in_data_file = wxString::FromUTF8(j["camera"].get<std::string>());
	else
		workStation.configuration_errors.Add("\"camera\" is missing or is not a string.");

	// CameraManufacturer
	if (j.contains("camera_manufacturer") && j["camera_manufacturer"].is_string()) {
		const wxString cameraManufacturerStr = wxString(j["camera_manufacturer"].get<std::string>());
		if (cameraManufacturerStr.Lower() == "advacam")
			workStation.camera_manufacturer = SettingsVariables::CameraManufacturers::ADVACAM;
		else if (cameraManufacturerStr.Lower() == "moravian_instruments")
			workStation.camera_manufacturer = SettingsVariables::CameraManufacturers::MORAVIAN_INSTRUMENTS;
		else if (cameraManufacturerStr.Lower() == "tucsen")
			workStation.camera_manufacturer = SettingsVariables::CameraManufacturers::TUCSEN;
		else if (cameraManufacturerStr.Lower() == "ximea")
			workStation.camera_manufacturer = SettingsVariables::CameraManufacturers::XIMEA;
		else
			workStation.configuration_errors.Add("\"camera_manufacturer\" has unsupported value \"" + cameraManufacturerStr + "\".");
	}
	else
		workStation.configuration_errors.Add("\"camera_manufacturer\" is missing or is not a string.");

	// PixelSizeUM
	if (j.contains("pixel_size_um") && j["pixel_size_um"].is_number() && j["pixel_size_um"].get<double>() > 0.0)
		workStation.pixelSizeUM = j["pixel_size_um"].get<double>();
	else
		// 0 would later be used as a divisor (report generation, scale bar)
		workStation.configuration_errors.Add("\"pixel_size_um\" is missing, not a number, or not greater than 0.");

	// Station
	if (j.contains("station") && j["station"].is_string()) {
		const wxString stationName = wxString(j["station"].get<std::string>());
		if (!stationName.IsEmpty())
			workStation.work_station_name = stationName;
	}
	else
		workStation.configuration_errors.Add("\"station\" is missing or is not a string; the file name is used instead.");

	m_WorkStations->all_work_station_array_str.Add(workStation.work_station_name);
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

auto cSettings::InitializeXeryonAndCheckPython() -> void
{
	wxBusyCursor busy;

	ReadStagePositionsFromJSONFile();

	// Checking for Python and modules
	if (!IsPythonInstalled())
	{
		wxLogError("Python is not installed on current machine");
	}

	std::vector<wxString> missing = GetMissingPythonModules("requirements.txt");
	if (!missing.empty())
	{
		wxString message = "Missing modules:\n";
		for (const auto& mod : missing)
		{
			message += mod + "\n";
		}
		wxLogMessage(message);
	}
}

auto cSettings::ReadStagePositionsFromJSONFile() -> void
{
	auto stages = ReadJson(m_StagesPositionsFilename);
	if (stages.empty()) return;

	const auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
	for (auto& sn : ws.selected_motors_in_data_file) 
	{
		if (sn == "None") continue;

		const auto it = ws.motor_vendor_by_sn.find(sn);
		if (it == ws.motor_vendor_by_sn.end()) continue; // vendor unknown -> skip
		if (it->second != SettingsVariables::XERYON) continue; // only XERYON

		for (const auto& stage : stages) {
			if (stage.SerialNumber == sn.ToStdString()) {
				if (m_XeryonMotors)
					m_XeryonMotors->SetCurrentPositionForTheMotor(sn.ToStdString(), stage.LastKnownPosition);
				break;
			}
		}
	}
}

auto cSettings::PrepareStagesDataAndWriteThemIntoJSONFile() -> void
{
	// Nothing to update: don't overwrite the stored Xeryon positions (previously a
	// Standa-only station wiped the whole file on every move).
	if (!m_XeryonMotors) return;

	// Merge into the existing file instead of replacing it: only motors that are
	// selected in the current work station have a trustworthy position (unselected
	// ones are never restored and would otherwise be stored as 0).
	auto out = ReadJson(m_StagesPositionsFilename);

	const auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
	auto isSelected = [&ws](const std::string& sn)
		{
			for (const auto& selected : ws.selected_motors_in_data_file)
				if (selected.ToStdString() == sn) return true;
			return false;
		};

	auto serials = m_XeryonMotors->GetSerialNumbersWithRanges();
	for (auto& s : serials)
	{
		if (!isSelected(s.first)) continue;

		auto it = std::find_if(out.begin(), out.end(),
			[&s](const SettingsVariables::Stage& st) { return st.SerialNumber == s.first; });
		if (it == out.end())
		{
			out.emplace_back();
			it = std::prev(out.end());
			it->SerialNumber = s.first;
		}
		it->COMPort = m_XeryonMotors->GetMotorCOMPort(s.first);
		it->LastKnownPosition = m_XeryonMotors->GetActualStagePos(s.first);
	}
	WriteJson(m_StagesPositionsFilename, out);
}

auto cSettings::SetBackgroundColor(const wxColour& color) -> void
{
	m_MotorsNotebook->SetBackgroundColour(color);
	
	SetBackgroundColour(color);

	Refresh();
}
