#include "HEMAX_ParameterWidget.h"

#include "moc_HEMAX_ParameterWidget.cpp"

#include "../HEMAX_Parameter.h"

#include <unordered_map>

#if defined(HEMAX_VERSION_2018) || defined(HEMAX_VERSION_2019)
#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qcolordialog.h>
#endif

#ifdef HEMAX_VERSION_2017
#include <QtGui/qfiledialog.h>
#include <QtGui/qcolordialog.h>
#endif

const char *NoHDAText = "No Houdini Digital Asset selected";

HEMAX_ParameterWidget::HEMAX_ParameterWidget()
{
    MainLayout = new QVBoxLayout;

    MainBox = new QWidget(this);
    MainBoxLayout = new QVBoxLayout;

    ParametersBox = new QGroupBox;
    ParametersBoxLayout = new QGridLayout;
    ParametersSelectedAssetLabel = new QLabel("Active Asset:");
    ParametersSelectedAssetName = new QLabel(NoHDAText);
    ParametersSelectionLockedButton =
        new QPushButton("Lock to Current Selection");

    NodeOptionsBox = new QGroupBox("Node Options");
    NodeOptionsBoxLayout = new QGridLayout;
    NodeOptions_AutoRecook = new QCheckBox("Enable automatic recooking");
    NodeOptions_RealtimeRecook =
        new QCheckBox("Cook while dragging parameter slider");
    NodeOptions_InputUpdate =
        new QCheckBox("Automatically cook when an input node changes");

    NodeInputBox = new QGroupBox("Node Inputs");
    NodeInputBoxLayout = new QGridLayout;

    ParametersDetailBox = new QWidget;
    ParametersDetailGridLayout = new QGridLayout;

    MainBoxLayout->setAlignment(Qt::AlignTop);
    NodeOptionsBox->setAlignment(Qt::AlignTop);
    NodeOptionsBoxLayout->setAlignment(Qt::AlignTop);
    NodeInputBox->setAlignment(Qt::AlignTop);
    NodeInputBoxLayout->setAlignment(Qt::AlignTop);
    ParametersBoxLayout->setAlignment(Qt::AlignTop);
    ParametersDetailGridLayout->setAlignment(Qt::AlignTop);

    ParametersBoxLayout->addWidget(ParametersSelectedAssetLabel, 0, 0);
    ParametersBoxLayout->addWidget(ParametersSelectedAssetName, 0, 1);
    ParametersBoxLayout->addWidget(ParametersSelectionLockedButton, 1, 0);

    ParametersBox->setLayout(ParametersBoxLayout);

    MainBoxLayout->addWidget(ParametersBox);

    NodeOptionsBoxLayout->addWidget(NodeOptions_AutoRecook, 0, 0);
    NodeOptionsBoxLayout->addWidget(NodeOptions_RealtimeRecook, 1, 0);
    // NodeOptionsBoxLayout->addWidget(NodeOptions_InputUpdate, 2, 0);

    NodeOptionsBox->setLayout(NodeOptionsBoxLayout);

    MainBoxLayout->addWidget(NodeOptionsBox);

    NodeInputBox->setLayout(NodeInputBoxLayout);
    MainBoxLayout->addWidget(NodeInputBox);

    // ParametersDetailBox->setLayout(ParametersDetailBoxLayout);
    ParametersDetailBox->setLayout(ParametersDetailGridLayout);

    MainBoxLayout->addWidget(ParametersDetailBox);

    MainBox->setLayout(MainBoxLayout);
    MainLayout->addWidget(MainBox);

    this->setLayout(MainLayout);

    CurrentNode = nullptr;
    SelectionLocked = false;

    NodeOptionsBox->setVisible(false);
    NodeInputBox->setVisible(false);
    ParametersDetailBox->setVisible(false);

    QObject::connect(ParametersSelectionLockedButton, SIGNAL(clicked()), this,
                     SLOT(Slot_LockSelectionButton_Clicked()));
    QObject::connect(NodeOptions_AutoRecook, SIGNAL(stateChanged(int)), this,
                     SLOT(Slot_NodeOptions_AutoRecook_StateChanged(int)));
    QObject::connect(NodeOptions_RealtimeRecook, SIGNAL(stateChanged(int)),
                     this,
                     SLOT(Slot_NodeOptions_RealtimeRecook_StateChanged(int)));
    QObject::connect(NodeOptions_InputUpdate, SIGNAL(stateChanged(int)), this,
                     SLOT(Slot_NodeOptions_InputUpdate_StateChanged(int)));
}

HEMAX_ParameterWidget::~HEMAX_ParameterWidget()
{
    for (int i = 0; i < ParametersDetailRows.size(); i++)
    {
        delete ParametersDetailRows[i];
    }

    ParametersDetailRows.clear();

    for (QWidget *Widget : ParameterWidgets)
    {
        if (Widget)
        {
            delete Widget;
        }
    }

    for (QWidget *Widget : SubnetworkInputs)
    {
        if (Widget)
        {
            delete Widget;
        }
    }

    delete ParametersDetailGridLayout;
    delete ParametersDetailBox;

    delete NodeOptions_InputUpdate;
    delete NodeOptions_RealtimeRecook;
    delete NodeOptions_AutoRecook;
    delete NodeOptionsBoxLayout;
    delete NodeOptionsBox;

    delete ParametersSelectionLockedButton;
    delete ParametersSelectedAssetName;
    delete ParametersSelectedAssetLabel;
    delete ParametersBoxLayout;
    delete ParametersBox;

    delete NodeInputBoxLayout;
    delete NodeInputBox;

    delete MainBoxLayout;
    delete MainBox;

    delete MainLayout;
}

void
HEMAX_ParameterWidget::SelectHDA(HEMAX_Node *TheSelectedNode)
{
    if (!SelectionLocked)
    {
        if (!CurrentNode || !TheSelectedNode ||
            (CurrentNode != TheSelectedNode))
        {
            CurrentNode = TheSelectedNode;
            LastActiveFolderTabs.clear();
            UpdateParameterUI(false);
        }
    }
}

void
HEMAX_ParameterWidget::RefreshUI(bool DeleteLater)
{
    UpdateParameterUI(DeleteLater);
}

HEMAX_Node *
HEMAX_ParameterWidget::GetCurrentSelectedNode()
{
    return CurrentNode;
}

void
HEMAX_ParameterWidget::DisableSubnetworkInputUI(int Subnetwork)
{
    if (SubnetworkInputs.size() >= Subnetwork)
    {
        SubnetworkInputs[Subnetwork]->setDisabled(true);
    }
}

