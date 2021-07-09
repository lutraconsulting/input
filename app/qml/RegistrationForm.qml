

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
import QtQuick.Templates 2.1 as T
import QtQuick 2.7
import QtQuick.Controls 2.7
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2
import QtGraphicalEffects 1.0
import QgsQuick 0.1 as QgsQuick
import lc 1.0
import "." // import InputStyle singleton
import "./components"

/**
  * Body of the AuthPanel with the registration form - username, email, password, ...
  */
Rectangle {
  id: registerForm
  color: root.bgColor

  function clean() {
    registerName.text = ""
    email.text = ""
    passwordField.password.text = ""
    passwordConfirmField.password.text = ""
    acceptTOC.checked = false
  }

  ScrollView {
    width: registerForm.width
    height: registerForm.height

  Column {
    id: columnLayout
    spacing: root.panelMargin / 2
    width: parent.width
    anchors.bottom: parent.bottom

    Image {
      source: "mergin_color.svg"
      height: fieldHeight
      sourceSize.height: height
      anchors.horizontalCenter: parent.horizontalCenter
    }

    Row {
      id: row
      width: registerForm.width
      height: fieldHeight
      spacing: 0

      Rectangle {
        id: iconContainer
        height: fieldHeight
        width: fieldHeight
        color: root.bgColor

        Image {
          anchors.margins: root.panelMargin
          id: icon
          height: fieldHeight
          width: fieldHeight
          anchors.fill: parent
          source: 'account.svg'
          sourceSize.width: width
          sourceSize.height: height
          fillMode: Image.PreserveAspectFit
        }

        ColorOverlay {
          anchors.fill: icon
          source: icon
          color: root.fontColor
        }
      }

      TextField {
        id: registerName
        x: iconContainer.width
        width: parent.width - iconContainer.width
        height: fieldHeight
        font.pixelSize: InputStyle.fontPixelSizeNormal
        color: root.fontColor
        placeholderText: qsTr("Username")
        font.capitalization: Font.MixedCase
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        background: Rectangle {
          color: root.bgColor
        }
      }
    }

    Rectangle {
      id: registerNameBorder
      color: root.fontColor
      y: registerName.height - height
      height: 2 * QgsQuick.Utils.dp
      opacity: registerName.focus ? 1 : 0.6
      width: parent.width - fieldHeight / 2
      anchors.horizontalCenter: parent.horizontalCenter
    }

    Row {
      width: registerForm.width
      height: fieldHeight
      spacing: 0

      Rectangle {
        id: iconContainer4
        height: fieldHeight
        width: fieldHeight
        color: root.bgColor

        Image {
          anchors.margins: (fieldHeight / 4)
          id: icon4
          height: fieldHeight
          width: fieldHeight
          anchors.fill: parent
          source: 'envelope-solid.svg'
          sourceSize.width: width
          sourceSize.height: height
          fillMode: Image.PreserveAspectFit
        }

        ColorOverlay {
          anchors.fill: icon4
          source: icon4
          color: root.fontColor
        }
      }

      TextField {
        id: email
        width: parent.width - iconContainer.width
        height: fieldHeight
        font.pixelSize: InputStyle.fontPixelSizeNormal
        color: root.fontColor
        placeholderText: qsTr("Email")
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        background: Rectangle {
          color: root.bgColor
        }
      }
    }

    Rectangle {
      id: emailBorder
      color: InputStyle.panelBackgroundDark
      height: 2 * QgsQuick.Utils.dp
      y: email.height - height
      opacity: email.focus ? 1 : 0.6
      width: registerForm.width - fieldHeight / 2
      anchors.horizontalCenter: parent.horizontalCenter
    }

    PasswordField {
      id: passwordField
      width: registerForm.width
      height: fieldHeight
      fontColor: root.fontColor
      bgColor: root.bgColor
    }

    Rectangle {
      id: passBorder
      color: InputStyle.panelBackgroundDark
      height: 2 * QgsQuick.Utils.dp
      y: fieldHeight - height
      opacity: passwordField.password.focus ? 1 : 0.6
      width: registerForm.width - fieldHeight / 2
      anchors.horizontalCenter: parent.horizontalCenter
    }

    PasswordField {
      id: passwordConfirmField
      width: registerForm.width
      height: fieldHeight
      fontColor: root.fontColor
      bgColor: root.bgColor
      placeholderText:  qsTr('Confirm password')
    }


    Rectangle {
      id: confirmPassBorder
      color: InputStyle.panelBackgroundDark
      height: 2 * QgsQuick.Utils.dp
      y: fieldHeight - height
      opacity: passwordConfirmField.password.focus ? 1 : 0.6
      width: registerForm.width - fieldHeight / 2
      anchors.horizontalCenter: parent.horizontalCenter
    }

    Row {
      width: registerForm.width
      height: fieldHeight
      spacing: 0

      LeftCheckBox {
        id: acceptTOC
        anchors.margins: (fieldHeight / 4)
        baseColor: root.fontColor
        height: fieldHeight
        width: registerForm.width
        text: qsTr("I accept the Mergin %1Terms and Conditions%3 and %2Privacy Policy%3")
              .arg("<a href='"+__merginApi.apiRoot + "assets/tos.html'>")
              .arg("<a href='"+ __inputHelp.privacyPolicyLink +"'>")
              .arg("</a>")
      }
    }

    Rectangle {
      id: acceptTOCBorder
      color: InputStyle.panelBackgroundDark
      height: 2 * QgsQuick.Utils.dp
      y: acceptTOC.height - height
      opacity: acceptTOC.focus ? 1 : 0.6
      width: registerForm.width - fieldHeight / 2
      anchors.horizontalCenter: parent.horizontalCenter
    }

    Button {
      id: registerButton
      enabled: !stackView.pending
      width: registerForm.width - 2 * root.panelMargin
      height: fieldHeight
      text: qsTr("Sign up")
      font.pixelSize: InputStyle.fontPixelSizeTitle
      anchors.horizontalCenter: parent.horizontalCenter
      onClicked: {
        stackView.pending = true
        __merginApi.registerUser(registerName.text, email.text, passwordField.password.text,
                                 passwordConfirmField.password.text, acceptTOC.checked)
      }
      background: Rectangle {
        color: InputStyle.highlightColor
      }

      contentItem: Text {
        text: registerButton.text
        font: registerButton.font
        opacity: enabled ? 1.0 : 0.3
        color: root.bgColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
      }
    }
    }
  }
}
