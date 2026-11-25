#include "cHistogramPanel.h"

#include <cmath>

BEGIN_EVENT_TABLE(cHistogramPanel, wxPanel)
	EVT_PAINT(cHistogramPanel::PaintEvent)
	EVT_SIZE(cHistogramPanel::OnSize)
	EVT_MOUSEWHEEL(cHistogramPanel::OnMouseWheel)
	EVT_LEFT_DOWN(cHistogramPanel::OnMouseDown)
	EVT_MOTION(cHistogramPanel::OnMouseMove)
	EVT_LEFT_UP(cHistogramPanel::OnMouseUp)
	EVT_MOUSE_CAPTURE_LOST(cHistogramPanel::OnMouseCaptureLost)
	EVT_RIGHT_UP(cHistogramPanel::OnRightUp)
	EVT_LEFT_DCLICK(cHistogramPanel::OnDoubleClick)
	EVT_ENTER_WINDOW(cHistogramPanel::OnMouseEnter)
	EVT_LEAVE_WINDOW(cHistogramPanel::OnMouseLeave)
END_EVENT_TABLE()

namespace
{
	// Layout constants (px)
	constexpr double kPadTop = 4.0;
	constexpr double kPadRight = 8.0;
	constexpr double kAxisWidth = 46.0;   // room for the count labels on the left
	constexpr double kRampGap = 4.0;
	constexpr double kRampHeight = 7.0;
	constexpr double kLabelRow = 15.0;    // value labels under the ramp
	constexpr double kHeadroom = 0.86;    // tallest bar uses this fraction of the plot height
	constexpr double kHandleGrab = 5.0;   // handle hit tolerance
	constexpr unsigned int kMinViewSpan = 8;

	const wxColour kAmber(255, 170, 40);

	auto NiceStep(double span, int targetTicks) -> double
	{
		if (span <= 0.0) return 1.0;
		const double raw = span / std::max(1, targetTicks);
		const double pow10 = std::pow(10.0, std::floor(std::log10(raw)));
		const double frac = raw / pow10;
		const double step = frac < 1.5 ? 1.0 : frac < 3.5 ? 2.0 : frac < 7.5 ? 5.0 : 10.0;
		return step * pow10;
	}

	auto WithAlpha(const wxColour& c, int alpha) -> wxColour
	{
		return wxColour(c.Red(), c.Green(), c.Blue(), (unsigned char)std::clamp(alpha, 0, 255));
	}

	auto Scale(const wxColour& c, double f) -> wxColour
	{
		auto s = [f](unsigned char v) { return (unsigned char)std::clamp((int)std::lround(v * f), 0, 255); };
		return wxColour(s(c.Red()), s(c.Green()), s(c.Blue()));
	}
}

cHistogramPanel::cHistogramPanel
(
	wxWindow* parent_frame,
	wxSizer* parent_sizer,
	wxTextCtrl* parent_black_txt_ctrl,
	wxTextCtrl* parent_white_txt_ctrl,
	const int borderSize,
	const wxString title
) :
	wxPanel(parent_frame),
	m_ParentLeftBorder(parent_black_txt_ctrl),
	m_ParentRightBorder(parent_white_txt_ctrl),
	m_TitleStr(title)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	parent_sizer->Add(this, 1, wxEXPAND | wxLEFT | wxRIGHT, borderSize);

	SetMinSize(wxSize(200, 120));

	m_NotifyTimer.SetOwner(this);
	Bind(wxEVT_TIMER, &cHistogramPanel::OnNotifyTimer, this, m_NotifyTimer.GetId());
}

// ---------------------------------------------------------------------------
// Data
// ---------------------------------------------------------------------------

auto cHistogramPanel::SetHistogram
(
	const HistogramPanelVariables::ImageDataTypes data_type,
	unsigned long long* data,
	unsigned long long min_value,
	unsigned long long max_value
) -> std::optional<int>
{
	// Take ownership first: callers hand over a released buffer.
	std::unique_ptr<unsigned long long[]> owned(data);

	if (!owned || !m_CanvasSize.GetWidth() || !m_CanvasSize.GetHeight()) return std::optional<int>(1);

	m_DataType = data_type;
	m_HistogramData = std::move(owned);
	m_MaxHistogramValue = m_DataType == HistogramPanelVariables::ImageDataTypes::RAW_12BIT ? 4'095U : 65'535U;

	// Statistics: first/last populated bin, mean, median and mode
	{
		const unsigned long long* h = m_HistogramData.get();
		m_TotalCount = 0;
		long double weighted = 0.0;
		unsigned long long modeCount = 0;
		m_FirstNonZero = m_MaxHistogramValue + 1;
		m_LastNonZero = 0;
		m_Mode = 0;
		for (unsigned int v = 0; v <= m_MaxHistogramValue; ++v)
		{
			const auto c = h[v];
			if (!c) continue;
			if (m_FirstNonZero > m_MaxHistogramValue) m_FirstNonZero = v;
			m_LastNonZero = v;
			m_TotalCount += c;
			weighted += (long double)v * (long double)c;
			if (c > modeCount) { modeCount = c; m_Mode = v; }
		}

		if (m_TotalCount)
		{
			m_Mean = (double)(weighted / (long double)m_TotalCount);
			const unsigned long long half = (m_TotalCount + 1) / 2;
			unsigned long long cumulative = 0;
			m_Median = m_LastNonZero;
			const unsigned long long upper = m_TotalCount - m_TotalCount / 2000; // 99.95 %
			m_UpperPercentile = m_LastNonZero;
			bool medianFound = false;
			for (unsigned int v = m_FirstNonZero; v <= m_LastNonZero; ++v)
			{
				cumulative += h[v];
				if (!medianFound && cumulative >= half) { m_Median = v; medianFound = true; }
				if (cumulative >= upper) { m_UpperPercentile = v; break; }
			}
		}
		else
		{
			m_FirstNonZero = 0;
			m_LastNonZero = m_MaxHistogramValue;
			m_Mean = 0.0;
			m_Median = 0;
			m_UpperPercentile = m_MaxHistogramValue;
		}
	}

	m_AutoLeft = (int)std::min<unsigned long long>(min_value, m_MaxHistogramValue);
	m_AutoRight = (int)std::min<unsigned long long>(max_value, m_MaxHistogramValue);

	// Keep user-set levels across frames; otherwise follow the automatic ones.
	if (!m_WasRangeChanged)
	{
		m_Left = m_AutoLeft;
		m_Right = m_AutoRight;
	}
	m_Left = std::clamp(m_Left, 0, (int)m_MaxHistogramValue);
	m_Right = std::clamp(m_Right, 0, (int)m_MaxHistogramValue);

	// Auto-fit the view only while the user hasn't zoomed or panned.
	if (!m_UserAdjustedView || m_ViewMax > m_MaxHistogramValue)
		FitViewToData();

	RebuildOutline();
	Refresh(false);

	return std::optional<int>();
}

