import QtQuick

Item {
    id: root

    property bool isOpen: false
    property int targetItemIndex: -1
    property string targetItemName: ""
    property string targetItemIcon: ""
    property bool targetItemIsRunning: false
    property bool targetItemIsPinned: true
    property int targetRunningWindowCount: 0

    property real targetX: 0
    property real targetY: 0

    signal openNewInstance(int index)
    signal toggleKeepInDock(int index)
    signal launchOnDiscreteGpu(int index)
    signal closeAllWindows(int index)
    signal forceQuit(int index)

    visible: isOpen
    opacity: isOpen ? 1.0 : 0.0

    Behavior on opacity {
        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
    }

    // Dismiss overlay to close popup when clicking outside
    MouseArea {
        anchors.fill: parent
        onClicked: root.isOpen = false
    }

    // Glassmorphic Menu Container
    Rectangle {
        id: menuPill
        width: 220
        height: menuColumn.implicitHeight + 20
        radius: 14

        x: Math.min(Math.max(12, root.targetX - width / 2), root.width - width - 12)
        y: Math.max(10, root.targetY - height - 16)

        color: Qt.rgba(0.12, 0.14, 0.18, 0.88)
        border.color: Qt.rgba(1, 1, 1, 0.22)
        border.width: 1

        // Drop shadow outline
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.4)
            border.width: 1
            z: -1
        }

        Column {
            id: menuColumn
            anchors.centerIn: parent
            width: parent.width - 16
            spacing: 4

            // Header: App Title
            Item {
                width: parent.width
                height: 28
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.targetItemName
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    width: parent.width - 16
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.12)
            }

            // Option 1: Open New Window / Instance
            Rectangle {
                width: parent.width
                height: 28
                radius: 6
                color: optNewInstanceMouse.containsMouse ? Qt.rgba(0.2, 0.5, 0.9, 0.65) : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Open New Window"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                }
                MouseArea {
                    id: optNewInstanceMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.openNewInstance(root.targetItemIndex);
                        root.isOpen = false;
                    }
                }
            }

            // Option 2: Keep in Dock
            Rectangle {
                width: parent.width
                height: 28
                radius: 6
                color: optKeepMouse.containsMouse ? Qt.rgba(0.2, 0.5, 0.9, 0.65) : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.targetItemIsPinned ? "✓  Keep in Dock" : "    Keep in Dock"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                }
                MouseArea {
                    id: optKeepMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.toggleKeepInDock(root.targetItemIndex);
                        root.isOpen = false;
                    }
                }
            }

            // Option 3: Launch with Discrete GPU
            Rectangle {
                width: parent.width
                height: 28
                radius: 6
                color: optGpuMouse.containsMouse ? Qt.rgba(0.2, 0.5, 0.9, 0.65) : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Run with Discrete GPU"
                    color: Qt.rgba(0.9, 0.9, 0.95, 0.85)
                    font.pixelSize: 12
                }
                MouseArea {
                    id: optGpuMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.launchOnDiscreteGpu(root.targetItemIndex);
                        root.isOpen = false;
                    }
                }
            }

            // Section divider
            Rectangle {
                visible: root.targetItemIsRunning
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.12)
            }

            // Option 4: Close All Windows
            Rectangle {
                visible: root.targetItemIsRunning
                width: parent.width
                height: 28
                radius: 6
                color: optCloseMouse.containsMouse ? Qt.rgba(0.9, 0.25, 0.25, 0.65) : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Quit"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                }
                MouseArea {
                    id: optCloseMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.closeAllWindows(root.targetItemIndex);
                        root.isOpen = false;
                    }
                }
            }
        }
    }
}
