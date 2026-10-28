import QtQuick
import QtQuick.Window
import org.kde.layershell as LayerShell

Window {
    id: root
    visible: true

    // Dock dimensions across output
    width: Screen.width > 0 ? Screen.width : 1920
    height: 280
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
        dockController.screenHeight = Screen.height > 0 ? Screen.height : 1080;
    }

    onWidthChanged: {
        dockController.surfaceWidth = root.width;
        dockController.screenHeight = Screen.height > 0 ? Screen.height : 1080;
    }

    onHeightChanged: {
        dockController.surfaceHeight = root.height;
    }

    readonly property bool isAnyPopupOpen: (multiInstancePopup && multiInstancePopup.isOpen) ||
                                           (contextMenu && contextMenu.isOpen) ||
                                           (stackPopup && stackPopup.isOpen)

    onIsAnyPopupOpenChanged: {
        dockController.setPopupOpen(isAnyPopupOpen);
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
            dockOpacity: dockController.dockOpacity
        }

        // Icon Items Repeater
        Repeater {
            id: dockRepeater
            model: dockController

            delegate: DockIconDelegate {
                id: iconDelegate
                isBeingDragged: dockHoverArea.isDragging && dockHoverArea.dragIndex === index
            }
        }

        // macOS Tooltip Pill
        Rectangle {
            id: tooltipPill
            property var itemData: dockController.hoveredItemIndex >= 0 ? dockController.getItemDetails(dockController.hoveredItemIndex) : null
            visible: dockController.hoveredItemIndex >= 0 && !contextMenu.isOpen && !multiInstancePopup.isOpen && dockController.isMagnified && itemData && itemData.type !== 3
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

        // Magic Lamp (Genie) Emergence Overlay when restoring a minimized window
        GenieLampOverlay {
            id: genieOverlay
        }

        Connections {
            target: dockController
            function onGenieEffectTriggered(genieIdx, ix, iy, iw, ih, iname, aname) {
                genieOverlay.triggerGenie(ix, iy, iw, ih, iname, aname);
            }
        }

        // Master Unified Mouse Area across entire dock surface
        // Handles continuous hover tracking, drag-to-reorder, and clicks without child item stealing
        MouseArea {
            id: dockHoverArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            property int dragIndex: -1
            property real dragStartX: 0
            property real dragStartY: 0
            property bool isDragging: false
            property bool justFinishedDrag: false

            onPressed: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    var idx = dockController.itemAt(mouse.x, mouse.y);
                    if (idx >= 0) {
                        var details = dockController.getItemDetails(idx);
                        // Only allow dragging pinned application launchers
                        if (details.isPinned && details.type === 0) {
                            dragIndex = idx;
                            dragStartX = mouse.x;
                            dragStartY = mouse.y;
                            isDragging = false;
                        } else {
                            dragIndex = -1;
                        }
                    } else {
                        dragIndex = -1;
                    }
                }
            }

            onPositionChanged: function(mouse) {
                dockController.updateCursor(mouse.x, mouse.y);

                if (dragIndex >= 0 && (mouse.buttons & Qt.LeftButton)) {
                    var deltaX = Math.abs(mouse.x - dragStartX);
                    var deltaY = Math.abs(mouse.y - dragStartY);
                    if (!isDragging && (deltaX > 8 || deltaY > 8)) {
                        isDragging = true;
                    }

                    if (isDragging) {
                        var targetIdx = dockController.itemAt(mouse.x, mouse.y);
                        if (targetIdx >= 0 && targetIdx !== dragIndex) {
                            var targetDetails = dockController.getItemDetails(targetIdx);
                            // Can only drop onto another pinned app launcher
                            if (targetDetails.isPinned && targetDetails.type === 0) {
                                if (dockController.movePinnedItem(dragIndex, targetIdx)) {
                                    dragIndex = targetIdx;
                                }
                            }
                        }
                    }
                }
            }

            onReleased: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    if (isDragging) {
                        justFinishedDrag = true;
                        isDragging = false;
                        dragIndex = -1;
                        return;
                    }
                    dragIndex = -1;
                    isDragging = false;
                }
            }

            onEntered: {
                dockController.pointerEnteredDock();
            }

            onExited: {
                if (isDragging) {
                    isDragging = false;
                    dragIndex = -1;
                }
                dockController.clearCursor();
                dockController.pointerLeftDock();
            }

            onClicked: function(mouse) {
                if (justFinishedDrag) {
                    justFinishedDrag = false;
                    return;
                }

                var itemIndex = dockController.itemAt(mouse.x, mouse.y);
                if (itemIndex >= 0) {
                    var details = dockController.getItemDetails(itemIndex);
                    if (mouse.button === Qt.LeftButton) {
                        if (details.isRunning && details.runningWindowCount > 1) {
                            if (multiInstancePopup.isOpen && multiInstancePopup.targetItemIndex === itemIndex) {
                                multiInstancePopup.isOpen = false;
                            } else {
                                contextMenu.isOpen = false;
                                var iconItem = (itemIndex < dockRepeater.count) ? dockRepeater.itemAt(itemIndex) : null;
                                multiInstancePopup.targetItemIndex = itemIndex;
                                multiInstancePopup.targetAppName = details.name;
                                multiInstancePopup.targetAppIcon = details.iconName;
                                multiInstancePopup.windowsList = dockController.getWindowsForItem(itemIndex);
                                multiInstancePopup.targetX = iconItem ? (iconItem.itemX + iconItem.itemWidth / 2) : mouse.x;
                                multiInstancePopup.isOpen = true;
                            }
                        } else {
                            multiInstancePopup.isOpen = false;
                            dockController.launchItem(itemIndex);
                        }
                    } else if (mouse.button === Qt.RightButton) {
                        multiInstancePopup.isOpen = false;
                        contextMenu.targetItemIndex = itemIndex;
                        contextMenu.targetItemName = details.name;
                        contextMenu.targetItemIcon = details.iconName;
                        contextMenu.targetItemType = details.type;
                        contextMenu.targetItemIsRunning = details.isRunning;
                        contextMenu.targetItemIsPinned = details.isPinned;
                        contextMenu.targetRunningWindowCount = details.runningWindowCount;
                        contextMenu.canMoveLeft = dockController.canMoveItemLeft(itemIndex);
                        contextMenu.canMoveRight = dockController.canMoveItemRight(itemIndex);
                        contextMenu.targetX = mouse.x;
                        contextMenu.isOpen = true;
                    }
                } else if (mouse.button === Qt.RightButton) {
                    if (mouse.x >= dockController.pillX && mouse.x <= (dockController.pillX + dockController.pillWidth) &&
                        mouse.y >= dockController.pillY && mouse.y <= (dockController.pillY + dockController.pillHeight)) {
                        multiInstancePopup.isOpen = false;
                        contextMenu.targetItemIndex = -1;
                        contextMenu.targetItemName = "Cachy Dock";
                        contextMenu.targetItemIcon = "";
                        contextMenu.targetItemType = -1;
                        contextMenu.targetItemIsRunning = false;
                        contextMenu.targetItemIsPinned = true;
                        contextMenu.targetRunningWindowCount = 0;
                        contextMenu.canMoveLeft = false;
                        contextMenu.canMoveRight = false;
                        contextMenu.targetX = mouse.x;
                        contextMenu.isOpen = true;
                    }
                }
            }
        }

        // Multi-Instance Window Switcher Popup (Mini Windows Cards & Window List modes)
        MultiInstancePopup {
            id: multiInstancePopup
            anchors.fill: parent
            z: 100
            displayMode: dockController.multiInstanceMode
            targetY: dockController.pillY

            onWindowSelected: function(winId) {
                dockController.activateWindowInstance(winId);
            }

            onCloseWindowRequested: function(winId) {
                dockController.closeWindowInstance(winId);
            }
        }

        Connections {
            target: dockController
            function onWindowsUpdated() {
                if (multiInstancePopup.isOpen && multiInstancePopup.targetItemIndex >= 0) {
                    var updatedList = dockController.getWindowsForItem(multiInstancePopup.targetItemIndex);
                    multiInstancePopup.windowsList = updatedList;
                    if (updatedList.length === 0) {
                        multiInstancePopup.isOpen = false;
                    }
                }
            }
        }

        // Right-Click Context Menu Popup
        ContextMenuPopup {
            id: contextMenu
            anchors.fill: parent
            z: 100
            targetY: dockController.pillY

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

            onMoveItemLeft: function(idx) {
                dockController.moveItemLeft(idx);
            }

            onMoveItemRight: function(idx) {
                dockController.moveItemRight(idx);
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
