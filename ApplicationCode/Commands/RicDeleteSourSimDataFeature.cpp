/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017 Statoil ASA
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

#include "RicDeleteSourSimDataFeature.h"

#include "RimEclipseResultCase.h"

#include "cafSelectionManager.h"

#include <QAction>


CAF_CMD_SOURCE_INIT(RicDeleteSourSimDataFeature, "RicDeleteSourSimDataFeature");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicDeleteSourSimDataFeature::isCommandEnabled() 
{
    return getSelectedEclipseCase() != nullptr;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicDeleteSourSimDataFeature::onActionTriggered(bool isChecked)
{
    RimEclipseResultCase* eclipseCase = getSelectedEclipseCase();

    if (eclipseCase == nullptr) return;

    eclipseCase->setSourSimFileName(QString());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicDeleteSourSimDataFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setText("Delete SourSim Data");
    actionToSetup->setIcon(QIcon(":/Erase.png"));
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimEclipseResultCase* RicDeleteSourSimDataFeature::getSelectedEclipseCase()
{
    caf::PdmUiItem* selectedItem = caf::SelectionManager::instance()->selectedItem();
    RimEclipseResultCase* eclipseCase = dynamic_cast<RimEclipseResultCase*>(selectedItem);
    return eclipseCase;
}