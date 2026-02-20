//
// Created by apolline on 20/02/2026.
//

#pragma once
#include <algorithm>

struct ClampPanResult
{
    double x;
    double y;
};

inline ClampPanResult computeClampedPan(double panX, double panY, int imgW, int imgH, double scale,
                                        int viewW, int viewH, double marginPx)
{
    if (imgW <= 0 || imgH <= 0 || scale <= 0.0 || viewW <= 0 || viewH <= 0)
        return {panX, panY};

    const double sw = imgW * scale;
    const double sh = imgH * scale;
    const double vw = viewW;
    const double vh = viewH;

    double minX, maxX, minY, maxY;

    if (sw <= vw)
    {
        minX = maxX = (vw - sw) * 0.5;
    }
    else
    {
        maxX = vw - marginPx;
        minX = -(sw - marginPx);
    }

    if (sh <= vh)
    {
        minY = maxY = (vh - sh) * 0.5;
    }
    else
    {
        maxY = vh - marginPx;
        minY = -(sh - marginPx);
    }

    return {std::clamp(panX, minX, maxX), std::clamp(panY, minY, maxY)};
}
