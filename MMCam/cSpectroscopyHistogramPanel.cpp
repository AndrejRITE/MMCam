#include "cSpectroscopyHistogramPanel.h"

BEGIN_EVENT_TABLE(cSpectroscopyHistogramPanel, wxPanel)
	EVT_PAINT(cSpectroscopyHistogramPanel::PaintEvent)
	EVT_SIZE(cSpectroscopyHistogramPanel::OnSize)
	EVT_MOTION(cSpectroscopyHistogramPanel::OnMouseMove)
	EVT_LEAVE_WINDOW(cSpectroscopyHistogramPanel::OnMouseLeave)
	EVT_MOUSEWHEEL(cSpectroscopyHistogramPanel::OnMouseWheel)
	EVT_RIGHT_UP(cSpectroscopyHistogramPanel::OnRightUp)
	EVT_LEFT_DCLICK(cSpectroscopyHistogramPanel::OnLeftDClick)
END_EVENT_TABLE()

cSpectroscopyHistogramPanel::cSpectroscopyHistogramPanel(wxWindow* parent, wxSizer* parentSizer, const int borderSize)
    : wxPanel(parent)
{
    SetDoubleBuffered(true);
    SetMinSize(wxSize(200, 140));
    parentSizer->Add(this, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, borderSize);
}

void cSpectroscopyHistogramPanel::SetHistogram(const unsigned long long* histogram, size_t histogramSize, unsigned long long totalEvents)
{
    if (!histogram || histogramSize == 0)
        return;

    m_Histogram.assign(histogram, histogram + histogramSize);
    m_TotalEvents = totalEvents;

    if (!m_ViewInitialized || m_ViewMax >= histogramSize)
    {
        ResetViewToFull();
    }
    else
    {
        m_ViewMax = std::min<unsigned int>(m_ViewMax, static_cast<unsigned int>(histogramSize - 1));
        if (m_ViewMin > m_ViewMax)
            ResetViewToFull();
    }

    Refresh(false);
}

void cSpectroscopyHistogramPanel::ResetHistogram()
{
    m_Histogram.clear();
    m_TotalEvents = 0;
    m_ViewPeak = 0;
    m_ViewInitialized = false;
    Refresh(false);
}

void cSpectroscopyHistogramPanel::SetBackgroundColor(const wxColour& colour)
{
    m_BackgroundColour = colour;
    Refresh(false);
}

void cSpectroscopyHistogramPanel::SetLogScale(bool enabled)
{
    m_LogScale = enabled;
    Refresh(false);
}

void cSpectroscopyHistogramPanel::PaintEvent(wxPaintEvent& evt)
{
    wxBufferedPaintDC dc(this);
    Render(dc);
}

