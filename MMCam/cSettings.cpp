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

	if (needStanda)  m_StandaMotors = std::make_unique<StandaMotorArray>();
	if (needXeryon)  InitializeXeryonAndCheckPython(), m_XeryonMotors = std::make_unique<XeryonMotorArray>();

	SetMotorStepsPerMM();

	return result;
}

auto cSettings::SetMotorStepsPerMM() -> void
{
	auto& ws = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num];
	for (size_t i = 0; i < ws.selected_motors_in_data_file.size(); ++i) {
		const wxString sn = ws.selected_motors_in_data_file[i];
		const int steps = ws.motors_steps_per_mm[sn];
		if (sn != "None") {
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

	m_MainPanel->SetBackgroundColour(m_BackgroundColour);

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

		//work_station_static_box_sizer->AddStretchSpacer();
		work_station_static_box_sizer->Add(m_WorkStations->work_station_choice, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
		//work_station_static_box_sizer->AddStretchSpacer();
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
		
		m_Motors->m_Detector[0].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[0]);

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

		m_Motors->m_Detector[1].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[1]);
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

		m_Motors->m_Detector[2].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[2]);
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

		m_Motors->m_Optics[0].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[3]);
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

		m_Motors->m_Optics[1].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[4]);
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

		m_Motors->m_Optics[2].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[5]);
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

		m_Motors->m_Aux[0].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[6]);
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

		m_Motors->m_Aux[1].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[7]);

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

		m_Motors->m_Aux[2].motor->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[8]);
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
	if (selected_work_station > -1)
	{
		m_WorkStations->initialized_work_station_num = selected_work_station;
	}
	else
	{
		m_WorkStations->work_station_choice->SetSelection(m_WorkStations->initialized_work_station_num);
	}

	m_WorkStations->initialized_work_station = m_WorkStations->all_work_station_array_str[m_WorkStations->initialized_work_station_num];

	auto sizeOfSelectedMotors = m_WorkStations->work_station_data[0].selected_motors_in_data_file.size();

	for (auto i{ 0 }; i < sizeOfSelectedMotors; ++i)
	{
		auto motorSN = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_motors_in_data_file[i];
		auto steps_per_mm = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].motors_steps_per_mm[motorSN];

		auto label = wxString::Format(wxT("%i"), steps_per_mm);

		if (i < 3)
		{
			// SN
			m_Motors->m_Detector[i].motor->SetValue(motorSN);
			m_Motors->m_Detector[i].motor_sn = motorSN;
			// Steps/mm
			m_Motors->m_Detector[i].steps_per_mm->SetLabel(label);
		}
		else if (i >= 3 && i < 6)
		{
			// SN
			m_Motors->m_Optics[i - 3].motor->SetValue(motorSN);
			m_Motors->m_Optics[i - 3].motor_sn = motorSN;
			// Steps/mm
			m_Motors->m_Optics[i - 3].steps_per_mm->SetLabel(label);
		}
		else if (i >= 6 && i < 9)
		{
			// SN
			m_Motors->m_Aux[i - 6].motor->SetValue(motorSN);
			m_Motors->m_Aux[i - 6].motor_sn = motorSN;
			// Steps/mm
			m_Motors->m_Aux[i - 6].steps_per_mm->SetLabel(label);
		}
	}
	m_Camera->idTxtCtrl->SetValue(m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_camera_in_data_file);
	m_Camera->selectedCameraIDStr = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].selected_camera_in_data_file;

	m_PixelSizeUM = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].pixelSizeUM;

	m_CameraManufacturer = m_WorkStations->work_station_data[m_WorkStations->initialized_work_station_num].camera_manufacturer;

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
	std::ifstream fileStream(fileName);
	if (!fileStream.is_open())
		return;

	nlohmann::json j;
	try {
		fileStream >> j;
	}
	catch (...) {
		return;  // Malformed JSON
	}

	// Detector
	if (j.contains("detector")) {
		for (const auto& motor : j["detector"]) {
			const std::string sn = motor["SerialNumber"];
			const int stepsPerMM = motor["StepsPerMM"];

			SettingsVariables::MotorManufacturers fallback =
				(j.contains("motor_manufacturer") ? SettingsVariables::ParseVendor(j["motor_manufacturer"].get<std::string>())
					: SettingsVariables::MotorManufacturers::STANDA);

			SettingsVariables::MotorManufacturers v =
				(motor.contains("Manufacturer") ? SettingsVariables::ParseVendor(motor["Manufacturer"].get<std::string>())
					: fallback);

			m_WorkStations->work_station_data[fileNum].motor_vendor_by_sn.emplace(wxString(sn), v);
			m_WorkStations->work_station_data[fileNum].selected_motors_in_data_file.Add(wxString(sn));
			m_WorkStations->work_station_data[fileNum].motors_steps_per_mm.insert(std::make_pair(wxString(sn), stepsPerMM));
		}
	}

	// Optics
	if (j.contains("optics")) {
		for (const auto& motor : j["optics"]) {
			const std::string sn = motor["SerialNumber"];
			const int stepsPerMM = motor["StepsPerMM"];
			m_WorkStations->work_station_data[fileNum].selected_motors_in_data_file.Add(wxString(sn));
			m_WorkStations->work_station_data[fileNum].motors_steps_per_mm.insert(std::make_pair(wxString(sn), stepsPerMM));
		}
	}

	// Aux
	if (j.contains("aux")) {
		for (const auto& motor : j["aux"]) {
			const std::string sn = motor["SerialNumber"];
			const int stepsPerMM = motor["StepsPerMM"];
			m_WorkStations->work_station_data[fileNum].selected_motors_in_data_file.Add(wxString(sn));
			m_WorkStations->work_station_data[fileNum].motors_steps_per_mm.insert(std::make_pair(wxString(sn), stepsPerMM));
		}
	}

	// Camera
	if (j.contains("camera")) {
		m_WorkStations->work_station_data[fileNum].selected_camera_in_data_file = wxString(j["camera"].get<std::string>());
	}

	// CameraManufacturer
	if (j.contains("camera_manufacturer")) {
		const wxString cameraManufacturerStr = wxString(j["camera_manufacturer"].get<std::string>());
		if (cameraManufacturerStr.Lower() == "ximea")
			m_WorkStations->work_station_data[fileNum].camera_manufacturer = SettingsVariables::CameraManufacturers::XIMEA;
		else if (cameraManufacturerStr.Lower() == "moravian_instruments")
			m_WorkStations->work_station_data[fileNum].camera_manufacturer = SettingsVariables::CameraManufacturers::MORAVIAN_INSTRUMENTS;
		else if (cameraManufacturerStr.Lower() == "tucsen")
			m_WorkStations->work_station_data[fileNum].camera_manufacturer = SettingsVariables::CameraManufacturers::TUCSEN;
	}

	// PixelSizeUM
	if (j.contains("pixel_size_um")) {
		m_WorkStations->work_station_data[fileNum].pixelSizeUM = j["pixel_size_um"].get<double>();
	}

	// Station
	if (j.contains("station")) {
		const wxString stationName = wxString(j["station"].get<std::string>());
		m_WorkStations->work_station_data[fileNum].work_station_name = stationName;
		m_WorkStations->all_work_station_array_str.Add(stationName);
	}
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
		if (ws.motor_vendor_by_sn.at(sn) != SettingsVariables::XERYON) continue; // only XERYON
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
