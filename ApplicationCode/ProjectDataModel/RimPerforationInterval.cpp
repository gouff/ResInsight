/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-     Statoil ASA
//  Copyright (C) 2013-     Ceetron Solutions AS
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

#include "RimPerforationInterval.h"

#include "RimProject.h"

#include "cafPdmUiListEditor.h"
#include "cafPdmUiTextEditor.h"

CAF_PDM_SOURCE_INIT(RimPerforationInterval, "Perforation");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimPerforationInterval::RimPerforationInterval()
{
    CAF_PDM_InitObject("Perforation", ":/Default.png", "", "");
    m_name = "Perforation";
    CAF_PDM_InitField(&m_startMD, "StartMeasuredDepth", 0.0, "Start MD [m]", "", "", "");
    CAF_PDM_InitField(&m_endMD, "EndMeasuredDepth", 0.0, "End MD [m]", "", "", "");
    CAF_PDM_InitField(&m_radius, "Radius", 0.0, "Radius [m]", "", "", "");
    CAF_PDM_InitField(&m_skinFactor, "SkinFactor", 0.0, "Skin Factor", "", "", "");
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimPerforationInterval::~RimPerforationInterval()
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimPerforationInterval::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimProject* proj;
    this->firstAncestorOrThisOfType(proj);
    if (proj) proj->createDisplayModelAndRedrawAllViews();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimPerforationInterval::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    uiOrdering.add(&m_name);

    uiOrdering.add(&m_startMD);
    uiOrdering.add(&m_endMD);
    uiOrdering.add(&m_radius);
    uiOrdering.add(&m_skinFactor);
}
