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

	result = wxDialog::ShowModal();  // Call the base class method
#ifndef _DEBUG
#endif // !_DEBUG

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

	auto btnSizer = CreateButtonSizer(wxOK | wxCANCEL | wxOK_DEFAULT);
	main_sizer->Add(btnSizer, 0, wxCENTER | wxALL, 5);
	SetSizerAndFit(main_sizer);
}

void cSettings::CreateMotorsSelection(wxBoxSizer* panel_sizer)
{	
	m_MainPanel = std::make_unique<wxPanel>(this);

	wxBoxSizer* main_panel_sizer = new wxBoxSizer(wxVERTICAL);

	/* Work Station */
	wxSizer* const work_station_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, m_MainPanel.get(), "&Work Station");
	{
		auto work_station_txt_ctrl_size = wxSize(120, 24);
		m_WorkStations->work_station_choice = new wxChoice
		(
			m_MainPanel.get(), 
			SettingsVariables::ID::WORK_STATION_CHOICE, 
			wxDefaultPosition, 
			wxDefaultSize,
			m_WorkStations->all_work_station_array_str
		);
		m_WorkStations->work_station_choice->SetSelection(m_WorkStations->initialized_work_station_num);

		work_station_static_box_sizer->Add(m_WorkStations->work_station_choice, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
	}
	main_panel_sizer->Add(work_station_static_box_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);

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

	main_panel_sizer->Add(m_MotorsNotebook, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);

	//wxSizer* const motors_static_box_sizer = new wxStaticBoxSizer(wxVERTICAL, m_MainPanel.get(), "&Motors");
	{
		//CreateDetectorGroup(m_MainPanel.get(), motors_static_box_sizer, motorTxtCtrlSize, topOffsetStaticText);

		//CreateOpticsGroup(m_MainPanel.get(), motors_static_box_sizer, motorTxtCtrlSize, topOffsetStaticText);

		//CreateAuxGroup(m_MainPanel.get(), motors_static_box_sizer, motorTxtCtrlSize, topOffsetStaticText);
	}
	//main_panel_sizer->Add(motors_static_box_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);

	/* Camera */
	CreateCameraSection(m_MainPanel.get(), main_panel_sizer);

	main_panel_sizer->AddSpacer(5);
	main_panel_sizer->AddStretchSpacer();

	m_MainPanel->SetSizer(main_panel_sizer);
	panel_sizer->Add(m_MainPanel.get(), 1, wxEXPAND);
}

auto cSettings::CreateDetectorPage(wxWindow* parent, const wxSize& txtCtrlSize, const int& topOffset) -> wxWindow*
{
	auto page = new wxPanel(parent);
	page->SetBackgroundColour(m_backgroundColour);
	auto sizerPage = new wxBoxSizer(wxVERTICAL);

	/* X */
	wxSizer* const det_x_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&X");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Detector[0].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_DET_X_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
		);
		
		sn_static_box_sizer->Add(m_Motors->m_Detector[0].motor);

		det_x_static_box_sizer->Add(sn_static_box_sizer);
	}
	/* Steps/mm */
	det_x_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Detector[0].steps_per_mm = new wxStaticText(
			page,
			SettingsVariables::ID::MOT_DET_X_STEPS_PER_MM_ST_TEXT,
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Detector[0].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		det_x_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}
	sizerPage->Add(det_x_static_box_sizer, 0, wxCENTRE);

	sizerPage->AddSpacer(2);
	sizerPage->AddStretchSpacer();

	/* Y */
	wxSizer* const det_y_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Y");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Detector[1].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_DET_Y_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		//m_Motors->m_Detector[1].motors->SetSelection(0);

		sn_static_box_sizer->Add(m_Motors->m_Detector[1].motor);

		det_y_static_box_sizer->Add(sn_static_box_sizer);
	}
	/* Steps/mm */
	det_y_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Detector[1].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_DET_Y_STEPS_PER_MM_ST_TEXT, 	
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize,
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Detector[1].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		det_y_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}
	sizerPage->Add(det_y_static_box_sizer, 0, wxCENTRE);
	sizerPage->AddSpacer(2);
	sizerPage->AddStretchSpacer();

	/* Z */
	wxSizer* const det_z_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Z");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Detector[2].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_DET_Z_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		//m_Motors->m_Detector[2].motors->SetSelection(0);

		sn_static_box_sizer->Add(m_Motors->m_Detector[2].motor);

		det_z_static_box_sizer->Add(sn_static_box_sizer);
	}
	/* Steps/mm */
	det_z_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Detector[2].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_DET_Z_STEPS_PER_MM_ST_TEXT, 	
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Detector[2].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		det_z_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}
	sizerPage->Add(det_z_static_box_sizer, 0, wxCENTRE);

	page->SetSizer(sizerPage);
	return page;
}