auto cHistogramPanel::SetBordersPos(const int leftBorder, const int rightBorder) -> void
{
	if (leftBorder != m_Left || rightBorder != m_Right)
	{
		m_Left = leftBorder;
		m_Right = rightBorder;
		m_WasRangeChanged = true;
	}
	Refresh(false);
}

auto cHistogramPanel::SetBackgroundColor(wxColour color) -> void
{
	SetBackgroundColour(color);
	Refresh(false);
}

auto cHistogramPanel::FitViewToData() -> void
{
	if (!m_HistogramData || !m_MaxHistogramValue)
	{
		SetView(0, std::max(1u, m_MaxHistogramValue));
		return;
	}

	long long lo = std::min<long long>(m_FirstNonZero, m_Left);
	// Ignore a few stray hot pixels: fit to 99.95 % of the pixels (and the levels)
	long long hi = std::max<long long>(m_UpperPercentile, m_Right);
	const long long pad = std::max<long long>(1, (hi - lo) / 12);
	lo -= pad / 3;
	hi += pad;
	if (hi - lo + 1 < (long long)kMinViewSpan)
	{
		const long long mid = (lo + hi) / 2;
		lo = mid - kMinViewSpan / 2;
		hi = lo + kMinViewSpan - 1;
	}
	SetView(lo, hi);
}

