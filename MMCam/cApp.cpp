#include "cApp.h"

wxIMPLEMENT_APP(cApp);

bool cApp::OnInit()
{
	SetAppearance(Appearance::System);

	::wxInitAllImageHandlers();
	auto frame = new cMain(wxT("MMCam"));
	return true;
}
