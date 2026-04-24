import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import QtQuick.Window 2.15

ApplicationWindow {
    id: root
    width: 860
    height: 620
    minimumWidth: 640
    minimumHeight: 520
    visible: true
    title: qsTr("WaveDiff — A/B audio comparison")

    Material.theme: Material.Dark
    Material.accent: Material.Teal
    Material.primary: Material.BlueGrey

    readonly property color panelColor: Qt.darker(Material.background, 1.08)
    readonly property color borderColor: Qt.lighter(Material.background, 1.4)

    header: ToolBar {
        Material.elevation: 2
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            Label {
                text: "▲▼"
                font.pixelSize: 20
                color: Material.accent
            }
            Label {
                text: qsTr("WaveDiff")
                font.pixelSize: 18
                font.weight: Font.Medium
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("v0.2")
                opacity: 0.6
            }
        }
    }

    FileDialog {
        id: fileDialogA
        title: qsTr("Select file A (reference)")
        selectMultiple: false
        nameFilters: [ qsTr("Wave files (*.wav)"), qsTr("All files (*)") ]
        onAccepted: fileAField.text = fileDialogA.fileUrl.toString()
    }

    FileDialog {
        id: fileDialogB
        title: qsTr("Select file B (comparison)")
        selectMultiple: false
        nameFilters: [ qsTr("Wave files (*.wav)"), qsTr("All files (*)") ]
        onAccepted: fileBField.text = fileDialogB.fileUrl.toString()
    }

    MessageDialog {
        id: errorDialog
        title: qsTr("WaveDiff error")
        icon: StandardIcon.Warning
    }

    Connections {
        target: backend
        function onLastErrorChanged() {
            if (backend.lastError && backend.lastError.length > 0) {
                errorDialog.text = backend.lastError
                errorDialog.open()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Pane {
            Layout.fillWidth: true
            Material.background: root.panelColor
            Material.elevation: 1
            padding: 18

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                Label {
                    text: qsTr("Inputs")
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    opacity: 0.9
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 12
                    rowSpacing: 10

                    Label { text: qsTr("File A"); Layout.alignment: Qt.AlignVCenter }
                    TextField {
                        id: fileAField
                        Layout.fillWidth: true
                        placeholderText: qsTr("/path/to/reference.wav")
                        selectByMouse: true
                    }
                    Button {
                        text: qsTr("Browse…")
                        onClicked: fileDialogA.open()
                    }

                    Label { text: qsTr("File B"); Layout.alignment: Qt.AlignVCenter }
                    TextField {
                        id: fileBField
                        Layout.fillWidth: true
                        placeholderText: qsTr("/path/to/comparison.wav")
                        selectByMouse: true
                    }
                    Button {
                        text: qsTr("Browse…")
                        onClicked: fileDialogB.open()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        text: qsTr("Cancel")
                        enabled: backend.busy
                        onClicked: backend.cancel()
                    }

                    Button {
                        id: runButton
                        text: backend.busy ? qsTr("Running…") : qsTr("Run Diff")
                        highlighted: true
                        enabled: !backend.busy
                                 && fileAField.text.length > 0
                                 && fileBField.text.length > 0
                        onClicked: backend.runDiff(fileAField.text, fileBField.text)
                    }
                }
            }
        }

        Pane {
            Layout.fillWidth: true
            Material.background: root.panelColor
            Material.elevation: 1
            padding: 16

            RowLayout {
                anchors.fill: parent
                spacing: 12

                BusyIndicator {
                    running: backend.busy
                    visible: backend.busy
                    implicitWidth: 22
                    implicitHeight: 22
                }
                Label {
                    text: backend.statusMessage.length > 0
                          ? backend.statusMessage
                          : qsTr("Idle — select two wave files and click Run Diff.")
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                }
            }
        }

        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Material.background: root.panelColor
            Material.elevation: 1
            padding: 16

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: qsTr("Results")
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        opacity: 0.9
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        visible: backend.mergedFilePath.length > 0
                        text: qsTr("Merged: %1").arg(backend.mergedFilePath)
                        opacity: 0.65
                        elide: Text.ElideLeft
                        Layout.maximumWidth: 420
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Qt.darker(root.panelColor, 1.25)
                    border.color: root.borderColor
                    border.width: 1
                    radius: 4

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true

                        TextArea {
                            id: resultsArea
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextArea.NoWrap
                            font.family: "monospace"
                            font.pixelSize: 13
                            text: backend.resultsText.length > 0
                                  ? backend.resultsText
                                  : qsTr("No results yet. Run a diff to see per-pair RMS and null-test values.")
                            background: null
                        }
                    }
                }
            }
        }
    }
}
