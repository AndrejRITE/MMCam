#include "cPreviewTools.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	/// 12345 -> "12,345"
	wxString FormatThousands(long long value)
	{
		const bool negative = value < 0;
		const std::string digits = std::to_string(negative ? -value : value);
		std::string out;
		for (size_t i = 0; i < digits.size(); ++i)
		{
			if (i && (digits.size() - i) % 3 == 0) out.push_back(',');
			out.push_back(digits[i]);
		}
		return wxString(negative ? "-" + out : out);
	}

	/// Averages of several pixels get one decimal, single pixels are whole numbers
	wxString FormatProfileValue(double v, bool averaged)
	{
		if (!averaged) return FormatThousands(std::llround(v));
		const long long tenths = std::llround(v * 10.0);
		return FormatThousands(tenths / 10) + wxString::Format(".%lld", std::llabs(tenths % 10));
	}

	const wxColour kRowColour(96, 214, 140);    ///< horizontal line and its profile
	const wxColour kColumnColour(255, 152, 72); ///< vertical line and its profile
}

Tool::Tool()
{

}

void Tool::SetImageDimensions(const wxSize& image_size)
{
	m_ImageSize = image_size;
}

void Tool::SetImageDataType(const int& data_type)
{
	m_DataType = data_type;
}

void Tool::SetCursorPosOnCanvas(const wxRealPoint& cursor_pos_on_canvas)
{
	m_CursorOnCanvas = { cursor_pos_on_canvas.x / m_ZoomOriginalSizeImage, cursor_pos_on_canvas.y / m_ZoomOriginalSizeImage };
}

void Tool::ActivateToolButton(bool activate)
{
	m_ToolButtonActive = activate;
}

bool Tool::IsToolButtonActive() const
{
	return m_ToolButtonActive;
}

void Tool::MouseHoverOverImage(bool moi)
{
	m_MouseHoverOverImage = moi;
}

void Tool::UpdateZoomValue(const double& zoom)
{
	m_Zoom = zoom;
}

void Tool::SetImageStartDrawPos(const wxRealPoint& start_draw_pos)
{
	m_ImageStartDraw = start_draw_pos;
}

void Tool::SetImageOnCanvasSize(const wxSize& img_size)
{
	m_ImageOnCanvasSize = img_size;
}

void Tool::SetZoomOfOriginalSizeImage(const double& original_size_image_zoom)
{
	m_ZoomOriginalSizeImage = original_size_image_zoom;
}

/* Hand Tool */
HandTool::HandTool()
{
}

void HandTool::SetCursorPositionOnImage(const wxRealPoint& cursor_position)
{
	if (!m_ToolButtonActive) return;
	else
	{
		m_HandActive = false;
		if (cursor_position.x >= 0 && cursor_position.x < m_ImageSize.GetWidth() &&
			cursor_position.y >= 0 && cursor_position.y < m_ImageSize.GetHeight())
			m_HandActive = true;
	}
}

wxStockCursor HandTool::UpdateCursor(const wxStockCursor& current_cursor) 
{
	auto edited_cursor = m_ToolButtonActive && m_HandActive && m_Zoom > 1.0 ? wxCURSOR_HAND : current_cursor;
	return edited_cursor;
}

/* CrossHair Tool */
CrossHairTool::CrossHairTool(wxTextCtrl* parent_x_pos, wxTextCtrl* parent_y_pos) 
	: m_ParentXPosTextCtrl(parent_x_pos), m_ParentYPosTextCtrl(parent_y_pos)
{
}

void CrossHairTool::UpdateZoomValue(const double& zoom)
{
	m_Zoom = zoom;
	
	m_ActualZoomedSizeOfImageOnCanvas =
	{
		(int)((double)m_ImageSize.GetWidth() * m_Zoom / m_ZoomOriginalSizeImage),
		(int)((double)m_ImageSize.GetHeight() * m_Zoom / m_ZoomOriginalSizeImage)
	};

	m_ActualHalfPixelSize =
	{
		m_Zoom / m_ZoomOriginalSizeImage / 2.0,
		m_Zoom / m_ZoomOriginalSizeImage / 2.0,
	};

	//CalculateCrossHairPositionOnCanvas();
}

void CrossHairTool::ActivateToolButton(bool vertical_line, bool horizontal_line)
{
	m_VerticalLineProfileActive = vertical_line;
	m_ShowVerticalLine = m_ShowDataOnVerticalLine = vertical_line;
	m_HorizontalLineProfileActive = m_ShowDataOnHorizontalLine = horizontal_line;
	m_ShowHorizontalLine = horizontal_line;
	m_ToolButtonActive = vertical_line || horizontal_line;
}

void CrossHairTool::ActivateValueDisplaying(bool activate)
{
}

void CrossHairTool::SetCursorPosOnCanvas(const wxRealPoint& cursor_pos_on_canvas)
{
	m_CursorOnCanvas = { cursor_pos_on_canvas.x / m_ZoomOriginalSizeImage, cursor_pos_on_canvas.y / m_ZoomOriginalSizeImage };
}

void CrossHairTool::SetCursorPosOnImage(const wxRealPoint& cursor_pos_on_image)
{
	m_CursorOnImage = cursor_pos_on_image;
	CheckIfMouseAboveCrossHair();
}

void CrossHairTool::SetXPosFromParent(const int& x_pos)
{
	m_CrossHairOnImage.x = x_pos;
	CalculateCrossHairPositionOnCanvas();
}

void CrossHairTool::SetYPosFromParent(const int& y_pos)
{
	m_CrossHairOnImage.y = y_pos;
	CalculateCrossHairPositionOnCanvas();
}

void CrossHairTool::ChangeStartPositionOfImage(const wxRealPoint& img_pos)
{
	SetImageStartDrawPos(img_pos);
}

void CrossHairTool::CheckIfMouseAboveCrossHair()
{
	if (m_MouseHoverOverImage)
	{
		m_CursorAboveHorizontalLine = m_CursorAboveVerticalLine = false;
		if ((m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_CrossHairOnCanvas.x + m_ActualHalfPixelSize.x - m_CrosshairRectangle.GetWidth() / 2.0) &&
			(m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_CrossHairOnCanvas.x + m_ActualHalfPixelSize.x + m_CrosshairRectangle.GetWidth() / 2.0) &&
			m_ShowVerticalLine)
		{
			m_CursorAboveVerticalLine = true;
		}
		if ((m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_CrossHairOnCanvas.y + m_ActualHalfPixelSize.y - m_CrosshairRectangle.GetHeight() / 2.0) && 
			(m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_CrossHairOnCanvas.y + m_ActualHalfPixelSize.y + m_CrosshairRectangle.GetHeight() / 2.0) && 
			m_ShowHorizontalLine)
		{
			m_CursorAboveHorizontalLine = true;
		}
	}
}