void
HEMAX_ParameterWidget::UpdateParameterUI(bool ScheduleDeleteLater)
{
    ParametersSelectedAssetName->setText(NoHDAText);
    NodeOptionsBox->setVisible(false);
    NodeInputBox->setVisible(false);
    ParametersDetailBox->setVisible(false);

    CurrentRow = 0;

    if (ParameterWidgets.size() > 0)
    {
        for (int i = 0; i < ParametersDetailRows.size(); i++)
        {
            ParametersDetailGridLayout->removeItem(ParametersDetailRows[i]);
            delete ParametersDetailRows[i];
            ParametersDetailRows.clear();
        }

        for (QWidget *Widget : ParameterWidgets)
        {
            if (ScheduleDeleteLater)
            {
                ((HEMAX_ParameterWidget_Parameter *)Widget)->MarkInvalid();
                Widget->deleteLater();
            }
            else
            {
                delete Widget;
            }
        }

        ParameterWidgets.clear();
    }

    if (SubnetworkInputs.size() > 0)
    {
        for (QWidget *Widget : SubnetworkInputs)
        {
            NodeInputBoxLayout->removeWidget(Widget);

            if (ScheduleDeleteLater)
            {
                ((HEMAX_ParameterWidget_Parameter *)Widget)->MarkInvalid();
                Widget->deleteLater();
            }
            else
            {
                delete Widget;
            }
        }
        SubnetworkInputs.clear();
    }

    if (CurrentNode)
    {
        if (CurrentNode->Type == HEMAX_NODE_SOP)
        {
            for (int s = 0; s < CurrentNode->Info.inputCount; ++s)
            {
                std::string WidgetLabel = "Subnetwork Input #" + std::to_string(s);

                SubnetworkInputs.push_back(new HEMAX_ParameterWidget_Node(-1, WidgetLabel, ""));

                NodeInputBoxLayout->addWidget(SubnetworkInputs[s], s, 0);

                QObject::connect(((HEMAX_ParameterWidget_Node *)SubnetworkInputs[s])
                    ->SelectButton,
                    SIGNAL(clicked()), this,
                    SLOT(Slot_PWNODE_SubnetworkInputSelection()));
                QObject::connect(((HEMAX_ParameterWidget_Node *)SubnetworkInputs[s])
                    ->ClearButton,
                    SIGNAL(clicked()), this,
                    SLOT(Slot_PWNODE_SubnetworkClearSelection()));
            }
        }

        ParametersSelectedAssetName->setText(CurrentNode->AssetName.c_str());

        std::unordered_map<int, HEMAX_ParameterWidget_Folder *> ConstructionMap;
        std::unordered_map<int, HEMAX_ParameterWidget_MultiParameter *>
            MultiParamConstructionMap;

        bool HorizontalJoinNextParam = false;

        std::vector<HEMAX_Parameter> CurrentNodeParameters =
            GetAllParameters(*CurrentNode);

        for (auto ParametersIter = CurrentNodeParameters.begin();
             ParametersIter != CurrentNodeParameters.end(); ++ParametersIter)
        {
            HEMAX_Parameter *CurrentParameter = &(*ParametersIter);
            HEMAX_ParameterInfo ParmInfo = CurrentParameter->Info;
            HEMAX_ParameterType CurrentParameterType = CurrentParameter->Type;

            if (CurrentParameterType != HEMAX_PARAM_FOLDER &&
                !CurrentParameter->Info.invisible)
            {
                std::string CurrentParameterName =
                    GetParameterLabel(*CurrentParameter);
                int CurrentParameterSize = CurrentParameter->Info.size;

                HEMAX_ParameterWidget_Parameter *WidgetToAdd;

                bool MultiParameterChildFlag =
                    CurrentParameter->Info.isChildOfMultiParm;

                switch (CurrentParameterType)
                {
                case (HEMAX_PARAM_FOLDERLIST):
                {
                    WidgetToAdd = new HEMAX_ParameterWidget_Folderlist(
                        CurrentParameter->Info.id);

                    int ActiveTabIndex = -1;
                    int HiddenCount = 0;
                    bool ActiveTabFound = false;

                    auto ChildIter = CurrentNodeParameters.end();

                    for (int cIndex = 0; cIndex < CurrentNodeParameters.size();
                         cIndex++)
                    {
                        if (CurrentNodeParameters[cIndex].Info.id ==
                            CurrentParameter->Info.id)
                        {
                            ChildIter = CurrentNodeParameters.begin() +
                                        CurrentParameter->Info.id + 1;
                            break;
                        }
                    }

                    for (int fInd = 1; fInd <= CurrentParameterSize; ++fInd)
                    {
                        HEMAX_Parameter *Folder = &(*ChildIter);

                        if (!Folder->Info.invisible)
                        {
                            std::string FolderName = GetParameterLabel(*Folder);

                            HEMAX_ParameterWidget_Folder *FolderToAdd =
                                new HEMAX_ParameterWidget_Folder(
                                    CurrentParameter->Info.id, FolderName);

                            ((HEMAX_ParameterWidget_Folderlist *)WidgetToAdd)
                                ->AppendFolder(FolderToAdd);

                            ConstructionMap.insert(
                                {Folder->Info.id, FolderToAdd});

                            auto Iterator = std::find(
                                LastActiveFolderTabs.begin(),
                                LastActiveFolderTabs.end(), FolderName);

                            if (Iterator != LastActiveFolderTabs.end())
                            {
                                ActiveTabIndex = fInd - 1;
                                ActiveTabFound = true;
                            }

                            FolderToAdd = nullptr;
                        }
                        else
                        {
                            if (!ActiveTabFound)
                            {
                                ++HiddenCount;
                            }
                        }

                        ++ChildIter;
                    }

                    ((HEMAX_ParameterWidget_Folderlist *)WidgetToAdd)
                        ->BuildDirectory();

                    if (ActiveTabIndex > -1)
                    {
                        ((HEMAX_ParameterWidget_Folderlist *)WidgetToAdd)
                            ->FolderContainer->setCurrentIndex(ActiveTabIndex -
                                                               HiddenCount);
                        ((HEMAX_ParameterWidget_Folderlist *)WidgetToAdd)
                            ->CurrentlyActiveIndex =
                            ActiveTabIndex - HiddenCount;
                    }

                    QObject::connect(
                        ((HEMAX_ParameterWidget_Folderlist *)WidgetToAdd)
                            ->FolderContainer,
                        SIGNAL(currentChanged(int)), this,
                        SLOT(Slot_PWFOLDERLIST_FolderChanged(int)));

                    break;
                }
                case (HEMAX_PARAM_MULTIPARMLIST):
                {
                    int MultiParamCount = CurrentParameter->Info.instanceCount;
                    WidgetToAdd = new HEMAX_ParameterWidget_MultiParameter(
                        CurrentParameter->Info.id, CurrentParameterName,
                        MultiParamCount);

                    MultiParamConstructionMap.insert(
                        {CurrentParameter->Info.id,
                         (HEMAX_ParameterWidget_MultiParameter *)WidgetToAdd});

                    QObject::connect(
                        WidgetToAdd,
                        SIGNAL(AddMultiParameterInstance(int, bool)), this,
                        SLOT(Slot_PWMULTIPARAMETER_Add(int, bool)));
                    QObject::connect(
                        WidgetToAdd,
                        SIGNAL(RemoveMultiParameterInstance(int, bool)), this,
                        SLOT(Slot_PWMULTIPARAMETER_Remove(int, bool)));

                    break;
                }
                case (HEMAX_PARAM_LABEL):
                {
                    WidgetToAdd = new HEMAX_ParameterWidget_Label(
                        CurrentParameter->Info.id,
                        GetParameterLabel(*CurrentParameter));
                    break;
                }
                case (HEMAX_PARAM_INTEGER):
                {
                    if (IsParameterChoiceList(*CurrentParameter))
                    {
                        std::vector<HEMAX_ParmChoice> ParmChoices =
                            GetIntParameterChoiceLists(*CurrentParameter);

                        std::vector<std::string> ChoiceLabels;

                        for (int x = 0; x < ParmChoices.size(); ++x)
                        {
                            ChoiceLabels.push_back(ParmChoices[x].ChoiceLabel);
                        }

                        std::vector<int> ChoiceIndex =
                            GetParameterIntValues(*CurrentParameter);

                        WidgetToAdd = new HEMAX_ParameterWidget_Integer_Choice(
                            CurrentParameter->Info.id, CurrentParameterName,
                            CurrentParameter->Info.choiceCount, ChoiceLabels,
                            ChoiceIndex[0]);

                        QObject::connect(
                            ((HEMAX_ParameterWidget_Integer_Choice *)
                                 WidgetToAdd)
                                ->IntegerChoiceValues,
                            SIGNAL(currentIndexChanged(int)), this,
                            SLOT(Slot_PWINTEGERCHOICE_Selection(int)));
                    }
                    else
                    {
                        std::vector<int> ParamIntValues =
                            GetParameterIntValues(*CurrentParameter);
                        if (CurrentParameterSize == 1 &&
                            CurrentParameter->Info.hasUIMax &&
                            CurrentParameter->Info.hasUIMin)
                        {
                            WidgetToAdd = new HEMAX_ParameterWidget_Integer(
                                CurrentParameter->Info.id, CurrentParameterName,
                                ParamIntValues, CurrentParameterSize,
                                CurrentParameter->Info.UIMin,
                                CurrentParameter->Info.UIMax);
                        }
                        else
                        {
                            WidgetToAdd = new HEMAX_ParameterWidget_Integer(
                                CurrentParameter->Info.id, CurrentParameterName,
                                ParamIntValues, CurrentParameterSize);
                        }

                        for (int z = 0; z < CurrentParameterSize; ++z)
                        {
                            if (CurrentParameterSize == 1)
                            {
                                QObject::connect(
                                    ((HEMAX_ParameterWidget_Integer *)
                                         WidgetToAdd),
                                    SIGNAL(Signal_Integer_SliderDone()), this,
                                    SLOT(Slot_PWINTEGER_SliderDone()));
                                QObject::connect(
                                    ((HEMAX_ParameterWidget_Integer *)
                                         WidgetToAdd),
                                    SIGNAL(Signal_Integer_SliderDrag()), this,
                                    SLOT(Slot_PWINTEGER_SliderDrag()));
                            }
                            QObject::connect(
                                ((HEMAX_ParameterWidget_Integer *)WidgetToAdd)
                                    ->IntegerValues[z],
                                SIGNAL(editingFinished()), this,
                                SLOT(Slot_PWINTEGER_EditingFinished()));
                        }
                    }
                    break;
                }
                case (HEMAX_PARAM_STRING):
                {
                    if (IsParameterChoiceList(*CurrentParameter))
                    {
                        std::vector<HEMAX_ParmChoice> ParmChoices =
                            GetStringParameterChoiceLists(*CurrentParameter);

                        std::vector<std::string> ChoiceLabels;

                        for (int x = 0; x < ParmChoices.size(); ++x)
                        {
                            ChoiceLabels.push_back(ParmChoices[x].ChoiceLabel);
                        }

                        std::vector<std::string> CurrentChoice =
                            GetParameterStringValues(*CurrentParameter, true);
                        int CurrentIndex = 0;

                        for (int c = 0; c < ParmChoices.size(); ++c)
                        {
                            if (CurrentChoice[0] == ParmChoices[c].ChoiceValue)
                            {
                                CurrentIndex = c;
                                break;
                            }
                        }

                        WidgetToAdd = new HEMAX_ParameterWidget_String_Choice(
                            CurrentParameter->Info.id, CurrentParameterName,
                            ChoiceLabels, CurrentIndex);

                        QObject::connect(
                            ((HEMAX_ParameterWidget_String_Choice *)WidgetToAdd)
                                ->StringChoiceValues,
                            SIGNAL(currentIndexChanged(int)), this,
                            SLOT(Slot_PWSTRINGCHOICE_Selection(int)));
                    }
                    else
                    {
                        std::vector<std::string> ParamStringValues =
                            GetParameterStringValues(*CurrentParameter, true);
                        WidgetToAdd = new HEMAX_ParameterWidget_String(
                            CurrentParameter->Info.id, CurrentParameterName,
                            ParamStringValues, CurrentParameterSize);

                        for (int z = 0; z < CurrentParameterSize; ++z)
                        {
                            QObject::connect(
                                ((HEMAX_ParameterWidget_String *)WidgetToAdd)
                                    ->StringValues[z],
                                SIGNAL(editingFinished()), this,
                                SLOT(Slot_PWSTRING_EditingFinished()));
                        }
                    }

                    break;
                }
                case (HEMAX_PARAM_FLOAT):
                {
                    std::vector<float> ParamFloatValues =
                        GetParameterFloatValues(*CurrentParameter);

                    if (CurrentParameterSize == 1 &&
                        CurrentParameter->Info.hasUIMax &&
                        CurrentParameter->Info.hasUIMin)
                    {
                        WidgetToAdd = new HEMAX_ParameterWidget_Float(
                            CurrentParameter->Info.id, CurrentParameterName,
                            ParamFloatValues, CurrentParameterSize,
                            CurrentParameter->Info.UIMin,
                            CurrentParameter->Info.UIMax);
                    }
                    else
                    {
                        WidgetToAdd = new HEMAX_ParameterWidget_Float(
                            CurrentParameter->Info.id, CurrentParameterName,
                            ParamFloatValues, CurrentParameterSize);
                    }

                    for (int z = 0; z < CurrentParameterSize; ++z)
                    {
                        if (CurrentParameterSize == 1)
                        {
                            QObject::connect(
                                ((HEMAX_ParameterWidget_Float *)WidgetToAdd),
                                SIGNAL(Signal_Float_SliderDone()), this,
                                SLOT(Slot_PWFLOAT_SliderDone()));
                            QObject::connect(
                                ((HEMAX_ParameterWidget_Float *)WidgetToAdd),
                                SIGNAL(Signal_Float_SliderDrag()), this,
                                SLOT(Slot_PWFLOAT_SliderDrag()));
                        }
                        QObject::connect(
                            ((HEMAX_ParameterWidget_Float *)WidgetToAdd)
                                ->FloatValues[z],
                            SIGNAL(editingFinished()), this,
                            SLOT(Slot_PWFLOAT_EditingFinished()));
                    }
                    break;
                }
                case (HEMAX_PARAM_TOGGLE):
                {
                    std::vector<int> ParamIntValues =
                        GetParameterIntValues(*CurrentParameter);
                    WidgetToAdd = new HEMAX_ParameterWidget_Toggle(
                        CurrentParameter->Info.id, CurrentParameterName,
                        ParamIntValues[0]);

                    QObject::connect(
                        ((HEMAX_ParameterWidget_Toggle *)WidgetToAdd)
                            ->ToggleValue,
                        SIGNAL(stateChanged(int)), this,
                        SLOT(Slot_PWTOGGLE_StateChanged()));

                    break;
                }
                case (HEMAX_PARAM_BUTTON):
                {
                    if (IsParameterChoiceList(*CurrentParameter))
                    {
                        std::vector<HEMAX_ParmChoice> ParmChoices =
                            GetStringParameterChoiceLists(*CurrentParameter);
                        WidgetToAdd = new HEMAX_ParameterWidget_Button_Choice(
                            CurrentParameter->Info.id, CurrentParameter,
                            CurrentParameterName, ParmChoices);

                        QObject::connect(
                            ((HEMAX_ParameterWidget_Button_Choice *)WidgetToAdd)
                                ->ChoiceValues,
                            SIGNAL(currentIndexChanged(int)), this,
                            SLOT(Slot_PWBUTTONCHOICE_Selection(int)));
                    }
                    else
                    {
                        WidgetToAdd = new HEMAX_ParameterWidget_Button(
                            CurrentParameter->Info.id, CurrentParameterName);

                        QObject::connect(
                            ((HEMAX_ParameterWidget_Button *)WidgetToAdd)
                                ->Button,
                            SIGNAL(clicked()), this,
                            SLOT(Slot_PWBUTTON_Clicked()));
                    }

                    break;
                }
                case (HEMAX_PARAM_PATH_FILE):
                case (HEMAX_PARAM_PATH_FILE_DIR):
                case (HEMAX_PARAM_PATH_FILE_GEO):
                case (HEMAX_PARAM_PATH_FILE_IMAGE):
                {
                    std::vector<std::string> ParamStringValues =
                        GetParameterStringValues(*CurrentParameter, true);
                    WidgetToAdd = new HEMAX_ParameterWidget_FilePath(
                        CurrentParameter->Info.id, CurrentParameterName,
                        ParamStringValues[0], CurrentParameterType);

                    QObject::connect(
                        ((HEMAX_ParameterWidget_FilePath *)WidgetToAdd)
                            ->PathEdit,
                        SIGNAL(editingFinished()), this,
                        SLOT(Slot_PWFILEPATH_Updated()));
                    QObject::connect(
                        ((HEMAX_ParameterWidget_FilePath *)WidgetToAdd),
                        SIGNAL(Signal_FilePath_Selected()), this,
                        SLOT(Slot_PWFILEPATH_Selected()));

                    break;
                }
                case (HEMAX_PARAM_NODE):
                {
                    std::string InputNodeName =
                        GetParameterInputNodeName(*CurrentParameter);

                    WidgetToAdd = new HEMAX_ParameterWidget_Node(
                        CurrentParameter->Info.id, CurrentParameterName,
                        InputNodeName);
                    QObject::connect(((HEMAX_ParameterWidget_Node *)WidgetToAdd)
                                         ->SelectButton,
                                     SIGNAL(clicked()), this,
                                     SLOT(Slot_PWNODE_InputSelection()));
                    QObject::connect(((HEMAX_ParameterWidget_Node *)WidgetToAdd)
                                         ->ClearButton,
                                     SIGNAL(clicked()), this,
                                     SLOT(Slot_PWNODE_ClearSelection()));
                    break;
                }
                case (HEMAX_PARAM_COLOR):
                {
                    WidgetToAdd = CreateParameterWidget_Color(*CurrentParameter, CurrentParameterName);
                } break;
                default:
                {
                    WidgetToAdd = new HEMAX_ParameterWidget_Parameter(-1);
                    break;
                }
                }

                if (CurrentParameter->Info.disabled)
                {
                    WidgetToAdd->setDisabled(true);
                }

                if (CurrentParameter->Info.invisible)
                {
                    WidgetToAdd->setVisible(false);
                }

                if (IsRootLevelParameter(*CurrentParameter))
                {
                    if (HorizontalJoinNextParam)
                    {
                        AppendWidgetToPreviousRow(WidgetToAdd);
                    }
                    else
                    {
                        AddWidgetToNewRow(WidgetToAdd);
                    }
                }
                else if (MultiParameterChildFlag)
                {
                    int ParentId = CurrentParameter->Info.parentId;
                    auto Search = MultiParamConstructionMap.find(ParentId);

                    if (Search != MultiParamConstructionMap.end())
                    {
                        HEMAX_ParameterWidget_MultiParameter *MultiParamParent =
                            (HEMAX_ParameterWidget_MultiParameter *)
                                Search->second;
                        MultiParamParent->AddParameterToInstance(
                            WidgetToAdd, GetMultiParameterInstancePosition(
                                             *CurrentParameter));
                    }
                    else
                    {
                        // If for some reason we can't find the multi-parameter
                        // container, it's possible that it belongs to a folder
                        // within the multi-parameter
                        auto Search = ConstructionMap.find(ParentId);

                        if (Search != ConstructionMap.end())
                        {
                            HEMAX_ParameterWidget_Folder *MultiFolder =
                                (HEMAX_ParameterWidget_Folder *)Search->second;
                            MultiFolder->AddWidget(WidgetToAdd);
                        }
                    }
                }
                else
                {
                    int ParentId = CurrentParameter->Info.parentId;
                    auto Search = ConstructionMap.find(ParentId);

                    HEMAX_ParameterWidget_Folder *TheFolder =
                        (HEMAX_ParameterWidget_Folder *)Search->second;

                    if (Search != ConstructionMap.end())
                    {
                        if (HorizontalJoinNextParam)
                        {
                            TheFolder->AddWidgetToPreviousRow(WidgetToAdd);
                        }
                        else
                        {
                            TheFolder->AddWidget(WidgetToAdd);
                        }
                    }
                }
                std::string HelpString = GetHelpString(*CurrentParameter);

                if (!HelpString.empty())
                {
                    WidgetToAdd->SetHelpToolTip(HelpString);
                }
            }
            HorizontalJoinNextParam = CurrentParameter->Info.joinNext;
        }

        NodeOptionsBox->setVisible(true);
        if (CurrentNode->Type == HEMAX_NODE_SOP && CurrentNode->Info.inputCount > 0)
        {
            NodeInputBox->setVisible(true);
        }
        ParametersDetailBox->setVisible(true);
        NodeOptions_AutoRecook->setChecked(
            CurrentNode->AutoRecookOnParameterUpdate);
        NodeOptions_RealtimeRecook->setChecked(
            CurrentNode->RealtimeRecookEnabled);
        NodeOptions_InputUpdate->setChecked(
            CurrentNode->AutoRecookOnInputChange);

        if (!CurrentNode->AutoRecookOnParameterUpdate)
        {
            NodeOptions_RealtimeRecook->setEnabled(false);
        }
    }

    this->update();
}

