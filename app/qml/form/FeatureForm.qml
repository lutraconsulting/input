/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQml.Models 2.2
import QtQml 2.2

// We use calendar in datetime widget that is not yet implemented in Controls 2.2
import QtQuick.Controls 1.4 as Controls1
import QgsQuick 0.1 as QgsQuick
import lc 1.0
import "../components"

Item {
  /**
   * When feature in the form is saved.
   */
  signal saved

  /**
   * When the form is about to be closed by closeButton or deleting a feature.
   */
  signal canceled

  /**
   * Signal emited when relation editor requests to open child feature form
   */
  signal openLinkedFeature( var linkedFeature )

  /**
   * Signal emited when relation editor requests to create child feature and open its form
   */
  signal createLinkedFeature( var parentController, var relation )

   /**
    * A handler for extra events in externalSourceWidget.
    */
  property var externalResourceHandler: QtObject {

        /**
         * Called when clicked on the camera icon to capture an image.
         * \param itemWidget editorWidget for modified field to send valueChanged signal.
         */
        property var capturePhoto: function captureImage(itemWidget) {
        }

        /**
         * Called when clicked on the gallery icon to choose a file in a gallery.
         * \param itemWidget editorWidget for modified field to send valueChanged signal.
         */
        property var chooseImage: function chooseImage(itemWidget) {
        }

        /**
          * Called when clicked on the photo image. Suppose to be used to bring a bigger preview.
          * \param imagePath Absolute path to the image.
          */
        property var previewImage: function previewImage(imagePath) {
        }

        /**
          * Called when clicked on the trash icon. Suppose to delete the value and optionally also the image.
          * \param itemWidget editorWidget for modified field to send valueChanged signal.
          * \param imagePath Absolute path to the image.
          */
        property var removeImage: function removeImage(itemWidget, imagePath) {
        }

        /**
          * Called when clicked on the OK icon after taking a photo with the Photo panel.
          * \param itemWidget editorWidget for modified field to send valueChanged signal.
          * \param prefixToRelativePath Together with the value creates absolute path
          * \param value Relative path of taken photo.
          */
        property var confirmImage: function confirmImage(itemWidget, prefixToRelativePath, value) {
          itemWidget.image.source = prefixToRelativePath + "/" + value
          itemWidget.valueChanged(value, value === "" || value === null)
        }
    }

  /**
   * Support for custom callback on events happening in widgets
   */
  property var customWidgetCallback: QtObject {

    /**
     * Called when user clicks on valuerelation widget and combobox shall open
     * \param widget valuerelation widget for specific field to send valueChanged signal.
     * \param valueRelationModel model of type FeaturesListModel bears features of related layer.
     */
    property var valueRelationOpened: function valueRelationOpened( widget, valueRelationModel ) {}

    /**
     * Called when field for value relation is created, by default it returns value "combobox".
     * Return value of this function sets corresponding widget type. Currently accepted values are:
     *    - combobox -> QML combobox component.
     *    - textfield -> custom text widget that shows only title of selected feature in value relation
     *                   and calls function "valueRelationOpened" when it is clicked.
     * \param widget valuerelation widget for specific field to send valueChanged signal.
     * \param valueRelationModel model of type FeaturesListModel bears features of related layer.
     */
    property var getTypeOfWidget: function getTypeOfWidget( widget, valueRelationModel ) {
      return "combobox"
    }
  }

  /**
   * A handler for extra events for a TextEdit widget .
   */
  property var importDataHandler: QtObject {

    /**
     * Suppose to set `supportsDataImport` variable of a feature form. If true, enables to set data by this handler.
     * \param name "Name" property of field item. Expecting alias if defined, otherwise field name.
     */
    property var supportsDataImport: function supportsDataImport(name) { return false }

    /**
     * Suppose to be called to invoke a component to set data automatically (e.g. code scanner, sensor).
     * \param itemWidget editorWidget for modified field to send valueChanged signal.
     */
    property var importData: function importData(itemWidget) {}

    /**
     * Suppose to be called after `importData` function as a callback to set the value to the widget.
     * \param value Value to be set.
     */
    property var setValue: function setValue(value) {}
  }

  /**
   * Active project.
   */
  property QgsQuick.Project project

  /**
   * Controller
   */
  property AttributeController controller

  /**
   * View for extra components like value relation page, relations page, etc.
   */
  property StackView extraView

  /**
   * The function used for a component loader to find qml edit widget components used in form.
   */
  property var loadWidgetFn: __inputUtils.getEditorComponentSource

  /**
   * Predefined form styling
   */
  property FeatureFormStyling style: FeatureFormStyling {}

  id: form

  states: [
    State {
      name: "ReadOnly"
    },
    State {
      name: "Edit"
    },
    State {
      name: "Add"
    }
  ]

  function reset() {
    master.reset()
  }

  function save() {
    if ( !controller.constraintsHardValid )
    {
      console.log( qsTr( 'Constraints not valid') )
      return
    }
    else if ( !controller.constraintsSoftValid )
    {
      console.log( qsTr( 'Note: soft constraints were not met') )
    }

    parent.focus = true

    if ( __inputUtils.isFeatureIdValid( controller.featureLayerPair.feature.id ) ) {
      controller.save()
    }
    else
    {
      controller.create()
    }

    saved()
  }

  function cancel() {
    // remove feature if we are in "Add" mode and it already has valid ID
    // it was saved to prefill relation reference field in child layer
    let featureId = form.controller.featureLayerPair.feature.id
    let shouldRemoveFeature = form.state === "Add" && __inputUtils.isFeatureIdValid( featureId )

    if ( shouldRemoveFeature ) {
      form.controller.deleteFeature()
    }

    canceled()
  }

  /**
   * This is a relay to forward private signals to internal components.
   */
  QtObject {
    id: master

    /**
     * This signal is emitted whenever the state of Flickables and TabBars should
     * be restored.
     */
    signal reset
  }

  StackView {
    id: formView

    anchors.fill: parent

    initialItem: container
  }

  Rectangle {
    id: container

    clip: true
    color: form.style.tabs.backgroundColor

    anchors.fill: parent

    Flickable {
      id: flickable
      anchors {
        left: parent.left
        right: parent.right
        leftMargin: form.style.fields.outerMargin
        rightMargin: form.style.fields.outerMargin
      }
      height: form.controller.hasTabs ? tabRow.height : 0

      flickableDirection: Flickable.HorizontalFlick
      contentWidth: tabRow.width

      // Tabs
      TabBar {
        id: tabRow
        visible: form.controller.hasTabs
        height: form.style.tabs.height
        spacing: form.style.tabs.spacing

        background: Rectangle {
          anchors.fill: parent
          color: form.style.tabs.backgroundColor
        }

        Connections {
          target: master
          onReset: tabRow.currentIndex = 0
        }

        Connections {
          target: swipeView
          onCurrentIndexChanged: tabRow.currentIndex = swipeView.currentIndex
        }

        Repeater {
          model: form.controller.attributeTabProxyModel

          TabButton {
            id: tabButton
            text: Name
            leftPadding: 8 * QgsQuick.Utils.dp
            rightPadding: 8 * QgsQuick.Utils.dp
            anchors.bottom: parent.bottom
            focusPolicy: Qt.NoFocus

            width: leftPadding + rightPadding
            height: form.style.tabs.buttonHeight

            contentItem: Text {
              // Make sure the width is derived from the text so we can get wider
              // than the parent item and the Flickable is useful
              Component.onCompleted: {
                tabButton.width = tabButton.width + paintedWidth
                if (tabRow.currentIndex == index)
                  tabButton.checked = true
              }

              width: paintedWidth
              text: tabButton.text
              color: !tabButton.enabled ? form.style.tabs.disabledColor : tabButton.down ||
                                          tabButton.checked ? form.style.tabs.activeColor : form.style.tabs.normalColor
              font.weight: Font.DemiBold
              font.underline: tabButton.checked ? true : false
              font.pointSize: form.style.tabs.tabLabelPointSize
              opacity: tabButton.checked ? 1 : 0.5

              horizontalAlignment: Text.AlignHCenter
              verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
              color: !tabButton.enabled ? form.style.tabs.disabledBackgroundColor : tabButton.down ||
                                                 tabButton.checked ? form.style.tabs.activeBackgroundColor : form.style.tabs.normalBackgroundColor
            }
          }
        }
      }
    }

    SwipeView {
      id: swipeView
      currentIndex: form.controller.hasTabs ? tabRow.currentIndex : 0
      anchors {
        top: flickable.bottom
        left: parent.left
        right: parent.right
        bottom: parent.bottom
     }

      Repeater {
        //One page per tab in tabbed forms, 1 page in auto forms

        model: form.controller.attributeTabProxyModel
        id: swipeViewRepeater

        Item {
          id: formPage
          property int tabIndex: model.TabIndex

          // The main form content area
          Rectangle {
            anchors.fill: parent
            color: form.style.backgroundColor
            opacity: form.style.backgroundOpacity
          }

          ListView {
            id: content
            anchors.fill: parent
            clip: true
            spacing: form.style.group.spacing
            section.property: "Group"
            section.labelPositioning: ViewSection.CurrentLabelAtStart | ViewSection.InlineLabels
            section.delegate: Component {

              // section header: group box name
              Item {
                id: headerContainer
                width: parent.width
                height: section === "" ? 0 : form.style.group.height + form.style.group.spacing // add space after section header

                Rectangle {
                  width: parent.width
                  height: section === "" ? 0 : form.style.group.height
                  color: form.style.group.marginColor
                  anchors.top: parent.top

                  Rectangle {
                    anchors.fill: parent
                    anchors {
                      leftMargin: form.style.group.leftMargin
                      rightMargin: form.style.group.rightMargin
                      topMargin: form.style.group.topMargin
                      bottomMargin: form.style.group.bottomMargin
                    }
                    color: form.style.group.backgroundColor

                    Text {
                      anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                      font.bold: true
                      font.pixelSize: form.style.group.fontPixelSize
                      text: section
                      color: form.style.group.fontColor
                    }
                  }
                }
              }
            }


            Connections {
              target: master
              onReset: content.contentY = 0
            }

            model: swipeViewRepeater.model.attributeFormProxyModel(formPage.tabIndex)

           delegate: fieldItem

            header: Rectangle {
              opacity: 1
              height: form.style.group.spacing
            }

            footer: Rectangle {
              opacity: 1
              height: 2 * form.style.group.spacing
            }
          }
        }
      }
    }

    // Borders
    Rectangle {
      width: parent.width
      height: form.style.tabs.borderWidth
      anchors.top: flickable.top
      color: form.style.tabs.borderColor
      visible: flickable.height
    }

    Rectangle {
      width: parent.width
      height: form.style.tabs.borderWidth
      anchors.bottom: flickable.bottom
      color: form.style.tabs.borderColor
      visible: flickable.height
    }
  }

  /**
   * A field editor
   */
  Component {
    id: fieldItem

    Item {
      id: fieldContainer

      property bool shouldBeVisible: Type === FormItem.Field || Type === FormItem.Relation

      visible: shouldBeVisible
      // We also need to set height to zero if Type is not field otherwise children created blank space in form
      height: shouldBeVisible ? childrenRect.height : 0

      anchors {
        left: parent.left
        right: parent.right
        leftMargin: form.style.fields.outerMargin
        rightMargin: form.style.fields.outerMargin
      }

      Item {
        id: labelPlaceholder
        height: fieldLabel.height + fieldHelperText.height + form.style.fields.sideMargin
        anchors {
          left: parent.left
          right: parent.right
          topMargin: form.style.fields.sideMargin
          bottomMargin: form.style.fields.sideMargin
        }

        Label {
          id: fieldLabel

          text: Name
          color: ConstraintSoftValid && ConstraintHardValid ? form.style.constraint.validColor : form.style.constraint.invalidColor
          leftPadding: form.style.fields.sideMargin
          font.pointSize: form.style.fields.labelPointSize
          horizontalAlignment: Text.AlignLeft
          verticalAlignment: Text.AlignVCenter
          anchors.top: parent.top
        }

        Label {
          id: fieldHelperText

          property string helperText: {
            if ( ValueValidity === FormItem.ValueOutOfRange )
              return qsTr( 'Number is outside of specified range' )
            else if ( ValueValidity === FormItem.InvalidValue )
              return qsTr( 'Value is not valid' )

            if ( ConstraintDescription )
              return ConstraintDescription

            return ''
          }

          property bool shouldShowhelperText: {
            if ( ( !ConstraintHardValid || !ConstraintSoftValid ) && !!ConstraintDescription )
              return true

            if ( ValueValidity !== FormItem.ValidValue )
              return true

            return false
          }

          anchors {
            left: parent.left
            right: parent.right
            top: fieldLabel.bottom
            leftMargin: form.style.fields.sideMargin
          }

          text: helperText
          visible: shouldShowhelperText
          height: visible ? undefined : 0
          wrapMode: Text.WordWrap
          color: form.style.constraint.descriptionColor
          horizontalAlignment: Text.AlignLeft
          verticalAlignment: Text.AlignVCenter
        }

      }

      Item {
        id: placeholder
        height: childrenRect.height
        anchors {
          left: parent.left
          right: rememberCheckboxContainer.left
          top: labelPlaceholder.bottom
        }


        Loader {
          id: attributeEditorLoader

          height: childrenRect.height
          anchors { left: parent.left; right: parent.right }

          signal dataHasChanged() // to propagate signal to valuerelation model from model

          property var value: AttributeValue
          property var config: EditorWidgetConfig
          property var widget: EditorWidget
          property var field: Field
          property var constraintHardValid: ConstraintHardValid
          property var constraintSoftValid: ConstraintSoftValid
          property bool constraintsHardValid: form.controller.constraintsHardValid
          property bool constraintsSoftValid: form.controller.constraintsSoftValid
          property var homePath: form.project ? form.project.homePath : ""
          property var customStyle: form.style
          property var externalResourceHandler: form.externalResourceHandler
          property bool readOnly: form.state == "ReadOnly" || !AttributeEditable
          property var featurePair: form.controller.featureLayerPair
          property var activeProject: form.project
          property var customWidget: form.customWidgetCallback
          property var labelAlias: Name
          property bool supportsDataImport: importDataHandler.supportsDataImport(Name)

          property var associatedRelation: Relation
          property var formView: extraView

          active: widget !== 'Hidden'
          Keys.forwardTo: backHandler

          source: {
            if ( widget !== undefined )
               return form.loadWidgetFn(widget.toLowerCase(), config)
            else return ''
          }
        }

        Connections {
          target: attributeEditorLoader.item
          onValueChanged: {
            AttributeValue = isNull ? undefined : value
          }
        }

        Connections {
          target: attributeEditorLoader.item
          ignoreUnknownSignals: true
          onImportDataRequested: {
           importDataHandler.importData(attributeEditorLoader.item)
          }

          onOpenLinkedFeature: {
            form.openLinkedFeature( linkedFeature )
          }

          onCreateLinkedFeature: {
            let parentHasValidId = __inputUtils.isFeatureIdValid( parentFeature.feature.id )

            if ( parentHasValidId ) {
              // parent feature in this case already have valid id, so we can open new form
              form.createLinkedFeature( form.controller, relation )
            }
            else {
              // parent feature do not have a valid ID yet, we need to save it and acquire ID
              form.controller.acquireId()
              form.createLinkedFeature( form.controller, relation )
            }
          }
        }

        Connections {
          target: form.controller
          onFormDataChanged: {
            if ( attributeEditorLoader.item && attributeEditorLoader.item.dataUpdated )
            {
              if ( roles.length === 1 && roles[0] === AttributeFormModel.ValueValidity )
                return // do not propagate data changed when this is only value validity, it would lead to infinite loop

              attributeEditorLoader.item.dataUpdated( form.controller.featureLayerPair.feature )
            }
          }
          onFeatureLayerPairChanged: {
            if ( attributeEditorLoader.item && attributeEditorLoader.item.featureLayerPairChanged )
            {
              attributeEditorLoader.item.featureLayerPairChanged()
            }
          }
        }

        Connections {
          target: form
          ignoreUnknownSignals: true
          onSaved: {
            if (attributeEditorLoader.item && typeof attributeEditorLoader.item.callbackOnSave === "function") {
              attributeEditorLoader.item.callbackOnSave()
            }
          }
        }

        Connections {
          target: form
          ignoreUnknownSignals: true
          onCanceled: {
            if (attributeEditorLoader.item && typeof attributeEditorLoader.item.callbackOnCancel === "function") {
              attributeEditorLoader.item.callbackOnCancel()
            }
          }
        }
      }

      Item {
        id: rememberCheckboxContainer
        visible: {
          form.controller.rememberAttributesController.rememberValuesAllowed && form.state === "Add" && EditorWidget !== "Hidden" && Type === FormItem.Field
        }

        implicitWidth: visible ? 35 * QgsQuick.Utils.dp : 0
        implicitHeight: placeholder.height

        anchors {
          top: labelPlaceholder.bottom
          right: parent.right
        }

        CheckboxComponent {
          id: rememberCheckbox
          visible: rememberCheckboxContainer.visible
          baseColor: form.style.checkboxComponent.baseColor

          implicitWidth: 40 * QgsQuick.Utils.dp
          implicitHeight: width
          y: rememberCheckboxContainer.height/2 - rememberCheckbox.height/2
          x: (parent.width + form.style.fields.outerMargin) / 7

          onCheckboxClicked: RememberValue = buttonState
          checked: RememberValue ? true : false
        }

        MouseArea {
          anchors.fill: parent
          onClicked: rememberCheckbox.checkboxClicked( !rememberCheckbox.checkState )
        }
      }
    }
  }

  Connections {
    target: Qt.inputMethod
    onVisibleChanged: {
      Qt.inputMethod.commit()
    }
  }
}
