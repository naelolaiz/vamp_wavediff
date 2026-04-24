import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: "Wave File Processor"

    Column {
        spacing: 10
        anchors.centerIn: parent

        TextField {
            id: fileInput1
            placeholderText: "Path to first wave file"
            readOnly: true
        }


        FileDialog {
            id: fileDialog1
            title: "Please choose a file"
            selectMultiple: false
            nameFilters: ["Wave files (*.wav)"]
        
            onAccepted: {
                fileInput1.text = fileDialog1.fileUrl.toString();
            }
        }

        Button {
            text: "Select File 1"
            onClicked: fileDialog1.open()
        }

        TextField {
            id: fileInput2
            placeholderText: "Path to second wave file"
            readOnly: true
        }

        FileDialog {
            id: fileDialog2
            title: "Please choose a file"
            selectMultiple: false
            nameFilters: ["Wave files (*.wav)"]
        
            onAccepted: {
                fileInput2.text = fileDialog2.fileUrl.toString();
            }
        }

        Button {
            text: "Select File 2"
            onClicked: fileDialog2.open()
        }

        // Add CheckBoxes for boolean options here

        Button {
            text: "Process"
            onClicked: backend.processFiles(fileInput1.text, fileInput2.text)
        }
    }
}