void
HEMAX_ParameterWidget::AddWidgetToNewRow(QWidget *TheWidget)
{
    ParameterWidgets.push_back(TheWidget);

    QHBoxLayout *NewRow = new QHBoxLayout;
    ParametersDetailRows.push_back(NewRow);

    ParametersDetailGridLayout->addLayout(NewRow, CurrentRow, 0);
    NewRow->addWidget(TheWidget);

    CurrentRow++;
}

void
HEMAX_ParameterWidget::AppendWidgetToPreviousRow(QWidget *TheWidget)
{
    if (ParametersDetailRows.size() > 0)
    {
        ParameterWidgets.push_back(TheWidget);
        ParametersDetailRows[CurrentRow - 1]->addWidget(TheWidget);
    }
    else
    {
        AddWidgetToNewRow(TheWidget);
    }
}

HEMAX_ParameterWidget_Parameter*
HEMAX_ParameterWidget::CreateParameterWidget_Color(HEMAX_Parameter& Parameter, std::string Label)
{
    // No Alpha
    if (Parameter.Info.size == 3)
    {
        std::vector<float> Vals = GetParameterFloatValues(Parameter);
        HEMAX_ParameterWidget_Color* ColorWidget =  new HEMAX_ParameterWidget_Color(Parameter.Info.id, Label, Vals, 3);
        QObject::connect(ColorWidget, SIGNAL(Signal_ColorChosen()), this, SLOT(Slot_PWCOLOR_Update()));
        return ColorWidget;
    }
    // Alpha
    else if (Parameter.Info.size == 4)
    {
        std::vector<float> Vals = GetParameterFloatValues(Parameter);
        HEMAX_ParameterWidget_Color* ColorWidget = new HEMAX_ParameterWidget_Color(Parameter.Info.id, Label, Vals, 4);
        QObject::connect(ColorWidget, SIGNAL(Signal_ColorChosen()), this, SLOT(Slot_PWCOLOR_Update()));
        return ColorWidget;
    }
    // Invalid
    else
    {
        return new HEMAX_ParameterWidget_Parameter(-1);
    }
}