void CrossHairTool::CalculateCrossHairOnImage()
{
	m_CrossHairOnImage.x = m_CrossHairOnCanvas.x / m_Zoom * m_ZoomOriginalSizeImage;
	m_CrossHairOnImage.y = m_CrossHairOnCanvas.y / m_Zoom * m_ZoomOriginalSizeImage;

	/* Checking X */
	m_CrossHairOnImage.x = m_CrossHairOnImage.x >= 0.0 ? m_CrossHairOnImage.x : 0.0;
	m_CrossHairOnImage.x = m_CrossHairOnImage.x <= m_ImageSize.GetWidth() - 1.0 ? m_CrossHairOnImage.x : m_ImageSize.GetWidth() - 1.0;
	/* Checking Y */
	m_CrossHairOnImage.y = m_CrossHairOnImage.y >= 0.0 ? m_CrossHairOnImage.y : 0.0;
	m_CrossHairOnImage.y = m_CrossHairOnImage.y <= m_ImageSize.GetHeight() - 1.0 ? m_CrossHairOnImage.y : m_ImageSize.GetHeight() - 1.0;
}

void CrossHairTool::CalculateCrossHairPositionOnCanvas()
{
	m_CrossHairOnCanvas.x = m_CrossHairOnImage.x * m_Zoom / m_ZoomOriginalSizeImage;
	m_CrossHairOnCanvas.y = m_CrossHairOnImage.y * m_Zoom / m_ZoomOriginalSizeImage;
}

void CrossHairTool::UpdateParentCrossHairTextCtrls()
{
	m_ParentXPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.x + m_TextCtrlPixelOffset)));
	m_ParentYPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.y + m_TextCtrlPixelOffset)));
}

wxStockCursor CrossHairTool::UpdateCursor(const wxStockCursor& current_cursor)
{
	CheckIfMouseAboveCrossHair();
	auto edited_cursor = current_cursor;
	if (m_ToolButtonActive)
	{
		edited_cursor = m_SelectingPositionFromParentWindow ? wxCURSOR_QUESTION_ARROW : edited_cursor;
	}
	//if (m_MouseHoverOverImage)
	//{
		//edited_cursor = m_CursorAboveVerticalLine && m_ShowVerticalLine ? wxCURSOR_SIZEWE : edited_cursor;
		//edited_cursor = m_CursorAboveHorizontalLine && m_ShowHorizontalLine ? wxCURSOR_SIZENS : edited_cursor;
		//edited_cursor = m_CursorAboveHorizontalLine && m_CursorAboveVerticalLine && m_ShowVerticalLine && m_ShowHorizontalLine ? wxCURSOR_SIZING : edited_cursor;
		//edited_cursor = m_ChangingHorizontalLine || m_ChangingVerticalLine ? wxCURSOR_CLOSED_HAND : edited_cursor;
	//}
	return edited_cursor;
}

void CrossHairTool::DrawCrossHair(wxGraphicsContext* gc, const ToolsVariables::ProfileView& view)
{
	if (!gc || !view.data || view.imageSize.GetWidth() <= 0 || view.imageSize.GetHeight() <= 0 || view.scale <= 0.0)
		return;
	if (!m_ShowVerticalLine && !m_ShowHorizontalLine) return;

	const int W = view.imageSize.GetWidth(), H = view.imageSize.GetHeight();
	const double k = view.scale;
	const int cx = std::clamp(view.position.x, 0, W - 1);
	const int cy = std::clamp(view.position.y, 0, H - 1);

	// Visible part of the image on the canvas
	const double left = std::max(0.0, view.origin.x), right = std::min<double>(view.canvas.GetWidth(), view.origin.x + W * k);
	const double top = std::max(0.0, view.origin.y), bottom = std::min<double>(view.canvas.GetHeight(), view.origin.y + H * k);
	if (right <= left || bottom <= top) return;

	// Same band as the profile: [c - (w - 1) / 2, c + w / 2], clipped to the image
	const int band = std::max(1, m_AveragingWidthPX);
	auto bandLimits = [band](int c, int size) { return std::make_pair(std::max(0, c - (band - 1) / 2), std::min(size - 1, c + band / 2)); };

	gc->Clip(left, top, right - left, bottom - top);

	auto drawLine = [&](double x0, double y0, double x1, double y1, const wxColour& colour, bool highlighted)
		{
			gc->SetPen(wxPen(wxColour(0, 0, 0, highlighted ? 160 : 130), highlighted ? 5 : 3));
			gc->StrokeLine(x0, y0, x1, y1);
			gc->SetPen(wxPen(highlighted ? colour.ChangeLightness(125) : colour, highlighted ? 2 : 1));
			gc->StrokeLine(x0, y0, x1, y1);
		};

	if (m_ShowHorizontalLine)
	{
		const auto [y0, y1] = bandLimits(cy, H);
		if (band > 1)
		{
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(wxColour(kRowColour.Red(), kRowColour.Green(), kRowColour.Blue(), 22)));
			gc->DrawRectangle(left, view.origin.y + y0 * k, right - left, std::max(1.0, (y1 - y0 + 1) * k));
			wxPen edge(wxColour(kRowColour.Red(), kRowColour.Green(), kRowColour.Blue(), 170), 1, wxPENSTYLE_SHORT_DASH);
			gc->SetPen(edge);
			gc->StrokeLine(left, view.origin.y + y0 * k, right, view.origin.y + y0 * k);
			gc->StrokeLine(left, view.origin.y + (y1 + 1) * k, right, view.origin.y + (y1 + 1) * k);
		}
		const double ly = view.origin.y + (cy + 0.5) * k;
		drawLine(left, ly, right, ly, kRowColour, m_HighlightRow);
	}
	if (m_ShowVerticalLine)
	{
		const auto [x0, x1] = bandLimits(cx, W);
		if (band > 1)
		{
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(wxColour(kColumnColour.Red(), kColumnColour.Green(), kColumnColour.Blue(), 22)));
			gc->DrawRectangle(view.origin.x + x0 * k, top, std::max(1.0, (x1 - x0 + 1) * k), bottom - top);
			wxPen edge(wxColour(kColumnColour.Red(), kColumnColour.Green(), kColumnColour.Blue(), 170), 1, wxPENSTYLE_SHORT_DASH);
			gc->SetPen(edge);
			gc->StrokeLine(view.origin.x + x0 * k, top, view.origin.x + x0 * k, bottom);
			gc->StrokeLine(view.origin.x + (x1 + 1) * k, top, view.origin.x + (x1 + 1) * k, bottom);
		}
		const double lx = view.origin.x + (cx + 0.5) * k;
		drawLine(lx, top, lx, bottom, kColumnColour, m_HighlightColumn);
	}
	if (m_ShowHorizontalLine && m_ShowVerticalLine)
	{
		const double hx = view.origin.x + (cx + 0.5) * k, hy = view.origin.y + (cy + 0.5) * k;
		const double r = (m_HighlightRow && m_HighlightColumn) ? 6.0 : 4.0;
		gc->SetPen(wxPen(wxColour(20, 20, 24, 200), 1.5));
		gc->SetBrush(wxBrush(*wxWHITE));
		gc->DrawEllipse(hx - r, hy - r, 2 * r, 2 * r);
	}
	gc->ResetClip();

	if (m_ShowDataOnHorizontalLine && m_ShowHorizontalLine)
	{
		UpdateProfile(m_RowProfile, true, view);
		DrawProfilePanel(gc, m_RowProfile, true, view, kRowColour);
	}
	if (m_ShowDataOnVerticalLine && m_ShowVerticalLine)
	{
		UpdateProfile(m_ColumnProfile, false, view);
		DrawProfilePanel(gc, m_ColumnProfile, false, view, kColumnColour);
	}
}

