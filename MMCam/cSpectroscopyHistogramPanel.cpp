#include "cSpectroscopyHistogramPanel.h"

#include "wx/file.h"

#include <limits>
#include <utility>

namespace
{
    constexpr int HISTOGRAM_LEFT_MARGIN = 92;
    constexpr int HISTOGRAM_RIGHT_MARGIN = 18;
    constexpr int HISTOGRAM_TOP_MARGIN = 34;
    constexpr int HISTOGRAM_BOTTOM_MARGIN = 56;

    constexpr int HISTOGRAM_AXIS_LABEL_FONT_SIZE = 15;
    constexpr int HISTOGRAM_TITLE_FONT_SIZE = 9;

    constexpr int HISTOGRAM_MIN_MAJOR_Y_GRID_SPACING_PX = 48;
    constexpr int HISTOGRAM_MIN_MINOR_Y_GRID_SPACING_PX = 24;
    constexpr int HISTOGRAM_MIN_LABEL_SPACING_PX = 42;

    constexpr int HISTOGRAM_MIN_X_LABEL_SPACING_PX = 92;
    constexpr int HISTOGRAM_X_TICK_HEIGHT = 6;

    constexpr int HISTOGRAM_MIN_MAJOR_Y_TICKS = 4;
    constexpr int HISTOGRAM_MAX_MAJOR_Y_TICKS = 11;

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

    int CalculateSmartMajorTickTarget(const int plotHeight, const unsigned long long peak)
    {
        if (plotHeight <= 0 || peak == 0)
            return HISTOGRAM_MIN_MAJOR_Y_TICKS;

        int target = std::clamp
        (
            plotHeight / HISTOGRAM_MIN_MAJOR_Y_GRID_SPACING_PX + 1,
            HISTOGRAM_MIN_MAJOR_Y_TICKS,
            HISTOGRAM_MAX_MAJOR_Y_TICKS
        );

        /*
            If the visible peak is very small, do not create fractional-looking
            over-dense scales. Integer count values are more readable here.
        */
        if (peak <= 10)
            target = std::min<int>(target, static_cast<int>(peak) + 1);

        return std::max(HISTOGRAM_MIN_MAJOR_Y_TICKS, target);
    }

    bool IsFarEnoughFromExistingLabels(const std::vector<int>& usedY, const int y)
    {
        for (const int existingY : usedY)
        {
            if (std::abs(existingY - y) < HISTOGRAM_MIN_LABEL_SPACING_PX)
                return false;
        }

        return true;
    }

    unsigned int CalculateNiceBinStep(unsigned int span, unsigned int targetStepCount)
    {
        if (span <= 1 || targetStepCount <= 1)
            return 1;

        const double rawStep = static_cast<double>(span) / static_cast<double>(targetStepCount);
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

        return std::max(1u, static_cast<unsigned int>(std::llround(niceNormalized * magnitude)));
    }

    long double CalculateHistogramTotal(const std::vector<unsigned long long>& histogram)
    {
        long double total = 0.0L;

        for (const auto value : histogram)
            total += static_cast<long double>(value);

        return total;
    }

    unsigned int CalculateWeightedQuantileBin
    (
        const std::vector<unsigned long long>& histogram,
        const long double quantile
    )
    {
        if (histogram.empty())
            return 0;

        const long double total = CalculateHistogramTotal(histogram);

        if (total <= 0.0L)
            return 0;

        const long double clampedQuantile = std::clamp<long double>(quantile, 0.0L, 1.0L);
        const long double target = total * clampedQuantile;

        long double accumulated = 0.0L;

        for (size_t i = 0; i < histogram.size(); ++i)
        {
            accumulated += static_cast<long double>(histogram[i]);

            if (accumulated >= target)
                return static_cast<unsigned int>(i);
        }

        return static_cast<unsigned int>(histogram.size() - 1);
    }

    long long FloorToMultiple(long long value, unsigned int step)
    {
        if (step == 0)
            return value;

        const long long s = static_cast<long long>(step);

        if (value >= 0)
            return (value / s) * s;

        return -(((-value + s - 1) / s) * s);
    }

    long long CeilToMultiple(long long value, unsigned int step)
    {
        if (step == 0)
            return value;

        const long long s = static_cast<long long>(step);

        if (value >= 0)
            return ((value + s - 1) / s) * s;

        return -((-value / s) * s);
    }

    double CalculateColourLuminance(const wxColour& colour)
    {
        /*
            Perceptual luminance approximation.
            Good enough for UI contrast decisions.
        */
        return
            0.2126 * static_cast<double>(colour.Red()) +
            0.7152 * static_cast<double>(colour.Green()) +
            0.0722 * static_cast<double>(colour.Blue());
    }

    bool IsDarkColour(const wxColour& colour)
    {
        return CalculateColourLuminance(colour) < 128.0;
    }

    wxColour MakeColourWithAlpha(const wxColour& colour, const unsigned char alpha)
    {
        return wxColour
        (
            colour.Red(),
            colour.Green(),
            colour.Blue(),
            alpha
        );
    }

