import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import QtQuick.Window 2.15

ApplicationWindow {
    id: root
    width: 920
    height: 760
    minimumWidth: 720
    minimumHeight: 600
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
                    Layout.minimumHeight: 280
                    color: Qt.darker(root.panelColor, 1.25)
                    border.color: root.borderColor
                    border.width: 1
                    radius: 4

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Canvas {
                            id: plotCanvas
                            Layout.fillWidth: true
                            Layout.preferredHeight: 220
                            Layout.minimumHeight: 160

                            readonly property var pad: ({ top: 12, right: 12, bottom: 24, left: 60 })

                            Connections {
                                target: backend
                                function onPlotChanged() { plotCanvas.requestPaint() }
                            }
                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()

                                var W = width, H = height
                                var p = pad
                                var plotW = W - p.left - p.right
                                var plotH = H - p.top - p.bottom

                                // Background.
                                ctx.fillStyle = Qt.darker(root.panelColor, 1.6)
                                ctx.fillRect(p.left, p.top, plotW, plotH)
                                ctx.strokeStyle = root.borderColor
                                ctx.lineWidth = 1
                                ctx.strokeRect(p.left + 0.5, p.top + 0.5, plotW - 1, plotH - 1)

                                var series = backend.plotSeries
                                var maxX = backend.totalSamples
                                var maxY = backend.maxValue
                                if (!series || series.length === 0 || maxX <= 0 || maxY <= 0) {
                                    ctx.fillStyle = "#888"
                                    ctx.font = "12px sans-serif"
                                    ctx.textAlign = "center"
                                    ctx.textBaseline = "middle"
                                    ctx.fillText(qsTr("Plot will appear after a diff run."),
                                                 p.left + plotW / 2, p.top + plotH / 2)
                                    return
                                }

                                // Round y-max up a bit for headroom.
                                var yScale = maxY * 1.08

                                // Gridlines + y labels.
                                ctx.strokeStyle = Qt.lighter(root.borderColor, 1.05)
                                ctx.fillStyle = "#aab0b3"
                                ctx.font = "10px sans-serif"
                                ctx.textAlign = "right"
                                ctx.textBaseline = "middle"
                                for (var k = 0; k <= 4; ++k) {
                                    var gy = p.top + plotH * k / 4
                                    ctx.beginPath()
                                    ctx.moveTo(p.left, gy + 0.5)
                                    ctx.lineTo(p.left + plotW, gy + 0.5)
                                    ctx.stroke()
                                    var v = yScale * (1 - k / 4)
                                    ctx.fillText(v.toFixed(4), p.left - 6, gy)
                                }

                                // x labels (samples).
                                ctx.textAlign = "center"
                                ctx.textBaseline = "top"
                                ctx.fillText("0", p.left, p.top + plotH + 6)
                                ctx.fillText(maxX + " samp", p.left + plotW, p.top + plotH + 6)

                                // Series.
                                ctx.lineJoin = "round"
                                ctx.lineCap = "round"
                                for (var i = 0; i < series.length; ++i) {
                                    var s = series[i]
                                    if (!s.points || s.points.length === 0) continue
                                    ctx.strokeStyle = s.color
                                    ctx.lineWidth = 2
                                    ctx.beginPath()
                                    var first = true
                                    for (var j = 0; j < s.points.length; ++j) {
                                        var pt = s.points[j]
                                        var px = p.left + (pt.x / maxX) * plotW
                                        var py = p.top + plotH * (1 - pt.y / yScale)
                                        if (first) { ctx.moveTo(px, py); first = false }
                                        else { ctx.lineTo(px, py) }
                                    }
                                    ctx.stroke()
                                }
                            }
                        }

                        // Legend with overall values.
                        Flow {
                            Layout.fillWidth: true
                            spacing: 16

                            Repeater {
                                model: backend.plotSeries
                                Row {
                                    spacing: 6
                                    Rectangle {
                                        width: 14; height: 4
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: modelData.color
                                        radius: 2
                                    }
                                    Label {
                                        text: modelData.hasOverall
                                              ? qsTr("%1 — overall %2").arg(modelData.label).arg(modelData.overall.toFixed(6))
                                              : modelData.label
                                        font.pixelSize: 12
                                        opacity: 0.85
                                    }
                                }
                            }
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
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
}
