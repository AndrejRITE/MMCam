#include "cSpectroscopyHistogramPanel.h"

namespace
{
    constexpr int HISTOGRAM_LEFT_MARGIN = 64;
    constexpr int HISTOGRAM_RIGHT_MARGIN = 8;
    constexpr int HISTOGRAM_TOP_MARGIN = 30;
    constexpr int HISTOGRAM_BOTTOM_MARGIN = 24;

    constexpr int HISTOGRAM_TARGET_Y_TICKS = 5;

    double CalculateNiceTickStep(double maxValue, int targetTickCount)
    {
        if (maxValue <= 0.0 || targetTickCount <= 1)
            return 1.0;

        const double rawStep = maxValue / static_cast<double>(targetTickCount - 1);
        const double magnitude = std::pow(10.0, std::floor(std::log10(rawStep)));
        const double normalized = rawStep / magnitude;

        double niceNormalized = 1.0;

        if (normalized <= 1.0)
            niceNormalized = 1.0;
        else if (normalized <= 2.0)
            niceNormalized = 2.0;
        else if (normalized <= 5.0)
            niceNormalized = 5.0;
        else
            niceNormalized = 10.0;

        return niceNormalized * magnitude;
    }
}

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
    SetMinSize(wxSize(200, 200));

    if (parentSizer)
        parentSizer->Add(this, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, borderSize);
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

    const wxRect plotRect = GetPlotRect();

    const double cursorFrac = plotRect.GetWidth() > 1
        ? std::clamp
        (
            static_cast<double>(evt.GetX() - plotRect.GetLeft()) /
            static_cast<double>(plotRect.GetWidth() - 1),
            0.0,
            1.0
        )
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

    UpdateViewPeak();

    DrawAxes(gc);
    DrawHistogram(gc);
    DrawCursorOverlay(gc);

    delete gc;
}

void cSpectroscopyHistogramPanel::DrawHistogram(wxGraphicsContext* gc)
{
    if (!gc || m_Histogram.empty() || m_CanvasSize.GetWidth() <= 2 || m_CanvasSize.GetHeight() <= 2)
        return;

    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 2 || plotRect.GetHeight() <= 2)
        return;

    const unsigned int viewMin = std::min<unsigned int>(m_ViewMin, static_cast<unsigned int>(m_Histogram.size() - 1));
    const unsigned int viewMax = std::min<unsigned int>(m_ViewMax, static_cast<unsigned int>(m_Histogram.size() - 1));

    if (viewMax <= viewMin || m_ViewPeak == 0)
        return;

    gc->SetPen(wxPen(m_HistogramColour, 1));
    gc->SetBrush(wxBrush(m_HistogramColour));

    const int plotLeft = plotRect.GetLeft();
    const int plotRight = plotRect.GetRight();
    const int plotBottom = plotRect.GetBottom();

    const int plotWidth = std::max(1, plotRect.GetWidth());
    const unsigned int span = std::max(1u, viewMax - viewMin);

    for (int px = 0; px < plotWidth; ++px)
    {
        const unsigned int bin0 = viewMin + static_cast<unsigned int>
            (
                (static_cast<unsigned long long>(px) * span) / std::max(1, plotWidth - 1)
                );

        const unsigned int bin1 = viewMin + static_cast<unsigned int>
            (
                (static_cast<unsigned long long>(px + 1) * span) / std::max(1, plotWidth - 1)
                );

        unsigned long long count = 0;

        for (unsigned int b = bin0; b <= std::min(bin1, viewMax); ++b)
            count = std::max(count, m_Histogram[b]);

        if (count == 0)
            continue;

        const int x = std::min(plotLeft + px, plotRight);
        const int y = CountToCanvasY(count);

        gc->StrokeLine(x, plotBottom, x, y);
    }
}

