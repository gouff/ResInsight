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

#include "RigEclipseToStimPlanCellTransmissibilityCalculator.h"

#include "RigFractureTransmissibilityEquations.h"

#include "RigStimPlanFracTemplateCell.h"
#include "RigResultAccessorFactory.h"
#include "RigEclipseCaseData.h"
#include "RigActiveCellInfo.h"
#include "RigCellGeometryTools.h"
#include "RigMainGrid.h"

#include "RimEclipseCase.h"
#include "RimReservoirCellResultsStorage.h"

#include "cvfGeometryTools.h"
#include "RigFractureTransCalc.h"

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RigEclipseToStimPlanCellTransmissibilityCalculator::RigEclipseToStimPlanCellTransmissibilityCalculator(const RimEclipseCase* caseToApply,
                                                                                                       cvf::Mat4f fractureTransform,
                                                                                                       double skinFactor,
                                                                                                       double cDarcy,
                                                                                                       const RigStimPlanFracTemplateCell& stimPlanCell)
    : m_case(caseToApply),
    m_fractureTransform(fractureTransform),
    m_fractureSkinFactor(skinFactor),
    m_cDarcy(cDarcy),
    m_stimPlanCell(stimPlanCell)
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const std::vector<size_t>& RigEclipseToStimPlanCellTransmissibilityCalculator::globalIndeciesToContributingEclipseCells()
{
    if (m_globalIndeciesToContributingEclipseCells.size() < 1)
    {
        calculateStimPlanCellsMatrixTransmissibility();
    }

    return m_globalIndeciesToContributingEclipseCells;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const std::vector<double>& RigEclipseToStimPlanCellTransmissibilityCalculator::contributingEclipseCellTransmissibilities()
{
    if (m_globalIndeciesToContributingEclipseCells.size() < 1)
    {
        calculateStimPlanCellsMatrixTransmissibility();
    }

    return m_contributingEclipseCellTransmissibilities;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RigEclipseToStimPlanCellTransmissibilityCalculator::calculateStimPlanCellsMatrixTransmissibility()
{
    // Not calculating flow into fracture if stimPlan cell cond value is 0 (assumed to be outside the fracture):
    if (m_stimPlanCell.getConductivtyValue() < 1e-7) return;

    const RigEclipseCaseData* eclipseCaseData = m_case->eclipseCaseData();

    RifReaderInterface::PorosityModelResultType porosityModel = RifReaderInterface::MATRIX_RESULTS;
    RimReservoirCellResultsStorage* gridCellResults = m_case->results(porosityModel);

    size_t scalarSetIndex;
    scalarSetIndex = gridCellResults->findOrLoadScalarResult(RimDefines::STATIC_NATIVE, "DX");
    cvf::ref<RigResultAccessor> dataAccessObjectDx = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, porosityModel, 0, "DX"); //assuming 0 time step and main grid (so grid index =0) 
    scalarSetIndex = gridCellResults->findOrLoadScalarResult(RimDefines::STATIC_NATIVE, "DY");
    cvf::ref<RigResultAccessor> dataAccessObjectDy = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, porosityModel, 0, "DY"); //assuming 0 time step and main grid (so grid index =0) 
    scalarSetIndex = gridCellResults->findOrLoadScalarResult(RimDefines::STATIC_NATIVE, "DZ");
    cvf::ref<RigResultAccessor> dataAccessObjectDz = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, porosityModel, 0, "DZ"); //assuming 0 time step and main grid (so grid index =0) 

    scalarSetIndex = gridCellResults->findOrLoadScalarResult(RimDefines::STATIC_NATIVE, "PERMX");
    cvf::ref<RigResultAccessor> dataAccessObjectPermX = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, porosityModel, 0, "PERMX"); //assuming 0 time step and main grid (so grid index =0) 
    scalarSetIndex = gridCellResults->findOrLoadScalarResult(RimDefines::STATIC_NATIVE, "PERMY");
    cvf::ref<RigResultAccessor> dataAccessObjectPermY = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, porosityModel, 0, "PERMY"); //assuming 0 time step and main grid (so grid index =0) 
    scalarSetIndex = gridCellResults->findOrLoadScalarResult(RimDefines::STATIC_NATIVE, "PERMZ");
    cvf::ref<RigResultAccessor> dataAccessObjectPermZ = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, porosityModel, 0, "PERMZ"); //assuming 0 time step and main grid (so grid index =0) 
    scalarSetIndex = gridCellResults->findOrLoadScalarResult(RimDefines::STATIC_NATIVE, "NTG");
    cvf::ref<RigResultAccessor> dataAccessObjectNTG = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, porosityModel, 0, "NTG"); //assuming 0 time step and main grid (so grid index =0) 

    const RigActiveCellInfo* activeCellInfo = eclipseCaseData->activeCellInfo(porosityModel);

    std::vector<cvf::Vec3d> stimPlanPolygonTransformed;
    for (cvf::Vec3d v : m_stimPlanCell.getPolygon())
    {
        cvf::Vec3f stimPlanPolygonNode = cvf::Vec3f(v);
        stimPlanPolygonNode.transformPoint(m_fractureTransform);
        stimPlanPolygonTransformed.push_back(cvf::Vec3d(stimPlanPolygonNode));
    }

    std::vector<size_t> fracCells = getPotentiallyFracturedCellsForPolygon(stimPlanPolygonTransformed);
    for (size_t fracCell : fracCells)
    {
        bool cellIsActive = activeCellInfo->isActive(fracCell);
        if (!cellIsActive) continue;

        double permX = dataAccessObjectPermX->cellScalarGlobIdx(fracCell);
        double permY = dataAccessObjectPermY->cellScalarGlobIdx(fracCell);
        double permZ = dataAccessObjectPermZ->cellScalarGlobIdx(fracCell);

        double dx = dataAccessObjectDx->cellScalarGlobIdx(fracCell);
        double dy = dataAccessObjectDy->cellScalarGlobIdx(fracCell);
        double dz = dataAccessObjectDz->cellScalarGlobIdx(fracCell);

        double NTG = dataAccessObjectNTG->cellScalarGlobIdx(fracCell);

        const RigMainGrid* mainGrid = m_case->eclipseCaseData()->mainGrid();
        cvf::Vec3d hexCorners[8];
        mainGrid->cellCornerVertices(fracCell, hexCorners);

        std::vector<std::vector<cvf::Vec3d> > planeCellPolygons;
        bool isPlanIntersected = RigFractureTransCalc::planeCellIntersectionPolygons(hexCorners, m_fractureTransform, planeCellPolygons);
        if (!isPlanIntersected || planeCellPolygons.size() == 0) continue;

        cvf::Vec3d localX;
        cvf::Vec3d localY;
        cvf::Vec3d localZ;
        RigCellGeometryTools::findCellLocalXYZ(hexCorners, localX, localY, localZ);

        //Transform planCell polygon(s) and averageZdirection to x/y coordinate system (where fracturePolygon already is located)
        cvf::Mat4f invertedTransMatrix = m_fractureTransform.getInverted();
        for (std::vector<cvf::Vec3d> & planeCellPolygon : planeCellPolygons)
        {
            for (cvf::Vec3d& v : planeCellPolygon)
            {
                v.transformPoint(static_cast<cvf::Mat4d>(invertedTransMatrix));
            }
        }

        cvf::Vec3d localZinFracPlane;
        localZinFracPlane = localZ;
        localZinFracPlane.transformVector(static_cast<cvf::Mat4d>(invertedTransMatrix));
        cvf::Vec3d directionOfLength = cvf::Vec3d::ZERO;
        directionOfLength.cross(localZinFracPlane, cvf::Vec3d(0, 0, 1));
        directionOfLength.normalize();

        std::vector<std::vector<cvf::Vec3d> > polygonsForStimPlanCellInEclipseCell;
        cvf::Vec3d areaVector;
        std::vector<cvf::Vec3d> stimPlanPolygon = m_stimPlanCell.getPolygon();

        for (std::vector<cvf::Vec3d> planeCellPolygon : planeCellPolygons)
        {
            std::vector<std::vector<cvf::Vec3d> >clippedPolygons = RigCellGeometryTools::intersectPolygons(planeCellPolygon, stimPlanPolygon);
            for (std::vector<cvf::Vec3d> clippedPolygon : clippedPolygons)
            {
                polygonsForStimPlanCellInEclipseCell.push_back(clippedPolygon);
            }
        }

        if (polygonsForStimPlanCellInEclipseCell.size() == 0) continue;

        double area;
        std::vector<double> areaOfFractureParts;
        double length;
        std::vector<double> lengthXareaOfFractureParts;
        double Ax = 0.0, Ay = 0.0, Az = 0.0;

        for (std::vector<cvf::Vec3d> fracturePartPolygon : polygonsForStimPlanCellInEclipseCell)
        {
            areaVector = cvf::GeometryTools::polygonAreaNormal3D(fracturePartPolygon);
            area = areaVector.length();
            areaOfFractureParts.push_back(area);

            //TODO: the l in the sl/pi term in the denominator of the Tmj expression should be the length of the full Eclipse cell
            //In the current form the implementation gives correct result only if s=0 (fracture templte skin factor). 
            length = RigCellGeometryTools::polygonAreaWeightedLength(directionOfLength, fracturePartPolygon);
            lengthXareaOfFractureParts.push_back(length * area);

            cvf::Plane fracturePlane;
            bool isCellIntersected = false;

            fracturePlane.setFromPointAndNormal(static_cast<cvf::Vec3d>(m_fractureTransform.translation()),
                                                static_cast<cvf::Vec3d>(m_fractureTransform.col(2)));

            Ax += abs(area*(fracturePlane.normal().dot(localY)));
            Ay += abs(area*(fracturePlane.normal().dot(localX)));
            Az += abs(area*(fracturePlane.normal().dot(localZ)));
        }

        double fractureArea = 0.0;
        for (double area : areaOfFractureParts) fractureArea += area;

        double totalAreaXLength = 0.0;
        for (double lengtXarea : lengthXareaOfFractureParts) totalAreaXLength += lengtXarea;

        double fractureAreaWeightedlength = totalAreaXLength / fractureArea;

        double transmissibility_X = RigFractureTransmissibilityEquations::calculateMatrixTransmissibility(permY, NTG, Ay, dx, m_fractureSkinFactor, fractureAreaWeightedlength, m_cDarcy);
        double transmissibility_Y = RigFractureTransmissibilityEquations::calculateMatrixTransmissibility(permX, NTG, Ax, dy, m_fractureSkinFactor, fractureAreaWeightedlength, m_cDarcy);
        double transmissibility_Z = RigFractureTransmissibilityEquations::calculateMatrixTransmissibility(permZ, 1.0, Az, dz, m_fractureSkinFactor, fractureAreaWeightedlength, m_cDarcy);

        double transmissibility = sqrt(transmissibility_X * transmissibility_X
                                       + transmissibility_Y * transmissibility_Y
                                       + transmissibility_Z * transmissibility_Z);


        m_globalIndeciesToContributingEclipseCells.push_back(fracCell);
        m_contributingEclipseCellTransmissibilities.push_back(transmissibility);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<size_t> RigEclipseToStimPlanCellTransmissibilityCalculator::getPotentiallyFracturedCellsForPolygon(std::vector<cvf::Vec3d> polygon)
{
    std::vector<size_t> cellIndices;

    const RigMainGrid* mainGrid = m_case->eclipseCaseData()->mainGrid();
    if (!mainGrid) return cellIndices;

    cvf::BoundingBox polygonBBox;
    for (cvf::Vec3d nodeCoord : polygon) polygonBBox.add(nodeCoord);

    mainGrid->findIntersectingCells(polygonBBox, &cellIndices);

    return cellIndices;
}