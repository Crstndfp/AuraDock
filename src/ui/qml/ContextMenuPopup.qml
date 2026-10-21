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
    property int targetItemType: 0

    property real targetX: 0
    property real targetY: 0

    signal openNewInstance(int index)
    signal toggleKeepInDock(int index)
    signal launchOnDiscreteGpu(int index)
    signal closeAllWindows(int index)
    signal forceQuit(int index)
    signal insertSeparator(int index)
    signal removeSeparator(int index)
    signal openSettings()

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

            // Option 1: Open New Window / Instance (non-separator only)
            Rectangle {
                visible: root.targetItemType !== 3
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

            // Option 2: Keep in Dock (non-separator only)
            Rectangle {
                visible: root.targetItemType !== 3
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

            // Option 3: Launch with Discrete GPU (non-separator only)
            Rectangle {
                visible: root.targetItemType !== 3
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

            // Option 4: Close All Windows (non-separator only)
            Rectangle {
                visible: root.targetItemType !== 3 && root.targetItemIsRunning
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

            // Option 5: Remove this separator (if separator)
            Rectangle {
                visible: root.targetItemType === 3
                width: parent.width
                height: 28
                radius: 6
                color: optRemoveSepMouse.containsMouse ? Qt.rgba(0.9, 0.25, 0.25, 0.65) : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Eliminar este separador"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                }
                MouseArea {
                    id: optRemoveSepMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.removeSeparator(root.targetItemIndex);
                        root.isOpen = false;
                    }
                }
            }

            // Section divider
            Rectangle {
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.12)
            }

            // Option 6: Insert separator to the right
            Rectangle {
                width: parent.width
                height: 28
                radius: 6
                color: optInsertSepMouse.containsMouse ? Qt.rgba(0.2, 0.5, 0.9, 0.65) : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Insertar separador a la derecha"
                    color: Qt.rgba(0.9, 0.9, 0.95, 0.9)
                    font.pixelSize: 12
                }
                MouseArea {
                    id: optInsertSepMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.insertSeparator(root.targetItemIndex);
                        root.isOpen = false;
                    }
                }
            }

            // Option 7: Open Dock Settings
            Rectangle {
                width: parent.width
                height: 28
                radius: 6
                color: optSettingsMouse.containsMouse ? Qt.rgba(0.2, 0.5, 0.9, 0.65) : "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Configuración del Dock..."
                    color: Qt.rgba(0.9, 0.9, 0.95, 0.9)
                    font.pixelSize: 12
                }
                MouseArea {
                    id: optSettingsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.openSettings();
                        root.isOpen = false;
                    }
                }
            }
        }
    }
}
