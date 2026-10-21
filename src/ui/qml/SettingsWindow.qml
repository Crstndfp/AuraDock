import QtQuick
import QtQuick.Window

Window {
    id: root
    title: "Cachy Dock Preferences"
    width: 480
    height: 620
    color: "transparent"
    flags: Qt.Dialog | Qt.FramelessWindowHint

    visible: dockController.isSettingsOpen

    // Glassmorphic background container
    Rectangle {
        id: bgContainer
        anchors.fill: parent
        radius: 16
        color: Qt.rgba(0.12, 0.14, 0.18, 0.94)
        border.color: Qt.rgba(1, 1, 1, 0.22)
        border.width: 1

        // Outer drop shadow border
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: parent.radius + 1
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.5)
            border.width: 1
            z: -1
        }

        // Window drag header
        Item {
            id: headerBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 50

            DragHandler {
                target: null
                onActiveChanged: if (active) root.startSystemMove()
            }

            // Window title
            Column {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: "Configuración de Cachy Dock"
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 15
                }
                Text {
                    text: "Dimensiones, curvas y organizadores"
                    color: Qt.rgba(1, 1, 1, 0.55)
                    font.pixelSize: 11
                }
            }

            // Close button
            Rectangle {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                width: 26
                height: 26
                radius: 13
                color: closeMouse.containsMouse ? Qt.rgba(0.9, 0.3, 0.3, 0.7) : Qt.rgba(1, 1, 1, 0.1)

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: "#FFFFFF"
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: dockController.closeSettingsWindow()
                }
            }

            // Header bottom separator line
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Qt.rgba(1, 1, 1, 0.1)
            }
        }

        // Settings Content Column
        Column {
            anchors.top: headerBar.bottom
            anchors.topMargin: 16
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            spacing: 16

            // Control 1: Bottom Margin
            Column {
                width: parent.width
                spacing: 6

                Item {
                    width: parent.width
                    height: 18

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Margen inferior de la pantalla"
                        color: "#EEEEEE"
                        font.pixelSize: 12
                        font.bold: true
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: Math.round(dockController.bottomMargin) + " px"
                        color: Qt.rgba(0.4, 0.75, 1.0, 1.0)
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                // Custom Slider Bar
                Item {
                    width: parent.width
                    height: 22

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 6
                        radius: 3
                        color: Qt.rgba(1, 1, 1, 0.15)

                        Rectangle {
                            height: parent.height
                            radius: 3
                            color: Qt.rgba(0.3, 0.6, 1.0, 0.85)
                            width: Math.max(0, Math.min(parent.width, (dockController.bottomMargin - 0) / (50 - 0) * parent.width))
                        }
                    }

                    Rectangle {
                        id: handleMargin
                        width: 16
                        height: 16
                        radius: 8
                        color: "#FFFFFF"
                        x: Math.max(0, Math.min(parent.width - width, (dockController.bottomMargin - 0) / (50 - 0) * (parent.width - width)))
                        anchors.verticalCenter: parent.verticalCenter
                        border.color: Qt.rgba(0, 0, 0, 0.3)
                        border.width: 1
                    }

                    MouseArea {
                        anchors.fill: parent
                        function updateVal(mouseX) {
                            var ratio = Math.max(0, Math.min(1, mouseX / parent.width));
                            dockController.bottomMargin = Math.round(0 + ratio * 50);
                        }
                        onPressed: function(mouse) { updateVal(mouse.x); }
                        onPositionChanged: function(mouse) { if (pressed) updateVal(mouse.x); }
                    }
                }
            }

            // Control 2: Dock Bar Height
            Column {
                width: parent.width
                spacing: 6

                Item {
                    width: parent.width
                    height: 18

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Alto de la barra (Grosor vertical)"
                        color: "#EEEEEE"
                        font.pixelSize: 12
                        font.bold: true
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: Math.round(dockController.barHeight) + " px"
                        color: Qt.rgba(0.4, 0.75, 1.0, 1.0)
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Item {
                    width: parent.width
                    height: 22

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 6
                        radius: 3
                        color: Qt.rgba(1, 1, 1, 0.15)

                        Rectangle {
                            height: parent.height
                            radius: 3
                            color: Qt.rgba(0.3, 0.6, 1.0, 0.85)
                            width: Math.max(0, Math.min(parent.width, (dockController.barHeight - 48) / (110 - 48) * parent.width))
                        }
                    }

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: "#FFFFFF"
                        x: Math.max(0, Math.min(parent.width - width, (dockController.barHeight - 48) / (110 - 48) * (parent.width - width)))
                        anchors.verticalCenter: parent.verticalCenter
                        border.color: Qt.rgba(0, 0, 0, 0.3)
                        border.width: 1
                    }

                    MouseArea {
                        anchors.fill: parent
                        function updateVal(mouseX) {
                            var ratio = Math.max(0, Math.min(1, mouseX / parent.width));
                            dockController.barHeight = Math.round(48 + ratio * (110 - 48));
                        }
                        onPressed: function(mouse) { updateVal(mouse.x); }
                        onPositionChanged: function(mouse) { if (pressed) updateVal(mouse.x); }
                    }
                }
            }

            // Control 2B: Base Icon Size
            Column {
                width: parent.width
                spacing: 6

                Item {
                    width: parent.width
                    height: 18

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Tamaño de los iconos"
                        color: "#EEEEEE"
                        font.pixelSize: 12
                        font.bold: true
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: Math.round(dockController.iconSize) + " px"
                        color: Qt.rgba(0.4, 0.75, 1.0, 1.0)
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Item {
                    width: parent.width
                    height: 22

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 6
                        radius: 3
                        color: Qt.rgba(1, 1, 1, 0.15)

                        Rectangle {
                            height: parent.height
                            radius: 3
                            color: Qt.rgba(0.3, 0.6, 1.0, 0.85)
                            width: Math.max(0, Math.min(parent.width, (dockController.iconSize - 32) / (64 - 32) * parent.width))
                        }
                    }

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: "#FFFFFF"
                        x: Math.max(0, Math.min(parent.width - width, (dockController.iconSize - 32) / (64 - 32) * (parent.width - width)))
                        anchors.verticalCenter: parent.verticalCenter
                        border.color: Qt.rgba(0, 0, 0, 0.3)
                        border.width: 1
                    }

                    MouseArea {
                        anchors.fill: parent
                        function updateVal(mouseX) {
                            var ratio = Math.max(0, Math.min(1, mouseX / parent.width));
                            dockController.iconSize = Math.round(32 + ratio * (64 - 32));
                        }
                        onPressed: function(mouse) { updateVal(mouse.x); }
                        onPositionChanged: function(mouse) { if (pressed) updateVal(mouse.x); }
                    }
                }
            }

            // Control 3: Icon Spacing
            Column {
                width: parent.width
                spacing: 6

                Item {
                    width: parent.width
                    height: 18

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Separación entre iconos"
                        color: "#EEEEEE"
                        font.pixelSize: 12
                        font.bold: true
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: Math.round(dockController.itemSpacing) + " px"
                        color: Qt.rgba(0.4, 0.75, 1.0, 1.0)
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Item {
                    width: parent.width
                    height: 22

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        height: 6
                        radius: 3
                        color: Qt.rgba(1, 1, 1, 0.15)

                        Rectangle {
                            height: parent.height
                            radius: 3
                            color: Qt.rgba(0.3, 0.6, 1.0, 0.85)
                            width: Math.max(0, Math.min(parent.width, (dockController.itemSpacing - 2) / (32 - 2) * parent.width))
                        }
                    }

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: "#FFFFFF"
                        x: Math.max(0, Math.min(parent.width - width, (dockController.itemSpacing - 2) / (32 - 2) * (parent.width - width)))
                        anchors.verticalCenter: parent.verticalCenter
                        border.color: Qt.rgba(0, 0, 0, 0.3)
                        border.width: 1
                    }

                    MouseArea {
                        anchors.fill: parent
                        function updateVal(mouseX) {
                            var ratio = Math.max(0, Math.min(1, mouseX / parent.width));
                            dockController.itemSpacing = Math.round(2 + ratio * (32 - 2));
                        }
                        onPressed: function(mouse) { updateVal(mouse.x); }
                        onPositionChanged: function(mouse) { if (pressed) updateVal(mouse.x); }
                    }
                }
            }

            // Control 4: Corner Radius & Auto Pill toggle
            Column {
                width: parent.width
                spacing: 8

                Item {
                    width: parent.width
                    height: 22

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Forma de píldora redondeada automática"
                        color: "#EEEEEE"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    // Toggle switch
                    Rectangle {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: 38
                        height: 20
                        radius: 10
                        color: dockController.autoCornerRadius ? Qt.rgba(0.3, 0.6, 1.0, 0.85) : Qt.rgba(1, 1, 1, 0.2)

                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            anchors.verticalCenter: parent.verticalCenter
                            x: dockController.autoCornerRadius ? parent.width - width - 2 : 2
                            color: "#FFFFFF"
                            Behavior on x { NumberAnimation { duration: 120 } }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: dockController.autoCornerRadius = !dockController.autoCornerRadius
                        }
                    }
                }

                // Custom corner radius slider (only active when autoCornerRadius is false)
                Column {
                    width: parent.width
                    opacity: dockController.autoCornerRadius ? 0.35 : 1.0
                    enabled: !dockController.autoCornerRadius
                    spacing: 6

                    Item {
                        width: parent.width
                        height: 18

                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Radio de esquinas personalizado"
                            color: "#CCCCCC"
                            font.pixelSize: 11
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: Math.round(dockController.cornerRadius) + " px"
                            color: Qt.rgba(0.4, 0.75, 1.0, 1.0)
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Item {
                        width: parent.width
                        height: 22

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            height: 6
                            radius: 3
                            color: Qt.rgba(1, 1, 1, 0.15)

                            Rectangle {
                                height: parent.height
                                radius: 3
                                color: Qt.rgba(0.3, 0.6, 1.0, 0.85)
                                width: Math.max(0, Math.min(parent.width, (dockController.cornerRadius - 0) / (48 - 0) * parent.width))
                            }
                        }

                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            color: "#FFFFFF"
                            x: Math.max(0, Math.min(parent.width - width, (dockController.cornerRadius - 0) / (48 - 0) * (parent.width - width)))
                            anchors.verticalCenter: parent.verticalCenter
                            border.color: Qt.rgba(0, 0, 0, 0.3)
                            border.width: 1
                        }

                        MouseArea {
                            anchors.fill: parent
                            function updateVal(mouseX) {
                                var ratio = Math.max(0, Math.min(1, mouseX / parent.width));
                                dockController.cornerRadius = Math.round(0 + ratio * 48);
                            }
                            onPressed: function(mouse) { updateVal(mouse.x); }
                            onPositionChanged: function(mouse) { if (pressed) updateVal(mouse.x); }
                        }
                    }
                }
            }

            // Divider line
            Rectangle {
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.1)
            }

            // Organization: Separator lines and Reset
            Row {
                width: parent.width
                spacing: 12

                // Add Separator Button
                Rectangle {
                    width: (parent.width - 12) / 2
                    height: 34
                    radius: 8
                    color: addSepMouse.containsMouse ? Qt.rgba(0.3, 0.6, 1.0, 0.4) : Qt.rgba(1, 1, 1, 0.1)
                    border.color: Qt.rgba(1, 1, 1, 0.2)
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "+ Añadir Separador"
                        color: "#FFFFFF"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    MouseArea {
                        id: addSepMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: dockController.insertSeparator(-1)
                    }
                }

                // Reset to Defaults Button
                Rectangle {
                    width: (parent.width - 12) / 2
                    height: 34
                    radius: 8
                    color: resetMouse.containsMouse ? Qt.rgba(0.9, 0.3, 0.3, 0.35) : Qt.rgba(1, 1, 1, 0.08)
                    border.color: Qt.rgba(1, 1, 1, 0.15)
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Restablecer Valores"
                        color: Qt.rgba(1, 1, 1, 0.85)
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: resetMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: dockController.resetSettingsToDefaults()
                    }
                }
            }
        }
    }
}
