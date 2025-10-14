#pragma once
#ifndef CAPP_H
#define CAPP_H

#include "cMain.h"

class cApp final : public wxApp
{
public:
	virtual bool OnInit();
};

#endif // !CAPP_H

