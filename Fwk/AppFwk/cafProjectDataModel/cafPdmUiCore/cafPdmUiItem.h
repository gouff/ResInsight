//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################


#pragma once

#include "cafPdmUiFieldSpecialization.h"

#include <QString>
#include <QIcon>
#include <QVariant>
#include <set>

namespace caf 
{

//==================================================================================================
/// Class to keep (principally static) gui presentation information 
/// of a data structure item (field or object) used by PdmUiItem
//==================================================================================================

class PdmUiItemInfo
{
public:  
    PdmUiItemInfo()
        : m_editorTypeName(""), m_isHidden(-1), m_isTreeChildrenHidden(-1), m_isReadOnly(-1), m_labelAlignment(LEFT)
    {}

    PdmUiItemInfo(const QString& uiName, QIcon icon = QIcon(), QString toolTip = "", QString whatsThis = "", QString extraDebugText = "")
        : m_uiName(uiName), m_icon(icon), m_toolTip(toolTip), m_whatsThis(whatsThis), m_extraDebugText(extraDebugText),
          m_editorTypeName(""), m_isHidden(false), m_isTreeChildrenHidden(false), m_isReadOnly(false), m_labelAlignment(LEFT)
    { }

    enum LabelPosType { LEFT, TOP, HIDDEN };

private: 
    friend class PdmUiItem;
    QString         m_uiName;
    QIcon           m_icon;         
    QString         m_toolTip;
    QString         m_whatsThis;
    QString         m_extraDebugText;
    QString         m_editorTypeName;       ///< Use this exact type of editor to edit this UiItem
    int             m_isHidden;             ///< UiItem should be hidden. -1 means not set
    int             m_isTreeChildrenHidden; ///< Children of UiItem should be hidden. -1 means not set
    int             m_isReadOnly;           ///< UiItem should be insensitive, or read only. -1 means not set.
    LabelPosType    m_labelAlignment;
};

//==================================================================================================
/// Class to keep Ui information about an option /choice in a Combobox or similar.
//==================================================================================================

class PdmOptionItemInfo
{
public:
    PdmOptionItemInfo(const QString& anOptionUiText, const QVariant& aValue, bool isReadOnly = false, QIcon anIcon = QIcon());
    PdmOptionItemInfo(const QString& anOptionUiText, caf::PdmObjectHandle* obj, bool isReadOnly = false, QIcon anIcon = QIcon());

    static PdmOptionItemInfo createHeader(const QString& anOptionUiText, bool isReadOnly = false, QIcon anIcon = QIcon());

    void            setLevel(int level);

    const QString   optionUiText() const;
    const QVariant  value() const;
    bool            isReadOnly() const;
    bool            isHeading() const;
    const QIcon     icon() const;
    int             level() const;


    // Static utility methods to handle QList of PdmOptionItemInfo
    // Please regard as private to the PDM system 
    
    static QStringList extractUiTexts(const QList<PdmOptionItemInfo>& optionList );
    template<typename T>
    static bool        findValues     (const QList<PdmOptionItemInfo>& optionList , QVariant fieldValue, 
                                      std::vector<unsigned int>& foundIndexes);

private:
    QString     m_optionUiText;
    QVariant    m_value;
    bool        m_isReadOnly;
    QIcon       m_icon;
    int         m_level;
};

class PdmUiEditorHandle;
//--------------------------------------------------------------------------------------------------
/// Finds the indexes into the optionList that the field value(s) corresponds to.
/// In the case where the field is some kind of array, several indexes might be returned
/// The returned bool is true if all the fieldValues were found.
//--------------------------------------------------------------------------------------------------
template<typename T>
bool PdmOptionItemInfo::findValues(const QList<PdmOptionItemInfo>& optionList, QVariant fieldValue, std::vector<unsigned int>& foundIndexes)
{
    foundIndexes.clear();

    // Find this fieldvalue in the optionlist if present

    // First handle lists/arrays of values
    if (fieldValue.type() == QVariant::List)
    {
        QList<QVariant> valuesSelectedInField = fieldValue.toList();

        if (valuesSelectedInField.size())
        {
            // Create a list to be able to remove items as they are matched with values
            std::list<std::pair<QVariant, unsigned int> > optionVariantAndIndexPairs;

            for (int i= 0 ; i < optionList.size(); ++i)
            {
                optionVariantAndIndexPairs.push_back(std::make_pair(optionList[i].value(), i));
            }

            for (int i = 0; i < valuesSelectedInField.size(); ++i)
            {
                std::list<std::pair<QVariant, unsigned int> >::iterator it;
                for (it = optionVariantAndIndexPairs.begin(); it != optionVariantAndIndexPairs.end(); ++it)
                {
                    if (PdmUiFieldSpecialization<T>::isDataElementEqual(valuesSelectedInField[i], it->first))
                    {
                        foundIndexes.push_back(it->second);

                        // Assuming that one option is referenced only once, the option is erased. Then break
                        // out of the inner loop, as this operation can be costly for fields with many options and many values

                        optionVariantAndIndexPairs.erase(it);
                        break;
                    }
                }
            }
        }

        return (static_cast<size_t>(valuesSelectedInField.size()) <= foundIndexes.size());
    }
    else  // Then handle single value fields
    {
        for (unsigned int opIdx = 0; opIdx < static_cast<unsigned int>(optionList.size()); ++opIdx)
        {
            if (PdmUiFieldSpecialization<T>::isDataElementEqual(optionList[opIdx].value(), fieldValue))
            {
                foundIndexes.push_back(opIdx);
                break;
            }
        }
        return (foundIndexes.size() > 0);
    }
}

//==================================================================================================
/// Base class for all datastructure items (fields or objects) to make them have information on 
/// how to display them in the GUI. All the information can have a static variant valid for all 
/// instances of a PDM object, and a dynamic variant that can be changed for a specific instance.
/// the dynamic values overrides the static ones if set.
//==================================================================================================

class PdmUiItem
{
public:
    PdmUiItem() : m_staticItemInfo(nullptr)                                                   { }
    virtual ~PdmUiItem();

