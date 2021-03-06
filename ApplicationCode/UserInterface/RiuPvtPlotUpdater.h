/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QString>
#include <QPointer>

class RiuSelectionItem;
class RiuPvtPlotPanel;
class Rim3dView;
class RimEclipseView;
class RigEclipseCaseData;


//==================================================================================================
//
//
//
//==================================================================================================
class RiuPvtPlotUpdater
{
public:
    RiuPvtPlotUpdater(RiuPvtPlotPanel* targetPlotPanel);

    void updateOnSelectionChanged(const RiuSelectionItem* selectionItem);
    void updateOnTimeStepChanged(Rim3dView* changedView);

private:
    static bool     queryDataAndUpdatePlot(const RimEclipseView& eclipseView, size_t gridIndex, size_t gridLocalCellIndex, RiuPvtPlotPanel* plotPanel);
    static QString  constructCellReferenceText(const RigEclipseCaseData* eclipseCaseData, size_t gridIndex, size_t gridLocalCellIndex, double pvtnum);

private:
    QPointer<RiuPvtPlotPanel>   m_targetPlotPanel;
    const Rim3dView*              m_sourceEclipseViewOfLastPlot;
};