void CrossHairTool::UpdateProfile(ProfileCache& cache, bool alongRow, const ToolsVariables::ProfileView& view)
{
	const int W = view.imageSize.GetWidth(), H = view.imageSize.GetHeight();
	const int centre = alongRow ? std::clamp(view.position.y, 0, H - 1) : std::clamp(view.position.x, 0, W - 1);
	const int across = alongRow ? H : W;
	const int band = std::max(1, m_AveragingWidthPX);
	const int first = std::max(0, centre - (band - 1) / 2);
	const int last = std::min(across - 1, centre + band / 2);

	if (cache.revision == view.revision && cache.data == view.data && cache.imageSize == view.imageSize &&
		cache.position == centre && cache.first == first && cache.last == last && !cache.values.empty())
		return;

	const int length = alongRow ? W : H;
	const int count = last - first + 1;
	cache.values.assign(length, 0.0f);

	if (alongRow)
	{
		// Sum whole rows (contiguous memory), then divide
		std::vector<unsigned long long> sums(W, 0ull);
		for (int y = first; y <= last; ++y)
		{
			const unsigned short* row = view.data + static_cast<size_t>(y) * W;
			for (int x = 0; x < W; ++x) sums[x] += row[x];
		}
		for (int x = 0; x < W; ++x) cache.values[x] = static_cast<float>(static_cast<double>(sums[x]) / count);
	}
	else
	{
		for (int y = 0; y < H; ++y)
		{
			const unsigned short* row = view.data + static_cast<size_t>(y) * W;
			unsigned long long sum = 0;
			for (int x = first; x <= last; ++x) sum += row[x];
			cache.values[y] = static_cast<float>(static_cast<double>(sum) / count);
		}
	}

	const auto [mn, mx] = std::minmax_element(cache.values.begin(), cache.values.end());
	cache.minValue = *mn;
	cache.maxValue = *mx;
	cache.position = centre;
	cache.first = first;
	cache.last = last;
	cache.revision = view.revision;
	cache.data = view.data;
	cache.imageSize = view.imageSize;
}

