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

#include "RimWellPltPlot.h"

#include "RiaApplication.h"
#include "RiaColorTables.h"
#include "RiaDateStringParser.h"
#include "RifReaderEclipseRft.h"
#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigSimWellData.h"
#include "RigWellPath.h"
#include "RimEclipseCase.h"
#include "RimEclipseCaseCollection.h"
#include "RimEclipseResultCase.h"
#include "RimEclipseResultDefinition.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimTools.h"
#include "RimWellLogExtractionCurve.h"
#include "RimWellLogFile.h"
#include "RimWellLogFileChannel.h"
#include "RimWellLogFileCurve.h"
#include "RimWellLogPlot.h"
#include "RimWellLogRftCurve.h"
#include "RimWellLogTrack.h"
#include "RimWellPath.h"
#include "RimWellPathCollection.h"
#include "RimWellFlowRateCurve.h"
#include "RiuWellPltPlot.h"
#include "cafPdmUiTreeSelectionEditor.h"
#include <tuple>
#include <algorithm>
#include <iterator>

CAF_PDM_SOURCE_INIT(RimWellPltPlot, "WellPltPlot"); 

namespace caf
{
template<>
void caf::AppEnum< RimWellPltPlot::FlowType>::setUp()
{
    addItem(RimWellPltPlot::FLOW_TYPE_TOTAL, "TOTAL", "Total Flow");
    addItem(RimWellPltPlot::FLOW_TYPE_PHASE_SPLIT, "PHASE_SPLIT", "Phase Split");
}

template<>
void caf::AppEnum< RimWellPltPlot::FlowPhase>::setUp()
{
    addItem(RimWellPltPlot::PHASE_OIL, "PHASE_OIL", "Oil");
    addItem(RimWellPltPlot::PHASE_GAS, "PHASE_GAS", "Gas");
    addItem(RimWellPltPlot::PHASE_WATER, "PHASE_WATER", "Water");
}
}

const QString RimWellPltPlot::OIL_CHANNEL_NAME = "QOZT";
const QString RimWellPltPlot::GAS_CHANNEL_NAME = "QGZT";
const QString RimWellPltPlot::WATER_CHANNEL_NAME = "QWZT";
const QString RimWellPltPlot::TOTAL_CHANNEL_NAME = "QTZT";

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const std::set<QString> RimWellPltPlot::FLOW_DATA_NAMES = { 
    OIL_CHANNEL_NAME,
    GAS_CHANNEL_NAME,
    WATER_CHANNEL_NAME,
    TOTAL_CHANNEL_NAME };
