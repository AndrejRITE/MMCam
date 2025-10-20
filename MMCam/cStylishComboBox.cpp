#include "cStylishComboBox.h"

void cStylishComboBox::OnDrawItem
(
    wxDC& dc,
    const wxRect& rect,
    int item,
    int flags
) const
{
    const bool paintingControl = (flags & wxODCB_PAINTING_CONTROL) != 0;
    const bool paintingSelected = (flags & wxODCB_PAINTING_SELECTED) != 0;

    // When there is no selected item yet, draw the current value in control area.
    if (item == wxNOT_FOUND)
    {
        // Draw the control text with normal window text color.
        wxDCTextColourChanger tcc
        (
            dc,
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
        );

        wxRect r(rect);
        r.Deflate(3);
        wxSize ts;
        dc.GetTextExtent(GetValue(), &ts.x, &ts.y);
        dc.DrawText(GetValue(),
            r.x + r.width / 2 - ts.x / 2,
            r.y + r.height / 2 - ts.y / 2);
        return;
    }

    wxRect r(rect);
    r.Deflate(3);
    r.height -= 2;

    // Use highlight text ONLY for selected items in the popup list, not for the control.
    if (!paintingControl && paintingSelected)
        dc.SetTextForeground
        (
            wxColour(255, 128, 0)
            //wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT)
        );
    else
        dc.SetTextForeground
        (
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
        );
    // or wxColour(30,30,30) if you prefer your custom dark gray

    // Keep pen neutral (used only for rectangles you draw)
    dc.SetPen(*wxBLACK_PEN);

    const int num_of_colour_squares{ 7 };
    wxBrush rect_fill{};
    unsigned char red{}, green{}, blue{};
    wxRect colorful_square{};
    int start_x{}, step_x{};
    start_x = r.x;
    step_x = static_cast<int>(rect.width / static_cast<double>(num_of_colour_squares));

    if (!(flags & wxODCB_PAINTING_CONTROL))
    {
        dc.DrawText
        (
            GetString(item),
            r.x + 3,
            (r.y + 0) + ((r.height / 2) - dc.GetCharHeight()) / 2
        );

        if (GetString(item) == "Grayscale")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                red = (unsigned char)(i * 255.0 / (num_of_colour_squares - 1));
                blue = green = red;
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }
        }
        else if (GetString(item) == "Invert")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                red = (unsigned char)((num_of_colour_squares - 1 - i) * 255.0 / (num_of_colour_squares - 1));
                blue = green = red;
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }

        }
        else if (GetString(item) == "Jet")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                /* Colors are calculated on the basis of MatLab Jet Colormap implementation algorithm */
                switch (i)
                {
                case 0:
                    red = 0;
                    green = 0;
                    blue = 131;
                    break;
                case 1:
                    red = 0;
                    green = 46;
                    blue = 255;
                    break;
                case 2:
                    red = 0;
                    green = 215;
                    blue = 255;
                    break;
                case 3:
                    red = 128;
                    green = 255;
                    blue = 128;
                    break;
                case 4:
                    red = 255;
                    green = 211;
                    blue = 0;
                    break;
                case 5:
                    red = 255;
                    green = 41;
                    blue = 0;
                    break;
                case 6:
                    red = 128;
                    green = 0;
                    blue = 0;
                    break;
                default:
                    break;
                }
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }

        }
        else if (GetString(item) == "16 Colors")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                /* Colors are calculated on the basis of MatLab Jet Colormap implementation algorithm */
                switch (i)
                {
                case 0:
                    red = 0;
                    green = 0;
                    blue = 0;
                    break;
                case 1:
                    red = 1;
                    green = 1;
                    blue = 224;
                    break;
                case 2:
                    red = 1;
                    green = 254;
                    blue = 1;
                    break;
                case 3:
                    red = 255;
                    green = 255;
                    blue = 0;
                    break;
                case 4:
                    red = 255;
                    green = 141;
                    blue = 0;
                    break;
                case 5:
                    red = 245;
                    green = 0;
                    blue = 222;
                    break;
                case 6:
                    red = 255;
                    green = 255;
                    blue = 255;
                    break;
                default:
                    break;
                }
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }
        }
        else if (GetString(item) == "Cool")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                /* Colors are calculated on the basis of OpenCV colormap pictures */
                switch (i)
                {
                case 0:
                    red = 0;
                    green = 255;
                    blue = 255;
                    break;
                case 1:
                    red = 43;
                    green = 212;
                    blue = 255;
                    break;
                case 2:
                    red = 87;
                    green = 169;
                    blue = 255;
                    break;
                case 3:
                    red = 128;
                    green = 127;
                    blue = 255;
                    break;
                case 4:
                    red = 171;
                    green = 85;
                    blue = 255;
                    break;
                case 5:
                    red = 214;
                    green = 41;
                    blue = 255;
                    break;
                case 6:
                    red = 255;
                    green = 0;
                    blue = 254;
                    break;
                default:
                    break;
                }
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }
        }
        else if (GetString(item) == "Hot")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                /* Colors are calculated on the basis of OpenCV colormap pictures */
                switch (i)
                {
                case 0:
                    red = 0;
                    green = 0;
                    blue = 0;
                    break;
                case 1:
                    red = 108;
                    green = 0;
                    blue = 0;
                    break;
                case 2:
                    red = 214;
                    green = 0;
                    blue = 0;
                    break;
                case 3:
                    red = 254;
                    green = 65;
                    blue = 0;
                    break;
                case 4:
                    red = 254;
                    green = 172;
                    blue = 0;
                    break;
                case 5:
                    red = 255;
                    green = 255;
                    blue = 51;
                    break;
                case 6:
                    red = 255;
                    green = 255;
                    blue = 251;
                    break;
                default:
                    break;
                }
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }
        }
        else if (GetString(item) == "Winter")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                /* Colors are calculated on the basis of OpenCV colormap pictures */
                switch (i)
                {
                case 0:
                    red = 0;
                    green = 0;
                    blue = 255;
                    break;
                case 1:
                    red = 0;
                    green = 42;
                    blue = 234;
                    break;
                case 2:
                    red = 0;
                    green = 84;
                    blue = 213;
                    break;
                case 3:
                    red = 0;
                    green = 128;
                    blue = 191;
                    break;
                case 4:
                    red = 0;
                    green = 169;
                    blue = 171;
                    break;
                case 5:
                    red = 0;
                    green = 211;
                    blue = 150;
                    break;
                case 6:
                    red = 0;
                    green = 255;
                    blue = 128;
                    break;
                default:
                    break;
                }
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }
        }
        else if (GetString(item) == "Copper")
        {
            for (auto i{ 0 }; i < num_of_colour_squares; ++i)
            {
                /* Colors are calculated on the basis of OpenCV colormap pictures */
                switch (i)
                {
                case 0:
                    red = 0;
                    green = 0;
                    blue = 0;
                    break;
                case 1:
                    red = 53;
                    green = 33;
                    blue = 21;
                    break;
                case 2:
                    red = 105;
                    green = 66;
                    blue = 42;
                    break;
                case 3:
                    red = 160;
                    green = 100;
                    blue = 64;
                    break;
                case 4:
                    red = 211;
                    green = 132;
                    blue = 84;
                    break;
                case 5:
                    red = 255;
                    green = 165;
                    blue = 105;
                    break;
                case 6:
                    red = 255;
                    green = 199;
                    blue = 127;
                    break;
                default:
                    break;
                }
                rect_fill = wxBrush(wxColour(red, green, blue));
                dc.SetBrush(rect_fill);
                colorful_square =
                {
                    start_x,
                    (int)(r.y + .65 * r.height),
                    step_x,
                    (int)(.35 * r.height)
                };
                dc.DrawRectangle(colorful_square);
                start_x += step_x - 1;
            }
        }
    }
    else
    {
        wxSize text_size{};
        dc.GetTextExtent(GetString(item), &text_size.x, &text_size.y);
        dc.DrawText
        (
            GetString(item),
            r.x + r.width / 2 - text_size.GetWidth() / 2,
            r.y + r.height / 2 - text_size.GetHeight() / 2
        );
    }
}