auto cHistogramPanel::SetView(long long viewMin, long long viewMax) -> void
{
	const long long full = std::max(1u, m_MaxHistogramValue);
	long long span = std::clamp<long long>(viewMax - viewMin + 1, std::min<long long>(kMinViewSpan, full + 1), full + 1);
	if (viewMin < 0) viewMin = 0;
	if (viewMin + span - 1 > full) viewMin = full - span + 1;
	m_ViewMin = (unsigned int)std::max<long long>(0, viewMin);
	m_ViewMax = (unsigned int)std::min<long long>(full, m_ViewMin + span - 1);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

auto cHistogramPanel::UpdateLayout() -> void
{
	const double W = m_CanvasSize.GetWidth();
	const double H = m_CanvasSize.GetHeight();

	const double plotH = std::max(10.0, H - kPadTop - kRampGap - kRampHeight - kLabelRow);
	m_Plot = wxRect2DDouble(kAxisWidth, kPadTop, std::max(10.0, W - kAxisWidth - kPadRight), plotH);
	m_Ramp = wxRect2DDouble(m_Plot.m_x, m_Plot.GetBottom() + kRampGap, m_Plot.m_width, kRampHeight);
}

auto cHistogramPanel::ValueToX(double v) const -> double
{
	const double span = std::max(1.0, (double)m_ViewMax + 1.0 - (double)m_ViewMin);
	return m_Plot.m_x + (v - (double)m_ViewMin) / span * m_Plot.m_width;
}

auto cHistogramPanel::XToValue(double x) const -> int
{
	const double span = std::max(1.0, (double)m_ViewMax + 1.0 - (double)m_ViewMin);
	const double t = m_Plot.m_width > 0.0 ? (x - m_Plot.m_x) / m_Plot.m_width : 0.0;
	const long long v = (long long)std::floor((double)m_ViewMin + t * span);
	return (int)std::clamp<long long>(v, 0, (long long)m_MaxHistogramValue);
}

auto cHistogramPanel::CountToY(double c) const -> double
{
	if (m_ViewPeak == 0) return m_Plot.GetBottom();
	const double num = m_LogScale ? std::log1p(c) : c;
	const double den = m_LogScale ? std::log1p((double)m_ViewPeak) : (double)m_ViewPeak;
	const double t = den > 0.0 ? std::clamp(num / den, 0.0, 1.0) : 0.0;
	return m_Plot.GetBottom() - t * m_Plot.m_height * kHeadroom;
}

auto cHistogramPanel::IsDark() const -> bool
{
	const wxColour& bg = m_backgroundColour.IsOk() ? m_backgroundColour : GetBackgroundColour();
	return 0.299 * bg.Red() + 0.587 * bg.Green() + 0.114 * bg.Blue() < 128.0;
}

/// Builds the outline of the histogram for the current view and size.
/// When bins are wider than a pixel it is a step curve, otherwise each pixel
/// column shows the largest bin it covers (so narrow spikes never disappear).
auto cHistogramPanel::RebuildOutline() -> void
{
	m_Outline.clear();
	m_ViewPeak = 0;
	if (!m_HistogramData || m_Plot.m_width < 2.0 || m_ViewMax > m_MaxHistogramValue) return;

	const unsigned long long* h = m_HistogramData.get();
	// Scale to the tallest bin, ignoring the clipped end bins (0 and full scale):
	// black-clipped or saturated pixels often outnumber everything else and would
	// flatten the real distribution. Those spikes are drawn cut off at the top.
	unsigned long long innerPeak = 0, edgePeak = 0;
	for (unsigned int v = m_ViewMin; v <= m_ViewMax; ++v)
	{
		if (v == 0 || v == m_MaxHistogramValue) edgePeak = std::max(edgePeak, h[v]);
		else innerPeak = std::max(innerPeak, h[v]);
	}
	m_ViewPeak = innerPeak ? innerPeak : edgePeak;

	const double spanBins = (double)m_ViewMax + 1.0 - (double)m_ViewMin;
	if (spanBins <= m_Plot.m_width)
	{
		m_Outline.reserve((size_t)spanBins * 2);
		for (unsigned int v = m_ViewMin; v <= m_ViewMax; ++v)
		{
			const double y = CountToY((double)h[v]);
			m_Outline.emplace_back(ValueToX(v), y);
			m_Outline.emplace_back(ValueToX(v + 1.0), y);
		}
	}
	else
	{
		const int columns = (int)std::ceil(m_Plot.m_width);
		m_Outline.reserve(columns + 2);
		const double binsPerPx = spanBins / m_Plot.m_width;
		for (int px = 0; px < columns; ++px)
		{
			const auto first = (unsigned int)std::min<double>(m_ViewMax, m_ViewMin + std::floor(px * binsPerPx));
			const auto last = (unsigned int)std::clamp<double>(m_ViewMin + std::ceil((px + 1) * binsPerPx) - 1.0, first, m_ViewMax);
			unsigned long long peak = 0;
			for (unsigned int v = first; v <= last; ++v) peak = std::max(peak, h[v]);
			const double y = CountToY((double)peak);
			if (px == 0) m_Outline.emplace_back(m_Plot.m_x, y);
			m_Outline.emplace_back(m_Plot.m_x + px + 0.5, y);
		}
		m_Outline.emplace_back(m_Plot.GetRight(), m_Outline.back().m_y);
	}
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void cHistogramPanel::PaintEvent(wxPaintEvent& WXUNUSED(evt))
{
	wxBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();

	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
	if (!gc) return;
	Render(gc.get());
}

void cHistogramPanel::Render(wxGraphicsContext* gc)
{
	if (m_Plot.m_width < 10.0 || m_Plot.m_height < 10.0) return;

	DrawPlotBackground(gc);

	if (!m_HistogramData || m_Outline.empty())
	{
		DrawEmptyState(gc);
		return;
	}

	DrawGrid(gc);
	DrawHistogram(gc);
	DrawOutOfRangeShade(gc);
	DrawRamp(gc);
	DrawHoverReadout(gc);
	DrawChips(gc);
	DrawStats(gc);
	DrawHandles(gc);

	if (!IsEnabled())
	{
		gc->SetPen(*wxTRANSPARENT_PEN);
		gc->SetBrush(wxBrush(WithAlpha(GetBackgroundColour(), 120)));
		gc->DrawRectangle(0, 0, m_CanvasSize.GetWidth(), m_CanvasSize.GetHeight());
	}
}

void cHistogramPanel::DrawPlotBackground(wxGraphicsContext* gc)
{
	const wxColour bg = GetBackgroundColour();
	const bool dark = IsDark();
	gc->SetPen(wxPen(dark ? WithAlpha(*wxWHITE, 22) : WithAlpha(*wxBLACK, 28)));
	gc->SetBrush(wxBrush(dark ? Scale(bg, 0.80) : Scale(bg, 0.97)));
	gc->DrawRoundedRectangle(m_Plot.m_x - 0.5, m_Plot.m_y - 0.5, m_Plot.m_width + 1.0, m_Plot.m_height + 1.0, 4.0);
}

void cHistogramPanel::DrawEmptyState(wxGraphicsContext* gc)
{
	const bool dark = IsDark();
	wxFont font = GetFont();
	font.SetPointSize(9);
	gc->SetFont(font, dark ? wxColour(150, 150, 158) : wxColour(110, 110, 118));

	const wxString text = m_TitleStr.IsEmpty() ? wxString("Histogram appears here after a capture or when an image is opened") : m_TitleStr;
	wxDouble tw{}, th{};
	gc->GetTextExtent(text, &tw, &th);
	gc->DrawText(text, m_Plot.m_x + (m_Plot.m_width - tw) / 2.0, m_Plot.m_y + (m_Plot.m_height - th) / 2.0);
}

void cHistogramPanel::DrawGrid(wxGraphicsContext* gc)
{
	const bool dark = IsDark();
	const wxColour grid = dark ? WithAlpha(*wxWHITE, 16) : WithAlpha(*wxBLACK, 18);
	const wxColour label = dark ? wxColour(165, 165, 172) : wxColour(95, 95, 102);

	wxFont font = GetFont();
	font.SetPointSize(8);
	gc->SetFont(font, label);
	gc->SetPen(wxPen(grid, 1));

	// --- Y (counts) ---
	std::vector<double> yTicks;
	const double peak = (double)std::max(1ULL, m_ViewPeak) / kHeadroom; // value at the very top
	if (!m_LogScale)
	{
		const int target = std::clamp((int)(m_Plot.m_height / 34.0), 2, 5);
		const double step = std::max(1.0, NiceStep(peak, target));
		for (double v = step; v <= peak; v += step) yTicks.push_back(v);
	}
	else
	{
		for (double v = 10.0; v <= std::expm1(std::log1p((double)std::max(1ULL, m_ViewPeak)) / kHeadroom); v *= 10.0)
			yTicks.push_back(v);
	}

	wxDouble tw{}, th{};
	// "0" at the baseline, then labels upwards, skipping any that would overlap the previous one
	gc->GetTextExtent("0", &tw, &th);
	gc->DrawText("0", m_Plot.m_x - 6.0 - tw, m_Plot.GetBottom() - th);
	double lastLabelTop = m_Plot.GetBottom() - th;

	for (double v : yTicks)
	{
		const double y = std::round(CountToY(v)) + 0.5;
		if (y < m_Plot.m_y + 10.0) continue; // keep the top free for the level markers
		gc->StrokeLine(m_Plot.m_x, y, m_Plot.GetRight(), y);
		const wxString s = HistogramPanelVariables::FormatCompact(v);
		gc->GetTextExtent(s, &tw, &th);
		const double ly = std::max(0.0, y - th / 2.0);
		if (ly + th > lastLabelTop - 1.0) continue;
		gc->DrawText(s, m_Plot.m_x - 6.0 - tw, ly);
		lastLabelTop = ly;
	}

	// --- X (values) ---
	const double span = (double)m_ViewMax + 1.0 - (double)m_ViewMin;
	const int target = std::clamp((int)(m_Plot.m_width / 90.0), 2, 12);
	const double step = std::max(1.0, NiceStep(span, target));
	const double labelY = m_Ramp.GetBottom() + 1.0;
	double lastLabelRight = -1e9;
	for (double v = std::ceil(m_ViewMin / step) * step; v <= m_ViewMax; v += step)
	{
		const double x = std::round(ValueToX(v + 0.5)) + 0.5;
		gc->StrokeLine(x, m_Plot.m_y, x, m_Plot.GetBottom());

		const wxString s = HistogramPanelVariables::FormatNumber((long long)v);
		gc->GetTextExtent(s, &tw, &th);
		double lx = std::clamp(x - tw / 2.0, m_Plot.m_x, m_CanvasSize.GetWidth() - tw - 2.0);
		if (lx < lastLabelRight + 8.0) continue;
		gc->DrawText(s, lx, labelY);
		lastLabelRight = lx + tw;
	}
}

void cHistogramPanel::DrawHistogram(wxGraphicsContext* gc)
{
	gc->Clip(m_Plot.m_x, m_Plot.m_y, m_Plot.m_width, m_Plot.m_height);

	const double bottom = m_Plot.GetBottom();
	wxGraphicsPath area = gc->CreatePath();
	area.MoveToPoint(m_Outline.front().m_x, bottom);
	for (const auto& p : m_Outline) area.AddLineToPoint(p);
	area.AddLineToPoint(m_Outline.back().m_x, bottom);
	area.CloseSubpath();

	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->SetBrush(gc->CreateLinearGradientBrush(0, m_Plot.m_y, 0, bottom, WithAlpha(m_Accent, 200), WithAlpha(m_Accent, 45)));
	gc->FillPath(area);

	wxGraphicsPath line = gc->CreatePath();
	line.MoveToPoint(m_Outline.front());
	for (size_t i = 1; i < m_Outline.size(); ++i) line.AddLineToPoint(m_Outline[i]);
	gc->SetPen(wxPen(m_Accent.ChangeLightness(IsDark() ? 125 : 85), 1));
	gc->StrokePath(line);

	gc->ResetClip();
}

void cHistogramPanel::DrawOutOfRangeShade(wxGraphicsContext* gc)
{
	const double xl = std::clamp(LevelToX(m_Left), m_Plot.m_x, m_Plot.GetRight());
	const double xr = std::clamp(LevelToX(m_Right), m_Plot.m_x, m_Plot.GetRight());
	const wxColour shade = WithAlpha(IsDark() ? Scale(GetBackgroundColour(), 0.80) : Scale(GetBackgroundColour(), 0.97), 150);

	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->SetBrush(wxBrush(shade));
	if (xl > m_Plot.m_x) gc->DrawRectangle(m_Plot.m_x, m_Plot.m_y, xl - m_Plot.m_x, m_Plot.m_height);
	if (xr < m_Plot.GetRight()) gc->DrawRectangle(xr, m_Plot.m_y, m_Plot.GetRight() - xr, m_Plot.m_height);
}

void cHistogramPanel::DrawRamp(wxGraphicsContext* gc)
{
	// Shows how values map to display intensity: black below the left level,
	// a linear ramp between the levels and white above the right one.
	const double x0 = m_Ramp.m_x, x1 = m_Ramp.GetRight();
	const double xl = std::clamp(LevelToX(m_Left), x0, x1);
	const double xr = std::clamp(LevelToX(m_Right), x0, x1);

	gc->Clip(m_Ramp.m_x, m_Ramp.m_y, m_Ramp.m_width, m_Ramp.m_height);
	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->SetBrush(wxBrush(*wxBLACK));
	gc->DrawRectangle(x0, m_Ramp.m_y, xl - x0 + 0.5, m_Ramp.m_height);
	gc->SetBrush(wxBrush(*wxWHITE));
	gc->DrawRectangle(xr - 0.5, m_Ramp.m_y, x1 - xr + 0.5, m_Ramp.m_height);
	if (xr > xl)
	{
		// Gradient spans the true (unclamped) level positions so a partially visible ramp stays correct
		const double gl = LevelToX(m_Left), gr = LevelToX(m_Right);
		gc->SetBrush(gc->CreateLinearGradientBrush(gl, 0, gr, 0, *wxBLACK, *wxWHITE));
		gc->DrawRectangle(xl, m_Ramp.m_y, xr - xl, m_Ramp.m_height);
	}
	gc->ResetClip();

	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->SetPen(wxPen(IsDark() ? WithAlpha(*wxWHITE, 40) : WithAlpha(*wxBLACK, 50)));
	gc->DrawRoundedRectangle(m_Ramp.m_x - 0.5, m_Ramp.m_y - 0.5, m_Ramp.m_width + 1.0, m_Ramp.m_height + 1.0, 2.0);
}

wxRect2DDouble cHistogramPanel::DrawPill(wxGraphicsContext* gc, const wxString& text, double x, double y,
	const wxColour& fill, const wxColour& textColour, const wxColour& border)
{
	wxFont font = GetFont().Bold();
	font.SetPointSize(8);
	gc->SetFont(font, textColour);
	wxDouble tw{}, th{};
	gc->GetTextExtent(text, &tw, &th);
	const wxRect2DDouble r(x, y, tw + 10.0, th + 2.0);
	gc->SetPen(border.IsOk() ? wxPen(border, 1) : *wxTRANSPARENT_PEN);
	gc->SetBrush(wxBrush(fill));
	gc->DrawRoundedRectangle(r.m_x, r.m_y, r.m_width, r.m_height, r.m_height / 2.0);
	gc->DrawText(text, r.m_x + 5.0, r.m_y + 1.0);
	return r;
}

void cHistogramPanel::DrawHandles(wxGraphicsContext* gc)
{
	const bool dark = IsDark();
	wxFont font = GetFont().Bold();
	font.SetPointSize(8);
	gc->SetFont(font, *wxWHITE);

	const wxString leftText = HistogramPanelVariables::FormatNumber(m_Left);
	const wxString rightText = HistogramPanelVariables::FormatNumber(m_Right);
	wxDouble lw{}, rw{}, th{};
	gc->GetTextExtent(leftText, &lw, &th);
	gc->GetTextExtent(rightText, &rw, &th);
	lw += 10.0; rw += 10.0;

	const double xl = LevelToX(m_Left), xr = LevelToX(m_Right);
	const bool leftVisible = xl >= m_Plot.m_x - 0.5 && xl <= m_Plot.GetRight() + 0.5;
	const bool rightVisible = xr >= m_Plot.m_x - 0.5 && xr <= m_Plot.GetRight() + 0.5;

	// Pills sit inside the range; if they would collide, they go outside it.
	double leftPillX = xl + 4.0, rightPillX = xr - 4.0 - rw;
	if (leftPillX + lw + 4.0 > rightPillX)
	{
		leftPillX = xl - 4.0 - lw;
		rightPillX = xr + 4.0;
	}
	const double pillY = m_Plot.m_y + 9.0;

	auto drawHandle = [&](bool isLeft, double x, double pillX, const wxString& text)
	{
		const bool active = (isLeft && (m_Hover == Hover::Left || m_Drag == DragMode::Left)) ||
			(!isLeft && (m_Hover == Hover::Right || m_Drag == DragMode::Right)) ||
			m_Drag == DragMode::Select;
		const wxColour line = active ? kAmber : (dark ? WithAlpha(*wxWHITE, 170) : WithAlpha(*wxBLACK, 150));
		const double px = std::round(x) + 0.5;

		gc->SetPen(wxPen(line, active ? 2 : 1));
		gc->StrokeLine(px, m_Plot.m_y, px, m_Ramp.GetBottom());

		// Triangle marker at the top: black level dark, white level light
		wxGraphicsPath tri = gc->CreatePath();
		tri.MoveToPoint(px - 5.5, m_Plot.m_y);
		tri.AddLineToPoint(px + 5.5, m_Plot.m_y);
		tri.AddLineToPoint(px, m_Plot.m_y + 7.0);
		tri.CloseSubpath();
		gc->SetBrush(wxBrush(isLeft ? wxColour(24, 24, 28) : wxColour(245, 245, 245)));
		gc->SetPen(wxPen(active ? kAmber : (isLeft ? wxColour(235, 235, 235) : wxColour(40, 40, 44)), 1));
		gc->DrawPath(tri);

		const double clampedX = std::clamp(pillX, m_Plot.m_x + 2.0, m_Plot.GetRight() - (isLeft ? lw : rw) - 2.0);
		DrawPill(gc, text, clampedX, pillY,
			isLeft ? wxColour(20, 20, 24, 225) : wxColour(248, 248, 248, 235),
			isLeft ? *wxWHITE : wxColour(20, 20, 24),
			active ? kAmber : (isLeft ? wxColour(255, 255, 255, 60) : wxColour(0, 0, 0, 60)));
	};

	if (leftVisible) drawHandle(true, xl, leftPillX, leftText);
	if (rightVisible) drawHandle(false, xr, rightPillX, rightText);
}

void cHistogramPanel::DrawStats(wxGraphicsContext* gc)
{
	if (!m_TotalCount) return;

	const bool dark = IsDark();
	wxFont font = GetFont();
	font.SetPointSize(8);
	gc->SetFont(font, dark ? wxColour(190, 190, 198) : wxColour(80, 80, 88));

	const wxString text = wxString::Format("Mean %s   Median %s   Mode %s",
		HistogramPanelVariables::FormatNumber((long long)std::llround(m_Mean)),
		HistogramPanelVariables::FormatNumber(m_Median),
		HistogramPanelVariables::FormatNumber(m_Mode));
	wxDouble tw{}, th{};
	gc->GetTextExtent(text, &tw, &th);

	// Sits left of the chips in the top-right corner
	const double right = (m_ScaleChipRect.m_width > 0 ? m_ScaleChipRect.m_x : m_Plot.GetRight()) - 10.0;
	const double x = right - tw;
	if (x < m_Plot.m_x + m_Plot.m_width * 0.35) return; // not enough room
	gc->DrawText(text, x, m_Plot.GetBottom() - th - 3.0);
}

void cHistogramPanel::DrawChips(wxGraphicsContext* gc)
{
	const bool dark = IsDark();
	wxFont font = GetFont().Bold();
	font.SetPointSize(8);
	gc->SetFont(font, *wxWHITE);

	const bool canReset = m_WasRangeChanged || m_UserAdjustedView;
	const wxString scaleText = m_LogScale ? "LOG" : "LIN";
	const wxString resetText = canReset ? "RESET" : "AUTO";
	wxDouble sw{}, rw{}, th{};
	gc->GetTextExtent(scaleText, &sw, &th);
	gc->GetTextExtent(resetText, &rw, &th);

	const double y = m_Plot.GetBottom() - th - 5.0;
	const double resetX = m_Plot.GetRight() - 6.0 - (rw + 10.0);
	const double scaleX = resetX - 5.0 - (sw + 10.0);

	const wxColour neutralFill = dark ? wxColour(255, 255, 255, 28) : wxColour(0, 0, 0, 22);
	const wxColour neutralText = dark ? wxColour(210, 210, 216) : wxColour(60, 60, 66);

	const bool hoverScale = m_Hover == Hover::ScaleChip;
	m_ScaleChipRect = DrawPill(gc, scaleText, scaleX, y,
		m_LogScale ? WithAlpha(m_Accent, hoverScale ? 255 : 210) : (hoverScale ? WithAlpha(neutralText, 70) : neutralFill),
		m_LogScale ? *wxWHITE : neutralText);

	const bool hoverReset = m_Hover == Hover::ResetChip;
	m_ResetChipRect = DrawPill(gc, resetText, resetX, y,
		canReset ? WithAlpha(kAmber, hoverReset ? 255 : 215) : neutralFill,
		canReset ? wxColour(30, 22, 6) : WithAlpha(neutralText, 150));
	if (!canReset) m_ResetChipRect = wxRect2DDouble(); // "AUTO" is a status, not a button
}

void cHistogramPanel::DrawHoverReadout(wxGraphicsContext* gc)
{
	if (!m_IsMouseInside || m_Drag == DragMode::Pan || m_Hover != Hover::None) return;
	if (!m_Plot.Contains(wxPoint2DDouble(m_MousePos.x, m_MousePos.y))) return;

	const int v = std::clamp(XToValue(m_MousePos.x), (int)m_ViewMin, (int)m_ViewMax);
	const unsigned long long count = m_HistogramData[v];
	const bool dark = IsDark();

	const double x = std::round(m_MousePos.x) + 0.5;
	gc->SetPen(wxPen(dark ? WithAlpha(*wxWHITE, 70) : WithAlpha(*wxBLACK, 70), 1, wxPENSTYLE_SHORT_DASH));
	gc->StrokeLine(x, m_Plot.m_y, x, m_Plot.GetBottom());

	const double y = CountToY((double)count);
	gc->SetPen(wxPen(*wxWHITE, 1));
	gc->SetBrush(wxBrush(m_Accent));
	gc->DrawEllipse(x - 3.5, y - 3.5, 7.0, 7.0);

	wxFont font = GetFont().Bold();
	font.SetPointSize(8);
	gc->SetFont(font, *wxWHITE);
	const wxString text = wxString::Format("%s  →  %s px",
		HistogramPanelVariables::FormatNumber(v), HistogramPanelVariables::FormatNumber((long long)count));
	wxDouble tw{}, th{};
	gc->GetTextExtent(text, &tw, &th);
	double bx = x + 8.0;
	if (bx + tw + 10.0 > m_Plot.GetRight() - 4.0) bx = x - 8.0 - (tw + 10.0);
	const double by = m_Plot.m_y + m_Plot.m_height * 0.42;
	DrawPill(gc, text, bx, by, wxColour(20, 20, 24, 215), *wxWHITE, WithAlpha(m_Accent, 160));
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void cHistogramPanel::OnSize(wxSizeEvent& evt)
{
	m_CanvasSize = evt.GetSize();
	UpdateLayout();
	RebuildOutline();
	Refresh(false);
	evt.Skip();
}

auto cHistogramPanel::HitTest(const wxPoint& pt) const -> Hover
{
	if (!m_HistogramData) return Hover::None;

	const wxPoint2DDouble p(pt.x, pt.y);
	if (m_ScaleChipRect.m_width > 0 && m_ScaleChipRect.Contains(p)) return Hover::ScaleChip;
	if (m_ResetChipRect.m_width > 0 && m_ResetChipRect.Contains(p)) return Hover::ResetChip;

	if (p.m_y < m_Plot.m_y - 2.0 || p.m_y > m_Ramp.GetBottom() + 3.0) return Hover::None;

	const double xl = LevelToX(m_Left), xr = LevelToX(m_Right);
	const bool leftVisible = xl >= m_Plot.m_x - 0.5 && xl <= m_Plot.GetRight() + 0.5;
	const bool rightVisible = xr >= m_Plot.m_x - 0.5 && xr <= m_Plot.GetRight() + 0.5;
	const double dl = leftVisible ? std::abs(p.m_x - xl) : 1e9;
	const double dr = rightVisible ? std::abs(p.m_x - xr) : 1e9;
	if (std::min(dl, dr) > kHandleGrab) return Hover::None;
	if (std::abs(dl - dr) < 0.5) return p.m_x <= xl ? Hover::Left : Hover::Right; // handles overlap
	return dl < dr ? Hover::Left : Hover::Right;
}

void cHistogramPanel::UpdateHoverAndCursor(const wxPoint& pt)
{
	const Hover hover = HitTest(pt);
	if (hover != m_Hover) m_Hover = hover;

	switch (m_Hover)
	{
	case Hover::Left:
	case Hover::Right: SetCursor(wxCURSOR_SIZEWE); break;
	case Hover::ScaleChip:
	case Hover::ResetChip: SetCursor(wxCURSOR_HAND); break;
	default: SetCursor(wxNullCursor); break;
	}

	wxString tip;
	switch (m_Hover)
	{
	case Hover::Left: tip = "Black level - drag to change"; break;
	case Hover::Right: tip = "White level - drag to change"; break;
	case Hover::ScaleChip: tip = "Switch the count axis between linear and logarithmic (also right-click)"; break;
	case Hover::ResetChip: tip = "Restore the automatic levels and the full view (also double-click)"; break;
	default:
		tip = "Drag the triangles to set the black and white levels\n"
			"Shift+drag: select a new level range\n"
			"Drag: pan    Wheel: zoom\n"
			"Right-click: linear / log counts    Double-click: reset";
		break;
	}
	if (tip != m_CurrentTip)
	{
		m_CurrentTip = tip;
		SetToolTip(tip);
	}
}

void cHistogramPanel::OnMouseDown(wxMouseEvent& evt)
{
	if (!m_HistogramData) return;
	m_MousePos = evt.GetPosition();
	UpdateHoverAndCursor(m_MousePos);

	switch (m_Hover)
	{
	case Hover::ScaleChip: ToggleLogScale(); return;
	case Hover::ResetChip: ResetRangeAndView(); return;
	case Hover::Left: m_Drag = DragMode::Left; break;
	case Hover::Right: m_Drag = DragMode::Right; break;
	default:
		if (!m_Plot.Contains(wxPoint2DDouble(m_MousePos.x, m_MousePos.y))) return;
		if (evt.ShiftDown())
		{
			m_Drag = DragMode::Select;
			m_SelectAnchor = XToValue(m_MousePos.x);
			SetLevelsFromDrag(m_SelectAnchor, m_SelectAnchor + 1);
		}
		else
		{
			m_Drag = DragMode::Pan;
			m_PanStartX = m_MousePos.x;
			m_ViewMinAtDragStart = m_ViewMin;
			m_ViewMaxAtDragStart = m_ViewMax;
			SetCursor(wxCURSOR_SIZING);
		}
		break;
	}

	if (!HasCapture()) CaptureMouse();
	Refresh(false);
}

void cHistogramPanel::OnMouseMove(wxMouseEvent& evt)
{
	m_MousePos = evt.GetPosition();
	m_IsMouseInside = true; // enter events are not guaranteed (e.g. after a capture ends outside)
	if (!m_HistogramData) return;

	switch (m_Drag)
	{
	case DragMode::Left:
		SetLevelsFromDrag(std::min(XToValue(m_MousePos.x), m_Right - 1), m_Right);
		break;
	case DragMode::Right:
		SetLevelsFromDrag(m_Left, std::max(XToValue(m_MousePos.x), m_Left + 1));
		break;
	case DragMode::Select:
	{
		const int v = XToValue(m_MousePos.x);
		const int lo = std::min(v, m_SelectAnchor), hi = std::max(v, m_SelectAnchor);
		SetLevelsFromDrag(lo, std::max(hi, lo + 1));
		break;
	}
	case DragMode::Pan:
	{
		const double span = (double)m_ViewMaxAtDragStart + 1.0 - (double)m_ViewMinAtDragStart;
		const double binsPerPx = m_Plot.m_width > 0 ? span / m_Plot.m_width : 0.0;
		const long long delta = std::llround(-(m_MousePos.x - m_PanStartX) * binsPerPx);
		if (delta)
		{
			SetView((long long)m_ViewMinAtDragStart + delta, (long long)m_ViewMaxAtDragStart + delta);
			m_UserAdjustedView = true;
			RebuildOutline();
		}
		Refresh(false);
		break;
	}
	default:
		UpdateHoverAndCursor(m_MousePos);
		Refresh(false); // hover readout follows the cursor
		break;
	}
}

void cHistogramPanel::OnMouseUp(wxMouseEvent& WXUNUSED(evt))
{
	EndDrag();
}

void cHistogramPanel::OnMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(evt))
{
	// e.g. Alt+Tab or a modal dialog while dragging (wx asserts if this is not handled)
	EndDrag();
}

void cHistogramPanel::EndDrag()
{
	const DragMode finished = m_Drag;
	m_Drag = DragMode::None;
	if (HasCapture()) ReleaseMouse();

	m_NotifyTimer.Stop();
	if (finished == DragMode::Left || finished == DragMode::Right || finished == DragMode::Select)
	{
		m_NotifyPending = false;
		NotifyParent(); // final levels, without waiting for the timer
	}

	UpdateHoverAndCursor(m_MousePos);
	Refresh(false);
}

void cHistogramPanel::SetLevelsFromDrag(int left, int right)
{
	const int maxV = (int)m_MaxHistogramValue;
	left = std::clamp(left, 0, std::max(0, maxV - 1));
	right = std::clamp(right, left + 1, std::max(1, maxV));
	if (left == m_Left && right == m_Right) return;

	m_Left = left;
	m_Right = right;
	m_WasRangeChanged = true;

	// Paint the handle now. Recolouring a large preview takes much longer than a
	// mouse move, and Windows only delivers WM_PAINT once no input is waiting, so
	// relying on Refresh() alone left the handle frozen until the button was released.
	Refresh(false);
	Update();

	// Push the new levels to the preview once pending input has been handled; moves
	// arriving in the meantime are merged into a single update.
	m_NotifyPending = true;
	if (!m_NotifyTimer.IsRunning()) m_NotifyTimer.StartOnce(1);
}

void cHistogramPanel::OnNotifyTimer(wxTimerEvent& WXUNUSED(evt))
{
	if (!m_NotifyPending) return;
	m_NotifyPending = false;
	NotifyParent();
}

/// Pushes the levels to the parent, which re-renders the preview.
void cHistogramPanel::NotifyParent()
{
	if (!m_ParentLeftBorder || !m_ParentRightBorder) return;

	m_ParentLeftBorder->ChangeValue(wxString::Format(wxT("%i"), m_Left));
	m_ParentRightBorder->SetValue(wxString::Format(wxT("%i"), m_Right)); // fires the update in the parent
}

void cHistogramPanel::OnDoubleClick(wxMouseEvent& evt)
{
	// The second click of a double-click arrives as this event, so chips still act on it
	m_MousePos = evt.GetPosition();
	UpdateHoverAndCursor(m_MousePos);
	if (m_Hover == Hover::ScaleChip) { ToggleLogScale(); return; }
	ResetRangeAndView();
}

void cHistogramPanel::OnRightUp(wxMouseEvent& WXUNUSED(evt))
{
	ToggleLogScale();
}

void cHistogramPanel::ToggleLogScale()
{
	m_LogScale = !m_LogScale;
	RebuildOutline();
	Refresh(false);
}

void cHistogramPanel::ResetRangeAndView()
{
	m_Left = m_AutoLeft;
	m_Right = m_AutoRight;
	m_WasRangeChanged = false;
	m_UserAdjustedView = false;
	FitViewToData();
	RebuildOutline();
	if (m_HistogramData) NotifyParent();
	UpdateHoverAndCursor(m_MousePos);
	Refresh(false);
}

void cHistogramPanel::OnMouseWheel(wxMouseEvent& evt)
{
	if (!m_HistogramData || !m_MaxHistogramValue) return;

	const int rot = evt.GetWheelRotation();
	if (!rot) return;

	if (evt.ControlDown())
	{
		ToggleLogScale();
		return;
	}

	// Zoom the value axis around the value under the cursor
	const double span = (double)m_ViewMax + 1.0 - (double)m_ViewMin;
	const double t = m_Plot.m_width > 0 ? std::clamp((evt.GetX() - m_Plot.m_x) / m_Plot.m_width, 0.0, 1.0) : 0.5;
	const double anchor = m_ViewMin + t * span;
	const double newSpan = std::clamp(std::round(span * (rot > 0 ? 0.8 : 1.25)), (double)kMinViewSpan, (double)m_MaxHistogramValue + 1.0);
	const long long newMin = std::llround(anchor - t * newSpan);

	SetView(newMin, newMin + (long long)newSpan - 1);
	m_UserAdjustedView = true;
	RebuildOutline();
	Refresh(false);
}

void cHistogramPanel::OnMouseEnter(wxMouseEvent& evt)
{
	m_IsMouseInside = true;
	m_MousePos = evt.GetPosition();
	Refresh(false);
}

void cHistogramPanel::OnMouseLeave(wxMouseEvent& WXUNUSED(evt))
{
	m_IsMouseInside = false;
	if (m_Drag == DragMode::None)
	{
		m_Hover = Hover::None;
		SetCursor(wxNullCursor);
	}
	Refresh(false);
}