const char RimWellPltPlot::PLOT_NAME_QFORMAT_STRING[] = "PLT: %1";

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPltPlot::RimWellPltPlot()
{
    CAF_PDM_InitObject("Well Allocation Plot", ":/WellAllocPlot16x16.png", "", "");

    CAF_PDM_InitField(&m_userName, "PlotDescription", QString("PLT Plot"), "Name", "", "", "");
    m_userName.uiCapability()->setUiReadOnly(true);

    CAF_PDM_InitField(&m_showPlotTitle, "ShowPlotTitle", true, "Show Plot Title", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_wellLogPlot, "WellLog", "WellLog", "", "", "");
    m_wellLogPlot.uiCapability()->setUiHidden(true);
    m_wellLogPlot = new RimWellLogPlot();
    m_wellLogPlot->setDepthType(RimWellLogPlot::MEASURED_DEPTH);

    CAF_PDM_InitFieldNoDefault(&m_wellName, "WellName", "WellName", "", "", "");
    CAF_PDM_InitField(&m_branchIndex, "BranchIndex", 0, "BranchIndex", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_selectedSources, "Sources", "Sources", "", "", "");
    m_selectedSources.uiCapability()->setUiEditorTypeName(caf::PdmUiTreeSelectionEditor::uiEditorTypeName());
    m_selectedSources.uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);
    m_selectedSources.uiCapability()->setAutoAddingOptionFromValue(false);

    CAF_PDM_InitFieldNoDefault(&m_selectedTimeSteps, "TimeSteps", "TimeSteps", "", "", "");
    m_selectedTimeSteps.uiCapability()->setUiEditorTypeName(caf::PdmUiTreeSelectionEditor::uiEditorTypeName());
    m_selectedTimeSteps.uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);
    m_selectedTimeSteps.uiCapability()->setAutoAddingOptionFromValue(false);

    CAF_PDM_InitFieldNoDefault(&m_phaseSelectionMode, "PhaseSelectionMode", "Mode", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_phases, "Phases", "Phases", "", "", "");
    m_phases.uiCapability()->setUiEditorTypeName(caf::PdmUiTreeSelectionEditor::uiEditorTypeName());

    this->setAsPlotMdiWindow();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPltPlot::~RimWellPltPlot()
{
    removeMdiWindowFromMdiArea();

    deleteViewWidget();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::deleteViewWidget()
{
    if (m_wellLogPlotWidget)
    {
        m_wellLogPlotWidget->deleteLater();
        m_wellLogPlotWidget = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
//void RimWellPltPlot::applyCurveAppearance(RimWellLogCurve* newCurve)
//{
//    const std::pair<RifWellRftAddress, QDateTime>& newCurveDef = curveDefFromCurve(newCurve);
//
//    std::vector<cvf::Color3f> colorTable;
//    RiaColorTables::summaryCurveDefaultPaletteColors().color3fArray().toStdVector(&colorTable);
//
//    std::vector<RimPlotCurve::PointSymbolEnum> symbolTable =
//    {
//        RimPlotCurve::SYMBOL_ELLIPSE,
//        RimPlotCurve::SYMBOL_RECT,
//        RimPlotCurve::SYMBOL_DIAMOND,
//        RimPlotCurve::SYMBOL_TRIANGLE,
//        RimPlotCurve::SYMBOL_CROSS,
//        RimPlotCurve::SYMBOL_XCROSS
//    };
//
//    // State variables
//    static size_t  defaultColorTableIndex = 0;
//    static size_t  defaultSymbolTableIndex = 0;
//
//    cvf::Color3f                    currentColor;
//    RimPlotCurve::PointSymbolEnum   currentSymbol = symbolTable.front();
//    RimPlotCurve::LineStyleEnum     currentLineStyle = RimPlotCurve::STYLE_SOLID;
//    bool                            isCurrentColorSet = false;
//    bool                            isCurrentSymbolSet = false;
//
//    std::set<cvf::Color3f>                  assignedColors;
//    std::set<RimPlotCurve::PointSymbolEnum> assignedSymbols;
//
//    // Used colors and symbols
//    for (RimWellLogCurve* const curve : m_wellLogPlot->trackByIndex(0)->curvesVector())
//    {
//        if (curve == newCurve) continue;
//
//        std::pair<RifWellRftAddress, QDateTime> cDef = curveDefFromCurve(curve);
//        if (cDef.first == newCurveDef.first)
//        {
//            currentColor = curve->color();
//            isCurrentColorSet = true;
//        }
//        if (cDef.second == newCurveDef.second)
//        {
//            currentSymbol = curve->symbol();
//            isCurrentSymbolSet = true;
//        }
//        assignedColors.insert(curve->color());
//        assignedSymbols.insert(curve->symbol());
//    }
//
//    // Assign color
//    if (!isCurrentColorSet)
//    {
//        for(const auto& color : colorTable)
//        {
//            if (assignedColors.count(color) == 0)
//            {
//                currentColor = color;
//                isCurrentColorSet = true;
//                break;
//            }
//        }
//        if (!isCurrentColorSet)
//        {
//            currentColor = colorTable[defaultColorTableIndex];
//            if (++defaultColorTableIndex == colorTable.size())
//                defaultColorTableIndex = 0;
//
//        }
//    }
//
//    // Assign symbol
//    if (!isCurrentSymbolSet)
//    {
//        for (const auto& symbol : symbolTable)
//        {
//            if (assignedSymbols.count(symbol) == 0)
//            {
//                currentSymbol = symbol;
//                isCurrentSymbolSet = true;
//                break;
//            }
//        }
//        if (!isCurrentSymbolSet)
//        {
//            currentSymbol = symbolTable[defaultSymbolTableIndex];
//            if (++defaultSymbolTableIndex == symbolTable.size())
//                defaultSymbolTableIndex = 0;
//        }
//    }
//
//    // Observed data
//    currentLineStyle = newCurveDef.first.sourceType() == RftSourceType::OBSERVED
//        ? RimPlotCurve::STYLE_NONE : RimPlotCurve::STYLE_SOLID;
//
//    newCurve->setColor(currentColor);
//    newCurve->setSymbol(currentSymbol);
//    newCurve->setLineStyle(currentLineStyle);
//}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::updateSelectedTimeStepsFromSelectedSources()
{
    std::vector<QDateTime> newTimeStepsSelections;
    std::vector<RifWellRftAddress> selectedSourcesVector = selectedSources();
    auto selectedSources = std::set<RifWellRftAddress>(selectedSourcesVector.begin(), selectedSourcesVector.end());

    for (const QDateTime& timeStep : m_selectedTimeSteps())
    {
        if(m_timeStepsToAddresses.count(timeStep) > 0)
        {
            std::vector<RifWellRftAddress> intersectVector;
            const std::set<RifWellRftAddress>& addresses = m_timeStepsToAddresses[timeStep];
            std::set_intersection(selectedSources.begin(), selectedSources.end(),
                                  addresses.begin(), addresses.end(), std::inserter(intersectVector, intersectVector.end()));
            if(intersectVector.size() > 0)
            {
                newTimeStepsSelections.push_back(timeStep);
            }
        }
    }
    m_selectedTimeSteps = newTimeStepsSelections;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPltPlot::FlowPhase RimWellPltPlot::flowPhaseFromChannelName(const QString& channelName)
{
    if (QString::compare(channelName, OIL_CHANNEL_NAME, Qt::CaseInsensitive) == 0) return PHASE_OIL;
    if (QString::compare(channelName, GAS_CHANNEL_NAME, Qt::CaseInsensitive) == 0) return PHASE_GAS;
    if (QString::compare(channelName, WATER_CHANNEL_NAME, Qt::CaseInsensitive) == 0) return PHASE_WATER;
    if (QString::compare(channelName, TOTAL_CHANNEL_NAME, Qt::CaseInsensitive) == 0) return PHASE_TOTAL;
    return PHASE_NONE;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::setPlotXAxisTitles(RimWellLogTrack* plotTrack)
{
    std::vector<RimEclipseCase*> cases = eclipseCases();

    RiaEclipseUnitTools::UnitSystem unitSet = !cases.empty() ? 
        cases.front()->eclipseCaseData()->unitsType() : 
        RiaEclipseUnitTools::UNITS_UNKNOWN;

    QString unitText;
    switch (unitSet)
    {
    case RiaEclipseUnitTools::UNITS_METRIC:
        unitText = "[Liquid Sm<sup>3</sup>/day], [Gas kSm<sup>3</sup>/day]";
        break;
    case RiaEclipseUnitTools::UNITS_FIELD:
        unitText = "[Liquid BBL/day], [Gas BOE/day]";
        break;
    case RiaEclipseUnitTools::UNITS_LAB:
        unitText = "[cm<sup>3</sup>/hr]";
        break;
    default:
        unitText = "(unknown unit)";
        break;

    }
    plotTrack->setXAxisTitle("Surface Flow Rate " + unitText);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimEclipseCase*> RimWellPltPlot::eclipseCases() const
{
    std::vector<RimEclipseCase*> cases;
    RimProject* proj = RiaApplication::instance()->project();
    for (const auto& oilField : proj->oilFields)
    {
        for (const auto& eclCase : oilField->analysisModels()->cases)
        {
            cases.push_back(eclCase);
        }
    }
    return cases;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
//void RimWellPltPlot::applyInitialSelections()
//{
//    std::vector<std::tuple<RimEclipseResultCase*, bool, bool>> eclCaseTuples = eclipseCasesForWell(m_wellName);
//
//    std::vector<RifWellRftAddress> sourcesToSelect;
//    std::map<QDateTime, std::set<RifWellRftAddress>> rftTimeStepsMap;
//    std::map<QDateTime, std::set<RifWellRftAddress>> observedTimeStepsMap;
//    std::map<QDateTime, std::set<RifWellRftAddress>> gridTimeStepsMap;
//
//    for(RimEclipseResultCase* const rftCase : rftCasesFromEclipseCases(eclCaseTuples))
//    {
//        sourcesToSelect.push_back(RifWellRftAddress(RftSourceType::RFT, rftCase));
//        addTimeStepsToMap(rftTimeStepsMap, timeStepsFromRftCase(rftCase));
//    }
//    
//    for (RimEclipseResultCase* const gridCase : gridCasesFromEclipseCases(eclCaseTuples))
//    {
//        sourcesToSelect.push_back(RifWellRftAddress(RftSourceType::GRID, gridCase));
//        addTimeStepsToMap(gridTimeStepsMap, timeStepsFromGridCase(gridCase));
//    }
//    
//    std::vector<RimWellLogFile*> wellLogFiles = wellLogFilesContainingFlow(m_wellName);
//    if(wellLogFiles.size() > 0)
//    {
//        sourcesToSelect.push_back(RifWellRftAddress(RftSourceType::OBSERVED));
//        for (RimWellLogFile* const wellLogFile : wellLogFiles)
//        {
//            addTimeStepsToMap(observedTimeStepsMap, timeStepsFromWellLogFile(wellLogFile));
//        }
//    }
//
//    m_selectedSources = sourcesToSelect;
//    
//    std::set<QDateTime> timeStepsToSelect;
//    for (const std::pair<QDateTime, std::set<RifWellRftAddress>>& dateTimePair : rftTimeStepsMap)
//    {
//        timeStepsToSelect.insert(dateTimePair.first);
//    }
//    for (const std::pair<QDateTime, std::set<RifWellRftAddress>>& dateTimePair : observedTimeStepsMap)
//    {
//        timeStepsToSelect.insert(dateTimePair.first);
//    }
//    if (gridTimeStepsMap.size() > 0)
//        timeStepsToSelect.insert((*gridTimeStepsMap.begin()).first);
//
//    m_selectedTimeSteps = std::vector<QDateTime>(timeStepsToSelect.begin(), timeStepsToSelect.end());
//
//    syncCurvesFromUiSelection();
//}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::updateEditorsFromCurves()
{
//    std::set<RifWellRftAddress>                         selectedSources;
//    std::set<QDateTime>                                 selectedTimeSteps;
//    std::map<QDateTime, std::set<RifWellRftAddress>>    selectedTimeStepsMap;
//
//    for (const std::pair<RifWellRftAddress, QDateTime>& curveDef : curveDefsFromCurves())
//    {
//        if (curveDef.first.sourceType() == RftSourceType::OBSERVED)
//            selectedSources.insert(RifWellRftAddress(RftSourceType::OBSERVED));
//        else
//            selectedSources.insert(curveDef.first);
//
//        auto newTimeStepMap = std::map<QDateTime, std::set<RifWellRftAddress>>
//        {
//            { curveDef.second, std::set<RifWellRftAddress> { curveDef.first} }
//        };
//        addTimeStepsToMap(selectedTimeStepsMap, newTimeStepMap);
//        selectedTimeSteps.insert(curveDef.second);
//    }
//
//    m_selectedSources = std::vector<RifWellRftAddress>(selectedSources.begin(), selectedSources.end());
//    m_selectedTimeSteps = std::vector<QDateTime>(selectedTimeSteps.begin(), selectedTimeSteps.end());
//    addTimeStepsToMap(m_timeStepsToAddresses, selectedTimeStepsMap);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::updateWidgetTitleWindowTitle()
{
    updateMdiWindowTitle();

    if (m_wellLogPlotWidget)
    {
        if (m_showPlotTitle)
        {
            m_wellLogPlotWidget->showTitle(m_userName);
        }
        else
        {
            m_wellLogPlotWidget->hideTitle();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::syncCurvesFromUiSelection()
{
    RimWellLogTrack* plotTrack = m_wellLogPlot->trackByIndex(0);
    const std::set<std::pair<RifWellRftAddress, QDateTime>>& curveDefs = selectedCurveDefs();
    updateCurvesInPlot(curveDefs);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimWellLogFile*> RimWellPltPlot::wellLogFilesContainingFlow(const QString& wellName) const
{
    std::vector<RimWellLogFile*> wellLogFiles;
    const RimProject* const project = RiaApplication::instance()->project();

    for (const auto& oilField : project->oilFields)
    {
        auto wellPathsVector = std::vector<RimWellPath*>(oilField->wellPathCollection()->wellPaths.begin(), oilField->wellPathCollection()->wellPaths.end());

        for (const auto& wellPath : wellPathsVector)
        {
            bool hasPressure = false;
            const std::vector<RimWellLogFile*> files = wellPath->wellLogFiles();

            for (RimWellLogFile* const file : files)
            {
                size_t timeStepCount = timeStepsFromWellLogFile(file).size();

                if (timeStepCount == 0) continue;
                if (QString::compare(file->wellName(), wellName) != 0) continue;

                if (hasFlowData(file))
                {
                    wellLogFiles.push_back(file);
                }
            }
        }
    }
    return wellLogFiles;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimWellLogFileChannel*> RimWellPltPlot::getFlowChannelsFromWellFile(const RimWellLogFile* wellLogFile) const
{
    std::vector<RimWellLogFileChannel*> channels;
    if(wellLogFile != nullptr)
    {
        for (RimWellLogFileChannel* const channel : wellLogFile->wellLogChannels())
        {
            if (isFlowChannel(channel))
            {
                channels.push_back(channel);
            }
        }
    }
    return channels;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPath* RimWellPltPlot::wellPathFromWellLogFile(const RimWellLogFile* wellLogFile) const
{
    RimProject* const project = RiaApplication::instance()->project();
    for (const auto& oilField : project->oilFields)
    {
        auto wellPaths = std::vector<RimWellPath*>(oilField->wellPathCollection()->wellPaths.begin(), oilField->wellPathCollection()->wellPaths.end());

        for (const auto& wellPath : wellPaths)
        {
            for (RimWellLogFile* const file : wellPath->wellLogFiles())
            {
                if (file == wellLogFile)
                {
                    return wellPath;
                }
            }
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
//std::vector<std::tuple<RimEclipseResultCase*, bool/*hasPressure*/, bool /*hasRftData*/>> 
//RimWellPltPlot::eclipseCasesForWell(const QString& wellName) const
//{
//    std::vector<std::tuple<RimEclipseResultCase*, bool, bool>> cases;
//    const RimProject* const project = RiaApplication::instance()->project();
//
//    for (const auto& oilField : project->oilFields)
//    {
//        const RimEclipseCaseCollection* const eclCaseColl = oilField->analysisModels();
//        for (RimEclipseCase* eCase : eclCaseColl->cases())
//        {
//            auto eclCase = dynamic_cast<RimEclipseResultCase*>(eCase);
//            if (eclCase != nullptr)
//            {
//                RigEclipseCaseData* const eclipseCaseData = eclCase->eclipseCaseData();
//                for (const cvf::ref<RigSimWellData>& wellResult : eclipseCaseData->wellResults())
//                {
//                    if (QString::compare(wellResult->m_wellName, wellName) == 0)
//                    {
//                        bool hasPressure = hasPressureData(eclCase);
//                        bool hasRftData = eclCase->rftReader() != nullptr;
//                        cases.push_back(std::make_tuple(eclCase, hasPressure, hasRftData));
//                        break;
//                    }
//                }
//            }
//        }
//    }
//    return cases;
//}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
//std::vector<RimEclipseResultCase*> 
//RimWellPltPlot::gridCasesFromEclipseCases(const std::vector<std::tuple<RimEclipseResultCase*, bool, bool>>& eclipseCasesTuple) const
//{
//    std::vector<RimEclipseResultCase*> cases;
//    for (const std::tuple<RimEclipseResultCase*, bool, bool>& eclCaseTuple : eclipseCasesTuple)
//    {
//        bool hasPressureData = std::get<1>(eclCaseTuple);
//        size_t timeStepCount = timeStepsFromGridCase(std::get<0>(eclCaseTuple)).size();
//        if (hasPressureData && timeStepCount > 0)
//        {
//            cases.push_back(std::get<0>(eclCaseTuple));
//        }
//    }
//    return cases;
//}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
//std::vector<RimEclipseResultCase*> 
//RimWellPltPlot::rftCasesFromEclipseCases(const std::vector<std::tuple<RimEclipseResultCase*, bool, bool>>& eclipseCasesTuple) const
//{
//    std::vector<RimEclipseResultCase*> cases;
//    for (const std::tuple<RimEclipseResultCase*, bool, bool>& eclCaseTuple : eclipseCasesTuple)
//    {
//        bool hasRftData = std::get<2>(eclCaseTuple);
//        size_t timeStepCount = timeStepsFromRftCase(std::get<0>(eclCaseTuple)).size();
//        if (hasRftData && timeStepCount > 0)
//        {
//            cases.push_back(std::get<0>(eclCaseTuple));
//        }
//    }
//    return cases;
//}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
//std::map<QDateTime, std::set<RifWellRftAddress>> RimWellPltPlot::timeStepsFromRftCase(RimEclipseResultCase* rftCase) const
//{
//    std::map<QDateTime, std::set<RifWellRftAddress>> timeStepsMap;
//    RifReaderEclipseRft* const reader = rftCase->rftReader();
//    if (reader != nullptr)
//    {
//        for (const QDateTime& timeStep : reader->availableTimeSteps(m_wellName, RifEclipseRftAddress::PRESSURE))
//        {
//            if (timeStepsMap.count(timeStep) == 0)
//            {
//                timeStepsMap.insert(std::make_pair(timeStep, std::set<RifWellRftAddress>()));
//            }
//            timeStepsMap[timeStep].insert(RifWellRftAddress(RftSourceType::RFT, rftCase));
//        }
//    }
//    return timeStepsMap;
//}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
//std::map<QDateTime, std::set<RifWellRftAddress>> RimWellPltPlot::timeStepsFromGridCase(RimEclipseCase* gridCase) const
//{
//    const RigEclipseCaseData* const eclipseCaseData = gridCase->eclipseCaseData();
//    size_t resultIndex = eclipseCaseData != nullptr ? 
//        eclipseCaseData->results(RiaDefines::MATRIX_MODEL)->findScalarResultIndex(RiaDefines::DYNAMIC_NATIVE, PRESSURE_DATA_NAME) :
//        cvf::UNDEFINED_SIZE_T;
//
//    std::map<QDateTime, std::set<RifWellRftAddress>> timeStepsMap;
//    if (resultIndex != cvf::UNDEFINED_SIZE_T)
//    {
//        for (const QDateTime& timeStep : eclipseCaseData->results(RiaDefines::MATRIX_MODEL)->timeStepDates(resultIndex))
//        {
//            if (timeStepsMap.count(timeStep) == 0)
//            {
//                timeStepsMap.insert(std::make_pair(timeStep, std::set<RifWellRftAddress>()));
//            }
//            timeStepsMap[timeStep].insert(RifWellRftAddress(RftSourceType::GRID, gridCase));
//        }
//    }
//    return timeStepsMap;
//}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::map<QDateTime, std::set<RifWellRftAddress> > RimWellPltPlot::timeStepsFromWellLogFile(RimWellLogFile* wellLogFile) const
{
    std::map<QDateTime, std::set<RifWellRftAddress> > timeStepsMap;

    QDateTime timeStep = RiaDateStringParser::parseDateString(wellLogFile->date());

    if (timeStepsMap.count(timeStep) == 0)
    {
        timeStepsMap.insert(std::make_pair(timeStep, std::set<RifWellRftAddress>()));
    }
    timeStepsMap[timeStep].insert(RifWellRftAddress(RifWellRftAddress::OBSERVED, wellLogFile));

    return timeStepsMap;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::map<QDateTime, std::set<RifWellRftAddress>> 
RimWellPltPlot::adjacentTimeSteps(const std::vector<std::pair<QDateTime, std::set<RifWellRftAddress>>>& allTimeSteps,
                                  const std::pair<QDateTime, std::set<RifWellRftAddress>>& searchTimeStepPair)
{
    std::map<QDateTime, std::set<RifWellRftAddress>> timeStepsMap;

    if (allTimeSteps.size() > 0)
    {
        auto itr = std::find_if(allTimeSteps.begin(), allTimeSteps.end(), 
                                [searchTimeStepPair](const std::pair<QDateTime, std::set<RifWellRftAddress>>& dt)
        {
            return dt.first > searchTimeStepPair.first;
        });

        auto itrEnd = itr != allTimeSteps.end() ? itr + 1 : itr;

        for (itr = itrEnd - 1; itr != allTimeSteps.begin() && (*itr).first >= searchTimeStepPair.first; itr--);
        auto itrFirst = itr;

        timeStepsMap.insert(itrFirst, itrEnd);
    }

    // Add searched time step in case it is not included
    addTimeStepToMap(timeStepsMap, searchTimeStepPair);

    return timeStepsMap;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellPltPlot::mapContainsTimeStep(const std::map<QDateTime, std::set<RifWellRftAddress>>& map, const QDateTime& timeStep)
{
    return std::find_if(map.begin(), map.end(), [timeStep](const std::pair<QDateTime, std::set<RifWellRftAddress>>& pair)
    {
        return pair.first == timeStep;
    }) != map.end();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::set < std::pair<RifWellRftAddress, QDateTime>> RimWellPltPlot::selectedCurveDefs() const
{
    std::set<std::pair<RifWellRftAddress, QDateTime>> curveDefs;
    //const std::vector<std::tuple<RimEclipseResultCase*,bool,bool>>& eclipseCases = eclipseCasesForWell(m_wellName);
    //const std::vector<RimEclipseResultCase*> rftCases = rftCasesFromEclipseCases(eclipseCases);
    //const std::vector<RimEclipseResultCase*> gridCases = gridCasesFromEclipseCases(eclipseCases);

    for (const QDateTime& timeStep : m_selectedTimeSteps())
    {
        for (const RifWellRftAddress& addr : selectedSources())
        {
            //if (addr.sourceType() == RftSourceType::RFT)
            //{
            //    for (RimEclipseResultCase* const rftCase : rftCases)
            //    {
            //        const std::map<QDateTime, std::set<RifWellRftAddress>>& timeStepsMap = timeStepsFromRftCase(rftCase);
            //        if (mapContainsTimeStep(timeStepsMap , timeStep))
            //        {
            //            curveDefs.insert(std::make_pair(addr, timeStep));
            //        }
            //    }
            //}
            //else if (addr.sourceType() == RftSourceType::GRID)
            //{
            //    for (RimEclipseResultCase* const gridCase : gridCases)
            //    {
            //        const std::map<QDateTime, std::set<RifWellRftAddress>>& timeStepsMap = timeStepsFromGridCase(gridCase);
            //        if (mapContainsTimeStep(timeStepsMap, timeStep))
            //        {
            //            curveDefs.insert(std::make_pair(addr, timeStep));
            //        }
            //    }
            //}
            //else
            if (addr.sourceType() == RifWellRftAddress::OBSERVED)
            {
                if (addr.wellLogFile() != nullptr)
                {
                    const std::map<QDateTime, std::set<RifWellRftAddress>>& timeStepsMap = timeStepsFromWellLogFile(addr.wellLogFile());
                    if (mapContainsTimeStep(timeStepsMap, timeStep))
                    {
                        curveDefs.insert(std::make_pair(RifWellRftAddress(RifWellRftAddress::OBSERVED, addr.wellLogFile()), timeStep));
                    }
                }
            }
        }
    }
    return curveDefs;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::set<std::pair<RifWellRftAddress, QDateTime>> RimWellPltPlot::curveDefsFromCurves() const
{
    std::set<std::pair<RifWellRftAddress, QDateTime>> curveDefs;

    RimWellLogTrack* const plotTrack = m_wellLogPlot->trackByIndex(0);
    for (RimWellLogCurve* const curve : plotTrack->curvesVector())
    {
        curveDefs.insert(curveDefFromCurve(curve));
    }
    return curveDefs;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::pair<RifWellRftAddress, QDateTime> RimWellPltPlot::curveDefFromCurve(const RimWellLogCurve* curve) const
{
    const RimWellLogRftCurve* rftCurve = dynamic_cast<const RimWellLogRftCurve*>(curve);
    const RimWellLogExtractionCurve* gridCurve = dynamic_cast<const RimWellLogExtractionCurve*>(curve);
    const RimWellLogFileCurve* wellLogFileCurve = dynamic_cast<const RimWellLogFileCurve*>(curve);

    //if (rftCurve != nullptr)
    //{
    //    RimEclipseResultCase* rftCase = dynamic_cast<RimEclipseResultCase*>(rftCurve->eclipseResultCase());
    //    if (rftCase != nullptr)
    //    {
    //        const RifEclipseRftAddress rftAddress = rftCurve->rftAddress();
    //        const QDateTime timeStep = rftAddress.timeStep();
    //        return std::make_pair(RifWellRftAddress(RftSourceType::RFT, rftCase), timeStep);
    //    }
    //}
    //else if (gridCurve != nullptr)
    //{
    //    RimEclipseResultCase* gridCase = dynamic_cast<RimEclipseResultCase*>(gridCurve->rimCase());
    //    if (gridCase != nullptr)
    //    {
    //        size_t timeStepIndex = gridCurve->currentTimeStep();
    //        const std::map<QDateTime, std::set<RifWellRftAddress>>& timeStepsMap = timeStepsFromGridCase(gridCase);
    //        auto timeStepsVector = std::vector<std::pair<QDateTime, std::set<RifWellRftAddress>>>(
    //            timeStepsMap.begin(), timeStepsMap.end());
    //        if (timeStepIndex < timeStepsMap.size())
    //        {
    //            return std::make_pair(RifWellRftAddress(RftSourceType::GRID, gridCase),
    //                                  timeStepsVector[timeStepIndex].first);
    //        }
    //    }
    //}
    //else
    if (wellLogFileCurve != nullptr)
    {
        const RimWellPath* const wellPath = wellLogFileCurve->wellPath();
        RimWellLogFile* const wellLogFile = wellLogFileCurve->wellLogFile();

        if (wellLogFile != nullptr)
        {
            const QDateTime date = RiaDateStringParser::parseDateString(wellLogFile->date());

            if (date.isValid())
            {
                return std::make_pair(RifWellRftAddress(RifWellRftAddress::OBSERVED, wellLogFile), date);
            }
        }
    }
    return std::make_pair(RifWellRftAddress(), QDateTime());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::updateCurvesInPlot(const std::set<std::pair<RifWellRftAddress, QDateTime>>& curveDefs)
{
    RimWellLogTrack* const plotTrack = m_wellLogPlot->trackByIndex(0);

    setPlotXAxisTitles(plotTrack);

    // Delete existing curves
    const auto& curves = plotTrack->curvesVector();
    for (const auto& curve : curves)
    {
        plotTrack->removeCurve(curve);
    }

    // Add curves
    for (const std::pair<RifWellRftAddress, QDateTime>& curveDefToAdd : curveDefs)
    {
        //if (curveDefToAdd.first.sourceType() == RftSourceType::RFT)
        //{
        //    auto curve = new RimWellLogRftCurve();
        //    plotTrack->addCurve(curve);

        //    auto rftCase = curveDefToAdd.first.eclCase();
        //    curve->setEclipseResultCase(dynamic_cast<RimEclipseResultCase*>(rftCase));

        //    RifEclipseRftAddress address(m_wellName, curveDefToAdd.second, RifEclipseRftAddress::PRESSURE);
        //    curve->setRftAddress(address);
        //    curve->setZOrder(1);

        //    applyCurveAppearance(curve);
        //    curve->loadDataAndUpdate(true);
        //}
        //else if (curveDefToAdd.first.sourceType() == RftSourceType::GRID)
        //{
        //    auto curve = new RimWellLogExtractionCurve();
        //    plotTrack->addCurve(curve);

        //    cvf::Color3f curveColor = RiaColorTables::wellLogPlotPaletteColors().cycledColor3f(plotTrack->curveCount());
        //    curve->setColor(curveColor);
        //    curve->setFromSimulationWellName(m_wellName, m_branchIndex);

        //    // Fetch cases and time steps
        //    auto gridCase = curveDefToAdd.first.eclCase();
        //    if (gridCase != nullptr)
        //    {
        //        // Case
        //        curve->setCase(gridCase);

        //        // Result definition
        //        RimEclipseResultDefinition* resultDef = new RimEclipseResultDefinition();
        //        resultDef->setResultVariable(PRESSURE_DATA_NAME);
        //        curve->setEclipseResultDefinition(resultDef);

        //        // Time step
        //        const std::map<QDateTime, std::set<RifWellRftAddress>>& timeSteps = timeStepsFromGridCase(gridCase);
        //        auto currentTimeStepItr = std::find_if(timeSteps.begin(), timeSteps.end(), 
        //                                               [curveDefToAdd](std::pair<QDateTime, std::set<RifWellRftAddress>> pair) {return pair.first == curveDefToAdd.second; });
        //        auto currentTimeStepIndex = std::distance(timeSteps.begin(), currentTimeStepItr);
        //        curve->setCurrentTimeStep(currentTimeStepIndex);
        //        curve->setZOrder(0);

        //        applyCurveAppearance(curve);
        //        curve->loadDataAndUpdate(false);
        //    }
        //}
        //else 
        if (curveDefToAdd.first.sourceType() == RifWellRftAddress::OBSERVED)
        {
            RimWellLogFile* const wellLogFile = curveDefToAdd.first.wellLogFile();
            RimWellPath* const wellPath = wellPathFromWellLogFile(wellLogFile);
            if(wellLogFile!= nullptr)
            {
                std::set<FlowPhase> selectedPhases = m_phaseSelectionMode == FLOW_TYPE_PHASE_SPLIT ?
                    std::set<FlowPhase>(m_phases().begin(), m_phases().end()) :
                    std::set<FlowPhase>({ PHASE_TOTAL });

                RigWellLogFile* rigWellLogFile = wellLogFile->wellLogFile();

                if (rigWellLogFile != nullptr)
                {
                    for (RimWellLogFileChannel* channel : getFlowChannelsFromWellFile(wellLogFile))
                    {
                        const auto& channelName = channel->name();
                        if (selectedPhases.count(flowPhaseFromChannelName(channelName)) > 0)
                        {

                            addStackedCurve(channelName, rigWellLogFile->depthValues(), rigWellLogFile->values(channelName), plotTrack);
                        }
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::addStackedCurve(const QString& channelName,
                                     const std::vector<double>& depthValues,
                                     const std::vector<double>& accFlow,
                                     RimWellLogTrack* plotTrack)
{
    RimWellFlowRateCurve* curve = new RimWellFlowRateCurve;
    curve->setFlowValuesPrDepthValue(channelName, depthValues, accFlow);

    auto color = channelName == OIL_CHANNEL_NAME   ? cvf::Color3f::DARK_GREEN :
                 channelName == GAS_CHANNEL_NAME   ? cvf::Color3f::DARK_RED :
                 channelName == WATER_CHANNEL_NAME ? cvf::Color3f::BLUE : 
                 cvf::Color3f::DARK_GRAY;
        curve->setColor(color);

    plotTrack->addCurve(curve);

    curve->loadDataAndUpdate(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellPltPlot::isOnlyGridSourcesSelected() const
{
    const std::vector<RifWellRftAddress>& selSources = m_selectedSources();
    return std::find_if(selSources.begin(), selSources.end(), [](const RifWellRftAddress& addr)
    {
        return addr.sourceType() == RifWellRftAddress::RFT || addr.sourceType() == RifWellRftAddress::OBSERVED;
    }) == selSources.end();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellPltPlot::isAnySourceAddressSelected(const std::set<RifWellRftAddress>& addresses) const
{
    const std::vector<RifWellRftAddress>& selectedSourcesVector = m_selectedSources();
    const auto selectedSources = std::set<RifWellRftAddress>(selectedSourcesVector.begin(), selectedSourcesVector.end());
    std::vector<RifWellRftAddress> intersectVector;

    std::set_intersection(selectedSources.begin(), selectedSources.end(),
                          addresses.begin(), addresses.end(), std::inserter(intersectVector, intersectVector.end()));
    return intersectVector.size() > 0;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RifWellRftAddress> RimWellPltPlot::selectedSources() const
{
    std::vector<RifWellRftAddress> sources;
    for (const RifWellRftAddress& addr : m_selectedSources())
    {
        if (addr.sourceType() == RifWellRftAddress::OBSERVED)
        {
            for (RimWellLogFile* const wellLogFile : wellLogFilesContainingFlow(m_wellName))
            {
                sources.push_back(RifWellRftAddress(RifWellRftAddress::OBSERVED, wellLogFile));
            }
        }
        else
            sources.push_back(addr);
    }
    return sources;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QWidget* RimWellPltPlot::viewWidget()
{
    return m_wellLogPlotWidget;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::zoomAll()
{
    m_wellLogPlot()->zoomAll();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellLogPlot* RimWellPltPlot::wellLogPlot() const
{
    return m_wellLogPlot();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::setCurrentWellName(const QString& currWellName)
{
    m_wellName = currWellName;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellPltPlot::currentWellName() const
{
    return m_wellName;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellPltPlot::hasFlowData(const RimWellLogFile* wellLogFile)
{
    for (RimWellLogFileChannel* const wellLogChannel : wellLogFile->wellLogChannels())
    {
        if (isFlowChannel(wellLogChannel)) return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellPltPlot::hasFlowData(RimWellPath* wellPath)
{
    for (RimWellLogFile* const wellLogFile : wellPath->wellLogFiles())
    {
        if (hasFlowData(wellLogFile))
        {
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellPltPlot::isFlowChannel(RimWellLogFileChannel* channel)
{
    return FLOW_DATA_NAMES.count(channel->name()) > 0;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellPltPlot::hasFlowData(RimEclipseResultCase* gridCase)
{
    const RigEclipseCaseData* const eclipseCaseData = gridCase->eclipseCaseData();

    for (const QString& channelName : FLOW_DATA_NAMES)
    {
        size_t resultIndex = eclipseCaseData->results(RiaDefines::MATRIX_MODEL)->
            findScalarResultIndex(RiaDefines::DYNAMIC_NATIVE, channelName);

        if (resultIndex != cvf::UNDEFINED_SIZE_T) return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const char* RimWellPltPlot::plotNameFormatString()
{
    return PLOT_NAME_QFORMAT_STRING;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimWellPltPlot::calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly)
{
    QList<caf::PdmOptionItemInfo> options;

    if (fieldNeedingOptions == &m_wellName)
    {
        calculateValueOptionsForWells(options);
    }
    else if (fieldNeedingOptions == &m_selectedSources)
    {
        //const std::vector<std::tuple<RimEclipseResultCase*, bool, bool>>& eclipseCases = eclipseCasesForWell(m_wellName);

        //const std::vector<RimEclipseResultCase*> rftCases = rftCasesFromEclipseCases(eclipseCases);
        //if (rftCases.size() > 0)
        //{
        //    options.push_back(caf::PdmOptionItemInfo::createHeader(RifWellRftAddress::sourceTypeUiText(RftSourceType::RFT), true));
        //}
        //for (const auto& rftCase : rftCases)
        //{
        //    auto addr = RifWellRftAddress(RftSourceType::RFT, rftCase);
        //    auto item = caf::PdmOptionItemInfo(rftCase->caseUserDescription(), QVariant::fromValue(addr));
        //    item.setLevel(1);
        //    options.push_back(item);
        //}

        //const std::vector<RimEclipseResultCase*> gridCases = gridCasesFromEclipseCases(eclipseCases);
        //if (gridCases.size() > 0)
        //{
        //    options.push_back(caf::PdmOptionItemInfo::createHeader(RifWellRftAddress::sourceTypeUiText(RftSourceType::GRID), true));
        //}
        //for (const auto& gridCase : gridCases)
        //{
        //    auto addr = RifWellRftAddress(RftSourceType::GRID, gridCase);
        //    auto item = caf::PdmOptionItemInfo(gridCase->caseUserDescription(), QVariant::fromValue(addr));
        //    item.setLevel(1);
        //    options.push_back(item);
        //}

        if (wellLogFilesContainingFlow(m_wellName).size() > 0)
        {
            options.push_back(caf::PdmOptionItemInfo::createHeader(RifWellRftAddress::sourceTypeUiText(RifWellRftAddress::OBSERVED), true));

            auto addr = RifWellRftAddress(RifWellRftAddress::OBSERVED);
            auto item = caf::PdmOptionItemInfo("Observed Data", QVariant::fromValue(addr));
            item.setLevel(1);
            options.push_back(item);
        }
    }
    else if (fieldNeedingOptions == &m_selectedTimeSteps)
    {
        calculateValueOptionsForTimeSteps(m_wellName, options);
    }
    else if (fieldNeedingOptions == &m_branchIndex)
    {
        RimProject* proj = RiaApplication::instance()->project();

        size_t branchCount = proj->simulationWellBranches(m_wellName).size();

        for (int bIdx = 0; bIdx < static_cast<int>(branchCount); ++bIdx)
        {
            options.push_back(caf::PdmOptionItemInfo("Branch " + QString::number(bIdx + 1), QVariant::fromValue(bIdx)));
        }

        if (options.size() == 0)
        {
            options.push_front(caf::PdmOptionItemInfo("None", -1));
        }
    }

    if (fieldNeedingOptions == &m_phaseSelectionMode)
    {
    }
    else if (fieldNeedingOptions == &m_phases)
    {
        options.push_back(caf::PdmOptionItemInfo("Oil", PHASE_OIL));
        options.push_back(caf::PdmOptionItemInfo("Gas", PHASE_GAS));
        options.push_back(caf::PdmOptionItemInfo("Water", PHASE_WATER));
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimViewWindow::fieldChangedByUi(changedField, oldValue, newValue);

    if (changedField == &m_wellName)
    {
        setDescription(QString(plotNameFormatString()).arg(m_wellName));
    }

    if (changedField == &m_wellName || changedField == &m_branchIndex)
    {
        RimWellLogTrack* const plotTrack = m_wellLogPlot->trackByIndex(0);
        for (RimWellLogCurve* const curve : plotTrack->curvesVector())
        {
            plotTrack->removeCurve(curve);
        }
        m_timeStepsToAddresses.clear();
        updateEditorsFromCurves();
    }
    else if (changedField == &m_selectedSources)
    {
        // Update time steps selections based on source selections
        updateSelectedTimeStepsFromSelectedSources();
    }

    if (changedField == &m_selectedSources ||
        changedField == &m_selectedTimeSteps)
    {
        syncCurvesFromUiSelection();
    }
    else if (changedField == &m_showPlotTitle)
    {
        //m_wellLogPlot->setShowDescription(m_showPlotTitle);
    }

    if (changedField == &m_phaseSelectionMode ||
        changedField == &m_phases)
    {
        syncCurvesFromUiSelection();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QImage RimWellPltPlot::snapshotWindowContent()
{
    QImage image;

    if (m_wellLogPlotWidget)
    {
        QPixmap pix = QPixmap::grabWidget(m_wellLogPlotWidget);
        image = pix.toImage();
    }

    return image;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    //if (!m_selectedSourcesOrTimeStepsFieldsChanged)
    //{
    //    updateEditorsFromCurves();
    //}
    //m_selectedSourcesOrTimeStepsFieldsChanged = false;

    uiOrdering.add(&m_userName);
    uiOrdering.add(&m_wellName);
    
    RimProject* proj = RiaApplication::instance()->project();
    if (proj->simulationWellBranches(m_wellName).size() > 1)
    {
        uiOrdering.add(&m_branchIndex);
    }

    caf::PdmUiGroup* sourcesGroup = uiOrdering.addNewGroupWithKeyword("Sources", "Sources");
    sourcesGroup->add(&m_selectedSources);

    caf::PdmUiGroup* timeStepsGroup = uiOrdering.addNewGroupWithKeyword("Time Steps", "TimeSteps");
    timeStepsGroup->add(&m_selectedTimeSteps);

    caf::PdmUiGroup* flowGroup = uiOrdering.addNewGroupWithKeyword("Phase Selection", "PhaseSelection");
    flowGroup->add(&m_phaseSelectionMode);

    if (m_phaseSelectionMode == FLOW_TYPE_PHASE_SPLIT)
    {
        flowGroup->add(&m_phases);
    }

    //uiOrdering.add(&m_showPlotTitle);

    uiOrdering.skipRemainingFields(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute)
{
    if (field == &m_phases)
    {
        caf::PdmUiTreeSelectionEditorAttribute* attrib = dynamic_cast<caf::PdmUiTreeSelectionEditorAttribute*> (attribute);
        attrib->showTextFilter = false;
        attrib->showToggleAllCheckbox = false;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::addTimeStepToMap(std::map<QDateTime, std::set<RifWellRftAddress>>& destMap,
                                       const std::pair<QDateTime, std::set<RifWellRftAddress>>& timeStepToAdd)
{
    auto timeStepMapToAdd = std::map<QDateTime, std::set<RifWellRftAddress>> { timeStepToAdd };
    addTimeStepsToMap(destMap, timeStepMapToAdd);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::addTimeStepsToMap(std::map<QDateTime, std::set<RifWellRftAddress>>& destMap, 
                                       const std::map<QDateTime, std::set<RifWellRftAddress>>& timeStepsToAdd)
{
    for (const auto& timeStepPair : timeStepsToAdd)
    {
        if (timeStepPair.first.isValid())
        {
            if (destMap.count(timeStepPair.first) == 0)
            {
                destMap.insert(std::make_pair(timeStepPair.first, std::set<RifWellRftAddress>()));
            }
            auto addresses = timeStepPair.second;
            destMap[timeStepPair.first].insert(addresses.begin(), addresses.end());
        }
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::calculateValueOptionsForWells(QList<caf::PdmOptionItemInfo>& options)
{
    RimProject * proj = RiaApplication::instance()->project();

    if (proj != nullptr)
    {
        const std::vector<QString> simWellNames = proj->simulationWellNames();
        auto wellNames = std::set<QString>(simWellNames.begin(), simWellNames.end());

        // Observed wells
        for (const auto& oilField : proj->oilFields())
        {
            auto wellPathColl = oilField->wellPathCollection();
            for (const auto& wellPath : wellPathColl->wellPaths)
            {
                wellNames.insert(wellPath->name());
            }
        }

        for (const auto& wellName : wellNames)
        {
            options.push_back(caf::PdmOptionItemInfo(wellName, wellName));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::calculateValueOptionsForTimeSteps(const QString& wellName, QList<caf::PdmOptionItemInfo>& options)
{
    std::map<QDateTime, std::set<RifWellRftAddress>> displayTimeStepsMap, obsAndRftTimeStepsMap, gridTimeStepsMap;
    //const std::vector<std::tuple<RimEclipseResultCase*, bool, bool>>& eclipseCases = eclipseCasesForWell(wellName);
    //const std::vector<RimEclipseResultCase*> rftCases = rftCasesFromEclipseCases(eclipseCases);
    //const std::vector<RimEclipseResultCase*> gridCases = gridCasesFromEclipseCases(eclipseCases);

    for (const RifWellRftAddress& selection : selectedSources())
    {
        //if (selection.sourceType() == RftSourceType::RFT)
        //{
        //    for (RimEclipseResultCase* const rftCase : rftCases)
        //    {
        //        addTimeStepsToMap(obsAndRftTimeStepsMap, timeStepsFromRftCase(rftCase));
        //    }
        //}
        //else if (selection.sourceType() == RftSourceType::GRID)
        //{
        //    for (RimEclipseResultCase* const gridCase : gridCases)
        //    {
        //        addTimeStepsToMap(gridTimeStepsMap, timeStepsFromGridCase(gridCase));
        //    }
        //}
        //else
        if (selection.sourceType() == RifWellRftAddress::OBSERVED)
        {
            if (selection.wellLogFile() != nullptr)
            {
                addTimeStepsToMap(obsAndRftTimeStepsMap, timeStepsFromWellLogFile(selection.wellLogFile()));
            }
        }
    }

    if (isOnlyGridSourcesSelected())
    {
        displayTimeStepsMap = gridTimeStepsMap;
    }
    else
    {
        const auto gridTimeStepsVector = std::vector<std::pair<QDateTime, std::set<RifWellRftAddress>>>(gridTimeStepsMap.begin(), gridTimeStepsMap.end());

        for (const std::pair<QDateTime, std::set<RifWellRftAddress>>& timeStepPair : obsAndRftTimeStepsMap)
        {
            const std::map<QDateTime, std::set<RifWellRftAddress>>& adjTimeSteps = adjacentTimeSteps(gridTimeStepsVector, timeStepPair);
            addTimeStepsToMap(displayTimeStepsMap, adjTimeSteps);
        }

        // Add the first grid time step (from the total grid time steps list)
        if (gridTimeStepsVector.size() > 0)
        {
            addTimeStepToMap(displayTimeStepsMap, gridTimeStepsVector.front());
        }

        // Add already selected time steps
        for (const QDateTime& timeStep : m_selectedTimeSteps())
        {
            if (m_timeStepsToAddresses.count(timeStep) > 0)
            {
                const std::set<RifWellRftAddress> sourceAddresses = m_timeStepsToAddresses[timeStep];
                if (isAnySourceAddressSelected(sourceAddresses))
                {
                    addTimeStepToMap(displayTimeStepsMap, std::make_pair(timeStep, m_timeStepsToAddresses[timeStep]));
                }
            }
        }
    }

    addTimeStepsToMap(m_timeStepsToAddresses, displayTimeStepsMap);

    // Create vector of all time steps
    std::vector<QDateTime> allTimeSteps;
    for (const std::pair<QDateTime, std::set<RifWellRftAddress>>& timeStepPair : m_timeStepsToAddresses)
    {
        allTimeSteps.push_back(timeStepPair.first);
    }

    const QString dateFormatString = RimTools::createTimeFormatStringFromDates(allTimeSteps);
    for (const std::pair<QDateTime, std::set<RifWellRftAddress>>& timeStepPair : displayTimeStepsMap)
    {
        options.push_back(caf::PdmOptionItemInfo(timeStepPair.first.toString(dateFormatString), timeStepPair.first));
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::setDescription(const QString& description)
{
    m_userName = description;

    updateWidgetTitleWindowTitle();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellPltPlot::description() const
{
    return m_userName();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPltPlot::loadDataAndUpdate()
{
    updateMdiWindowVisibility();
    m_wellLogPlot->loadDataAndUpdate();
    updateEditorsFromCurves();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QWidget* RimWellPltPlot::createViewWidget(QWidget* mainWindowParent)
{
    m_wellLogPlotWidget = new RiuWellPltPlot(this, mainWindowParent);
    return m_wellLogPlotWidget;
}