void
HEMAX_ParameterWidget::Slot_LockSelectionButton_Clicked()
{
    if (SelectionLocked)
    {
        SelectionLocked = false;
        ParametersSelectionLockedButton->setText("Lock to Current Selection");
        SelectHDA(nullptr);
    }
    else if (CurrentNode && !SelectionLocked)
    {
        SelectionLocked = true;
        ParametersSelectionLockedButton->setText("Unlock");
    }
}

void
HEMAX_ParameterWidget::Slot_NodeOptions_AutoRecook_StateChanged(int State)
{
    if (CurrentNode)
    {
        CurrentNode->AutoRecookOnParameterUpdate = State;
    }

    if (State)
    {
        NodeOptions_RealtimeRecook->setEnabled(true);
    }
    else
    {
        NodeOptions_RealtimeRecook->setEnabled(false);
    }
}

void
HEMAX_ParameterWidget::Slot_NodeOptions_RealtimeRecook_StateChanged(int State)
{
    if (CurrentNode)
    {
        CurrentNode->RealtimeRecookEnabled = State;
    }
}

void
HEMAX_ParameterWidget::Slot_NodeOptions_InputUpdate_StateChanged(int State)
{
    if (CurrentNode)
    {
        CurrentNode->AutoRecookOnInputChange = State;
    }
}

void
HEMAX_ParameterWidget::Slot_PWINTEGER_EditingFinished()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Integer *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Integer *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<int> ActualIntegerValues(
            HEMAX_Sender->IntegerValues.size());

        for (int i = 0; i < HEMAX_Sender->IntegerValues.size(); ++i)
        {
            ActualIntegerValues[i] =
                HEMAX_Sender->IntegerValues[i]->text().toInt();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_UpdateParameterIntValues(CurrentNode, Parameter,
                                             ActualIntegerValues, false);
    }
}

void
HEMAX_ParameterWidget::Slot_PWINTEGER_SliderDone()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Integer *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Integer *>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<int> ActualIntegerValues(
            HEMAX_Sender->IntegerValues.size());

        for (int i = 0; i < HEMAX_Sender->IntegerValues.size(); ++i)
        {
            ActualIntegerValues[i] =
                HEMAX_Sender->IntegerValues[i]->text().toInt();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_UpdateParameterIntValues(CurrentNode, Parameter,
                                             ActualIntegerValues, false);
    }
}

void
HEMAX_ParameterWidget::Slot_PWINTEGER_SliderDrag()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Integer *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Integer *>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate &&
            CurrentNode->RealtimeRecookEnabled)
        {
            std::vector<int> ActualIntegerValues(
                HEMAX_Sender->IntegerValues.size());

            for (int i = 0; i < HEMAX_Sender->IntegerValues.size(); ++i)
            {
                ActualIntegerValues[i] =
                    HEMAX_Sender->IntegerValues[i]->text().toInt();
            }

            HEMAX_Parameter Parameter =
                GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

            emit Signal_UpdateParameterIntValues(CurrentNode, Parameter,
                                                 ActualIntegerValues, true);
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWINTEGERCHOICE_Selection(int Index)
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Integer_Choice *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Integer_Choice *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<int> Choice = {
            HEMAX_Sender->IntegerChoiceValues->currentIndex()};

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_UpdateParameterIntValues(CurrentNode, Parameter, Choice,
                                             false);
    }
}

void
HEMAX_ParameterWidget::Slot_PWSTRING_EditingFinished()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_String *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_String *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::string StringValue =
            HEMAX_Sender->StringValues[0]->text().toStdString();
        std::vector<std::string> StringValues = {StringValue};

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_UpdateParameterStringValues(CurrentNode, Parameter,
                                                StringValues);
    }
}

void
HEMAX_ParameterWidget::Slot_PWSTRINGCHOICE_Selection(int Index)
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_String_Choice *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_String_Choice *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        int Choice = {HEMAX_Sender->StringChoiceValues->currentIndex()};

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);
        std::vector<HEMAX_ParmChoice> Choices =
            GetStringParameterChoiceLists(Parameter);

        std::vector<std::string> SelectedChoice = {Choices[Choice].ChoiceValue};

        emit Signal_UpdateParameterStringValues(CurrentNode, Parameter,
                                                SelectedChoice);
    }
}

void
HEMAX_ParameterWidget::Slot_PWFLOAT_EditingFinished()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Float *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Float *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<float> ActualFloatValues(HEMAX_Sender->FloatValues.size());

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        for (int i = 0; i < HEMAX_Sender->FloatValues.size(); ++i)
        {
            ActualFloatValues[i] =
                HEMAX_Sender->FloatValues[i]->text().toFloat();
        }

        emit Signal_UpdateParameterFloatValues(CurrentNode, Parameter,
                                               ActualFloatValues, false);

        if (HEMAX_Sender->FloatValues.size() == 1)
        {
            HEMAX_Sender->UpdateSliderPosition();
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWFLOAT_SliderDone()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Float *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Float *>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<float> ActualFloatValues(HEMAX_Sender->FloatValues.size());

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        for (int i = 0; i < HEMAX_Sender->FloatValues.size(); ++i)
        {
            ActualFloatValues[i] =
                HEMAX_Sender->FloatValues[i]->text().toFloat();

            if (Parameter.Info.hasUIMax)
            {
                ActualFloatValues[i] =
                    (ActualFloatValues[i] > Parameter.Info.UIMax) ?
                        Parameter.Info.UIMax :
                        ActualFloatValues[i];
            }

            if (Parameter.Info.hasUIMin)
            {
                ActualFloatValues[i] =
                    (ActualFloatValues[i] < Parameter.Info.UIMin) ?
                        Parameter.Info.UIMin :
                        ActualFloatValues[i];
            }
        }

        emit Signal_UpdateParameterFloatValues(CurrentNode, Parameter,
                                               ActualFloatValues, false);

        if (HEMAX_Sender->FloatValues.size() == 1)
        {
            HEMAX_Sender->UpdateSliderPosition();
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWFLOAT_SliderDrag()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Float *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Float *>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate &&
            CurrentNode->RealtimeRecookEnabled)
        {
            std::vector<float> ActualFloatValues(
                HEMAX_Sender->FloatValues.size());

            HEMAX_Parameter Parameter =
                GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

            for (int i = 0; i < HEMAX_Sender->FloatValues.size(); ++i)
            {
                ActualFloatValues[i] =
                    HEMAX_Sender->FloatValues[i]->text().toFloat();

                if (Parameter.Info.hasUIMax)
                {
                    ActualFloatValues[i] =
                        (ActualFloatValues[i] > Parameter.Info.UIMax) ?
                            Parameter.Info.UIMax :
                            ActualFloatValues[i];
                }

                if (Parameter.Info.hasUIMin)
                {
                    ActualFloatValues[i] =
                        (ActualFloatValues[i] < Parameter.Info.UIMin) ?
                            Parameter.Info.UIMin :
                            ActualFloatValues[i];
                }
            }

            emit Signal_UpdateParameterFloatValues(CurrentNode, Parameter,
                                                   ActualFloatValues, true);
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWTOGGLE_StateChanged()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Toggle *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Toggle *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<int> IntValue;

        if (HEMAX_Sender->ToggleValue->isChecked())
        {
            IntValue = {1};
        }
        else
        {
            IntValue = {0};
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_UpdateParameterIntValues(CurrentNode, Parameter, IntValue,
                                             false);
    }
}

void
HEMAX_ParameterWidget::Slot_PWCOLOR_Update()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Color *HEMAX_Sender = qobject_cast<HEMAX_ParameterWidget_Color*>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<float> FloatValues;

        bool HasAlpha = HEMAX_Sender->ColorDialog->testOption(QColorDialog::ShowAlphaChannel);

        QColor ChosenColor = HEMAX_Sender->ColorDialog->currentColor();
        FloatValues.push_back(ChosenColor.red() / 255.0);
        FloatValues.push_back(ChosenColor.green() / 255.0);
        FloatValues.push_back(ChosenColor.blue() / 255.0);

        if (HasAlpha)
        {
            FloatValues.push_back(ChosenColor.alpha() / 255.0);
        }

        HEMAX_Parameter Parameter = GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_UpdateParameterFloatValues(CurrentNode, Parameter, FloatValues, false);
    }
}