void cSpectroscopyHistogramPanel::OnSize(wxSizeEvent& evt)
{
    m_CanvasSize = evt.GetSize();
    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnMouseMove(wxMouseEvent& evt)
{
    m_CursorPos = evt.GetPosition();
    m_MouseInside = true;
    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnMouseLeave(wxMouseEvent& evt)
{
    m_MouseInside = false;
    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnMouseWheel(wxMouseEvent& evt)
{
    if (m_Histogram.empty())
        return;

    const unsigned int fullMin = 0;
    const unsigned int fullMax = static_cast<unsigned int>(m_Histogram.size() - 1);
    const unsigned int fullSpan = std::max(1u, fullMax - fullMin);
    const unsigned int oldSpan = std::max(1u, m_ViewMax - m_ViewMin);

    const double factor = evt.GetWheelRotation() > 0 ? 0.80 : 1.25;
    unsigned int newSpan = static_cast<unsigned int>(std::round(oldSpan * factor));
    newSpan = std::clamp(newSpan, 1u, fullSpan);

    const double cursorFrac = m_CanvasSize.GetWidth() > 1
        ? std::clamp(static_cast<double>(evt.GetX()) / static_cast<double>(m_CanvasSize.GetWidth() - 1), 0.0, 1.0)
        : 0.5;

    const unsigned int anchor = CanvasXToBin(evt.GetX());

    long long newMin = static_cast<long long>(anchor) - static_cast<long long>(std::llround(cursorFrac * newSpan));
    long long newMax = newMin + static_cast<long long>(newSpan);

    if (newMin < fullMin)
    {
        newMin = fullMin;
        newMax = newMin + newSpan;
    }

    if (newMax > fullMax)
    {
        newMax = fullMax;
        newMin = newMax - newSpan;
    }

    if (newMin < fullMin)
        newMin = fullMin;

    m_ViewMin = static_cast<unsigned int>(newMin);
    m_ViewMax = static_cast<unsigned int>(newMax);

    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnRightUp(wxMouseEvent& evt)
{
    m_LogScale = !m_LogScale;
    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnLeftDClick(wxMouseEvent& evt)
{
    ResetViewToFull();
    Refresh(false);
}

void cSpectroscopyHistogramPanel::Render(wxBufferedPaintDC& dc)
{
    dc.SetBackground(wxBrush(m_BackgroundColour));
    dc.Clear();

    auto* gc = wxGraphicsContext::Create(dc);
    if (!gc)
        return;

    DrawHistogram(gc);
    DrawAxes(gc);
    DrawCursorOverlay(gc);

    delete gc;
}

void cSpectroscopyHistogramPanel::DrawHistogram(wxGraphicsContext* gc)
{
    if (!gc || m_Histogram.empty() || m_CanvasSize.GetWidth() <= 2 || m_CanvasSize.GetHeight() <= 2)
        return;

    const unsigned int viewMin = std::min<unsigned int>(m_ViewMin, static_cast<unsigned int>(m_Histogram.size() - 1));
    const unsigned int viewMax = std::min<unsigned int>(m_ViewMax, static_cast<unsigned int>(m_Histogram.size() - 1));

    if (viewMax <= viewMin)
        return;

    m_ViewPeak = 0;

    for (unsigned int i = viewMin; i <= viewMax; ++i)
        m_ViewPeak = std::max(m_ViewPeak, m_Histogram[i]);

    if (m_ViewPeak == 0)
        return;

    gc->SetPen(wxPen(m_HistogramColour, 1));
    gc->SetBrush(wxBrush(m_HistogramColour));

    const int W = m_CanvasSize.GetWidth();
    const int H = m_CanvasSize.GetHeight();
    const unsigned int span = std::max(1u, viewMax - viewMin);

    for (int x = 0; x < W; ++x)
    {
        const unsigned int bin0 = viewMin + static_cast<unsigned int>((static_cast<unsigned long long>(x) * span) / std::max(1, W - 1));
        const unsigned int bin1 = viewMin + static_cast<unsigned int>((static_cast<unsigned long long>(x + 1) * span) / std::max(1, W - 1));

        unsigned long long count = 0;

        for (unsigned int b = bin0; b <= std::min(bin1, viewMax); ++b)
            count = std::max(count, m_Histogram[b]);

        if (count == 0)
            continue;

        const int y = CountToCanvasY(count);
        gc->StrokeLine(x, H - 18, x, y);
    }
}

void cSpectroscopyHistogramPanel::DrawAxes(wxGraphicsContext* gc)
{
    if (!gc || m_CanvasSize.GetWidth() <= 0 || m_CanvasSize.GetHeight() <= 0)
        return;

    const int W = m_CanvasSize.GetWidth();
    const int H = m_CanvasSize.GetHeight();

    gc->SetPen(wxPen(m_AxisColour, 1));
    gc->StrokeLine(0, H - 18, W, H - 18);

    wxFont font(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    gc->SetFont(font, m_AxisColour);

    const wxString left = wxString::Format(wxT("%u"), m_ViewMin);
    const wxString right = wxString::Format(wxT("%u"), m_ViewMax);

    const wxString title = wxString::Format
    (
        wxT("Accumulated spectroscopy histogram  |  Events: %s  |  Peak: %s%s"),
        FormatCount(m_TotalEvents),
        FormatCount(m_ViewPeak),
        m_LogScale ? wxT("  |  log") : wxT("")
    );

    wxDouble tw{}, th{};
    gc->GetTextExtent(right, &tw, &th);

    gc->DrawText(left, 4, H - 16);
    gc->DrawText(right, W - tw - 4, H - 16);
    gc->DrawText(title, 6, 4);
}

void cSpectroscopyHistogramPanel::DrawCursorOverlay(wxGraphicsContext* gc)
{
    if (!gc || !m_MouseInside || m_Histogram.empty() || m_CanvasSize.GetWidth() <= 0)
        return;

    const unsigned int bin = CanvasXToBin(m_CursorPos.x);

    if (bin >= m_Histogram.size())
        return;

    const unsigned long long count = m_Histogram[bin];
    const int x = BinToCanvasX(bin);
    const int y = CountToCanvasY(count);

    gc->SetPen(wxPen(wxColour(255, 255, 255, 90), 1, wxPENSTYLE_DOT));
    gc->StrokeLine(x, 0, x, m_CanvasSize.GetHeight() - 18);

    const wxString text = wxString::Format(wxT("ADU %u  |  %s"), bin, FormatCount(count));

    wxFont font(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    gc->SetFont(font, m_OverlayColour);

    wxDouble tw{}, th{};
    gc->GetTextExtent(text, &tw, &th);

    double bx = std::clamp<double>
        (
            x - tw / 2.0 - 8.0,
            2.0,
            std::max(2.0, m_CanvasSize.GetWidth() - tw - 18.0)
        );

    double by = std::max(22.0, static_cast<double>(y) - th - 18.0);

    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(wxColour(0, 0, 0, 180)));
    gc->DrawRoundedRectangle(bx, by, tw + 16.0, th + 8.0, 5.0);
    gc->DrawText(text, bx + 8.0, by + 4.0);

    gc->SetPen(wxPen(m_HistogramColour, 2));
    gc->SetBrush(wxBrush(m_HistogramColour));
    gc->DrawEllipse(x - 3.0, y - 3.0, 6.0, 6.0);
}

void cSpectroscopyHistogramPanel::ResetViewToFull()
{
    m_ViewMin = 0;
    m_ViewMax = m_Histogram.empty() ? 0 : static_cast<unsigned int>(m_Histogram.size() - 1);
    m_ViewInitialized = !m_Histogram.empty();
}

unsigned int cSpectroscopyHistogramPanel::CanvasXToBin(int x) const
{
    if (m_Histogram.empty() || m_CanvasSize.GetWidth() <= 1)
        return 0;

    x = std::clamp(x, 0, m_CanvasSize.GetWidth() - 1);

    const unsigned int span = std::max(1u, m_ViewMax - m_ViewMin);

    return std::min<unsigned int>
        (
            static_cast<unsigned int>(m_Histogram.size() - 1),
            m_ViewMin + static_cast<unsigned int>
            (
                std::llround
                (
                    static_cast<double>(x) * span / static_cast<double>(m_CanvasSize.GetWidth() - 1)
                )
                )
        );
}

int cSpectroscopyHistogramPanel::BinToCanvasX(unsigned int bin) const
{
    if (m_CanvasSize.GetWidth() <= 1)
        return 0;

    const unsigned int span = std::max(1u, m_ViewMax - m_ViewMin);
    const double t = static_cast<double>(std::clamp(bin, m_ViewMin, m_ViewMax) - m_ViewMin) / static_cast<double>(span);

    return static_cast<int>(std::round(t * (m_CanvasSize.GetWidth() - 1)));
}

int cSpectroscopyHistogramPanel::CountToCanvasY(unsigned long long count) const
{
    const int H = std::max(1, m_CanvasSize.GetHeight() - 18);

    if (m_ViewPeak == 0 || count == 0)
        return H;

    double normalized = 0.0;

    if (m_LogScale)
        normalized = std::log1p(static_cast<double>(count)) / std::log1p(static_cast<double>(m_ViewPeak));
    else
        normalized = static_cast<double>(count) / static_cast<double>(m_ViewPeak);

    normalized = std::clamp(normalized, 0.0, 1.0);

    return static_cast<int>(std::round((H - 1) * (1.0 - normalized)));
}

wxString cSpectroscopyHistogramPanel::FormatCount(unsigned long long value) const
{
    wxString s = wxString::Format(wxT("%llu"), value);

    for (int i = static_cast<int>(s.length()) - 3; i > 0; i -= 3)
        s.insert(i, wxT("'"));

    return s;
}