    PdmUiItem(const PdmUiItem&) = delete;
    PdmUiItem&       operator=(const PdmUiItem&) = delete;

    const QString    uiName(QString uiConfigName = "")      const;
    void             setUiName(const QString& uiName, QString uiConfigName = "")           { m_configItemInfos[uiConfigName].m_uiName = uiName; } 

    const QIcon      uiIcon(QString uiConfigName = "")      const;
    void             setUiIcon(const QIcon& uiIcon, QString uiConfigName = "")             { m_configItemInfos[uiConfigName].m_icon = uiIcon; } 

    const QString    uiToolTip(QString uiConfigName = "")   const;
    void             setUiToolTip(const QString& uiToolTip, QString uiConfigName = "")     { m_configItemInfos[uiConfigName].m_toolTip = uiToolTip; } 

    const QString    uiWhatsThis(QString uiConfigName = "") const;
    void             setUiWhatsThis(const QString& uiWhatsThis, QString uiConfigName = "") { m_configItemInfos[uiConfigName].m_whatsThis = uiWhatsThis; } 

    bool             isUiHidden(QString uiConfigName = "") const;
    void             setUiHidden(bool isHidden, QString uiConfigName = "")                 { m_configItemInfos[uiConfigName].m_isHidden = isHidden; } 

    bool             isUiTreeHidden(QString uiConfigName = "") const;
    void             setUiTreeHidden(bool isHidden, QString uiConfigName = "")              { m_configItemInfos[uiConfigName].m_isHidden = isHidden; }

    bool             isUiTreeChildrenHidden(QString uiConfigName = "") const;
    void             setUiTreeChildrenHidden(bool isTreeChildrenHidden, QString uiConfigName = "")     { m_configItemInfos[uiConfigName].m_isTreeChildrenHidden = isTreeChildrenHidden; } 

    bool             isUiReadOnly(QString uiConfigName = "") const;
    void             setUiReadOnly(bool isReadOnly, QString uiConfigName = "")             { m_configItemInfos[uiConfigName].m_isReadOnly = isReadOnly; } 
   
    PdmUiItemInfo::LabelPosType  
                     uiLabelPosition(QString uiConfigName = "") const;
    void             setUiLabelPosition(PdmUiItemInfo::LabelPosType alignment, QString uiConfigName = "") { m_configItemInfos[uiConfigName].m_labelAlignment = alignment; } 

    QString          uiEditorTypeName(const QString& uiConfigName) const;
    void             setUiEditorTypeName(const QString& editorTypeName, QString uiConfigName = "") { m_configItemInfos[uiConfigName].m_editorTypeName = editorTypeName; }

    virtual bool     isUiGroup()                                                           { return false; }

    /// Intended to be called when fields in an object has been changed
    void             updateConnectedEditors();

    /// Intended to be called when an object has been created or deleted
    void             updateAllRequiredEditors();

    void             updateUiIconFromState(bool isActive,  QString uiConfigName = "");

    std::vector<PdmUiEditorHandle*>
                    connectedEditors() const;

    static bool     showExtraDebugText();
    static void     enableExtraDebugText(bool enable);

public: // Pdm-Private only
    //==================================================================================================
    /// This method sets the GUI description pointer, which is supposed to be statically allocated 
    /// somewhere. the PdmGuiEntry class will not delete it in any way, and always trust it to be present.
    /// Consider as PRIVATE to the PdmSystem
    //==================================================================================================

    void              setUiItemInfo(PdmUiItemInfo* itemInfo)            { m_staticItemInfo = itemInfo; }

    void              removeFieldEditor(PdmUiEditorHandle* fieldView)   { m_editors.erase(fieldView); }
    void              addFieldEditor(PdmUiEditorHandle* fieldView)      { m_editors.insert(fieldView); }

protected:
    std::set<PdmUiEditorHandle*>        m_editors;

private:
    const PdmUiItemInfo*                defaultInfo() const;
    const PdmUiItemInfo*                configInfo(QString uiConfigName) const;

    PdmUiItemInfo*                      m_staticItemInfo;
    std::map< QString, PdmUiItemInfo >  m_configItemInfos; 

    static bool                         sm_showExtraDebugText;
};



} // End of namespace caf

