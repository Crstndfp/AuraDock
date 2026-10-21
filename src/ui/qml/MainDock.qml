import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    id: root
    visible: true

    // Dock dimensions across output
    width: Screen.width > 0 ? Screen.width : 1920
    height: 180
    color: "transparent"
    flags: Qt.FramelessWindowHint

    // Wayland LayerShell configuration for KWin and wlroots compositors
    LayerShell.Window.anchors: LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerTop
    LayerShell.Window.exclusionZone: Math.round(dockController.exclusiveZone)
    LayerShell.Window.scope: "cachy-dock"
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone

    Component.onCompleted: {
        dockController.surfaceWidth = root.width;
        dockController.surfaceHeight = root.height;
    }

    onWidthChanged: {
        dockController.surfaceWidth = root.width;
    }

    // Slide transform for Smart Auto-Hide
    Item {
        id: dockTransformContainer
        anchors.fill: parent

        y: dockController.targetYOffset

        Behavior on y {
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
                onRunningChanged: {
                    if (!running) {
                        if (dockTransformContainer.y > 0) {
                            dockController.onHideAnimationFinished();
                        } else {
                            dockController.onRevealAnimationFinished();
                        }
                    }
                }
            }
        }

        // Frosted Glass Pill
        GlassPillBackground {
            id: glassPill
            pillX: dockController.pillX
            pillY: dockController.pillY
            pillWidth: dockController.pillWidth
            pillHeight: dockController.pillHeight
            pillRadius: dockController.pillRadius
        }

        // Icon Items Repeater
        Repeater {
            id: dockRepeater
            model: dockController

            delegate: DockIconDelegate {
                id: iconDelegate
            }
        }

        // macOS Tooltip Pill
        Rectangle {
            id: tooltipPill
            property var itemData: dockController.hoveredItemIndex >= 0 ? dockController.getItemDetails(dockController.hoveredItemIndex) : null
            visible: dockController.hoveredItemIndex >= 0 && !contextMenu.isOpen && dockController.isMagnified && itemData && itemData.type !== 3
            property var iconTransform: (dockController.hoveredItemIndex >= 0 && dockController.hoveredItemIndex < dockRepeater.count) ? dockRepeater.itemAt(dockController.hoveredItemIndex) : null

            x: iconTransform ? (iconTransform.itemX + iconTransform.itemWidth / 2 - width / 2) : 0
            y: iconTransform ? (iconTransform.itemY - height - 8) : 0
            width: tooltipText.implicitWidth + 18
            height: 24
            radius: 6
            color: Qt.rgba(0.12, 0.14, 0.18, 0.88)
            border.color: Qt.rgba(1, 1, 1, 0.22)
            border.width: 1

            Text {
                id: tooltipText
                anchors.centerIn: parent
                text: tooltipPill.itemData ? tooltipPill.itemData.name : ""
                color: "#FFFFFF"
                font.pixelSize: 11
                font.bold: true
            }
        }

        // Master Unified Mouse Area across entire dock surface
        // Handles continuous hover tracking and clicks without child item stealing
        MouseArea {
            id: dockHoverArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onPositionChanged: function(mouse) {
                dockController.updateCursor(mouse.x, mouse.y);
            }

            onEntered: {
                dockController.pointerEnteredDock();
            }

            onExited: {
                dockController.clearCursor();
                dockController.pointerLeftDock();
            }

            onClicked: function(mouse) {
                var itemIndex = dockController.itemAt(mouse.x, mouse.y);
                if (itemIndex >= 0) {
                    if (mouse.button === Qt.LeftButton) {
                        dockController.launchItem(itemIndex);
                    } else if (mouse.button === Qt.RightButton) {
                        var details = dockController.getItemDetails(itemIndex);
                        contextMenu.targetItemIndex = itemIndex;
                        contextMenu.targetItemName = details.name;
                        contextMenu.targetItemIcon = details.iconName;
                        contextMenu.targetItemType = details.type;
                        contextMenu.targetItemIsRunning = details.isRunning;
                        contextMenu.targetItemIsPinned = details.isPinned;
                        contextMenu.targetRunningWindowCount = details.runningWindowCount;
                        contextMenu.targetX = mouse.x;
                        contextMenu.targetY = mouse.y;
                        contextMenu.isOpen = true;
                    }
                } else if (mouse.button === Qt.RightButton) {
                    if (mouse.x >= dockController.pillX && mouse.x <= (dockController.pillX + dockController.pillWidth) &&
                        mouse.y >= dockController.pillY && mouse.y <= (dockController.pillY + dockController.pillHeight)) {
                        contextMenu.targetItemIndex = -1;
                        contextMenu.targetItemName = "Cachy Dock";
                        contextMenu.targetItemIcon = "";
                        contextMenu.targetItemType = -1;
                        contextMenu.targetItemIsRunning = false;
                        contextMenu.targetItemIsPinned = true;
                        contextMenu.targetRunningWindowCount = 0;
                        contextMenu.targetX = mouse.x;
                        contextMenu.targetY = mouse.y;
                        contextMenu.isOpen = true;
                    }
                }
            }
        }

        // Right-Click Context Menu Popup
        ContextMenuPopup {
            id: contextMenu
            anchors.fill: parent
            z: 100

            onOpenNewInstance: function(idx) {
                dockController.launchItem(idx);
            }

            onToggleKeepInDock: function(idx) {
                var details = dockController.getItemDetails(idx);
                if (details.isPinned) {
                    dockController.unpinApp(idx);
                } else {
                    dockController.pinApp(idx);
                }
            }

            onLaunchOnDiscreteGpu: function(idx) {
                dockController.launchItem(idx);
            }

            onCloseAllWindows: function(idx) {
                dockController.closeItem(idx);
            }

            onInsertSeparator: function(idx) {
                dockController.insertSeparator(idx);
            }

            onRemoveSeparator: function(idx) {
                dockController.removeSeparator(idx);
            }

            onOpenSettings: {
                dockController.openSettingsWindow();
            }
        }

        // Document Stack Popup
        DocumentStackPopup {
            id: stackPopup
            anchors.fill: parent
            z: 100
        }
    }

    // Smart Auto-Hide Bottom Edge Sensor (2px strip at screen edge when hidden)
    MouseArea {
        id: edgeSensorArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 4
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        visible: dockController.isSensorActive

        Timer {
            id: dwellTimer
            interval: 100
            repeat: false
            onTriggered: {
                if (edgeSensorArea.containsMouse) {
                    dockController.pointerEnteredSensor();
                }
            }
        }

        onEntered: {
            dwellTimer.restart();
        }

        onExited: {
            dwellTimer.stop();
            dockController.pointerLeftSensor();
        }
    }

    // Standalone Settings Window
    SettingsWindow {
        id: settingsWindow
    }
}
