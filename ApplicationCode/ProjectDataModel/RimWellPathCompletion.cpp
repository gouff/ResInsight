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

#include "RimWellPathCompletion.h"

#include "RimProject.h"

#include "cafPdmUiListEditor.h"
#include "cafPdmUiTextEditor.h"

CAF_PDM_SOURCE_INIT(RimWellPathCompletion, "WellPathCompletion");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPathCompletion::RimWellPathCompletion()
{
    CAF_PDM_InitObject("WellPathCompletion", ":/Well.png", "", "");
    CAF_PDM_InitFieldNoDefault(&m_coordinates, "Coordinates", "Coordinates", "", "", "");
    m_coordinates.uiCapability()->setUiHidden(true);
    CAF_PDM_InitFieldNoDefault(&m_measuredDepths, "MeasuredDepth", "MeasuredDepth", "", "", "");
    m_measuredDepths.uiCapability()->setUiHidden(true);
    m_name.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_displayCoordinates, "DisplayCoordinates", "Coordinates", "", "", "");
    m_displayCoordinates.registerGetMethod(this, &RimWellPathCompletion::displayCoordinates);
    m_displayCoordinates.uiCapability()->setUiReadOnly(true);
    m_displayCoordinates.uiCapability()->setUiEditorTypeName(caf::PdmUiTextEditor::uiEditorTypeName());
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPathCompletion::~RimWellPathCompletion()
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathCompletion::setCoordinates(std::vector< cvf::Vec3d > coordinates)
{
    m_coordinates = coordinates;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathCompletion::setMeasuredDepths(std::vector< double > measuredDepths)
{
    m_measuredDepths = measuredDepths;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathCompletion::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimProject* proj;
    this->firstAncestorOrThisOfType(proj);
    if (proj) proj->createDisplayModelAndRedrawAllViews();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathCompletion::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    caf::PdmUiGroup* geometryGroup =  uiOrdering.addNewGroup("Geometry");
    geometryGroup->add(&m_displayCoordinates);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellPathCompletion::displayCoordinates() const
{
    CVF_ASSERT(m_coordinates().size() == m_measuredDepths().size());

    QStringList displayValues;

    displayValues.push_back(QString("X\tY\tZ\tMD"));
    for (size_t i = 0; i < m_coordinates().size(); i++)
    {
        const cvf::Vec3d& coords = m_coordinates()[i];
        const double& measuredDepth = m_measuredDepths()[i];
        displayValues.push_back(QString("%1\t%2\t%3\t%4").arg(coords.x()).arg(coords.y()).arg(coords.z()).arg(measuredDepth));
    }

    return displayValues.join("\n");
}