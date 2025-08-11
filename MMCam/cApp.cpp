#include "cApp.h"

wxIMPLEMENT_APP(cApp);

cApp::cApp()
{
}

bool cApp::OnInit()
{
	SetAppearance(Appearance::System);

	::wxInitAllImageHandlers();
	m_cMainFrame = new cMain(wxT("MMCam"));
	return true;
}

cApp::~cApp()
{

}
