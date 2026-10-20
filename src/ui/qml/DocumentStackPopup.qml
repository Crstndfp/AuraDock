import QtQuick

Item {
    id: root

    property bool isOpen: false
    property string folderPath: ""
    property string folderName: ""
    property var filesList: []
    property real targetX: 0
    property real targetY: 0

    signal fileSelected(string filePath)
    signal openFolderRequested(string folderPath)

    visible: isOpen
    opacity: isOpen ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
    }

    // Dismiss overlay
    MouseArea {
        anchors.fill: parent
        onClicked: root.isOpen = false
    }

    // Glassmorphic Grid / Fan Container
    Rectangle {
        id: stackPill
        width: 320
        height: Math.min(380, contentColumn.implicitHeight + 24)
        radius: 16

        x: Math.min(Math.max(12, root.targetX - width / 2), root.width - width - 12)
        y: Math.max(12, root.targetY - height - 16)

        color: Qt.rgba(0.10, 0.12, 0.16, 0.90)
        border.color: Qt.rgba(1, 1, 1, 0.22)
        border.width: 1

        Column {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // Header: Folder Name
            Row {
                width: parent.width
                height: 24
                spacing: 8

                Text {
                    text: root.folderName
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 14
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.12)
            }

            // List of items in stack
            ListView {
                width: parent.width
                height: Math.min(260, count * 36)
                clip: true
                model: root.filesList

                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 34
                    radius: 6
                    color: itemMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : "transparent"

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Rectangle {
                            width: 22
                            height: 22
                            radius: 4
                            color: Qt.rgba(0.2, 0.5, 0.8, 0.6)
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                anchors.centerIn: parent
                                text: "📄"
                                font.pixelSize: 12
                            }
                        }

                        Text {
                            text: modelData.name || modelData
                            color: "#FFFFFF"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            width: parent.width - 40
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.fileSelected(modelData.path || modelData);
                            root.isOpen = false;
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.12)
            }

            // Footer: Open in Dolphin
            Rectangle {
                width: parent.width
                height: 28
                radius: 6
                color: footerMouse.containsMouse ? Qt.rgba(0.2, 0.5, 0.9, 0.6) : Qt.rgba(1, 1, 1, 0.06)

                Text {
                    anchors.centerIn: parent
                    text: "Open in File Manager"
                    color: "#FFFFFF"
                    font.pixelSize: 11
                }

                MouseArea {
                    id: footerMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.openFolderRequested(root.folderPath);
                        root.isOpen = false;
                    }
                }
            }
        }
    }
}
