#pragma once
#ifndef CSTYLISH_COMBO_BOX_H
#define CSTYLIGH_COMBO_BOX_H

#include "wx/wx.h"
#include "wx/odcombo.h"

class cStylishComboBox : public wxOwnerDrawnComboBox
{
public:
    virtual void OnDrawItem
    (
        wxDC& dc,
        const wxRect& rect,
        int item,
        int flags
    ) const wxOVERRIDE;

    virtual void OnDrawBackground
    (
        wxDC& dc, 
        const wxRect& rect,
        int item, 
        int flags
    ) const wxOVERRIDE;

    virtual wxCoord OnMeasureItem(size_t item) const wxOVERRIDE;

    virtual wxCoord OnMeasureItemWidth(size_t WXUNUSED(item)) const wxOVERRIDE;
};

#endif // !CSTYLISH_COMBO_BOX_H