    wxColour GetReadableAxisColour(const wxColour& backgroundColour)
    {
        return IsDarkColour(backgroundColour)
            ? wxColour(230, 230, 230)
            : wxColour(35, 35, 35);
    }

    wxColour GetReadableTickColour(const wxColour& backgroundColour)
    {
        return IsDarkColour(backgroundColour)
            ? wxColour(245, 245, 245)
            : wxColour(25, 25, 25);
    }

    wxColour GetReadableMajorGridColour(const wxColour& backgroundColour)
    {
        return IsDarkColour(backgroundColour)
            ? wxColour(255, 255, 255, 48)
            : wxColour(0, 0, 0, 62);
    }

    wxColour GetReadableMinorGridColour(const wxColour& backgroundColour)
    {
        return IsDarkColour(backgroundColour)
            ? wxColour(255, 255, 255, 22)
            : wxColour(0, 0, 0, 28);
    }

    wxColour GetReadableLabelColour(const wxColour& backgroundColour, const wxColour& preferredColour)
    {
        /*
            If the user-provided label colour already contrasts with the background,
            keep it. Otherwise fall back to black/white depending on the background.
        */
        const double backgroundLuminance = CalculateColourLuminance(backgroundColour);
        const double preferredLuminance = CalculateColourLuminance(preferredColour);

        if (std::abs(backgroundLuminance - preferredLuminance) >= 85.0)
            return preferredColour;

        return IsDarkColour(backgroundColour)
            ? wxColour(245, 245, 245)
            : wxColour(25, 25, 25);
    }
}

BEGIN_EVENT_TABLE(cSpectroscopyHistogramPanel, wxPanel)
	EVT_PAINT(cSpectroscopyHistogramPanel::PaintEvent)
	EVT_SIZE(cSpectroscopyHistogramPanel::OnSize)
	EVT_MOTION(cSpectroscopyHistogramPanel::OnMouseMove)
	EVT_LEFT_DOWN(cSpectroscopyHistogramPanel::OnLeftDown)
	EVT_LEFT_UP(cSpectroscopyHistogramPanel::OnLeftUp)
	EVT_MOUSE_CAPTURE_LOST(cSpectroscopyHistogramPanel::OnCaptureLost)
	EVT_LEAVE_WINDOW(cSpectroscopyHistogramPanel::OnMouseLeave)
	EVT_MOUSEWHEEL(cSpectroscopyHistogramPanel::OnMouseWheel)
	EVT_RIGHT_UP(cSpectroscopyHistogramPanel::OnRightUp)
	EVT_LEFT_DCLICK(cSpectroscopyHistogramPanel::OnLeftDClick)
END_EVENT_TABLE()

cSpectroscopyHistogramPanel::cSpectroscopyHistogramPanel
(
    wxWindow* parent, 
    wxSizer* parentSizer, 
	wxColour labelsColor,
    const int borderSize
)
	: wxPanel(parent), m_LabelsColour(std::move(labelsColor))
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

    if (!m_ViewInitialized || !m_UserAdjustedView)
    {
        ResetViewToAutomaticRange();
    }
    else
    {
        ClampViewToHistogram();
    }

    Refresh(false);
}