void CrossHairTool::DrawProfilePanel(wxGraphicsContext* gc, const ProfileCache& cache, bool alongRow,
	const ToolsVariables::ProfileView& view, const wxColour& colour)
{
	if (cache.values.empty()) return;

	const int W = view.imageSize.GetWidth(), H = view.imageSize.GetHeight();
	const double k = view.scale;
	const int length = static_cast<int>(cache.values.size());

	// Along the profile ("u"): visible span of the image on the canvas. Across it ("v"): panel.
	const double originU = alongRow ? view.origin.x : view.origin.y;
	const double canvasU = alongRow ? view.canvas.GetWidth() : view.canvas.GetHeight();
	const double canvasV = alongRow ? view.canvas.GetHeight() : view.canvas.GetWidth();
	const double u0 = std::max(0.0, originU), u1 = std::min(canvasU, originU + length * k);
	if (u1 - u0 < 40.0) return;

	// Panel thickness and placement: before the line (above / left) if there is room, else after it
	const double thickness = std::clamp(canvasV * 0.2, 70.0, 150.0);
	const int centre = alongRow ? std::clamp(view.position.y, 0, H - 1) : std::clamp(view.position.x, 0, W - 1);
	const double originV = alongRow ? view.origin.y : view.origin.x;
	const double bandStart = originV + cache.first * k, bandEnd = originV + (cache.last + 1) * k;
	const double gap = 6.0;
	double v0 = bandStart - gap - thickness;
	if (v0 < 4.0) v0 = bandEnd + gap;
	v0 = std::clamp(v0, 4.0, std::max(4.0, canvasV - 4.0 - thickness));
	const double v1 = v0 + thickness;

	auto toXY = [alongRow](double u, double v) { return alongRow ? wxPoint2DDouble(u, v) : wxPoint2DDouble(v, u); };

	// Card
	{
		const auto a = toXY(u0, v0), b = toXY(u1, v1);
		gc->SetBrush(wxBrush(wxColour(20, 20, 24, 170)));
		gc->SetPen(wxPen(wxColour(255, 255, 255, 40), 1));
		gc->DrawRoundedRectangle(a.m_x, a.m_y, b.m_x - a.m_x, b.m_y - a.m_y, 6.0);
	}

	// Labels
	const wxFont font(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	const wxFont bold(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
	gc->SetFont(font, *wxWHITE);
	wxDouble tw{}, th{};
	gc->GetTextExtent("Ag", &tw, &th);
	const double pad = 6.0;

	// Value axis: adaptive = this profile's range, otherwise the full data range
	const bool averaged = cache.last > cache.first;
	const double lo = m_AdaptiveScaling ? cache.minValue : 0.0;
	double hi = m_AdaptiveScaling ? cache.maxValue : (m_DataType == ToolsVariables::DATA_U12 ? 4095.0 : 65535.0);
	if (hi - lo < 1e-6) hi = lo + 1.0;

	// Plot area inside the card (across: value, along: position)
	const double plotV0 = alongRow ? v0 + pad + th + 2.0 : v0 + pad;          // value = hi
	const double plotV1 = alongRow ? v1 - pad : v1 - pad - th - 2.0;           // value = lo
	if (plotV1 - plotV0 < 10.0) return;
	// For rows the value grows upwards (v decreases); for columns it grows to the right (v increases)
	auto valueToV = [&](double value)
		{
			const double t = std::clamp((value - lo) / (hi - lo), 0.0, 1.0);
			return alongRow ? plotV1 - t * (plotV1 - plotV0) : plotV0 + t * (plotV1 - plotV0);
		};
	const double baseV = alongRow ? plotV1 : plotV0;

	// Curve: one or two points per canvas pixel. Zoomed out, each canvas pixel covers
	// several image pixels; its min and max are both drawn so narrow peaks never vanish.
	std::vector<wxPoint2DDouble> pts;
	if (k >= 1.0)
	{
		const int i0 = std::max(0, (int)std::floor((u0 - originU) / k));
		const int i1 = std::min(length - 1, (int)std::ceil((u1 - originU) / k));
		pts.reserve(i1 - i0 + 1);
		for (int i = i0; i <= i1; ++i)
			pts.push_back(toXY(std::clamp(originU + (i + 0.5) * k, u0, u1), valueToV(cache.values[i])));
	}
	else
	{
		pts.reserve(static_cast<size_t>((u1 - u0) * 2) + 2);
		for (double u = std::floor(u0); u < u1; u += 1.0)
		{
			const int i0 = std::clamp((int)std::floor((u - originU) / k), 0, length - 1);
			const int i1 = std::clamp((int)std::floor((u + 1.0 - originU) / k) - 1, i0, length - 1);
			const auto [mn, mx] = std::minmax_element(cache.values.begin() + i0, cache.values.begin() + i1 + 1);
			const double uc = std::clamp(u + 0.5, u0, u1);
			pts.push_back(toXY(uc, valueToV(*mx)));
			if (*mn != *mx) pts.push_back(toXY(uc, valueToV(*mn)));
		}
	}
	if (pts.size() < 2) return;

	// Area under the curve, then the curve itself
	{
		wxGraphicsPath area = gc->CreatePath();
		const auto& f = pts.front();
		const auto& l = pts.back();
		area.MoveToPoint(alongRow ? wxPoint2DDouble(f.m_x, baseV) : wxPoint2DDouble(baseV, f.m_y));
		for (const auto& pt : pts) area.AddLineToPoint(pt);
		area.AddLineToPoint(alongRow ? wxPoint2DDouble(l.m_x, baseV) : wxPoint2DDouble(baseV, l.m_y));
		area.CloseSubpath();
		gc->SetPen(*wxTRANSPARENT_PEN);
		gc->SetBrush(wxBrush(wxColour(colour.Red(), colour.Green(), colour.Blue(), 55)));
		gc->FillPath(area);

		wxGraphicsPath line = gc->CreatePath();
		line.MoveToPoint(pts.front());
		for (size_t i = 1; i < pts.size(); ++i) line.AddLineToPoint(pts[i]);
		gc->SetBrush(*wxTRANSPARENT_BRUSH);
		gc->SetPen(wxPen(colour, 1.5));
		gc->StrokePath(line);
	}

	// Title and scale labels
	const wxString title = (alongRow ? wxString("Row ") : wxString("Column ")) + FormatThousands(centre + 1) +
		(averaged ? wxString::FromUTF8("  \xC2\xB7  avg ") + wxString::Format("%d px", cache.last - cache.first + 1) : wxString());
	const wxString hiText = FormatProfileValue(hi, averaged && m_AdaptiveScaling);
	const wxString loText = FormatProfileValue(lo, averaged && m_AdaptiveScaling);
	wxDouble hw{}, hh{}, lw{}, lh{};
	gc->SetFont(font, wxColour(185, 185, 190));
	gc->GetTextExtent(hiText, &hw, &hh);
	gc->GetTextExtent(loText, &lw, &lh);

	// Labels sit on a small dark backdrop so the curve never makes them unreadable
	const wxColour muted(190, 190, 196);
	auto drawLabel = [&](const wxString& text, double x, double y, const wxFont& f, const wxColour& c)
		{
			gc->SetFont(f, c);
			wxDouble w{}, h{};
			gc->GetTextExtent(text, &w, &h);
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->SetBrush(wxBrush(wxColour(20, 20, 24, 210)));
			gc->DrawRoundedRectangle(x - 3.0, y - 1.0, w + 6.0, h + 2.0, 3.0);
			gc->DrawText(text, x, y);
		};

	if (alongRow)
	{
		drawLabel(title, u0 + pad, v0 + pad, bold, colour);
		drawLabel(hiText, u1 - pad - hw, v0 + pad, font, muted);
		drawLabel(loText, u1 - pad - lw, plotV1 - lh, font, muted);
	}
	else
	{
		// Columns: title at the top, value range along the bottom (low left, high right)
		gc->SetFont(bold, colour);
		wxDouble titleW{}, titleH{};
		gc->GetTextExtent(title, &titleW, &titleH);
		if (titleW <= v1 - v0 - 2 * pad)
			drawLabel(title, v0 + pad, u0 + pad, bold, colour);
		else
			drawLabel((wxString("Col ") + FormatThousands(centre + 1)), v0 + pad, u0 + pad, bold, colour);
		drawLabel(loText, v0 + pad, u1 - pad - lh, font, muted);
		if (lw + hw + 12.0 <= v1 - v0 - 2 * pad)
			drawLabel(hiText, v1 - pad - hw, u1 - pad - hh, font, muted);
	}

	// Value where the other line crosses this profile
	const bool otherShown = alongRow ? m_ShowVerticalLine : m_ShowHorizontalLine;
	if (!otherShown) return;
	const int crossIndex = alongRow ? std::clamp(view.position.x, 0, W - 1) : std::clamp(view.position.y, 0, H - 1);
	const double crossU = originU + (crossIndex + 0.5) * k;
	if (crossU < u0 || crossU > u1) return;

	const double value = cache.values[crossIndex];
	const auto dot = toXY(crossU, valueToV(value));
	gc->SetPen(wxPen(wxColour(20, 20, 24), 1.5));
	gc->SetBrush(wxBrush(*wxWHITE));
	gc->DrawEllipse(dot.m_x - 3.5, dot.m_y - 3.5, 7.0, 7.0);

	const wxString valueText = FormatProfileValue(value, averaged);
	gc->SetFont(bold, *wxBLACK);
	wxDouble vw{}, vh{};
	gc->GetTextExtent(valueText, &vw, &vh);
	const double pillW = vw + 10.0, pillH = vh + 4.0;
	// Next to the dot, kept inside the card
	const auto cardA = toXY(u0, v0), cardB = toXY(u1, v1);
	double px = dot.m_x + 8.0, py = dot.m_y - pillH - 4.0;
	if (px + pillW > cardB.m_x - 2.0) px = dot.m_x - 8.0 - pillW;
	if (py < cardA.m_y + 2.0) py = dot.m_y + 6.0;
	px = std::clamp(px, cardA.m_x + 2.0, std::max(cardA.m_x + 2.0, cardB.m_x - 2.0 - pillW));
	py = std::clamp(py, cardA.m_y + 2.0, std::max(cardA.m_y + 2.0, cardB.m_y - 2.0 - pillH));
	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->SetBrush(wxBrush(colour));
	gc->DrawRoundedRectangle(px, py, pillW, pillH, pillH / 2.0);
	gc->DrawText(valueText, px + 5.0, py + 2.0);
}

bool CrossHairTool::PositionCanBeChanged() const
{
	return (m_ShowHorizontalLine && m_CursorAboveHorizontalLine) || (m_ShowVerticalLine && m_CursorAboveVerticalLine);
}

bool CrossHairTool::CanProcessPanning() const
{
	return (m_ShowHorizontalLine || m_ShowVerticalLine) && (!m_CursorAboveHorizontalLine && !m_CursorAboveVerticalLine);
}

void CrossHairTool::StopChangingCrossHairPos()
{
	m_ChangingHorizontalLine = m_ChangingVerticalLine = false;
}

bool CrossHairTool::GetShowVerticalLine() const
{
	return m_ShowVerticalLine;
}

int CrossHairTool::GetXPos() const
{
	return m_CrossHairOnImage.x;
}

bool CrossHairTool::GetShowHorizontalLine() const
{
	return m_ShowHorizontalLine;
}

int CrossHairTool::GetYPos() const
{
	return m_CrossHairOnImage.y;
}

auto CrossHairTool::ActivateSetPositionFromParentWindow(bool activate) -> void
{
	m_SelectingPositionFromParentWindow = activate;
}

auto CrossHairTool::SetAveragingWidthPX(const int& avgWidth) -> void
{
	m_AveragingWidthPX = std::clamp(avgWidth, 1, 10'000);
}

void CrossHairTool::ProcessChangingCrossHairPos()
{
	m_ChangingHorizontalLine = m_ChangingVerticalLine = false;
	if (m_CursorAboveHorizontalLine && m_ShowHorizontalLine && !m_CursorAboveVerticalLine)
	{
		m_ChangingHorizontalLine = true;
	}
	else if (m_CursorAboveVerticalLine && m_ShowVerticalLine && !m_CursorAboveHorizontalLine)
	{
		m_ChangingVerticalLine = true;
	}
	else if (m_CursorAboveHorizontalLine && m_CursorAboveVerticalLine && m_ShowHorizontalLine && m_ShowVerticalLine)
	{
		m_ChangingHorizontalLine = m_ChangingVerticalLine = true;
	}

	LoopChangingCrossHairPos();
}

void CrossHairTool::UpdateParentCrossHairTextCtrlsWithRefresh()
{
	CalculateCrossHairOnImage();
	/* Here should be SetValue to initiate Refresh() from parent window */
	m_ParentXPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.x + m_TextCtrlPixelOffset)));
	m_ParentYPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_CrossHairOnImage.y + m_TextCtrlPixelOffset)));
}