void
HEMAX_ParameterWidget::Slot_PWBUTTONCHOICE_Selection(int Index)
{
    if (Index != 0)
    {
        QObject *Sender = QObject::sender();

        HEMAX_ParameterWidget_Button_Choice *HEMAX_Sender =
            qobject_cast<HEMAX_ParameterWidget_Button_Choice *>(
                Sender->parent());

        if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
        {
            if (CurrentNode->AutoRecookOnParameterUpdate)
            {
                HEMAX_Sender->MarkInvalid();
            }

            std::vector<int> Choice = {
                HEMAX_Sender->ChoiceValues->currentIndex() - 1};

            HEMAX_Sender->ChoiceValues->setCurrentIndex(0);

            emit Signal_UpdateParameterIntValues(
                CurrentNode, *HEMAX_Sender->Parameter, Choice, false);
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWBUTTON_Clicked()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Button *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Button *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        std::vector<int> PressButton = {1};

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_UpdateParameterIntValues(CurrentNode, Parameter,
                                             PressButton, false);
    }
}

void
HEMAX_ParameterWidget::Slot_PWFILEPATH_Updated()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_FilePath *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_FilePath *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        std::string Path = HEMAX_Sender->PathEdit->text().toStdString();
        std::vector<std::string> StringValues = {Path};

        emit Signal_UpdateParameterStringValues(CurrentNode, Parameter,
                                                StringValues);
    }
}

void
HEMAX_ParameterWidget::Slot_PWFILEPATH_Selected()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_FilePath *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_FilePath *>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        std::string Path = HEMAX_Sender->PathEdit->text().toStdString();
        std::vector<std::string> StringValues = {Path};

        emit Signal_UpdateParameterStringValues(CurrentNode, Parameter,
                                                StringValues);
    }
}

void
HEMAX_ParameterWidget::Slot_PWNODE_InputSelection()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Node *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Node *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_InputSelection(CurrentNode, Parameter, false);
    }
}

void
HEMAX_ParameterWidget::Slot_PWNODE_ClearSelection()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Node *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Node *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        emit Signal_InputSelection(CurrentNode, Parameter, true);
    }
}