void cSpectroscopyHistogramPanel::ResetHistogram()
{
    m_Histogram.clear();

    m_TotalEvents = 0;
    m_ViewPeak = 0;

    m_ViewMin = 0;
    m_ViewMax = 0;
    m_ViewInitialized = false;
    m_UserAdjustedView = false;

    m_IsPanning = false;

    if (HasCapture())
        ReleaseMouse();

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

bool cSpectroscopyHistogramPanel::HasHistogramData() const
{
    return !m_Histogram.empty() && m_ViewInitialized && m_ViewMax >= m_ViewMin;
}

bool cSpectroscopyHistogramPanel::ExportVisibleHistogramToCsv(const wxString& filePath, wxString* errorMessage) const
{
    if (!HasHistogramData())
    {
        if (errorMessage)
            *errorMessage = wxT("There is no histogram data to export.");

        return false;
    }

    const unsigned int histogramMax = static_cast<unsigned int>(m_Histogram.size() - 1);
    const unsigned int viewMin = std::min<unsigned int>(m_ViewMin, histogramMax);
    const unsigned int viewMax = std::min<unsigned int>(m_ViewMax, histogramMax);

    if (viewMax < viewMin)
    {
        if (errorMessage)
            *errorMessage = wxT("The displayed histogram range is invalid.");

        return false;
    }

    wxString content;
    content.reserve(static_cast<size_t>(viewMax - viewMin + 1u) * 24u);

    content += wxT("Position,Value\n");

    for (unsigned int bin = viewMin; bin <= viewMax; ++bin)
    {
        content += wxString::Format(wxT("%u,%llu\n"), bin, m_Histogram[bin]);

        if (bin == viewMax)
            break;
    }

    wxFile file(filePath, wxFile::write);

    if (!file.IsOpened())
    {
        if (errorMessage)
            *errorMessage = wxString::Format(wxT("Cannot open file for writing:\n%s"), filePath);

        return false;
    }

    if (!file.Write(content, wxConvUTF8))
    {
        if (errorMessage)
            *errorMessage = wxString::Format(wxT("Cannot write histogram data to file:\n%s"), filePath);

        return false;
    }

    return true;
}

bool cSpectroscopyHistogramPanel::ExportVisibleHistogramToTxt(const wxString& filePath, wxString* errorMessage) const
{
    if (!HasHistogramData())
    {
        if (errorMessage)
            *errorMessage = wxT("There is no histogram data to export.");

        return false;
    }

    const unsigned int histogramMax = static_cast<unsigned int>(m_Histogram.size() - 1);
    const unsigned int viewMin = std::min<unsigned int>(m_ViewMin, histogramMax);
    const unsigned int viewMax = std::min<unsigned int>(m_ViewMax, histogramMax);

    if (viewMax < viewMin)
    {
        if (errorMessage)
            *errorMessage = wxT("The displayed histogram range is invalid.");

        return false;
    }

    wxString content;
    content.reserve(static_cast<size_t>(viewMax - viewMin + 1u) * 28u);

    content += wxT("# MMCam spectroscopy histogram export\n");
    content += wxString::Format(wxT("# Displayed range: %u - %u\n"), viewMin, viewMax);
    content += wxString::Format(wxT("# Total events: %llu\n"), m_TotalEvents);
    content += wxT("# Columns: Position\tValue\n");
    content += wxT("Position\tValue\n");

    for (unsigned int bin = viewMin; bin <= viewMax; ++bin)
    {
        content += wxString::Format(wxT("%u\t%llu\n"), bin, m_Histogram[bin]);

        if (bin == viewMax)
            break;
    }

    wxFile file(filePath, wxFile::write);

    if (!file.IsOpened())
    {
        if (errorMessage)
            *errorMessage = wxString::Format(wxT("Cannot open file for writing:\n%s"), filePath);

        return false;
    }

    if (!file.Write(content, wxConvUTF8))
    {
        if (errorMessage)
            *errorMessage = wxString::Format(wxT("Cannot write histogram data to file:\n%s"), filePath);

        return false;
    }

    return true;
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

    if (m_IsPanning && evt.LeftIsDown() && !m_Histogram.empty())
    {
        const wxRect plotRect = GetPlotRect();

        if (plotRect.GetWidth() > 1)
        {
            const unsigned int fullMax = static_cast<unsigned int>(m_Histogram.size() - 1);
            const unsigned int currentSpan = std::max(1u, m_PanStartViewMax - m_PanStartViewMin);

            const int deltaX = evt.GetX() - m_PanStartX;

            const long long deltaBins = std::llround
            (
                static_cast<double>(deltaX) *
                static_cast<double>(currentSpan) /
                static_cast<double>(std::max(1, plotRect.GetWidth() - 1))
            );

            long long newMin = static_cast<long long>(m_PanStartViewMin) - deltaBins;
            long long newMax = static_cast<long long>(m_PanStartViewMax) - deltaBins;

            if (newMin < 0)
            {
                newMin = 0;
                newMax = static_cast<long long>(currentSpan);
            }

            if (newMax > static_cast<long long>(fullMax))
            {
                newMax = static_cast<long long>(fullMax);
                newMin = newMax - static_cast<long long>(currentSpan);
            }

            if (newMin < 0)
                newMin = 0;

            m_ViewMin = static_cast<unsigned int>(newMin);
            m_ViewMax = static_cast<unsigned int>(newMax);

            MarkViewUserAdjusted();
        }
    }

    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnMouseLeave(wxMouseEvent& evt)
{
    m_MouseInside = false;

    if (!m_IsPanning)
        SetCursor(wxCursor(wxCURSOR_ARROW));

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

    MarkViewUserAdjusted();

    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnRightUp(wxMouseEvent& evt)
{
    m_LogScale = !m_LogScale;
    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnLeftDClick(wxMouseEvent& evt)
{
    m_UserAdjustedView = false;
    ResetViewToAutomaticRange();
    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnLeftDown(wxMouseEvent& evt)
{
    m_CursorPos = evt.GetPosition();
    m_MouseInside = true;

    if (!m_Histogram.empty() && GetPlotRect().Contains(m_CursorPos))
    {
        m_IsPanning = true;
        m_PanStartX = evt.GetX();
        m_PanStartViewMin = m_ViewMin;
        m_PanStartViewMax = m_ViewMax;

        if (!HasCapture())
            CaptureMouse();

        SetCursor(wxCursor(wxCURSOR_SIZEWE));
    }

    Refresh(false);
}

void cSpectroscopyHistogramPanel::OnLeftUp(wxMouseEvent& evt)
{
    m_CursorPos = evt.GetPosition();

    if (m_IsPanning)
    {
        m_IsPanning = false;

        if (HasCapture())
            ReleaseMouse();

        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh(false);
    }
}

void cSpectroscopyHistogramPanel::OnCaptureLost(wxMouseCaptureLostEvent& evt)
{
    m_IsPanning = false;
    SetCursor(wxCursor(wxCURSOR_ARROW));
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

    const unsigned int histogramMax = static_cast<unsigned int>(m_Histogram.size() - 1);

    const unsigned int viewMin = std::min<unsigned int>(m_ViewMin, histogramMax);
    const unsigned int viewMax = std::min<unsigned int>(m_ViewMax, histogramMax);

    if (viewMax < viewMin || m_ViewPeak == 0)
        return;

    const wxColour histogramColour
    (
        m_HistogramColour.Red(),
        m_HistogramColour.Green(),
        m_HistogramColour.Blue(),
        m_MouseInside ? 120 : 255
    );

    gc->SetPen(wxPen(histogramColour, 1));
    gc->SetBrush(wxBrush(histogramColour));

    const int plotLeft = plotRect.GetLeft();
    const int plotBottom = plotRect.GetBottom();
    const int plotWidth = std::max(1, plotRect.GetWidth());

    const unsigned int visibleBinCount = viewMax - viewMin + 1u;

    for (unsigned int bin = viewMin; bin <= viewMax; ++bin)
    {
        const unsigned long long count = m_Histogram[bin];

        if (count == 0)
            continue;

        const unsigned int localBin = bin - viewMin;

        const double x0 =
            static_cast<double>(plotLeft) +
            (
                static_cast<double>(localBin) *
                static_cast<double>(plotWidth) /
                static_cast<double>(visibleBinCount)
                );

        const double x1 =
            static_cast<double>(plotLeft) +
            (
                static_cast<double>(localBin + 1u) *
                static_cast<double>(plotWidth) /
                static_cast<double>(visibleBinCount)
                );

        const double rectX = std::floor(x0);
        const double rectW = std::max(1.0, std::ceil(x1) - std::floor(x0));

        const int y = CountToCanvasY(count);
        const double rectH = static_cast<double>(plotBottom - y);

        if (rectH <= 0.0)
            continue;

        gc->DrawRectangle
        (
            rectX,
            static_cast<double>(y),
            rectW,
            rectH
        );

        if (bin == viewMax)
            break;
    }
}

void cSpectroscopyHistogramPanel::DrawAxes(wxGraphicsContext* gc)
{
    if (!gc || m_CanvasSize.GetWidth() <= 0 || m_CanvasSize.GetHeight() <= 0)
        return;

    const int W = m_CanvasSize.GetWidth();
    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 2 || plotRect.GetHeight() <= 2)
        return;

    /*
        Draw grid/scales first, then draw the main axes on top.
    */
    DrawHorizontalScale(gc);
    DrawXAxisScale(gc);

    const wxColour axisColour = GetReadableAxisColour(m_BackgroundColour);
    const wxColour labelColour = GetReadableLabelColour(m_BackgroundColour, m_LabelsColour);

    gc->SetPen(wxPen(axisColour, 1));

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

    const wxString title = wxString::Format
    (
        wxT("Accumulated spectroscopy histogram  |  Events: %s  |  Peak: %s%s"),
        FormatCompactCount(m_TotalEvents),
        FormatCompactCount(m_ViewPeak),
        m_LogScale ? wxT("  |  log") : wxT("")
    );

    wxFont titleFont
    (
        HISTOGRAM_TITLE_FONT_SIZE,
        wxFONTFAMILY_SWISS,
        wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL
    );

    gc->SetFont(titleFont, labelColour);

    wxDouble tw{}, th{};
    gc->GetTextExtent(title, &tw, &th);

    gc->DrawText
    (
        title,
        std::max(4.0, static_cast<double>(W) - tw - 8.0),
        4.0
    );
}

void cSpectroscopyHistogramPanel::DrawHorizontalScale(wxGraphicsContext* gc)
{
    if (!gc)
        return;

    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 2 || plotRect.GetHeight() <= 2)
        return;

    wxFont font
    (
        HISTOGRAM_AXIS_LABEL_FONT_SIZE,
        wxFONTFAMILY_SWISS,
        wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL
    );

    const wxColour majorGridColour = GetReadableMajorGridColour(m_BackgroundColour);
    const wxColour minorGridColour = GetReadableMinorGridColour(m_BackgroundColour);
    const wxColour textColour = GetReadableLabelColour(m_BackgroundColour, m_LabelsColour);

    std::vector<int> labelledY{};

    auto drawScaleLine =
        [&]
        (
            const unsigned long long value,
            const bool drawLabel,
            const bool majorLine
            )
        {
            if (value > m_ViewPeak)
                return false;

            const int y = CountToCanvasY(value);

            if (y < plotRect.GetTop() || y > plotRect.GetBottom())
                return false;

            gc->SetPen(wxPen(majorLine ? majorGridColour : minorGridColour, 1));
            gc->StrokeLine(plotRect.GetLeft(), y, plotRect.GetRight(), y);

            if (!drawLabel)
                return true;

            if (!IsFarEnoughFromExistingLabels(labelledY, y))
                return true;

            const wxString label = FormatCompactCount(value);

            wxDouble tw{}, th{};
            gc->SetFont(font, textColour);
            gc->GetTextExtent(label, &tw, &th);

            const double labelX =
                std::max
                (
                    4.0,
                    static_cast<double>(plotRect.GetLeft()) - tw - 12.0
                );

            const double labelY =
                std::clamp
                (
                    static_cast<double>(y) - th / 2.0,
                    2.0,
                    static_cast<double>(plotRect.GetBottom()) - th - 2.0
                );

            gc->DrawText(label, labelX, labelY);

            labelledY.push_back(y);

            return true;
        };

    drawScaleLine(0, true, true);

    if (m_ViewPeak == 0)
        return;

    if (m_LogScale)
    {
        std::vector<unsigned long long> majorValues{};

        for (unsigned long long value = 1; value <= m_ViewPeak;)
        {
            majorValues.push_back(value);

            if (value > std::numeric_limits<unsigned long long>::max() / 10ULL)
                break;

            value *= 10ULL;
        }

        if (majorValues.empty() || majorValues.back() != m_ViewPeak)
            majorValues.push_back(m_ViewPeak);

        for (const auto value : majorValues)
            drawScaleLine(value, true, true);

        /*
            Minor log grid lines are useful only when there is enough vertical room.
            Otherwise they become visual noise.
        */
        if (plotRect.GetHeight() >= 260)
        {
            for (unsigned long long decade = 1; decade <= m_ViewPeak;)
            {
                for (unsigned int multiplier = 2; multiplier <= 9; ++multiplier)
                {
                    if (decade > std::numeric_limits<unsigned long long>::max() / multiplier)
                        continue;

                    const unsigned long long value = decade * multiplier;

                    if (value > m_ViewPeak)
                        break;

                    const int y = CountToCanvasY(value);

                    bool farEnough = true;

                    for (const int existingY : labelledY)
                    {
                        if (std::abs(existingY - y) < HISTOGRAM_MIN_MINOR_Y_GRID_SPACING_PX)
                        {
                            farEnough = false;
                            break;
                        }
                    }

                    if (farEnough)
                        drawScaleLine(value, false, false);
                }

                if (decade > std::numeric_limits<unsigned long long>::max() / 10ULL)
                    break;

                decade *= 10ULL;
            }
        }

        return;
    }

    const int targetMajorTicks = CalculateSmartMajorTickTarget(plotRect.GetHeight(), m_ViewPeak);
    const double majorStep = CalculateNiceTickStep(static_cast<double>(m_ViewPeak), targetMajorTicks);

    if (majorStep <= 0.0)
    {
        drawScaleLine(m_ViewPeak, true, true);
        return;
    }

    std::vector<unsigned long long> majorValues{};

    majorValues.push_back(0);

    for (double value = majorStep; value < static_cast<double>(m_ViewPeak); value += majorStep)
    {
        const unsigned long long roundedValue = static_cast<unsigned long long>(std::llround(value));

        if (roundedValue > 0 && roundedValue < m_ViewPeak)
            majorValues.push_back(roundedValue);
    }

    majorValues.push_back(m_ViewPeak);

    std::sort(majorValues.begin(), majorValues.end());
    majorValues.erase(std::unique(majorValues.begin(), majorValues.end()), majorValues.end());

    for (const auto value : majorValues)
        drawScaleLine(value, true, true);

    /*
        Add minor horizontal grid lines only when major lines are far apart.
        They are intentionally unlabeled to avoid cluttering the Y axis.
    */
    if (majorValues.size() >= 2)
    {
        for (size_t i = 1; i < majorValues.size(); ++i)
        {
            const unsigned long long a = majorValues[i - 1];
            const unsigned long long b = majorValues[i];

            if (b <= a)
                continue;

            const int yA = CountToCanvasY(a);
            const int yB = CountToCanvasY(b);

            if (std::abs(yA - yB) < HISTOGRAM_MIN_MAJOR_Y_GRID_SPACING_PX * 2)
                continue;

            const unsigned long long mid = a + (b - a) / 2ULL;

            if (mid > a && mid < b)
                drawScaleLine(mid, false, false);
        }
    }
}

void cSpectroscopyHistogramPanel::DrawXAxisScale(wxGraphicsContext* gc)
{
    if (!gc)
        return;

    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 2 || plotRect.GetHeight() <= 2)
        return;

    if (m_ViewMax < m_ViewMin)
        return;

    wxFont labelFont
    (
        HISTOGRAM_AXIS_LABEL_FONT_SIZE,
        wxFONTFAMILY_SWISS,
        wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL
    );

    const wxColour majorGridColour = GetReadableMajorGridColour(m_BackgroundColour);
    const wxColour tickColour = GetReadableTickColour(m_BackgroundColour);
    const wxColour textColour = GetReadableLabelColour(m_BackgroundColour, m_LabelsColour);

    const unsigned int visibleSpan = std::max(1u, m_ViewMax - m_ViewMin);
    const int targetIntervalCount = std::clamp
    (
        plotRect.GetWidth() / HISTOGRAM_MIN_X_LABEL_SPACING_PX,
        2,
        10
    );

    const unsigned int step = CalculateNiceBinStep
    (
        visibleSpan,
        static_cast<unsigned int>(targetIntervalCount)
    );

    std::vector<unsigned int> tickValues{};
    tickValues.reserve(16);

    tickValues.push_back(m_ViewMin);

    if (step > 0)
    {
        unsigned int firstTick = 0;

        if (m_ViewMin % step == 0)
        {
            firstTick = m_ViewMin;
        }
        else
        {
            const unsigned int remainder = m_ViewMin % step;
            firstTick = m_ViewMin + (step - remainder);
        }

        for (unsigned int value = firstTick; value < m_ViewMax;)
        {
            if (value > m_ViewMin)
                tickValues.push_back(value);

            if (value > std::numeric_limits<unsigned int>::max() - step)
                break;

            value += step;
        }
    }

    tickValues.push_back(m_ViewMax);

    std::sort(tickValues.begin(), tickValues.end());
    tickValues.erase(std::unique(tickValues.begin(), tickValues.end()), tickValues.end());

    gc->SetFont(labelFont, textColour);

    struct UsedLabelRange
    {
        double left{};
        double right{};
    };

    std::vector<UsedLabelRange> usedLabelRanges{};

    auto canDrawLabel =
        [&usedLabelRanges](const double left, const double right) -> bool
        {
            for (const auto& used : usedLabelRanges)
            {
                if (right >= used.left && left <= used.right)
                    return false;
            }

            return true;
        };

    auto drawTick =
        [&]
        (
            const unsigned int value,
            const bool forceLabel
            )
        {
            const int x = BinToCanvasXCenter(value);

            if (x < plotRect.GetLeft() || x > plotRect.GetRight())
                return;

            gc->SetPen(wxPen(majorGridColour, 1));
            gc->StrokeLine
            (
                x,
                plotRect.GetTop(),
                x,
                plotRect.GetBottom()
            );

            gc->SetPen(wxPen(tickColour, 2));
            gc->StrokeLine
            (
                x,
                plotRect.GetBottom(),
                x,
                plotRect.GetBottom() + HISTOGRAM_X_TICK_HEIGHT
            );

            const wxString label = wxString::Format(wxT("%u"), value);

            wxDouble tw{}, th{};
            gc->GetTextExtent(label, &tw, &th);

            double textX = static_cast<double>(x) - tw / 2.0;

            textX = std::clamp
            (
                textX,
                static_cast<double>(plotRect.GetLeft()),
                std::max
                (
                    static_cast<double>(plotRect.GetLeft()),
                    static_cast<double>(plotRect.GetRight()) - tw
                )
            );

            const double textY =
                static_cast<double>(plotRect.GetBottom()) +
                static_cast<double>(HISTOGRAM_X_TICK_HEIGHT) +
                5.0;

            const double paddedLeft = textX - 8.0;
            const double paddedRight = textX + tw + 8.0;

            if (!forceLabel && !canDrawLabel(paddedLeft, paddedRight))
                return;

            gc->DrawText(label, textX, textY);

            usedLabelRanges.push_back
            (
                UsedLabelRange
                {
                    paddedLeft,
                    paddedRight
                }
            );
        };

    /*
        Draw edge labels first so that the visible range is always explicit.
        Then draw intermediate labels only if they have enough room.
    */
    drawTick(m_ViewMin, true);
    drawTick(m_ViewMax, true);

    for (const auto value : tickValues)
    {
        if (value == m_ViewMin || value == m_ViewMax)
            continue;

        drawTick(value, false);
    }

    const wxString axisTitle = wxT("ADU");

    wxDouble titleW{}, titleH{};
    gc->GetTextExtent(axisTitle, &titleW, &titleH);

    const double titleX =
        static_cast<double>(plotRect.GetLeft()) +
        (static_cast<double>(plotRect.GetWidth()) - titleW) / 2.0;

    const double titleY =
        static_cast<double>(plotRect.GetBottom()) +
        static_cast<double>(HISTOGRAM_X_TICK_HEIGHT) +
        static_cast<double>(HISTOGRAM_AXIS_LABEL_FONT_SIZE) +
        10.0;

    if (titleY + titleH < static_cast<double>(m_CanvasSize.GetHeight()) - 2.0)
    {
        gc->DrawText
        (
            axisTitle,
            std::max(2.0, titleX),
            titleY
        );
    }
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
    const int x = BinToCanvasXCenter(bin);
    const int y = CountToCanvasY(count);

    gc->SetPen(wxPen(wxColour(255, 255, 255, 90), 1, wxPENSTYLE_DOT));
    gc->StrokeLine(x, plotRect.GetTop(), x, plotRect.GetBottom());

    const wxString text = wxString::Format(wxT("ADU %u  |  %s"), bin, FormatCompactCount(count));

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
        const wxColour markerColour
        (
            m_HistogramColour.Red(),
            m_HistogramColour.Green(),
            m_HistogramColour.Blue(),
            255
        );

        gc->SetPen(wxPen(markerColour, 2));
        gc->SetBrush(wxBrush(markerColour));
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

void cSpectroscopyHistogramPanel::ResetViewToAutomaticRange()
{
    if (m_Histogram.empty())
    {
        ResetViewToFull();
        return;
    }

    auto firstIt = std::find_if
    (
        m_Histogram.begin(),
        m_Histogram.end(),
        [](const unsigned long long value)
        {
            return value != 0;
        }
    );

    if (firstIt == m_Histogram.end())
    {
        ResetViewToFull();
        return;
    }

    auto lastIt = std::find_if
    (
        m_Histogram.rbegin(),
        m_Histogram.rend(),
        [](const unsigned long long value)
        {
            return value != 0;
        }
    );

    const unsigned int histogramMax = static_cast<unsigned int>(m_Histogram.size() - 1);

    const unsigned int firstNonZero = static_cast<unsigned int>
        (
            std::distance(m_Histogram.begin(), firstIt)
            );

    const unsigned int lastNonZero = static_cast<unsigned int>
        (
            m_Histogram.size() - 1ULL - static_cast<size_t>(std::distance(m_Histogram.rbegin(), lastIt))
            );

    unsigned int interestingMin = firstNonZero;
    unsigned int interestingMax = lastNonZero;

    const long double totalEvents = CalculateHistogramTotal(m_Histogram);

    /*
        For sufficiently populated histograms, use a robust central range.
        This prevents one or two accidental non-zero outlier bins from forcing
        the automatic viewport to become unnecessarily wide.

        The exact first/last non-zero range is still used for small histograms,
        where quantile trimming would be unstable and visually misleading.
    */
    if (totalEvents >= 1000.0L && lastNonZero > firstNonZero)
    {
        const unsigned int qLow = CalculateWeightedQuantileBin(m_Histogram, 0.0025L);
        const unsigned int qHigh = CalculateWeightedQuantileBin(m_Histogram, 0.9975L);

        if (qHigh > qLow)
        {
            interestingMin = std::clamp(qLow, firstNonZero, lastNonZero);
            interestingMax = std::clamp(qHigh, firstNonZero, lastNonZero);
        }
    }

    if (interestingMax < interestingMin)
        std::swap(interestingMin, interestingMax);

    const unsigned int interestingSpan = std::max(1u, interestingMax - interestingMin + 1u);

    /*
        Minimum visible range prevents the automatic viewport from looking
        over-zoomed when only a few adjacent bins are currently non-zero.
    */
    const unsigned int minVisibleSpan = std::min<unsigned int>
        (
            static_cast<unsigned int>(m_Histogram.size()),
            128u
        );

    /*
        Padding gives the user visual context around the populated region.
        It scales with the signal width but never drops below a few bins.
    */
    const unsigned int padding = std::max
    (
        8u,
        static_cast<unsigned int>(std::llround(static_cast<double>(interestingSpan) * 0.12))
    );

    long long viewMin = static_cast<long long>(interestingMin) - static_cast<long long>(padding);
    long long viewMax = static_cast<long long>(interestingMax) + static_cast<long long>(padding);

    unsigned int currentSpan = static_cast<unsigned int>
        (
            std::max<long long>(1, viewMax - viewMin + 1)
            );

    if (currentSpan < minVisibleSpan)
    {
        const unsigned int missing = minVisibleSpan - currentSpan;

        viewMin -= static_cast<long long>(missing / 2);
        viewMax += static_cast<long long>(missing - missing / 2);

        currentSpan = minVisibleSpan;
    }

    /*
        Snap the range to readable horizontal boundaries. This is what makes
        the automatic range look intentional instead of mechanically cropped.
    */
    const unsigned int niceStep = CalculateNiceBinStep(currentSpan, 10u);

    viewMin = FloorToMultiple(viewMin, niceStep);
    viewMax = CeilToMultiple(viewMax, niceStep);

    if (viewMin < 0)
    {
        const long long shift = -viewMin;
        viewMin += shift;
        viewMax += shift;
    }

    if (viewMax > static_cast<long long>(histogramMax))
    {
        const long long shift = viewMax - static_cast<long long>(histogramMax);
        viewMin -= shift;
        viewMax -= shift;
    }

    viewMin = std::clamp<long long>(viewMin, 0, static_cast<long long>(histogramMax));
    viewMax = std::clamp<long long>(viewMax, 0, static_cast<long long>(histogramMax));

    if (viewMax <= viewMin)
    {
        viewMin = std::max<long long>(0, static_cast<long long>(interestingMin) - 1);
        viewMax = std::min<long long>(static_cast<long long>(histogramMax), static_cast<long long>(interestingMax) + 1);
    }

    m_ViewMin = static_cast<unsigned int>(viewMin);
    m_ViewMax = static_cast<unsigned int>(viewMax);
    m_ViewInitialized = true;
}

void cSpectroscopyHistogramPanel::ClampViewToHistogram()
{
    if (m_Histogram.empty())
    {
        ResetViewToFull();
        return;
    }

    const unsigned int fullMax = static_cast<unsigned int>(m_Histogram.size() - 1);

    m_ViewMin = std::min(m_ViewMin, fullMax);
    m_ViewMax = std::min(m_ViewMax, fullMax);

    if (m_ViewMax < m_ViewMin)
        m_ViewMax = m_ViewMin;

    m_ViewInitialized = true;
}

void cSpectroscopyHistogramPanel::MarkViewUserAdjusted()
{
    if (m_ViewInitialized)
        m_UserAdjustedView = true;
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
        std::max(1, W - HISTOGRAM_LEFT_MARGIN - HISTOGRAM_RIGHT_MARGIN),
        std::max(1, H - HISTOGRAM_TOP_MARGIN - HISTOGRAM_BOTTOM_MARGIN)
    );
}

unsigned int cSpectroscopyHistogramPanel::CanvasXToBin(int x) const
{
    if (m_Histogram.empty())
        return 0;

    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 1)
        return std::min<unsigned int>(m_ViewMin, static_cast<unsigned int>(m_Histogram.size() - 1));

    if (m_ViewMax < m_ViewMin)
        return std::min<unsigned int>(m_ViewMin, static_cast<unsigned int>(m_Histogram.size() - 1));

    const unsigned int histogramMax = static_cast<unsigned int>(m_Histogram.size() - 1);

    const unsigned int viewMin = std::min<unsigned int>(m_ViewMin, histogramMax);
    const unsigned int viewMax = std::min<unsigned int>(m_ViewMax, histogramMax);

    if (viewMax < viewMin)
        return viewMin;

    x = std::clamp(x, plotRect.GetLeft(), plotRect.GetRight());

    const unsigned int visibleBinCount = viewMax - viewMin + 1u;

    const double t = std::clamp
    (
        static_cast<double>(x - plotRect.GetLeft()) / static_cast<double>(std::max(1, plotRect.GetWidth())),
        0.0,
        std::nextafter(1.0, 0.0)
    );

    const unsigned int localBin = static_cast<unsigned int>
        (
            std::floor(t * static_cast<double>(visibleBinCount))
            );

    return std::min<unsigned int>(viewMin + localBin, viewMax);
}

int cSpectroscopyHistogramPanel::BinToCanvasX(unsigned int bin) const
{
    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 1)
        return plotRect.GetLeft();

    if (m_ViewMax < m_ViewMin)
        return plotRect.GetLeft();

    const unsigned int clampedBin = std::clamp(bin, m_ViewMin, m_ViewMax);
    const unsigned int visibleBinCount = std::max(1u, m_ViewMax - m_ViewMin + 1u);

    const double slotLeft =
        static_cast<double>(plotRect.GetLeft()) +
        (
            static_cast<double>(clampedBin - m_ViewMin) *
            static_cast<double>(plotRect.GetWidth()) /
            static_cast<double>(visibleBinCount)
            );

    return static_cast<int>(std::floor(slotLeft));
}