auto CrossHairTool::CheckIfPixelValueIsInsideTheImage(const int& x, const int& y) -> bool
{
	if (x < 0 || x >= m_ImageSize.GetWidth()) return false;
	if (y < 0 || y >= m_ImageSize.GetHeight()) return false;
	return true;
}

void CrossHairTool::LoopChangingCrossHairPos()
{
	if (m_ChangingHorizontalLine && !m_ChangingVerticalLine)
	{
		m_CrossHairOnCanvas = 
		{
			m_CrossHairOnCanvas.x, 
			m_CursorOnCanvas.y - m_ImageStartDraw.y
		};
	}
	else if (!m_ChangingHorizontalLine && m_ChangingVerticalLine)
	{
		m_CrossHairOnCanvas = 
		{
			m_CursorOnCanvas.x - m_ImageStartDraw.x,
			m_CrossHairOnCanvas.y
		};
	}
	else if (m_ChangingHorizontalLine && m_ChangingVerticalLine)
	{
		m_CrossHairOnCanvas = 
		{
			m_CursorOnCanvas.x - m_ImageStartDraw.x,
			m_CursorOnCanvas.y - m_ImageStartDraw.y
		};
	}
	//UpdateParentCrossHairTextCtrlsWithRefresh();
}

/* _____ Rectangle Tool _____ */

RectangleSelectionTool::RectangleSelectionTool(
	wxTextCtrl* parent_start_x_pos, 
	wxTextCtrl* parent_start_y_pos, 
	wxTextCtrl* parent_width, 
	wxTextCtrl* parent_height) 
	: m_ParentStartXPosTextCtrl(parent_start_x_pos), 
	m_ParentStartYPosTextCtrl(parent_start_y_pos), 
	m_ParentWidthTextCtrl(parent_width), 
	m_ParentHeightTextCtrl(parent_height)
{
}

wxStockCursor RectangleSelectionTool::UpdateCursor(const wxStockCursor& current_cursor)
{
	auto edited_cursor = current_cursor;
	if (m_MouseHoverOverImage && m_ToolButtonActive)
	{
		edited_cursor = m_CursorOutsideOfRectangleSelection ? wxCURSOR_CROSS : edited_cursor;
		edited_cursor = m_CursorInsideRectangleSelection ? wxCURSOR_SIZING : edited_cursor;
		edited_cursor = m_CursorAboveVerticalLeftLineMiddlePoint || m_CursorAboveVerticalRightLineMiddlePoint ? wxCURSOR_SIZEWE : edited_cursor;
		edited_cursor = m_CursorAboveHorizontalUpperLineMiddlePoint || m_CursorAboveHorizontalBottomLineMiddlePoint ? wxCURSOR_SIZENS : edited_cursor;
		edited_cursor = m_CursorAboveLeftUpperCornerPoint || m_CursorAboveRightBottomCornerPoint ? wxCURSOR_SIZENWSE : edited_cursor;
		edited_cursor = m_CursorAboveRightUpperCornerPoint || m_CursorAboveLeftBottomCornerPoint ? wxCURSOR_SIZENESW : edited_cursor;
		edited_cursor = m_UserChangingSizeSelectedRectangleSelection ? wxCURSOR_CLOSED_HAND : edited_cursor;
#if 0
		/* User Selecting or Changing Position/Size of Rectangle Selection */
		edited_cursor = m_ChangingLUPointRectangleSelection || m_ChangingRUPointRectangleSelection || m_ChangingRBPointRectangleSelection || m_ChangingLBPointRectangleSelection ? wxCURSOR_CLOSED_HAND : current_cursor;
#endif
		
		edited_cursor = m_UserSelectingAreaOfRectangle ? wxCURSOR_CROSS : edited_cursor;
		edited_cursor = m_UserPanningSelectedRectangleSelection ? wxCURSOR_CLOSED_HAND : edited_cursor;
	}
	return edited_cursor;
}

void RectangleSelectionTool::UpdateZoomValue(const double& zoom)
{
	m_Zoom = zoom;

	m_ActualPixelSize = { m_Zoom / m_ZoomOriginalSizeImage, m_Zoom / m_ZoomOriginalSizeImage };
}

void RectangleSelectionTool::SetStartXPosFromParent(const int& x_pos)
{
	m_RectangleSelectionOnImage.SetLeft(x_pos);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::SetStartYPosFromParent(const int& y_pos)
{
	m_RectangleSelectionOnImage.SetTop(y_pos);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::SetWidthFromParent(const int& width)
{
	m_RectangleSelectionOnImage.SetWidth(width);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::SetHeightFromParent(const int& height)
{
	m_RectangleSelectionOnImage.SetHeight(height);
	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrls();
}

void RectangleSelectionTool::UpdateParentRectangleTextCtrls()
{
	m_ParentStartXPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetLeft() + m_TextCtrlPixelOffset)));
	m_ParentStartYPosTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetTop() + m_TextCtrlPixelOffset)));
	m_ParentWidthTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetWidth())));
	m_ParentHeightTextCtrl->ChangeValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetHeight())));
}

