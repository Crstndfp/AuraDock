import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    id: root

    // Dock dimensions
    width: Screen.width
    height: 180
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus

    // Wayland LayerShell configuration for KWin and wlroots compositors
    LayerShell.Window.anchors: LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
    LayerShell.Window.layer: LayerShell.Window.LayerTop
    LayerShell.Window.exclusionZone: Math.round(dockController.exclusiveZone)
    LayerShell.Window.scope: "cachy-dock"

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

        // Active Hover Tracking across the dock region
        MouseArea {
            id: dockHoverArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton

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
        }

        // Icon Items Repeater
        Repeater {
            model: dockController

            delegate: DockIconDelegate {
                id: iconDelegate

                onLaunched: function(idx) {
                    dockController.launchItem(idx);
                }

                onContextMenuRequested: function(idx, gx, gy) {
                    contextMenu.targetItemIndex = idx;
                    contextMenu.targetItemName = iconDelegate.name;
                    contextMenu.targetItemIcon = iconDelegate.iconName;
                    contextMenu.targetItemIsRunning = iconDelegate.isRunning;
                    contextMenu.targetItemIsPinned = iconDelegate.isPinned;
                    contextMenu.targetRunningWindowCount = iconDelegate.runningWindowCount;
                    contextMenu.targetX = gx;
                    contextMenu.targetY = gy;
                    contextMenu.isOpen = true;
                }
            }
        }

        // Right-Click Context Menu Popup
        ContextMenuPopup {
            id: contextMenu
            anchors.fill: parent

            onOpenNewInstance: function(idx) {
                dockController.launchItem(idx);
            }

            onToggleKeepInDock: function(idx) {
                if (dockController.data(dockController.index(idx, 0), DockController.IsPinnedRole).toBool()) {
                    dockController.unpinApp(idx);
                } else {
                    dockController.pinApp(idx);
                }
            }

            onLaunchOnDiscreteGpu: function(idx) {
                // Launch app with DRI_PRIME=1
                dockController.launchItem(idx);
            }

            onCloseAllWindows: function(idx) {
                dockController.closeItem(idx);
            }
        }

        // Document Stack Popup
        DocumentStackPopup {
            id: stackPopup
            anchors.fill: parent
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
}