int cSpectroscopyHistogramPanel::BinToCanvasXCenter(unsigned int bin) const
{
    const wxRect plotRect = GetPlotRect();

    if (plotRect.GetWidth() <= 1)
        return plotRect.GetLeft();

    if (m_ViewMax < m_ViewMin)
        return plotRect.GetLeft();

    const unsigned int clampedBin = std::clamp(bin, m_ViewMin, m_ViewMax);
    const unsigned int visibleBinCount = std::max(1u, m_ViewMax - m_ViewMin + 1u);

    const double slotCenter =
        static_cast<double>(plotRect.GetLeft()) +
        (
            (static_cast<double>(clampedBin - m_ViewMin) + 0.5) *
            static_cast<double>(plotRect.GetWidth()) /
            static_cast<double>(visibleBinCount)
            );

    return static_cast<int>(std::round(slotCenter));
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

wxString cSpectroscopyHistogramPanel::FormatCompactCount(unsigned long long value) const
{
    struct Unit
    {
        unsigned long long factor;
        const wxChar* suffix;
    };

    static constexpr Unit units[]
    {
        { 1'000'000'000'000'000'000ull, wxT("Qi") }, // quintillion
        { 1'000'000'000'000'000ull,     wxT("Qa") }, // quadrillion
        { 1'000'000'000'000ull,         wxT("T")  }, // trillion
        { 1'000'000'000ull,             wxT("B")  }, // billion
        { 1'000'000ull,                 wxT("M")  }, // million
        { 1'000ull,                     wxT("K")  }  // thousand
    };

    for (std::size_t i = 0; i < std::size(units); ++i)
    {
        if (value < units[i].factor)
            continue;

        double scaled =
            static_cast<double>(value) /
            static_cast<double>(units[i].factor);

        /*
            Avoid labels such as 1000K or 1000M after rounding.
            Promote them to the next larger suffix when possible.
        */
        if (scaled >= 999.5 && i > 0)
        {
            scaled =
                static_cast<double>(value) /
                static_cast<double>(units[i - 1].factor);

            if (scaled >= 100.0)
                return wxString::Format(wxT("%.0f%s"), scaled, units[i - 1].suffix);

            if (scaled >= 10.0)
                return wxString::Format(wxT("%.1f%s"), scaled, units[i - 1].suffix);

            return wxString::Format(wxT("%.2f%s"), scaled, units[i - 1].suffix);
        }

        if (scaled >= 100.0)
            return wxString::Format(wxT("%.0f%s"), scaled, units[i].suffix);

        if (scaled >= 10.0)
            return wxString::Format(wxT("%.1f%s"), scaled, units[i].suffix);

        return wxString::Format(wxT("%.2f%s"), scaled, units[i].suffix);
    }

    return wxString::Format(wxT("%llu"), value);
}