void RectangleSelectionTool::DrawRectangleSelection(wxGraphicsContext* graphics_context)
{
	if (!m_ToolButtonActive) return;

	wxGraphicsPath path = graphics_context->CreatePath();

	wxRealPoint start_draw_rectangle{}, finish_draw_rectangle{};
	wxDouble rect_width{}, rect_height{};

	{
		start_draw_rectangle.x = m_RectangleSelectionOnCanvas.GetRight() > m_RectangleSelectionOnCanvas.GetLeft() ? m_RectangleSelectionOnCanvas.GetLeft() : m_RectangleSelectionOnCanvas.GetRight();
		start_draw_rectangle.y = m_RectangleSelectionOnCanvas.GetBottom() > m_RectangleSelectionOnCanvas.GetTop() ? m_RectangleSelectionOnCanvas.GetTop() : m_RectangleSelectionOnCanvas.GetBottom();
		finish_draw_rectangle.x = m_RectangleSelectionOnCanvas.GetRight() > m_RectangleSelectionOnCanvas.GetLeft() ? m_RectangleSelectionOnCanvas.GetRight() : m_RectangleSelectionOnCanvas.GetLeft();
		finish_draw_rectangle.y = m_RectangleSelectionOnCanvas.GetBottom() > m_RectangleSelectionOnCanvas.GetTop() ? m_RectangleSelectionOnCanvas.GetBottom() : m_RectangleSelectionOnCanvas.GetTop();

		start_draw_rectangle.x += m_ImageStartDraw.x;
		start_draw_rectangle.y += m_ImageStartDraw.y;
		finish_draw_rectangle.x += m_ImageStartDraw.x;
		finish_draw_rectangle.y += m_ImageStartDraw.y;
	}

	{
		rect_width = abs(finish_draw_rectangle.x - start_draw_rectangle.x); 
		rect_height = abs(finish_draw_rectangle.y - start_draw_rectangle.y);

		rect_width += m_ActualPixelSize.x;
		rect_height += m_ActualPixelSize.y;

		rect_width = m_Zoom / m_ZoomOriginalSizeImage > 1.0 ? rect_width - 1.0 : rect_width;
		rect_height = m_Zoom / m_ZoomOriginalSizeImage > 1.0 ? rect_height - 1.0 : rect_height;
	}

	if (rect_width <= 0.0 || rect_height <= 0.0) return;

	path.AddRectangle(start_draw_rectangle.x, start_draw_rectangle.y, rect_width, rect_height);

	graphics_context->DrawPath(path);

	DrawRectangleSelectionPoints(graphics_context);
}

void RectangleSelectionTool::DrawRectangleSelectionPoints(wxGraphicsContext* gc)
{
	if (!m_DrawHorizontalPoints || !m_DrawVerticalPoints) return;

	wxGraphicsPath path = gc->CreatePath();
	gc->SetPen(*wxBLACK_PEN);
	gc->SetBrush(*wxWHITE_BRUSH);

	wxRealPoint start_draw_point = {};
	/* Left Upper Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Right Upper Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetRight() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Right Bottom Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetRight() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetBottom() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}	
	/* Left Bottom Corner */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetBottom() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Left Vertical Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Upper Horizontal Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Right Vertical Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetRight() + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	/* Bottom Horizontal Line Middle Point */
	{
		start_draw_point.x = m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 + m_ImageStartDraw.x - m_PointSize.x / 2.0;
		start_draw_point.y = m_VisualRectangleSelectionOnCanvas.GetBottom() + m_ImageStartDraw.y - m_PointSize.y / 2.0;
		path.AddRectangle(start_draw_point.x, start_draw_point.y, m_PointSize.GetWidth(), m_PointSize.GetHeight());
	}
	gc->DrawPath(path);

}

void RectangleSelectionTool::SetCursorPosOnImage(const wxRealPoint& cursor_pos_on_image)
{
	m_CursorOnImage = cursor_pos_on_image;
	CheckIfMouseAboveRectangleSelection();
}

bool RectangleSelectionTool::CanSelectRegion() const
{
	return m_ToolButtonActive && m_MouseHoverOverImage && m_CursorOutsideOfRectangleSelection;
}

void RectangleSelectionTool::ProcessSelectingRegion()
{
	m_UserSelectingAreaOfRectangle = true;
	m_StartSelectingAreaPoint  = 
	{ 
		m_CursorOnCanvas.x - m_ImageStartDraw.x, 
		m_CursorOnCanvas.y - m_ImageStartDraw.y 
	};
	m_RectangleSelectionOnCanvas.SetLeftTop(m_StartSelectingAreaPoint);
	m_RectangleSelectionOnCanvas.SetRightBottom(m_StartSelectingAreaPoint);
	LoopSelectingRegion();
}

void RectangleSelectionTool::LoopSelectingRegion()
{
	ChangeLUorRBRectanglePoints();
	CalculateRectangleSelectionPosOnImage();
	UpdateParentRectangleTextCtrlsWithRefresh();
}