void
HEMAX_ParameterWidget::Slot_PWNODE_SubnetworkInputSelection()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Node *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Node *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        for (int i = 0; i < SubnetworkInputs.size(); i++)
        {
            if (HEMAX_Sender == SubnetworkInputs[i])
            {
                emit Signal_SubnetworkInputSelection(CurrentNode, i, false);
            }
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWNODE_SubnetworkClearSelection()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_Node *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_Node *>(Sender->parent());

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (CurrentNode->AutoRecookOnParameterUpdate)
        {
            HEMAX_Sender->MarkInvalid();
        }

        for (int i = 0; i < SubnetworkInputs.size(); i++)
        {
            if (HEMAX_Sender == SubnetworkInputs[i])
            {
                emit Signal_SubnetworkInputSelection(CurrentNode, i, true);
            }
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWMULTIPARAMETER_Add(int Position, bool DoNotCook)
{
    static int AddCount = 0;

    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_MultiParameter *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_MultiParameter *>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (!DoNotCook)
        {
            HEMAX_Sender->MarkInvalid();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);
        int ParameterId = Parameter.Info.id;

        InsertMultiParameterInstance(Parameter, Position);
        ++AddCount;

        if (!DoNotCook)
        {
            emit Signal_NodeRequiresRecook(CurrentNode, false);
            Parameter = GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

            HEMAX_MultiParameterChangeInfo ChangeInfo;
            ChangeInfo.Added = true;
            ChangeInfo.Position = Position;
            ChangeInfo.Count = AddCount;

            emit Signal_UpdateMultiParameterList(CurrentNode, Parameter,
                                                 ChangeInfo);

            AddCount = 0;
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWMULTIPARAMETER_Remove(int Position,
                                                    bool DoNotCook)
{
    static int RemoveCount = 0;

    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_MultiParameter *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_MultiParameter *>(Sender);

    if (!HEMAX_Sender->IsUIInvalid() && CurrentNode)
    {
        if (!DoNotCook)
        {
            HEMAX_Sender->MarkInvalid();
        }

        HEMAX_Parameter Parameter =
            GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

        RemoveMultiParameterInstance(Parameter, Position);
        int ParameterId = Parameter.Info.id;

        ++RemoveCount;

        if (!DoNotCook)
        {
            emit Signal_NodeRequiresRecook(CurrentNode, false);
            Parameter = GetParameter(*CurrentNode, HEMAX_Sender->ParameterId);

            HEMAX_MultiParameterChangeInfo ChangeInfo;
            ChangeInfo.Added = false;
            ChangeInfo.Position = Position;
            ChangeInfo.Count = RemoveCount;

            emit Signal_UpdateMultiParameterList(CurrentNode, Parameter,
                                                 ChangeInfo);

            RemoveCount = 0;
        }
    }
}

void
HEMAX_ParameterWidget::Slot_PWFOLDERLIST_FolderChanged(int Index)
{
    QObject *Sender = QObject::sender();

    QTabWidget *HEMAX_Sender = qobject_cast<QTabWidget *>(Sender);

    HEMAX_ParameterWidget_Folderlist *FolderlistSender =
        qobject_cast<HEMAX_ParameterWidget_Folderlist *>(
            HEMAX_Sender->parentWidget());

    if (!FolderlistSender->IsUIInvalid() && CurrentNode)
    {
        int OldActiveTab = FolderlistSender->CurrentlyActiveIndex;

        if (OldActiveTab != -1)
        {
            std::string OldActiveTabName =
                HEMAX_Sender->tabText(OldActiveTab).toStdString();

            auto Iterator =
                std::find(LastActiveFolderTabs.begin(),
                          LastActiveFolderTabs.end(), OldActiveTabName);

            if (Iterator != LastActiveFolderTabs.end())
            {
                std::swap(*Iterator, LastActiveFolderTabs.back());
                LastActiveFolderTabs.pop_back();
            }
        }
        if (OldActiveTab != Index)
        {
            LastActiveFolderTabs.push_back(
                HEMAX_Sender->tabText(Index).toStdString());
            FolderlistSender->CurrentlyActiveIndex = Index;
        }
    }
}

HEMAX_ParameterWidget_Parameter::HEMAX_ParameterWidget_Parameter(
    int ParameterId)
    : ParameterId(ParameterId)
{
    InvalidUI = false;
}

void
HEMAX_ParameterWidget_Parameter::MarkInvalid()
{
    InvalidUI = true;
}

void
HEMAX_ParameterWidget_Parameter::SetHelpToolTip(std::string HelpString)
{
    this->setToolTip(HelpString.c_str());
}

bool
HEMAX_ParameterWidget_Parameter::IsUIInvalid()
{
    return InvalidUI;
}

int
HEMAX_ParameterWidget_Parameter::GetParameterId()
{
    return ParameterId;
}

HEMAX_ParameterWidget_Folder::HEMAX_ParameterWidget_Folder(
    int ParameterId, std::string FolderName)
    : HEMAX_ParameterWidget_Parameter(ParameterId), FolderName(FolderName)
{
    MainLayout = new QGridLayout;

    MainLayout->setAlignment(Qt::AlignTop);

    this->setLayout(MainLayout);

    CurrentRow = 0;
    CurrentColumn = 0;
}

std::string
HEMAX_ParameterWidget_Folder::GetFolderName()
{
    return FolderName;
}

void
HEMAX_ParameterWidget_Folder::AddWidget(QWidget *Widget)
{
    ParameterWidgets.push_back(Widget);

    QHBoxLayout *NewRow = new QHBoxLayout;
    Rows.push_back(NewRow);

    MainLayout->addLayout(NewRow, CurrentRow, 0);
    NewRow->addWidget(Widget);

    CurrentRow++;
}

void
HEMAX_ParameterWidget_Folder::AddWidgetToPreviousRow(QWidget *Widget)
{
    if (CurrentRow > 0)
    {
        ParameterWidgets.push_back(Widget);
        Rows[CurrentRow - 1]->addWidget(Widget);
    }
    else
    {
        AddWidget(Widget);
    }
}

void
HEMAX_ParameterWidget_Folder::MarkInvalid()
{
    this->InvalidUI = true;

    for (QWidget *Widget : ParameterWidgets)
    {
        if (Widget)
        {
            ((HEMAX_ParameterWidget_Parameter *)Widget)->MarkInvalid();
        }
    }
}

void
HEMAX_ParameterWidget_Folder::SetHelpToolTip(std::string HelpString)
{
    // Do nothing
}

HEMAX_ParameterWidget_Folder::~HEMAX_ParameterWidget_Folder()
{
    for (int i = 0; i < Rows.size(); i++)
    {
        delete Rows[i];
    }

    for (int i = 0; i < ParameterWidgets.size(); ++i)
    {
        delete ParameterWidgets[i];
    }

    delete MainLayout;
}

HEMAX_ParameterWidget_Folderlist::HEMAX_ParameterWidget_Folderlist(
    int ParameterId)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    FolderContainer = new QTabWidget;
    Layout = new QVBoxLayout;
    Layout->setAlignment(Qt::AlignTop);
    Layout->addWidget(FolderContainer);
    Folders.resize(0);

    CurrentlyActiveIndex = -1;

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Folderlist::AppendFolder(
    HEMAX_ParameterWidget_Folder *Folder)
{
    Folders.push_back(Folder);
}

HEMAX_ParameterWidget_Folder *
HEMAX_ParameterWidget_Folderlist::RetrieveFolder(int FolderIndex)
{
    return Folders[FolderIndex];
}

void
HEMAX_ParameterWidget_Folderlist::BuildDirectory()
{
    for (int i = 0; i < Folders.size(); ++i)
    {
        FolderContainer->addTab(Folders[i],
                                Folders[i]->GetFolderName().c_str());
    }

    if (Folders.size() == 0)
    {
        FolderContainer->hide();
    }
    else
    {
        FolderContainer->show();
    }
}

void
HEMAX_ParameterWidget_Folderlist::MarkInvalid()
{
    this->InvalidUI = true;

    for (HEMAX_ParameterWidget_Folder *Folder : Folders)
    {
        if (Folder)
        {
            Folder->MarkInvalid();
        }
    }
}

void
HEMAX_ParameterWidget_Folderlist::SetHelpToolTip(std::string HelpString)
{
    // Override to do nothing
}

HEMAX_ParameterWidget_Folderlist::~HEMAX_ParameterWidget_Folderlist()
{
    for (int i = 0; i < Folders.size(); ++i)
    {
        delete Folders[i];
    }

    delete FolderContainer;
    delete Layout;
}

HEMAX_ParameterWidget_Label::HEMAX_ParameterWidget_Label(int ParameterId,
                                                         std::string LabelText)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Label = new QLabel(LabelText.c_str());
    Layout->setAlignment(Qt::AlignTop);
    Layout->addWidget(Label, 0, 0);
    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Label::SetHelpToolTip(std::string HelpString)
{
    if (Label)
    {
        Label->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_Label::~HEMAX_ParameterWidget_Label()
{
    delete Label;
    delete Layout;
}

HEMAX_ParameterWidget_Integer::HEMAX_ParameterWidget_Integer(
    int ParameterId, std::string Label, std::vector<int> Values, int ParamSize)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());

    for (int i = 0; i < ParamSize; ++i)
    {
        IntegerValues.push_back(
            new QLineEdit(std::to_string(Values[i]).c_str()));
    }

    ParameterLabel->setAlignment(Qt::AlignCenter);

    Layout->setColumnMinimumWidth(0, 100);

    Layout->addWidget(ParameterLabel, 0, 0);

    for (int i = 0; i < ParamSize; ++i)
    {
        Layout->addWidget(IntegerValues[i], 0, i + 1);
    }

    this->setLayout(Layout);
}

HEMAX_ParameterWidget_Integer::HEMAX_ParameterWidget_Integer(
    int ParameterId, std::string Label, std::vector<int> Values, int ParamSize,
    int UIMin, int UIMax)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());

    for (int i = 0; i < ParamSize; ++i)
    {
        IntegerValues.push_back(
            new QLineEdit(std::to_string(Values[i]).c_str()));
    }

    IntegerSlider = new QSlider(Qt::Horizontal);

    ParameterLabel->setAlignment(Qt::AlignCenter);

    Layout->setColumnMinimumWidth(0, 100);

    Layout->addWidget(ParameterLabel, 0, 0);

    for (int i = 0; i < ParamSize; ++i)
    {
        Layout->addWidget(IntegerValues[i], 0, i + 1);
    }

    IntegerSlider->setRange(UIMin, UIMax);
    IntegerSlider->setSingleStep(1);
    IntegerSlider->setPageStep(1);

    IntegerSlider->setSliderPosition(CalculateSliderPosition());

    QObject::connect(IntegerSlider, SIGNAL(valueChanged(int)), this,
                     SLOT(Slot_Slider(int)));
    QObject::connect(IntegerSlider, SIGNAL(sliderReleased()), this,
                     SLOT(Slot_SliderReleased()));

    Layout->addWidget(IntegerSlider, 0, 2);

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Integer::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_Integer::~HEMAX_ParameterWidget_Integer()
{
    delete Layout;
    delete ParameterLabel;

    for (int i = 0; i < IntegerValues.size(); ++i)
    {
        delete IntegerValues[i];
    }

    IntegerValues.clear();

    if (IntegerValues.size() == 1)
    {
        delete IntegerSlider;
    }
}

int
HEMAX_ParameterWidget_Integer::CalculateSliderPosition()
{
    return (IntegerValues[0]->text().toInt());
}

void
HEMAX_ParameterWidget_Integer::UpdateSliderPosition()
{
    if (IntegerSlider)
    {
        IntegerSlider->setSliderPosition(CalculateSliderPosition());
    }
}

int
HEMAX_ParameterWidget_Integer::CalculateValueFromSliderPosition()
{
    return (IntegerSlider->sliderPosition());
}

void
HEMAX_ParameterWidget_Integer::Slot_Slider(int Value)
{
    IntegerValues[0]->setText(std::to_string(Value).c_str());

    emit Signal_Integer_SliderDrag();

    if (!IntegerSlider->isSliderDown())
    {
        emit Signal_Integer_SliderDone();
    }
}

void
HEMAX_ParameterWidget_Integer::Slot_SliderReleased()
{
    IntegerValues[0]->setText(
        std::to_string(CalculateValueFromSliderPosition()).c_str());
    emit Signal_Integer_SliderDone();
}

HEMAX_ParameterWidget_Integer_Choice::HEMAX_ParameterWidget_Integer_Choice(
    int ParameterId, std::string Label, int ChoiceSize,
    std::vector<std::string> Choices, int CurrentChoiceIndex)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());

    IntegerChoiceValues = new QComboBox();

    QStringList QChoices;

    for (int i = 0; i < ChoiceSize; ++i)
    {
        QChoices << Choices[i].c_str();
    }

    IntegerChoiceValues->addItems(QChoices);

    IntegerChoiceValues->setCurrentIndex(CurrentChoiceIndex);

    ParameterLabel->setAlignment(Qt::AlignCenter);

    Layout->setColumnMinimumWidth(0, 100);

    Layout->addWidget(ParameterLabel, 0, 0);
    Layout->addWidget(IntegerChoiceValues, 0, 1);

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Integer_Choice::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_Integer_Choice::~HEMAX_ParameterWidget_Integer_Choice()
{
    delete Layout;
    delete ParameterLabel;
    delete IntegerChoiceValues;
}

HEMAX_ParameterWidget_String::HEMAX_ParameterWidget_String(
    int ParameterId, std::string Label, std::vector<std::string> Values,
    int ParamSize)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());

    for (int i = 0; i < ParamSize; ++i)
    {
        StringValues.push_back(new QLineEdit(Values[i].c_str()));
    }

    ParameterLabel->setAlignment(Qt::AlignCenter);

    Layout->setColumnMinimumWidth(0, 100);

    Layout->addWidget(ParameterLabel, 0, 0);

    for (int i = 0; i < ParamSize; i++)
    {
        Layout->addWidget(StringValues[i], 0, i + 1);
    }

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_String::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_String::~HEMAX_ParameterWidget_String()
{
    delete Layout;
    delete ParameterLabel;

    for (int i = 0; i < StringValues.size(); ++i)
    {
        delete StringValues[i];
    }

    StringValues.clear();
}

HEMAX_ParameterWidget_String_Choice::HEMAX_ParameterWidget_String_Choice(
    int ParameterId, std::string Label, std::vector<std::string> Choices,
    int CurrentChoiceIndex)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());
    StringChoiceValues = new QComboBox;

    Layout->setColumnMinimumWidth(0, 100);
    ParameterLabel->setAlignment(Qt::AlignCenter);

    QStringList QChoices;

    for (int i = 0; i < Choices.size(); ++i)
    {
        QChoices << Choices[i].c_str();
        ;
    }

    StringChoiceValues->addItems(QChoices);

    StringChoiceValues->setCurrentIndex(CurrentChoiceIndex);

    Layout->addWidget(ParameterLabel, 0, 0);
    Layout->addWidget(StringChoiceValues, 0, 1);

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_String_Choice::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_String_Choice::~HEMAX_ParameterWidget_String_Choice()
{
    delete Layout;
    delete ParameterLabel;
    delete StringChoiceValues;
}

HEMAX_ParameterWidget_Float::HEMAX_ParameterWidget_Float(
    int ParameterId, std::string Label, std::vector<float> Values,
    int ParamSize)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());

    for (int i = 0; i < ParamSize; ++i)
    {
        FloatValues.push_back(new QLineEdit(std::to_string(Values[i]).c_str()));
    }

    FloatSlider = nullptr;

    ParameterLabel->setAlignment(Qt::AlignCenter);

    Layout->setColumnMinimumWidth(0, 100);

    Layout->addWidget(ParameterLabel, 0, 0);

    for (int i = 0; i < ParamSize; ++i)
    {
        Layout->addWidget(FloatValues[i], 0, i + 1);
    }

    this->setLayout(Layout);
}