auto cSettings::CreateOpticsPage(wxWindow* parent, const wxSize& txtCtrlSize, const int& topOffset) -> wxWindow*
{
	auto page = new wxPanel(parent);
	auto sizerPage = new wxBoxSizer(wxVERTICAL);

	/* X */
	wxSizer* const opt_x_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&X");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Optics[0].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_OPT_X_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		//m_Motors->m_Optics[0].motors->SetSelection(0);

		sn_static_box_sizer->Add(m_Motors->m_Optics[0].motor);

		opt_x_static_box_sizer->Add(sn_static_box_sizer);
	}
	/* Steps/mm */
	opt_x_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Optics[0].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_OPT_X_STEPS_PER_MM_ST_TEXT, 	
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Optics[0].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		opt_x_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}
	sizerPage->Add(opt_x_static_box_sizer, 0, wxCENTRE);
	sizerPage->AddSpacer(2);
	sizerPage->AddStretchSpacer();

	/* Y */
	wxSizer* const opt_y_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Y");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Optics[1].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_OPT_Y_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		//m_Motors->m_Optics[1].motors->SetSelection(0);

		sn_static_box_sizer->Add(m_Motors->m_Optics[1].motor);

		opt_y_static_box_sizer->Add(sn_static_box_sizer);
	}
	/* Steps/mm */
	opt_y_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Optics[1].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_OPT_Y_STEPS_PER_MM_ST_TEXT, 
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Optics[1].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		opt_y_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}
	sizerPage->Add(opt_y_static_box_sizer, 0, wxCENTRE);
	sizerPage->AddSpacer(2);
	sizerPage->AddStretchSpacer();

	/* Z */
	wxSizer* const opt_z_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Z");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Optics[2].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_OPT_Z_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		//m_Motors->m_Optics[2].motors->SetSelection(0);

		sn_static_box_sizer->Add(m_Motors->m_Optics[2].motor);

		opt_z_static_box_sizer->Add(sn_static_box_sizer);
	}
	/* Steps/mm */
	opt_z_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Optics[2].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_OPT_Z_STEPS_PER_MM_ST_TEXT, 
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Optics[2].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		opt_z_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}
	sizerPage->Add(opt_z_static_box_sizer, 0, wxCENTRE);

	page->SetSizer(sizerPage);
	return page;
}