void RectangleSelectionTool::ChangeLUorRBRectanglePoints()
{
	if (!m_MouseHoverOverImage) return;

	wxRealPoint temp_point = 
	{ 
		m_CursorOnCanvas.x - m_ImageStartDraw.x, 
		m_CursorOnCanvas.y - m_ImageStartDraw.y 
	};

	/* I quadrant (right upper) */
	if (
		temp_point.x > m_StartSelectingAreaPoint.m_x && 
		temp_point.y <= m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetRight(temp_point.x);
		m_RectangleSelectionOnCanvas.SetTop(temp_point.y);
		m_RectangleSelectionOnCanvas.SetBottom(m_StartSelectingAreaPoint.m_y);
	}
	/* II quadrant (left upper) */
	else if (
		temp_point.x <= m_StartSelectingAreaPoint.m_x && 
		temp_point.y <= m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(temp_point.x);
		m_RectangleSelectionOnCanvas.SetRight(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetTop(temp_point.y);
		m_RectangleSelectionOnCanvas.SetBottom(m_StartSelectingAreaPoint.m_y);
	}
	/* III quadrant (left bottom) */
	else if (
		temp_point.x <= m_StartSelectingAreaPoint.m_x && 
		temp_point.y > m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(temp_point.x);
		m_RectangleSelectionOnCanvas.SetRight(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetTop(m_StartSelectingAreaPoint.m_y);
		m_RectangleSelectionOnCanvas.SetBottom(temp_point.y);
	}
	else if (
		temp_point.x > m_StartSelectingAreaPoint.m_x &&
		temp_point.y > m_StartSelectingAreaPoint.m_y)
	{
		m_RectangleSelectionOnCanvas.SetLeft(m_StartSelectingAreaPoint.m_x);
		m_RectangleSelectionOnCanvas.SetRight(temp_point.x);
		m_RectangleSelectionOnCanvas.SetTop(m_StartSelectingAreaPoint.m_y);
		m_RectangleSelectionOnCanvas.SetBottom(temp_point.y);
	}
}

void RectangleSelectionTool::StopSelectingRegion()
{
	m_UserSelectingAreaOfRectangle = false;
}

bool RectangleSelectionTool::CanPanSelectedRegion() const
{
	return m_ToolButtonActive && m_MouseHoverOverImage && m_CursorInsideRectangleSelection;
}

void RectangleSelectionTool::ProcessPanningSelectedRegion()
{
	m_UserPanningSelectedRectangleSelection = true;
	m_StartPanRectangleSelection = m_RectangleSelectionOnImage.GetLeftTop();
	m_StartPanSizeCursorPos = m_CursorOnImage;
	LoopPanningSelectedRegion();
}

void RectangleSelectionTool::LoopPanningSelectedRegion()
{
	m_DeltaPoint =
	{
		(int)(m_CursorOnImage.x - m_StartPanSizeCursorPos.x),
		(int)(m_CursorOnImage.y - m_StartPanSizeCursorPos.y)
	};
	m_RectangleSelectionOnImage.SetLeft(m_StartPanRectangleSelection.x + m_DeltaPoint.x >= 0 ? m_StartPanRectangleSelection.x + m_DeltaPoint.x : 0);
	m_RectangleSelectionOnImage.SetLeft(m_StartPanRectangleSelection.x + m_DeltaPoint.x + m_RectangleSelectionOnImage.GetWidth() < m_ImageSize.GetWidth() ? m_RectangleSelectionOnImage.GetLeft() : m_ImageSize.GetWidth() - m_RectangleSelectionOnImage.GetWidth());
	m_RectangleSelectionOnImage.SetTop(m_StartPanRectangleSelection.y + m_DeltaPoint.y >= 0 ? m_StartPanRectangleSelection.y + m_DeltaPoint.y : 0);
	m_RectangleSelectionOnImage.SetTop(m_StartPanRectangleSelection.y + m_DeltaPoint.y + m_RectangleSelectionOnImage.GetHeight() < m_ImageSize.GetHeight() ? m_RectangleSelectionOnImage.GetTop() : m_ImageSize.GetHeight() - m_RectangleSelectionOnImage.GetHeight());

	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrlsWithRefresh();
}

void RectangleSelectionTool::StopPanningSelectedRegion()
{
	m_UserPanningSelectedRectangleSelection = false;
}

bool RectangleSelectionTool::CanChangeSize() const
{
	return m_MouseHoverOverImage && m_ToolButtonActive &&
		(
			m_CursorAboveVerticalLeftLineMiddlePoint || 
			m_CursorAboveLeftUpperCornerPoint || 
			m_CursorAboveHorizontalUpperLineMiddlePoint || 
			m_CursorAboveRightUpperCornerPoint || 
			m_CursorAboveVerticalRightLineMiddlePoint || 
			m_CursorAboveRightBottomCornerPoint || 
			m_CursorAboveHorizontalBottomLineMiddlePoint || 
			m_CursorAboveLeftBottomCornerPoint || 
			m_CursorAboveVerticalLeftLineMiddlePoint
			);
}

void RectangleSelectionTool::ProcessChangingSizeSelectedRegion()
{
	m_UserChangingSizeSelectedRectangleSelection = true;
	m_ChangingLMPoint = m_ChangingUMPoint = m_ChangingRMPoint = m_ChangingBMPoint = false;
	m_ChangingLUPoint = m_ChangingRUPoint = m_ChangingRBPoint = m_ChangingLBPoint = false;

	if (m_CursorAboveVerticalLeftLineMiddlePoint) m_ChangingLMPoint = true;
	else if (m_CursorAboveLeftUpperCornerPoint) m_ChangingLUPoint = true;
	else if (m_CursorAboveHorizontalUpperLineMiddlePoint) m_ChangingUMPoint = true;
	else if (m_CursorAboveRightUpperCornerPoint) m_ChangingRUPoint = true;
	else if (m_CursorAboveVerticalRightLineMiddlePoint) m_ChangingRMPoint = true;
	else if (m_CursorAboveRightBottomCornerPoint) m_ChangingRBPoint = true;
	else if (m_CursorAboveHorizontalBottomLineMiddlePoint) m_ChangingBMPoint = true;
	else if (m_CursorAboveLeftBottomCornerPoint) m_ChangingLBPoint = true;

	m_StartSizingRectangleSelection = m_RectangleSelectionOnImage;
	//m_StartPanRectangleSelection = m_RectangleSelectionOnImage.GetLeftTop();
	m_StartPanSizeCursorPos = m_CursorOnImage;
	LoopChangingSizeSelectedRegion();
}

void RectangleSelectionTool::LoopChangingSizeSelectedRegion()
{
	m_DeltaPoint =
	{
		(int)(m_CursorOnImage.x - m_StartPanSizeCursorPos.x),
		(int)(m_CursorOnImage.y - m_StartPanSizeCursorPos.y)
	};
	if (m_ChangingLMPoint)
	{
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight());
	}
	else if (m_ChangingLUPoint)
	{
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetRightBottom(m_StartSizingRectangleSelection.GetRightBottom());
	}
	else if (m_ChangingUMPoint)
	{
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom());
	}
	else if (m_ChangingRUPoint)
	{
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetLeftBottom(m_StartSizingRectangleSelection.GetLeftBottom());
	}
	else if (m_ChangingRMPoint)
	{
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft());
	}
	else if (m_ChangingRBPoint)
	{
		m_RectangleSelectionOnImage.SetRight(m_StartSizingRectangleSelection.GetRight() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetLeftTop(m_StartSizingRectangleSelection.GetLeftTop());
	}
	else if (m_ChangingBMPoint)
	{
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetTop(m_StartSizingRectangleSelection.GetTop());
	}
	else if (m_ChangingLBPoint)
	{
		m_RectangleSelectionOnImage.SetLeft(m_StartSizingRectangleSelection.GetLeft() + m_DeltaPoint.x);
		m_RectangleSelectionOnImage.SetBottom(m_StartSizingRectangleSelection.GetBottom() + m_DeltaPoint.y);
		m_RectangleSelectionOnImage.SetRightTop(m_StartSizingRectangleSelection.GetRightTop());
	}

	CalculateRectangleSelectionPosOnCanvas();
	UpdateParentRectangleTextCtrlsWithRefresh();
}

void RectangleSelectionTool::StopChangingSizeSelectedRegion()
{
	m_UserChangingSizeSelectedRectangleSelection = false;
}

void RectangleSelectionTool::UpdateParentRectangleTextCtrlsWithRefresh()
{
	m_ParentStartXPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetLeft() + m_TextCtrlPixelOffset)));
	m_ParentStartYPosTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetTop() + m_TextCtrlPixelOffset)));
	m_ParentWidthTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetWidth())));
	m_ParentHeightTextCtrl->SetValue(wxString::Format(wxT("%i"), (int)(m_RectangleSelectionOnImage.GetHeight())));
}