HEMAX_ParameterWidget_Float::HEMAX_ParameterWidget_Float(
    int ParameterId, std::string Label, std::vector<float> Values,
    int ParamSize, float UIMin, float UIMax)
    : HEMAX_ParameterWidget_Parameter(ParameterId), UIMin(UIMin), UIMax(UIMax)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());

    for (int i = 0; i < ParamSize; ++i)
    {
        FloatValues.push_back(new QLineEdit(std::to_string(Values[i]).c_str()));
    }

    FloatSlider = new QSlider(Qt::Horizontal);

    ParameterLabel->setAlignment(Qt::AlignCenter);

    Layout->setColumnMinimumWidth(0, 100);

    Layout->addWidget(ParameterLabel, 0, 0);

    for (int i = 0; i < ParamSize; ++i)
    {
        Layout->addWidget(FloatValues[i], 0, i + 1);
    }

    FloatSlider->setRange(0, 100);
    FloatSlider->setSingleStep(1);

    FloatSlider->setSliderPosition(CalculateSliderPosition());

    Layout->addWidget(FloatSlider, 0, 2);

    QObject::connect(FloatSlider, SIGNAL(valueChanged(int)), this,
                     SLOT(Slot_Slider(int)));
    QObject::connect(FloatSlider, SIGNAL(sliderReleased()), this,
                     SLOT(Slot_SliderReleased()));

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Float::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

int
HEMAX_ParameterWidget_Float::CalculateSliderPosition()
{
    return (100 *
            ((FloatValues[0]->text().toFloat() - UIMin) / (UIMax - UIMin)));
}

void
HEMAX_ParameterWidget_Float::UpdateSliderPosition()
{
    if (FloatSlider)
    {
        FloatSlider->setSliderPosition(CalculateSliderPosition());
    }
}

float
HEMAX_ParameterWidget_Float::CalculateFloatValueFromSliderPosition()
{
    float SliderPercentage = FloatSlider->sliderPosition() / 100.0;

    return (((UIMax - UIMin) * SliderPercentage) + UIMin);
}

float
HEMAX_ParameterWidget_Float::CalculateFloatValueFromSliderPosition(int Position)
{
    float SliderPercentage = Position / 100.0;

    return (((UIMax - UIMin) * SliderPercentage) + UIMin);
}

void
HEMAX_ParameterWidget_Float::Slot_Slider(int Value)
{
    FloatValues[0]->setText(
        std::to_string(CalculateFloatValueFromSliderPosition(Value)).c_str());

    emit Signal_Float_SliderDrag();

    if (!FloatSlider->isSliderDown())
    {
        emit Signal_Float_SliderDone();
    }
}

void
HEMAX_ParameterWidget_Float::Slot_SliderReleased()
{
    FloatValues[0]->setText(
        std::to_string(CalculateFloatValueFromSliderPosition()).c_str());
    emit Signal_Float_SliderDone();
}

HEMAX_ParameterWidget_Float::~HEMAX_ParameterWidget_Float()
{
    delete Layout;
    delete ParameterLabel;

    for (int i = 0; i < FloatValues.size(); ++i)
    {
        delete FloatValues[i];
    }

    FloatValues.clear();

    if (FloatValues.size() == 1)
    {
        delete FloatSlider;
    }
}

HEMAX_ParameterWidget_Toggle::HEMAX_ParameterWidget_Toggle(int ParameterId,
                                                           std::string Label,
                                                           bool Checked)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QHBoxLayout;
    Layout->setAlignment(Qt::AlignTop);
    ToggleValue = new QCheckBox();
    ParameterLabel = new QLabel(Label.c_str());
    Spacer = new QSpacerItem(HEMAX_SPACER_ITEM_WIDTH, 0);

    ToggleValue->setChecked(Checked);

    Layout->setAlignment(Qt::AlignLeft);

    Layout->addSpacerItem(Spacer);
    Layout->addWidget(ToggleValue);
    Layout->addWidget(ParameterLabel);

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Toggle::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_Toggle::~HEMAX_ParameterWidget_Toggle()
{
    Layout->removeItem(Spacer);
    delete Spacer;
    delete ToggleValue;
    delete ParameterLabel;
    delete Layout;
}

HEMAX_ParameterWidget_Button::HEMAX_ParameterWidget_Button(int ParameterId,
                                                           std::string Label)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    Button = new QPushButton(Label.c_str());

    Layout->addWidget(Button);

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Button::SetHelpToolTip(std::string HelpString)
{
    if (Button)
    {
        Button->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_Button::~HEMAX_ParameterWidget_Button()
{
    delete Layout;
    delete Button;
}

HEMAX_ParameterWidget_Button_Choice::HEMAX_ParameterWidget_Button_Choice(
    int ParameterId, HEMAX_Parameter *Parameter, std::string Title,
    std::vector<HEMAX_ParmChoice> Choices)
    : HEMAX_ParameterWidget_Parameter(ParameterId),
      Parameter(Parameter),
      ChoiceTitle(Title)
{
    Layout = new QGridLayout;
    Layout->setAlignment(Qt::AlignTop);
    ChoiceValues = new QComboBox;

    QStringList QChoices;

    QChoices << Title.c_str();

    for (int i = 0; i < Choices.size(); ++i)
    {
        QChoices << Choices[i].ChoiceLabel.c_str();
    }

    ChoiceValues->addItems(QChoices);

    Layout->addWidget(ChoiceValues);

    this->setLayout(Layout);
}

void
HEMAX_ParameterWidget_Button_Choice::SetHelpToolTip(std::string HelpString)
{
    if (ChoiceValues)
    {
        ChoiceValues->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_Button_Choice::~HEMAX_ParameterWidget_Button_Choice()
{
    delete Layout;
    delete ChoiceValues;
}

HEMAX_ParameterWidget_FilePath::HEMAX_ParameterWidget_FilePath(
    int ParameterId, std::string Label,
    std::string CurrentPathValue, HEMAX_ParameterType ParmType)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Type = ParmType;

    Layout = new QHBoxLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());
    PathEdit = new QLineEdit;
    BrowseButton = new QPushButton("...");

    PathEdit->setText(CurrentPathValue.c_str());

    Layout->addWidget(ParameterLabel);
    Layout->addWidget(PathEdit);
    Layout->addWidget(BrowseButton);

    this->setLayout(Layout);

    QObject::connect(BrowseButton, SIGNAL(clicked()), this,
                     SLOT(Slot_BrowseButton_Clicked()));
}

void
HEMAX_ParameterWidget_FilePath::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

HEMAX_ParameterWidget_FilePath::~HEMAX_ParameterWidget_FilePath()
{
    delete Layout;
    delete ParameterLabel;
    delete PathEdit;
    delete BrowseButton;
}

void
HEMAX_ParameterWidget_FilePath::Slot_BrowseButton_Clicked()
{
    switch (Type)
    {
    case (HEMAX_PARAM_PATH_FILE):
    case (HEMAX_PARAM_PATH_FILE_GEO):
    case (HEMAX_PARAM_PATH_FILE_IMAGE):
    {
        PathEdit->setText(QFileDialog::getSaveFileName());
    } break;
    case (HEMAX_PARAM_PATH_FILE_DIR):
    {
        PathEdit->setText(QFileDialog::getExistingDirectory());
    } break;
    default:
    {

    } break;
    }

    emit Signal_FilePath_Selected();
}

HEMAX_ParameterWidget_Node::HEMAX_ParameterWidget_Node(
    int ParameterId, std::string Label, std::string InputNodeName)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QHBoxLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());
    NodeEdit = new QLineEdit;
    SelectButton = new QPushButton("Select");
    ClearButton = new QPushButton("Clear Selection");

    NodeEdit->setReadOnly(true);

    Layout->addWidget(ParameterLabel);
    Layout->addWidget(NodeEdit);
    Layout->addWidget(SelectButton);
    Layout->addWidget(ClearButton);

    this->setLayout(Layout);

    NodeEdit->setText(InputNodeName.c_str());
}

void
HEMAX_ParameterWidget_Node::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

void
HEMAX_ParameterWidget_Node::SetInputName(std::string Name)
{
    NodeEdit->setText(Name.c_str());
}

HEMAX_ParameterWidget_Node::~HEMAX_ParameterWidget_Node()
{
    delete Layout;
    delete ParameterLabel;
    delete NodeEdit;
    delete SelectButton;
    delete ClearButton;
}

HEMAX_ParameterWidget_Color::HEMAX_ParameterWidget_Color(int ParameterId, std::string Label, std::vector<float> ColorVals, int Size)
    : HEMAX_ParameterWidget_Parameter(ParameterId)
{
    Layout = new QGridLayout;
    ParameterLabel = new QLabel(Label.c_str());
    ColorButton = new QPushButton;
    ColorDialog = new QColorDialog;

    QColor ButtonColor;
    ButtonColor.setRgb(255 * ColorVals[0], 255 * ColorVals[1], 255 * ColorVals[2]);

    if (Size == 3)
    {
        ColorDialog->setOption(QColorDialog::ShowAlphaChannel, false);
        ColorDialog->setCurrentColor(ButtonColor);
    }
    else if (Size == 4)
    {
        ButtonColor.setAlpha(255 * ColorVals[3]);
        ColorDialog->setOption(QColorDialog::ShowAlphaChannel, true);
        ColorDialog->setCurrentColor(ButtonColor);
        ButtonColor.setAlpha(255);
    }

    Layout->addWidget(ParameterLabel, 0, 0, Qt::AlignHCenter);
    Layout->addWidget(ColorButton, 0, 1);

    QPalette ButtonPalette;
    ButtonPalette.setColor(QPalette::Button, ButtonColor);

    ColorButton->setPalette(ButtonPalette);

    this->setLayout(Layout);

    QObject::connect(ColorButton, SIGNAL(clicked()), this, SLOT(Slot_OpenColorPickerDialog()));
}

