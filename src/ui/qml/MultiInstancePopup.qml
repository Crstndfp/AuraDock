import QtQuick

Item {
    id: root

    property bool isOpen: false
    property int targetItemIndex: -1
    property string targetAppName: ""
    property string targetAppIcon: ""
    property var windowsList: []
    property int displayMode: 0 // 0 = MiniWindows (Cards), 1 = WindowList (List)
    property real targetX: 0
    property real targetY: 0

    signal windowSelected(var windowId)
    signal closeWindowRequested(var windowId)

    visible: isOpen
    opacity: isOpen ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }

    // Dismiss overlay when clicking outside the popup container
    MouseArea {
        anchors.fill: parent
        onClicked: root.isOpen = false
    }

    // =========================================================================
    // MODE 0: MINI WINDOWS (Horizontal Glassmorphic Cards)
    // =========================================================================
    Rectangle {
        id: miniWindowsContainer
        visible: root.displayMode === 0
        width: Math.min(root.width - 24, miniWindowsRow.implicitWidth + 28)
        height: 168
        radius: 16

        x: Math.min(Math.max(12, root.targetX - width / 2), root.width - width - 12)
        y: Math.max(10, root.targetY - height - 14)

        color: Qt.rgba(0.10, 0.12, 0.16, 0.92)
        border.color: Qt.rgba(1, 1, 1, 0.24)
        border.width: 1

        // Prevent click-through to dismiss overlay when clicking inside container background
        MouseArea {
            anchors.fill: parent
        }

        // Outer subtle shadow border
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.45)
            border.width: 1
            z: -1
        }

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // Header with App Name and instance count badge
            Item {
                width: parent.width
                height: 20

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    Image {
                        width: 16
                        height: 16
                        sourceSize.width: 32
                        sourceSize.height: 32
                        source: root.targetAppIcon.length > 0 ? ("image://icon/" + root.targetAppIcon) : ""
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.targetAppName
                        color: "#FFFFFF"
                        font.bold: true
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: countLabel.implicitWidth + 12
                    height: 18
                    radius: 9
                    color: Qt.rgba(0.3, 0.6, 1.0, 0.28)
                    border.color: Qt.rgba(0.4, 0.75, 1.0, 0.45)
                    border.width: 1

                    Text {
                        id: countLabel
                        anchors.centerIn: parent
                        text: root.windowsList.length + " ventanas"
                        color: "#B8E0FF"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
            }

            // Horizontal Row of Mini Window Cards
            Row {
                id: miniWindowsRow
                spacing: 10
                height: 116

                Repeater {
                    model: root.windowsList

                    delegate: Rectangle {
                        required property var modelData
                        required property int index

                        width: 184
                        height: 116
                        radius: 10
                        color: cardMouse.containsMouse
                            ? Qt.rgba(0.25, 0.52, 0.92, 0.32)
                            : (modelData.isActive ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.06))
                        border.color: cardMouse.containsMouse
                            ? Qt.rgba(0.45, 0.78, 1.0, 0.95)
                            : (modelData.isActive ? Qt.rgba(0.35, 0.70, 1.0, 0.70) : Qt.rgba(1, 1, 1, 0.16))
                        border.width: (cardMouse.containsMouse || modelData.isActive) ? 1.5 : 1

                        Behavior on color { ColorAnimation { duration: 110 } }

                        // Main click area for selecting this window instance
                        MouseArea {
                            id: cardMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                root.windowSelected(modelData.windowId);
                                root.isOpen = false;
                            }
                        }

                        Column {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 6

                            // Card Header: Window Title + Close Button
                            Item {
                                width: parent.width
                                height: 18

                                Text {
                                    anchors.left: parent.left
                                    anchors.right: closeCardBtn.left
                                    anchors.rightMargin: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.title || root.targetAppName
                                    color: "#FFFFFF"
                                    font.pixelSize: 11
                                    font.bold: modelData.isActive
                                    elide: Text.ElideRight
                                }

                                Rectangle {
                                    id: closeCardBtn
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 18
                                    height: 18
                                    radius: 9
                                    color: closeCardMouse.containsMouse ? Qt.rgba(0.92, 0.28, 0.28, 0.85) : Qt.rgba(1, 1, 1, 0.12)

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✕"
                                        color: "#FFFFFF"
                                        font.pixelSize: 9
                                        font.bold: true
                                    }

                                    MouseArea {
                                        id: closeCardMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            root.closeWindowRequested(modelData.windowId);
                                        }
                                    }
                                }
                            }

                            // Stylized Miniature macOS Window Viewport
                            Rectangle {
                                width: parent.width
                                height: 76
                                radius: 6
                                color: Qt.rgba(0.06, 0.08, 0.11, modelData.isMinimized ? 0.55 : 0.85)
                                border.color: Qt.rgba(1, 1, 1, 0.12)
                                border.width: 1
                                clip: true

                                // Mini window titlebar with traffic lights
                                Rectangle {
                                    id: miniTitleBar
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    height: 14
                                    color: Qt.rgba(1, 1, 1, 0.07)

                                    Row {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 6
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 4

                                        Rectangle { width: 6; height: 6; radius: 3; color: "#FF5F56" }
                                        Rectangle { width: 6; height: 6; radius: 3; color: "#FFBD2E" }
                                        Rectangle { width: 6; height: 6; radius: 3; color: "#27C93F" }
                                    }
                                }

                                // Centered application icon inside miniature window
                                Image {
                                    anchors.centerIn: parent
                                    anchors.verticalCenterOffset: 5
                                    width: 34
                                    height: 34
                                    sourceSize.width: 68
                                    sourceSize.height: 68
                                    opacity: modelData.isMinimized ? 0.55 : 0.95
                                    source: (modelData.iconName && modelData.iconName.length > 0)
                                        ? ("image://icon/" + modelData.iconName)
                                        : (root.targetAppIcon.length > 0 ? ("image://icon/" + root.targetAppIcon) : "")
                                }

                                // Status badge in bottom-right of miniature window
                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.margins: 5
                                    width: statusText.implicitWidth + 10
                                    height: 15
                                    radius: 4
                                    color: modelData.isActive
                                        ? Qt.rgba(0.2, 0.65, 1.0, 0.35)
                                        : (modelData.isMinimized ? Qt.rgba(0.95, 0.65, 0.15, 0.30) : Qt.rgba(1, 1, 1, 0.10))

                                    Text {
                                        id: statusText
                                        anchors.centerIn: parent
                                        text: modelData.isActive ? "En foco" : (modelData.isMinimized ? "Minimizada" : "Abierta")
                                        color: modelData.isActive ? "#80D4FF" : (modelData.isMinimized ? "#FFD166" : "#CCCCCC")
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // =========================================================================
    // MODE 1: WINDOW LIST (Compact Vertical Glassmorphic List)
    // =========================================================================
    Rectangle {
        id: windowListContainer
        visible: root.displayMode === 1
        width: 300
        height: Math.min(280, listColumn.implicitHeight + 24)
        radius: 14

        x: Math.min(Math.max(12, root.targetX - width / 2), root.width - width - 12)
        y: Math.max(10, root.targetY - height - 14)

        color: Qt.rgba(0.10, 0.12, 0.16, 0.92)
        border.color: Qt.rgba(1, 1, 1, 0.24)
        border.width: 1

        MouseArea {
            anchors.fill: parent
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.45)
            border.width: 1
            z: -1
        }

        Column {
            id: listColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6

            // Header: App Name & Window Count
            Item {
                width: parent.width
                height: 22

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.targetAppName
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    width: parent.width - 80
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.windowsList.length + " instancias"
                    color: Qt.rgba(0.55, 0.82, 1.0, 0.9)
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.12)
            }

            // List of Window Instances
            ListView {
                width: parent.width
                height: Math.min(210, count * 38)
                clip: true
                spacing: 4
                model: root.windowsList

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    width: ListView.view.width
                    height: 34
                    radius: 7
                    color: rowMouse.containsMouse
                        ? Qt.rgba(0.25, 0.55, 0.95, 0.45)
                        : (modelData.isActive ? Qt.rgba(1, 1, 1, 0.10) : "transparent")

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.windowSelected(modelData.windowId);
                            root.isOpen = false;
                        }
                    }

                    // Active / Minimized Left Indicator Bar
                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: 16
                        radius: 1.5
                        color: modelData.isActive
                            ? "#4CC2FF"
                            : (modelData.isMinimized ? "#FFB84D" : Qt.rgba(1, 1, 1, 0.28))
                    }

                    Image {
                        id: rowIcon
                        anchors.left: parent.left
                        anchors.leftMargin: 15
                        anchors.verticalCenter: parent.verticalCenter
                        width: 18
                        height: 18
                        sourceSize.width: 36
                        sourceSize.height: 36
                        source: (modelData.iconName && modelData.iconName.length > 0)
                            ? ("image://icon/" + modelData.iconName)
                            : (root.targetAppIcon.length > 0 ? ("image://icon/" + root.targetAppIcon) : "")
                    }

                    Text {
                        anchors.left: rowIcon.right
                        anchors.leftMargin: 8
                        anchors.right: rowCloseBtn.left
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.title || root.targetAppName
                        color: modelData.isMinimized ? Qt.rgba(1, 1, 1, 0.65) : "#FFFFFF"
                        font.pixelSize: 12
                        font.bold: modelData.isActive
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        id: rowCloseBtn
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20
                        height: 20
                        radius: 10
                        color: rowCloseMouse.containsMouse ? Qt.rgba(0.92, 0.28, 0.28, 0.85) : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: rowCloseMouse.containsMouse ? "#FFFFFF" : Qt.rgba(1, 1, 1, 0.55)
                            font.pixelSize: 10
                            font.bold: true
                        }

                        MouseArea {
                            id: rowCloseMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                root.closeWindowRequested(modelData.windowId);
                            }
                        }
                    }
                }
            }
        }
    }
}