void RectangleSelectionTool::CheckIfMouseAboveRectangleSelection()
{
	if (m_MouseHoverOverImage)
	{
		/* Reset values */
		m_CursorInsideRectangleSelection = false;
		m_CursorOutsideOfRectangleSelection = true;
		m_CursorAboveVerticalLeftLineMiddlePoint = m_CursorAboveHorizontalUpperLineMiddlePoint = m_CursorAboveVerticalRightLineMiddlePoint = m_CursorAboveHorizontalBottomLineMiddlePoint = false;
		m_CursorAboveLeftUpperCornerPoint = m_CursorAboveRightUpperCornerPoint = m_CursorAboveRightBottomCornerPoint = m_CursorAboveLeftBottomCornerPoint = false;;


		/* Left Column */
		if (
			m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() - m_PointSize.GetWidth() / 2.0 &&
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() + m_PointSize.GetWidth() / 2.0)
		{
			/* Left Upper Point */
			if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveLeftUpperCornerPoint = true;
			}
			/* Left Middle Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveVerticalLeftLineMiddlePoint = true;
			}
			/* Left Bottom Point */
			else if (m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveLeftBottomCornerPoint = true;
			}
			m_CursorOutsideOfRectangleSelection = false;
		}
		/* Middle Column */
		else if (
			m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 - m_PointSize.GetWidth() / 2.0 &&
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() + m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() / 2.0 + m_PointSize.GetWidth() / 2.0)
		{
			/* Upper Middle Point */
			if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveHorizontalUpperLineMiddlePoint = true;
			}
			/* Lower Middle Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveHorizontalBottomLineMiddlePoint = true;
			}
			m_CursorOutsideOfRectangleSelection = false;
		}
		/* Right Column */
		else if (
			m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetRight() - m_PointSize.GetWidth() / 2.0 &&
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetRight() + m_PointSize.GetWidth() / 2.0)
		{
			/* Upper Right Point */
			if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveRightUpperCornerPoint = true;
			}
			/* Middle Right Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() + m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() / 2.0 + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveVerticalRightLineMiddlePoint = true;
			}
			/* Bottom Right Point */
			else if (
				m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() - m_PointSize.GetHeight() / 2.0 &&
				m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom() + m_PointSize.GetHeight() / 2.0)
			{
				m_CursorAboveRightBottomCornerPoint = true;
			}
			m_CursorOutsideOfRectangleSelection = false;
		}

		if (m_CursorOnCanvas.x >= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetLeft() && 
			m_CursorOnCanvas.x <= m_ImageStartDraw.x + m_VisualRectangleSelectionOnCanvas.GetRight() &&
			m_CursorOnCanvas.y >= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetTop() && 
			m_CursorOnCanvas.y <= m_ImageStartDraw.y + m_VisualRectangleSelectionOnCanvas.GetBottom())
		{
			if (
				!m_CursorAboveVerticalLeftLineMiddlePoint &&
				!m_CursorAboveLeftUpperCornerPoint &&
				!m_CursorAboveHorizontalUpperLineMiddlePoint &&
				!m_CursorAboveRightUpperCornerPoint &&
				!m_CursorAboveVerticalRightLineMiddlePoint &&
				!m_CursorAboveRightBottomCornerPoint &&
				!m_CursorAboveHorizontalBottomLineMiddlePoint &&
				!m_CursorAboveLeftBottomCornerPoint)
			{
				m_CursorInsideRectangleSelection = true;
				m_CursorOutsideOfRectangleSelection = false;
			}
		}

	}
}

void RectangleSelectionTool::CalculateRectangleSelectionPosOnCanvas()
{
	double multiplication_coefficient = m_Zoom / m_ZoomOriginalSizeImage;
	m_RectangleSelectionOnCanvas.SetLeft(m_RectangleSelectionOnImage.GetLeft() * multiplication_coefficient);
	m_RectangleSelectionOnCanvas.SetTop(m_RectangleSelectionOnImage.GetTop() * multiplication_coefficient);
	m_RectangleSelectionOnCanvas.SetRight(m_RectangleSelectionOnImage.GetRight() * multiplication_coefficient);
	m_RectangleSelectionOnCanvas.SetBottom(m_RectangleSelectionOnImage.GetBottom() * multiplication_coefficient);


	{
		m_VisualRectangleSelectionOnCanvas.SetLeftTop(m_RectangleSelectionOnCanvas.GetLeftTop());
		m_VisualRectangleSelectionOnCanvas.SetRight(m_RectangleSelectionOnCanvas.GetRight() + m_ActualPixelSize.x);
		m_VisualRectangleSelectionOnCanvas.SetBottom(m_RectangleSelectionOnCanvas.GetBottom() + m_ActualPixelSize.y);

		if (m_Zoom / m_ZoomOriginalSizeImage > 1.0)
		{
			m_VisualRectangleSelectionOnCanvas.SetRight(m_VisualRectangleSelectionOnCanvas.GetRight() - 1.0);
			m_VisualRectangleSelectionOnCanvas.SetBottom(m_VisualRectangleSelectionOnCanvas.GetBottom() - 1.0);
		}

		/* Checking if it's necessary to draw points on the edges of the rectangle */
		m_DrawHorizontalPoints = m_DrawVerticalPoints = false;
		if (m_VisualRectangleSelectionOnCanvas.GetSize().GetWidth() > 2 * m_PointSize.GetWidth())
			m_DrawHorizontalPoints = true;
		if (m_VisualRectangleSelectionOnCanvas.GetSize().GetHeight() > 2 * m_PointSize.GetHeight())
			m_DrawVerticalPoints = true;
	}
}

void RectangleSelectionTool::CalculateRectangleSelectionPosOnImage()
{

	double multiplication_factor = m_ZoomOriginalSizeImage / m_Zoom;

	m_RectangleSelectionOnImage.SetLeft(m_RectangleSelectionOnCanvas.GetLeft() * multiplication_factor);
	m_RectangleSelectionOnImage.SetTop(m_RectangleSelectionOnCanvas.GetTop() * multiplication_factor);
	m_RectangleSelectionOnImage.SetRight(m_RectangleSelectionOnCanvas.GetRight() * multiplication_factor);
	m_RectangleSelectionOnImage.SetBottom(m_RectangleSelectionOnCanvas.GetBottom() * multiplication_factor);

	/* Checking Left Line */
	m_RectangleSelectionOnImage.SetLeft(m_RectangleSelectionOnImage.GetLeft() >= 0 ? m_RectangleSelectionOnImage.GetLeft() : 0);
	m_RectangleSelectionOnImage.SetLeft(m_RectangleSelectionOnImage.GetLeft() < m_ImageSize.GetWidth() ? m_RectangleSelectionOnImage.GetLeft() : m_ImageSize.GetWidth() - 1);
	/* Checking Right Line */
	m_RectangleSelectionOnImage.SetRight(m_RectangleSelectionOnImage.GetRight() >= 0 ? m_RectangleSelectionOnImage.GetRight() : 0);
	m_RectangleSelectionOnImage.SetRight(m_RectangleSelectionOnImage.GetRight() < m_ImageSize.GetWidth() ? m_RectangleSelectionOnImage.GetRight() : m_ImageSize.GetWidth() - 1);
	/* Checking Upper Line */
	m_RectangleSelectionOnImage.SetTop(m_RectangleSelectionOnImage.GetTop() >= 0 ? m_RectangleSelectionOnImage.GetTop() : 0);
	m_RectangleSelectionOnImage.SetTop(m_RectangleSelectionOnImage.GetTop() < m_ImageSize.GetHeight() ? m_RectangleSelectionOnImage.GetTop() : m_ImageSize.GetHeight() - 1);
	/* Checking Bottom Line */
	m_RectangleSelectionOnImage.SetBottom(m_RectangleSelectionOnImage.GetBottom() >= 0 ? m_RectangleSelectionOnImage.GetBottom() : 0);
	m_RectangleSelectionOnImage.SetBottom(m_RectangleSelectionOnImage.GetBottom() < m_ImageSize.GetHeight() ? m_RectangleSelectionOnImage.GetBottom() : m_ImageSize.GetHeight() - 1);
}