void cSpectroscopyHistogramPanel::DrawAxes(wxGraphicsContext* gc)
{
    if (!gc || m_CanvasSize.GetWidth() <= 0 || m_CanvasSize.GetHeight() <= 0)
        return;

    const int W = m_CanvasSize.GetWidth();

    const wxRect plotRect = GetPlotRect();

    DrawHorizontalScale(gc);

    gc->SetPen(wxPen(m_AxisColour, 1));

    // Left vertical axis.
    gc->StrokeLine
    (
        plotRect.GetLeft(),
        plotRect.GetTop(),
        plotRect.GetLeft(),
        plotRect.GetBottom()
    );

    // Bottom horizontal axis.
    gc->StrokeLine
    (
        plotRect.GetLeft(),
        plotRect.GetBottom(),
        plotRect.GetRight(),
        plotRect.GetBottom()
    );

    wxFont font(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    gc->SetFont(font, m_AxisColour);

    const wxString left = wxString::Format(wxT("%u"), m_ViewMin);
    const wxString right = wxString::Format(wxT("%u"), m_ViewMax);

    wxDouble tw{}, th{};

    gc->DrawText(left, plotRect.GetLeft(), plotRect.GetBottom() + 4);

    gc->GetTextExtent(right, &tw, &th);
    gc->DrawText(right, plotRect.GetRight() - tw, plotRect.GetBottom() + 4);

    const wxString title = wxString::Format
    (
        wxT("Accumulated spectroscopy histogram  |  Events: %s  |  Peak: %s%s"),
        FormatCount(m_TotalEvents),
        FormatCount(m_ViewPeak),
        m_LogScale ? wxT("  |  log") : wxT("")
    );

    gc->GetTextExtent(title, &tw, &th);
    gc->DrawText(title, std::max(4.0, static_cast<double>(W) - tw - 6.0), 4);
}

void cSpectroscopyHistogramPanel::DrawHorizontalScale(wxGraphicsContext* gc)
{
    if (!gc)
        return;

    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 2 || plotRect.GetHeight() <= 2)
        return;

    wxFont font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    gc->SetFont(font, wxColour(170, 170, 170));

    const wxColour gridColour(255, 255, 255, 36);
    const wxColour textColour(185, 185, 185);

    gc->SetPen(wxPen(gridColour, 1));

    auto drawScaleLine = [&](unsigned long long value)
        {
            if (value > m_ViewPeak)
                return;

            const int y = CountToCanvasY(value);

            if (y < plotRect.GetTop() || y > plotRect.GetBottom())
                return;

            gc->SetPen(wxPen(gridColour, 1));
            gc->StrokeLine(plotRect.GetLeft(), y, plotRect.GetRight(), y);

            const wxString label = FormatCount(value);

            wxDouble tw{}, th{};
            gc->GetTextExtent(label, &tw, &th);

            gc->SetFont(font, textColour);
            gc->DrawText
            (
                label,
                std::max(2.0, static_cast<double>(plotRect.GetLeft()) - tw - 8.0),
                static_cast<double>(y) - th / 2.0
            );
        };

    drawScaleLine(0);

    if (m_ViewPeak == 0)
        return;

    if (m_LogScale)
    {
        unsigned long long value = 1;

        while (value <= m_ViewPeak)
        {
            drawScaleLine(value);

            if (value > std::numeric_limits<unsigned long long>::max() / 10ULL)
                break;

            value *= 10ULL;
        }

        drawScaleLine(m_ViewPeak);
        return;
    }

    const double step = CalculateNiceTickStep(static_cast<double>(m_ViewPeak), HISTOGRAM_TARGET_Y_TICKS);

    for (double value = step; value < static_cast<double>(m_ViewPeak); value += step)
    {
        drawScaleLine(static_cast<unsigned long long>(std::llround(value)));
    }

    drawScaleLine(m_ViewPeak);
}

void cSpectroscopyHistogramPanel::DrawCursorOverlay(wxGraphicsContext* gc)
{
    if (!gc || !m_MouseInside || m_Histogram.empty() || m_CanvasSize.GetWidth() <= 0)
        return;

    const wxRect plotRect = GetPlotRect();

    if (!plotRect.Contains(m_CursorPos))
        return;

    const unsigned int bin = CanvasXToBin(m_CursorPos.x);

    if (bin >= m_Histogram.size())
        return;

    const unsigned long long count = m_Histogram[bin];
    const int x = BinToCanvasX(bin);
    const int y = CountToCanvasY(count);

    gc->SetPen(wxPen(wxColour(255, 255, 255, 90), 1, wxPENSTYLE_DOT));
    gc->StrokeLine(x, plotRect.GetTop(), x, plotRect.GetBottom());

    const wxString text = wxString::Format(wxT("ADU %u  |  %s"), bin, FormatCount(count));

    wxFont font(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    gc->SetFont(font, m_OverlayColour);

    wxDouble tw{}, th{};
    gc->GetTextExtent(text, &tw, &th);

    double bx = std::clamp<double>
        (
            x - tw / 2.0 - 8.0,
            static_cast<double>(plotRect.GetLeft()),
            std::max
            (
                static_cast<double>(plotRect.GetLeft()),
                static_cast<double>(plotRect.GetRight()) - tw - 16.0
            )
        );

    double by = std::max
    (
        static_cast<double>(plotRect.GetTop()) + 4.0,
        static_cast<double>(y) - th - 18.0
    );

    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(wxColour(0, 0, 0, 180)));
    gc->DrawRoundedRectangle(bx, by, tw + 16.0, th + 8.0, 5.0);
    gc->DrawText(text, bx + 8.0, by + 4.0);

    if (count > 0)
    {
        gc->SetPen(wxPen(m_HistogramColour, 2));
        gc->SetBrush(wxBrush(m_HistogramColour));
        gc->DrawEllipse(x - 3.0, y - 3.0, 6.0, 6.0);
    }
}

