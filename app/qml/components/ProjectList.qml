﻿/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import lc 1.0
import "../"
import "."

Item {
  id: root

  property int projectModelType: ProjectsModel.EmptyProjectsModel
  property string activeProjectId: ""

  signal openProjectRequested( string projectId, string projectFilePath )
  signal showLocalChangesRequested( string projectId )
  signal activeProjectDeleted()

  function searchTextChanged( searchText ) {
    if ( projectModelType === ProjectsModel.PublicProjectsModel )
    {
      controllerModel.listProjects( searchText )
    }
    else viewModel.searchExpression = searchText
  }

  function refreshProjectList( searchText ) {
    controllerModel.listProjects( searchText )
  }

  function modelData( fromRole, fromValue, desiredRole ) {
    controllerModel.dataFrom( fromRole, fromValue, desiredRole )
  }

  ListView {
    id: listview

    Component.onCompleted: {
      // set proper footer (add project / fetch more)
      if ( root.projectModelType === ProjectsModel.LocalProjectsModel )
        listview.footer = addProjectButtonComponent
      else
        listview.footer = fetchMoreButtonComponent
    }

    anchors.fill: parent
    clip: true
    maximumFlickVelocity: __androidUtils.isAndroid ? InputStyle.scrollVelocityAndroid : maximumFlickVelocity
    visible: !storagePermissionText.visible

    // Proxy model with source projects model
    model: ProjectsProxyModel {
      id: viewModel

      projectSourceModel: ProjectsModel {
        id: controllerModel

        merginApi: __merginApi
        localProjectsManager: __localProjectsManager
        modelType: root.projectModelType
      }
    }

    // Project delegate
    delegate: ProjectDelegateItem {
      id: projectDelegate

      width: parent.width
      height: InputStyle.rowHeightHeader * 1.2

      projectDisplayName: root.projectModelType === ProjectsModel.CreatedProjectsModel ? model.ProjectName : model.ProjectFullName
      projectId: model.ProjectId
      projectDescription: model.ProjectDescription
      projectStatus: model.ProjectSyncStatus ? model.ProjectSyncStatus : ProjectStatus.NoVersion
      projectIsValid: model.ProjectIsValid
      projectIsPending: model.ProjectPending ? model.ProjectPending : false
      projectSyncProgress: model.ProjectSyncProgress ? model.ProjectSyncProgress : -1
      projectIsLocal: model.ProjectIsLocal
      projectIsMergin: model.ProjectIsMergin
      projectRemoteError: model.ProjectRemoteError ? model.ProjectRemoteError : ""

      highlight: model.ProjectId === root.activeProjectId

      viewContentY: ListView.view.contentY
      viewHeight: ListView.view.height

      onOpenRequested: {
        if ( model.ProjectIsLocal )
          root.openProjectRequested( projectId, model.ProjectFilePath )
        else if ( !model.ProjectIsLocal && model.ProjectIsMergin ) {
          downloadProjectDialog.relatedProjectId = model.ProjectId
          downloadProjectDialog.open()
        }
      }
      onSyncRequested: controllerModel.syncProject( projectId )
      onMigrateRequested: controllerModel.migrateProject( projectId )
      onRemoveRequested: {
        removeDialog.relatedProjectId = projectId
        removeDialog.open()
      }
      onStopSyncRequested: controllerModel.stopProjectSync( projectId )
      onShowChangesRequested: root.showLocalChangesRequested( projectId )
    }
  }

  Component {
    id: fetchMoreButtonComponent

    DelegateButton {
      width: parent.width
      height: visible ? InputStyle.rowHeight : 0
      text: qsTr( "Fetch more" )

      visible: controllerModel.hasMoreProjects
      onClicked: controllerModel.fetchAnotherPage( viewModel.searchExpression )
    }
  }

  Component {
    id: addProjectButtonComponent

    DelegateButton {
      width: parent.width
      height: InputStyle.rowHeight
      text: qsTr("Create project")

      onClicked: {
        if ( __inputUtils.hasStoragePermission() ) {
          stackView.push(projectWizardComp)
        }
        else if ( __inputUtils.acquireStoragePermission() ) {
          restartAppDialog.open()
        }
      }
    }
  }

  Text {
    id: storagePermissionText

    anchors.fill: parent
    textFormat: Text.RichText
    text: "<style>a:link { color: " + InputStyle.fontColor + "; }</style>" +
          qsTr("Input needs a storage permission, %1click to grant it%2 and then restart application.")
    .arg("<a href='missingPermission'>")
    .arg("</a>")

    onLinkActivated: {
      if ( __inputUtils.acquireStoragePermission() ) {
        restartAppDialog.open()
      }
    }
    visible: !__inputUtils.hasStoragePermission()
    color: InputStyle.fontColor
    font.pixelSize: InputStyle.fontPixelSizeNormal
    font.bold: true
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
    wrapMode: Text.WordWrap
    padding: InputStyle.panelMargin/2
  }

  Text {
    id: noLocalProjectsText

    anchors.fill: parent
    textFormat: Text.RichText
    text: "<style>a:link { color: " + InputStyle.fontColor + "; }</style>" +
          qsTr("No downloaded projects found.%1Learn %2how to create projects%3 and %4download them%3 onto your device.")
    .arg("<br/>")
    .arg("<a href='"+ __inputHelp.howToCreateNewProjectLink +"'>")
    .arg("</a>")
    .arg("<a href='"+ __inputHelp.howToDownloadProjectLink +"'>")

    onLinkActivated: Qt.openUrlExternally(link)
    visible: listview.count === 0 && !storagePermissionText.visible && projectModelType === ProjectsModel.LocalProjectsModel
    color: InputStyle.fontColor
    font.pixelSize: InputStyle.fontPixelSizeNormal
    font.bold: true
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
    wrapMode: Text.WordWrap
    padding: InputStyle.panelMargin/2
  }

  Label {
    id: noMerginProjectsTexts

    anchors.fill: parent
    horizontalAlignment: Qt.AlignHCenter
    verticalAlignment: Qt.AlignVCenter
    visible: reloadList.visible || ( projectModelType !== ProjectsModel.LocalProjectsModel && listview.count === 0 )
    text: reloadList.visible ? qsTr("Unable to get the list of projects.") : qsTr("No projects found!")
    color: InputStyle.fontColor
    font.pixelSize: InputStyle.fontPixelSizeNormal
    font.bold: true
  }

  Item {
    id: reloadList

    width: parent.width
    height: InputStyle.rowHeightHeader
    visible: false
    y: root.height/3 * 2

    Connections {
      target: __merginApi

      onListProjectsFailed: reloadList.visible = root.projectModelType !== ProjectsModel.LocalProjectsModel // show reload list to all models except local
      onListProjectsFinished: reloadList.visible = false // hide reload list on other models
    }

    Button {
        id: reloadBtn
        width: reloadList.width - 2* InputStyle.panelMargin
        height: reloadList.height
        text: qsTr("Retry")
        font.pixelSize: reloadBtn.height/2
        anchors.horizontalCenter: parent.horizontalCenter
        onClicked: {
          stackView.pending = true

          // filters suppose to not change
          projectsPage.refreshProjectList( true )
          reloadList.visible = false
        }
        background: Rectangle {
            color: InputStyle.highlightColor
            radius: InputStyle.cornerRadius
        }

        contentItem: Text {
            text: reloadBtn.text
            font: reloadBtn.font
            color: InputStyle.clrPanelMain
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
  }

  MessageDialog {
    id: removeDialog

    property string relatedProjectId

    title: qsTr( "Remove project" )
    text: qsTr( "Any unsynchronized changes will be lost." )
    icon: StandardIcon.Warning
    standardButtons: StandardButton.Ok | StandardButton.Cancel

    //! Using onButtonClicked instead of onAccepted,onRejected which have been called twice
    onButtonClicked: {
      if (clickedButton === StandardButton.Ok) {
        if (relatedProjectId === "")
          return

        if ( root.activeProjectId === relatedProjectId )
          root.activeProjectDeleted()

        controllerModel.removeLocalProject( relatedProjectId )

        removeDialog.relatedProjectId = ""
        visible = false
      }
      else if (clickedButton === StandardButton.Cancel) {
        removeDialog.relatedProjectId = ""
        visible = false
      }
    }
  }

  MessageDialog {
    id: downloadProjectDialog

    property string relatedProjectId

    title: qsTr( "Download project" )
    text: qsTr( "Would you like to download the project\n %1 ?" ).arg( relatedProjectId )
    icon: StandardIcon.Question
    standardButtons: StandardButton.Yes | StandardButton.No
    onYes: controllerModel.syncProject( relatedProjectId )
  }

  MessageDialog {
    id: restartAppDialog

    title: qsTr( "Input needs to be restarted" )
    text: qsTr( "To apply changes after granting storage permission, Input needs to be restarted. Click close and open Input again." )
    icon: StandardIcon.Warning
    visible: false
    standardButtons: StandardButton.Close
    onRejected: __inputUtils.quitApp()
  }
}