HEMAX_ParameterWidget_Color::~HEMAX_ParameterWidget_Color()
{
    delete ColorDialog;
    delete ColorButton;
    delete ParameterLabel;
    delete Layout;
}

void
HEMAX_ParameterWidget_Color::SetHelpToolTip(std::string HelpString)
{
    if (ParameterLabel)
    {
        ParameterLabel->setToolTip(HelpString.c_str());
    }
}

void
HEMAX_ParameterWidget_Color::Slot_OpenColorPickerDialog()
{
    if (ColorDialog->exec())
    {
        QColor ButtonColor = ColorDialog->currentColor();

        QPalette ButtonPalette;
        ButtonPalette.setColor(QPalette::Button, ButtonColor);
        ColorButton->setPalette(ButtonPalette);

        emit Signal_ColorChosen();
    }
}

HEMAX_ParameterWidget_MultiParameter::HEMAX_ParameterWidget_MultiParameter(
    int ParameterId, std::string Label, int InstanceCount)
    : HEMAX_ParameterWidget_Parameter(ParameterId), InstanceCount(InstanceCount)
{
    Layout = new QVBoxLayout;

    ControlWidget = new QWidget;
    ControlLayout = new QHBoxLayout;
    Layout->setAlignment(Qt::AlignTop);
    ParameterLabel = new QLabel(Label.c_str());
    InstanceParameterCount = new QLineEdit;
    AddChildParameterButton = new QPushButton("+");
    RemoveChildParameterButton = new QPushButton("-");
    ClearButton = new QPushButton("Clear");

    InstanceParameterCount->setText(std::to_string(InstanceCount).c_str());

    ControlLayout->addWidget(ParameterLabel);
    ControlLayout->addWidget(InstanceParameterCount);
    ControlLayout->addWidget(AddChildParameterButton);
    ControlLayout->addWidget(RemoveChildParameterButton);
    ControlLayout->addWidget(ClearButton);

    ControlWidget->setLayout(ControlLayout);

    Layout->addWidget(ControlWidget);

    this->setLayout(Layout);

    QObject::connect(InstanceParameterCount, SIGNAL(returnPressed()), this,
                     SLOT(Slot_ChildParameterCount_returnPressed()),
                     Qt::UniqueConnection);
    QObject::connect(AddChildParameterButton, SIGNAL(clicked()), this,
                     SLOT(Slot_AddChildParameterButton_Clicked()));
    QObject::connect(RemoveChildParameterButton, SIGNAL(clicked()), this,
                     SLOT(Slot_RemoveChildParameterButton_Clicked()));
    QObject::connect(ClearButton, SIGNAL(clicked()), this,
                     SLOT(Slot_ClearButton_Clicked()));

    Instances.resize(InstanceCount);

    for (int i = 0; i < InstanceCount; ++i)
    {
        Instances[i] = new HEMAX_ParameterWidget_MultiParameter_Instance(i);

        QObject::connect(Instances[i]->AddBefore, SIGNAL(clicked()), this,
                         SLOT(Slot_AddBefore_Clicked()));
        QObject::connect(Instances[i]->Remove, SIGNAL(clicked()), this,
                         SLOT(Slot_Remove_Clicked()));
    }
}

void
HEMAX_ParameterWidget_MultiParameter::SetHelpToolTip(std::string HelpString)
{
    // Do Nothing override
}

HEMAX_ParameterWidget_MultiParameter::~HEMAX_ParameterWidget_MultiParameter()
{
    for (int i = 0; i < Instances.size(); ++i)
    {
        if (Instances[i])
        {
            delete Instances[i];
        }
    }

    delete ControlLayout;
    delete ParameterLabel;
    delete InstanceParameterCount;
    delete ClearButton;
    delete AddChildParameterButton;
    delete RemoveChildParameterButton;
    delete Layout;
    delete ControlWidget;
}

void
HEMAX_ParameterWidget_MultiParameter::Slot_ChildParameterCount_returnPressed()
{
    int NewInstanceCount = InstanceParameterCount->text().toInt();

    if (NewInstanceCount > InstanceCount)
    {
        for (int i = InstanceCount; i < NewInstanceCount - 1; ++i)
        {
            emit AddMultiParameterInstance(i, true);
        }
        emit AddMultiParameterInstance(NewInstanceCount - 1, false);
    }
    else if (NewInstanceCount < InstanceCount)
    {
        for (int i = InstanceCount - 1; i >= NewInstanceCount + 1; --i)
        {
            emit RemoveMultiParameterInstance(i, true);
        }
        emit RemoveMultiParameterInstance(NewInstanceCount, false);
    }
}

void
HEMAX_ParameterWidget_MultiParameter::Slot_AddChildParameterButton_Clicked()
{
    emit AddMultiParameterInstance(InstanceCount, false);
}

void
HEMAX_ParameterWidget_MultiParameter::Slot_RemoveChildParameterButton_Clicked()
{
    if (InstanceCount >= 1)
    {
        emit RemoveMultiParameterInstance(InstanceCount - 1, false);
    }
}

void
HEMAX_ParameterWidget_MultiParameter::Slot_ClearButton_Clicked()
{
    for (int i = InstanceCount - 1; i >= 1; --i)
    {
        emit RemoveMultiParameterInstance(i, true);
    }
    emit RemoveMultiParameterInstance(0, false);
}

void
HEMAX_ParameterWidget_MultiParameter::AddParameterToInstance(
    QWidget *InstanceParameter, int Instance)
{
    if (Instance < Instances.size())
    {
        Instances[Instance]->AddParameter(InstanceParameter);
        Layout->addWidget(Instances[Instance]);
    }
}

HEMAX_ParameterWidget_MultiParameter_Instance::
    HEMAX_ParameterWidget_MultiParameter_Instance(int InstancePosition)
    : HEMAX_ParameterWidget_Parameter(-1), InstancePosition(InstancePosition)
{
    Layout = new QGridLayout;
    AddBefore = new QPushButton("+");
    Remove = new QPushButton("X");

    AddBefore->setMaximumWidth(HEMAX_MULTIPARAM_MAX_BUTTON_WIDTH);
    Remove->setMaximumWidth(HEMAX_MULTIPARAM_MAX_BUTTON_WIDTH);

    Layout->addWidget(AddBefore, 0, 0, Qt::AlignLeft);
    Layout->setColumnStretch(0, 0);
    Layout->addWidget(Remove, 0, 1, Qt::AlignLeft);
    Layout->setColumnStretch(1, 0);

    CurrentGridLayoutRow = 0;

    this->setLayout(Layout);
}

HEMAX_ParameterWidget_MultiParameter_Instance::
    ~HEMAX_ParameterWidget_MultiParameter_Instance()
{
    for (int i = 0; i < Parameters.size(); ++i)
    {
        delete Parameters[i];
    }

    delete AddBefore;
    delete Remove;
    delete Layout;
}

void
HEMAX_ParameterWidget_MultiParameter_Instance::AddParameter(QWidget *Parameter)
{
    Parameters.push_back(Parameter);

    int Column = 0;
    int ColumnSpan = 3;

    if (CurrentGridLayoutRow == 0)
    {
        Column = 2;
        ColumnSpan = 1;
    }

    Layout->addWidget(Parameter, CurrentGridLayoutRow++, Column, 1, ColumnSpan);
}

void
HEMAX_ParameterWidget_MultiParameter_Instance::MarkInvalid()
{
    this->InvalidUI = true;

    for (QWidget *Parameter : Parameters)
    {
        if (Parameter)
        {
            ((HEMAX_ParameterWidget_Parameter *)Parameter)->MarkInvalid();
        }
    }
}

void
HEMAX_ParameterWidget_MultiParameter_Instance::SetHelpToolTip(
    std::string HelpString)
{
    // Override to do nothing
}

void
HEMAX_ParameterWidget_MultiParameter::Slot_AddBefore_Clicked()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_MultiParameter_Instance *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_MultiParameter_Instance *>(
            Sender->parent());

    emit AddMultiParameterInstance(HEMAX_Sender->InstancePosition, false);
}

void
HEMAX_ParameterWidget_MultiParameter::Slot_Remove_Clicked()
{
    QObject *Sender = QObject::sender();

    HEMAX_ParameterWidget_MultiParameter_Instance *HEMAX_Sender =
        qobject_cast<HEMAX_ParameterWidget_MultiParameter_Instance *>(
            Sender->parent());

    emit RemoveMultiParameterInstance(HEMAX_Sender->InstancePosition, false);
}

void
HEMAX_ParameterWidget_MultiParameter::MarkInvalid()
{
    this->InvalidUI = true;

    for (HEMAX_ParameterWidget_MultiParameter_Instance *Instance : Instances)
    {
        if (Instance)
        {
            Instance->MarkInvalid();
        }
    }
}