auto cSettings::CreateAuxPage(wxWindow* parent, const wxSize& txtCtrlSize, const int& topOffset) -> wxWindow*
{
	auto page = new wxPanel(parent);
	auto sizerPage = new wxBoxSizer(wxVERTICAL);

	/* X */
	wxSizer* const x_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&X");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Aux[0].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_AUX_X_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		//m_Motors->m_Optics[0].motors->SetSelection(0);

		sn_static_box_sizer->Add(m_Motors->m_Aux[0].motor);

		x_static_box_sizer->Add(sn_static_box_sizer);
	}

	/* Steps/mm */
	x_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Aux[0].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_AUX_X_STEPS_PER_MM_ST_TEXT, 	
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Aux[0].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		x_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}

	sizerPage->Add(x_static_box_sizer, 0, wxCENTRE);
	sizerPage->AddSpacer(2);
	sizerPage->AddStretchSpacer();

	/* Y */
	wxSizer* const opt_y_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Y");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Aux[1].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_AUX_Y_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		sn_static_box_sizer->Add(m_Motors->m_Aux[1].motor);

		opt_y_static_box_sizer->Add(sn_static_box_sizer);
	}

	/* Steps/mm */
	opt_y_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Aux[1].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_AUX_Y_STEPS_PER_MM_ST_TEXT, 
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Aux[1].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		opt_y_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}

	sizerPage->Add(opt_y_static_box_sizer, 0, wxCENTRE);
	sizerPage->AddSpacer(2);
	sizerPage->AddStretchSpacer();

	/* Z */
	wxSizer* const opt_z_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Z");
	/* Serial Number */
	{
		wxSizer* const sn_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&S/N");

		m_Motors->m_Aux[2].motor = new wxTextCtrl(
			page, 
			SettingsVariables::ID::MOT_AUX_Z_MOTOR_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
			//m_Motors->unique_motors[0]
		);

		//m_Motors->m_Aux[2].motors->SetSelection(0);

		sn_static_box_sizer->Add(m_Motors->m_Aux[2].motor);

		opt_z_static_box_sizer->Add(sn_static_box_sizer);
	}

	/* Steps/mm */
	opt_z_static_box_sizer->AddSpacer(2);
	{
		wxSizer* const range_static_box_sizer = new wxStaticBoxSizer(wxHORIZONTAL, page, "&Steps/mm");

		m_Motors->m_Aux[2].steps_per_mm = new wxStaticText(
			page, 
			SettingsVariables::ID::MOT_AUX_Z_STEPS_PER_MM_ST_TEXT, 
			wxT("None"), 
			wxDefaultPosition, 
			wxDefaultSize, 
			wxALIGN_CENTRE_HORIZONTAL);

		range_static_box_sizer->AddStretchSpacer();
		range_static_box_sizer->Add(m_Motors->m_Aux[2].steps_per_mm, 0, wxEXPAND | wxTOP, topOffset);
		range_static_box_sizer->AddStretchSpacer();

		opt_z_static_box_sizer->Add(range_static_box_sizer, 0, wxEXPAND);
	}

	sizerPage->Add(opt_z_static_box_sizer, 0, wxCENTRE);

	page->SetSizer(sizerPage);
	return page;
}

auto cSettings::CreateCameraSection(wxPanel* panel, wxBoxSizer* panel_sizer) -> void
{
	wxSizer* const cameraStaticBoxSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, "&Camera");

	// ID
	{
		auto idBoxSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, "&ID");
		auto txtCtrlSize = wxSize(80, 24);
		m_Camera->idTxtCtrl = new wxTextCtrl
		(
			panel, 
			SettingsVariables::ID::CAM_ID_TXT_CTRL, 
			wxT("None"),
			wxDefaultPosition, 
			txtCtrlSize,
			wxTE_CENTRE | wxTE_READONLY
		);

		{
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
		}

		idBoxSizer->Add(m_Camera->idTxtCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
		cameraStaticBoxSizer->Add(idBoxSizer, 1, wxEXPAND);
	}

	panel_sizer->Add(cameraStaticBoxSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 2);
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
	for (const auto& entry : std::filesystem::directory_iterator(work_stations_path.ToStdString()))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".json")
		{
			++m_WorkStations->work_stations_count;
		}
	}
	m_WorkStations->work_station_data = std::make_unique<SettingsVariables::WorkStationData[]>(m_WorkStations->work_stations_count);

	auto i{ 0 };
	for (const auto& entry : std::filesystem::directory_iterator(work_stations_path.ToStdString())) 
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

		fileNameWithPath = work_stations_path.ToStdString() + entry.path().filename().string();
		ReadWorkStationFile(fileNameWithPath, i);

		++i;
	}
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
	if (j.contains("pixel_size_um") && j["pixel_size_um"].is_number())
		workStation.pixelSizeUM = j["pixel_size_um"].get<double>();
	else
		workStation.configuration_errors.Add("\"pixel_size_um\" is missing or is not a number.");

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
	std::vector<SettingsVariables::Stage> out;
	if (m_XeryonMotors) {
		auto serials = m_XeryonMotors->GetSerialNumbersWithRanges();
		for (auto& s : serials) {
			SettingsVariables::Stage st;
			st.SerialNumber = s.first;
			st.COMPort = m_XeryonMotors->GetMotorCOMPort(s.first);
			st.LastKnownPosition = m_XeryonMotors->GetActualStagePos(s.first);
			out.push_back(st);
		}
	}
	WriteJson(m_StagesPositionsFilename, out);
}

auto cSettings::SetBackgroundColor(const wxColour& color) -> void
{
	m_MotorsNotebook->SetBackgroundColour(color);
	
	SetBackgroundColour(color);

	Refresh();
}