void cSpectroscopyHistogramPanel::ResetViewToFull()
{
    m_ViewMin = 0;
    m_ViewMax = m_Histogram.empty() ? 0 : static_cast<unsigned int>(m_Histogram.size() - 1);
    m_ViewInitialized = !m_Histogram.empty();
}

void cSpectroscopyHistogramPanel::UpdateViewPeak()
{
    m_ViewPeak = 0;

    if (m_Histogram.empty())
        return;

    const unsigned int viewMin = std::min<unsigned int>(m_ViewMin, static_cast<unsigned int>(m_Histogram.size() - 1));
    const unsigned int viewMax = std::min<unsigned int>(m_ViewMax, static_cast<unsigned int>(m_Histogram.size() - 1));

    if (viewMax < viewMin)
        return;

    for (unsigned int i = viewMin; i <= viewMax; ++i)
        m_ViewPeak = std::max(m_ViewPeak, m_Histogram[i]);
}

wxRect cSpectroscopyHistogramPanel::GetPlotRect() const
{
    const int W = m_CanvasSize.GetWidth();
    const int H = m_CanvasSize.GetHeight();

    if (W <= HISTOGRAM_LEFT_MARGIN + HISTOGRAM_RIGHT_MARGIN + 2 ||
        H <= HISTOGRAM_TOP_MARGIN + HISTOGRAM_BOTTOM_MARGIN + 2)
    {
        return wxRect(0, 0, std::max(1, W), std::max(1, H));
    }

    return wxRect
    (
        HISTOGRAM_LEFT_MARGIN,
        HISTOGRAM_TOP_MARGIN,
        W - HISTOGRAM_LEFT_MARGIN - HISTOGRAM_RIGHT_MARGIN,
        H - HISTOGRAM_TOP_MARGIN - HISTOGRAM_BOTTOM_MARGIN
    );
}

unsigned int cSpectroscopyHistogramPanel::CanvasXToBin(int x) const
{
    if (m_Histogram.empty())
        return 0;

    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 1)
        return 0;

    x = std::clamp(x, plotRect.GetLeft(), plotRect.GetRight());

    const unsigned int span = std::max(1u, m_ViewMax - m_ViewMin);

    return std::min<unsigned int>
        (
            static_cast<unsigned int>(m_Histogram.size() - 1),
            m_ViewMin + static_cast<unsigned int>
            (
                std::llround
                (
                    static_cast<double>(x - plotRect.GetLeft()) *
                    static_cast<double>(span) /
                    static_cast<double>(std::max(1, plotRect.GetWidth() - 1))
                )
                )
        );
}

int cSpectroscopyHistogramPanel::BinToCanvasX(unsigned int bin) const
{
    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 1)
        return plotRect.GetLeft();

    const unsigned int span = std::max(1u, m_ViewMax - m_ViewMin);

    const double t =
        static_cast<double>(std::clamp(bin, m_ViewMin, m_ViewMax) - m_ViewMin) /
        static_cast<double>(span);

    return plotRect.GetLeft() + static_cast<int>
        (
            std::round(t * static_cast<double>(plotRect.GetWidth() - 1))
            );
}

int cSpectroscopyHistogramPanel::CountToCanvasY(unsigned long long count) const
{
    const wxRect plotRect = GetPlotRect();

    if (m_ViewPeak == 0 || count == 0)
        return plotRect.GetBottom();

    double normalized = 0.0;

    if (m_LogScale)
        normalized = std::log1p(static_cast<double>(count)) / std::log1p(static_cast<double>(m_ViewPeak));
    else
        normalized = static_cast<double>(count) / static_cast<double>(m_ViewPeak);

    normalized = std::clamp(normalized, 0.0, 1.0);

    return plotRect.GetTop() + static_cast<int>
        (
            std::round(static_cast<double>(plotRect.GetHeight() - 1) * (1.0 - normalized))
            );
}

wxString cSpectroscopyHistogramPanel::FormatCount(unsigned long long value) const
{
    wxString s = wxString::Format(wxT("%llu"), value);

    for (int i = static_cast<int>(s.length()) - 3; i > 0; i -= 3)
        s.insert(i, wxT("'"));

    return s;
}
