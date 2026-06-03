#pragma once
#ifndef CSPECTROSCOPY_HISTOGRAM_PANEL_H
#define CSPECTROSCOPY_HISTOGRAM_PANEL_H

#include "wx/wx.h"
#include "wx/dcbuffer.h"
#include "wx/graphics.h"
#include "wx/string.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <vector>
#include <iterator>

class cSpectroscopyHistogramPanel final : public wxPanel
{
public:
    explicit cSpectroscopyHistogramPanel
    (
        wxWindow* parent, 
        wxSizer* parentSizer, 
        wxColour labelsColor,
        const int borderSize = 1
    );

    void SetHistogram(const unsigned long long* histogram, size_t histogramSize, unsigned long long totalEvents);
    void ResetHistogram();
    void SetBackgroundColor(const wxColour& colour);
    void SetLogScale(bool enabled);

    bool HasHistogramData() const;
    bool ExportVisibleHistogramToCsv(const wxString& filePath, wxString* errorMessage = nullptr) const;
    bool ExportVisibleHistogramToTxt(const wxString& filePath, wxString* errorMessage = nullptr) const;

private:
    void PaintEvent(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt);
    void OnRightUp(wxMouseEvent& evt);
    void OnLeftDClick(wxMouseEvent& evt);

    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnCaptureLost(wxMouseCaptureLostEvent& evt);

    void Render(wxBufferedPaintDC& dc);
    void DrawHistogram(wxGraphicsContext* gc);
    void DrawAxes(wxGraphicsContext* gc);
    void DrawHorizontalScale(wxGraphicsContext* gc);
    void DrawXAxisScale(wxGraphicsContext* gc);
    void DrawCursorOverlay(wxGraphicsContext* gc);
    void ResetViewToFull();
    void UpdateViewPeak();

    void ResetViewToAutomaticRange();
    void ClampViewToHistogram();
    void MarkViewUserAdjusted();

    wxRect GetPlotRect() const;

    unsigned int CanvasXToBin(int x) const;
    int BinToCanvasX(unsigned int bin) const;
    int BinToCanvasXCenter(unsigned int bin) const;
    int CountToCanvasY(unsigned long long count) const;
    wxString FormatCompactCount(unsigned long long value) const;

private:
    std::vector<unsigned long long> m_Histogram{};
    unsigned long long m_TotalEvents{};
    unsigned long long m_ViewPeak{};

    unsigned int m_ViewMin{};
    unsigned int m_ViewMax{};
    bool m_ViewInitialized{};
    bool m_UserAdjustedView{};
    bool m_LogScale{};

    wxSize m_CanvasSize{};
    wxPoint m_CursorPos{};
    bool m_MouseInside{};

    bool m_IsPanning{};
    int m_PanStartX{};
    unsigned int m_PanStartViewMin{};
    unsigned int m_PanStartViewMax{};

    wxColour m_BackgroundColour{ 30, 30, 30 };
    wxColour m_HistogramColour{ 60, 220, 110 };
    wxColour m_AxisColour{ 190, 190, 190 };
    wxColour m_OverlayColour{ 245, 245, 245 };
    wxColour m_LabelsColour{ 255, 255, 255 };

    DECLARE_EVENT_TABLE();
};

#endif