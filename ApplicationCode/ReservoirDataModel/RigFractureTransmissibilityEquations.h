/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017-     Statoil ASA
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


class RigFractureTransmissibilityEquations
{
public:                      // centerToEdgeStimPlanCellTrans  
    static double               computeStimPlanCellTransmissibilityInFracture(double conductivity, 
                                                                              double sideLengthParallellTrans, 
                                                                              double sideLengthNormalTrans,
                                                                              double cDarcyForRelevantUnit);
                             // centerToCenterStimPlanCellTrans
    static double               computeStimPlanCellTransmissibilityInFractureCenterToCenterForTwoCells(double conductivityCell1,
                                                                                                       double sideLengthParallellTransCell1,
                                                                                                       double sideLengthNormalTransCell1,
                                                                                                       double conductivityCell2,
                                                                                                       double sideLengthParallellTransCell2,
                                                                                                       double sideLengthNormalTransCell2,
                                                                                                       double cDarcyForRelevantUnit);
                             // stimPlanCellToWellRadialTrans
    static double               computeRadialTransmissibilityToWellinStimPlanCell(double stimPlanCellConductivity, 
                                                                                  double stimPlanCellSizeX, 
                                                                                  double stimPlanCellSizeZ, 
                                                                                  double wellRadius, 
                                                                                  double skinFactor, 
                                                                                  double cDarcyForRelevantUnit);
                             // stimPlanCellToWellLinearTrans   
    static double               computeLinearTransmissibilityToWellinStimPlanCell(double stimPlanConductivity,  
                                                                                  double stimPlanCellSizeX, 
                                                                                  double stimPlanCellSizeZ, 
                                                                                  double perforationLengthVertical, 
                                                                                  double perforationLengthHorizontal, 
                                                                                  double perforationEfficiency,
                                                                                  double skinfactor,
                                                                                  double cDarcyForRelevantUnit);
                             // matrixToFractureTrans
    static double               calculateMatrixTransmissibility(double permX, 
                                                                double NTG, 
                                                                double Ay, 
                                                                double dx, 
                                                                double skinfactor, 
                                                                double fractureAreaWeightedlength, 
                                                                double cDarcy);

};