void cStylishComboBox::OnDrawBackground
(
    wxDC& dc,
    const wxRect& rect,
    int item,
    int flags
) const
{
    // Keep default look when painting the control itself or the selected item.
    if ((flags & (wxODCB_PAINTING_CONTROL | wxODCB_PAINTING_SELECTED)))
    {
        wxOwnerDrawnComboBox::OnDrawBackground(dc, rect, item, flags);
        return;
    }

    auto isDarkModeOn = wxSystemSettings::GetAppearance().IsDark();

    // Two gentle shades: even = very light tint, odd = a bit darker (your current color).
    const wxColour evenBg = isDarkModeOn ? wxColour(50, 50, 50) : wxColour(246, 246, 252);  // slightly darker than white, lighter than odd
    const wxColour oddBg = wxColour(evenBg.GetRed() - 30, evenBg.GetGreen() - 30, evenBg.GetRed() - 30);  // your existing alternate-row color

    const wxColour bg = (item & 1) == 0 ? evenBg : oddBg;

    dc.SetBrush(wxBrush(bg));
    dc.SetPen(wxPen(bg));
    dc.DrawRectangle(rect);
}

wxCoord cStylishComboBox::OnMeasureItem(size_t item) const
{
    // Simply demonstrate the ability to have variable-height items
    return FromDIP(36);
}

wxCoord cStylishComboBox::OnMeasureItemWidth(size_t WXUNUSED(item)) const
{
    return -1; // default - will be measured from text width
}