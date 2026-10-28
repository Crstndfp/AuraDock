import QtQuick

Item {
    id: root

    required property int index
    required property string itemId
    required property string name
    required property string iconName
    required property int itemType
    required property bool isPinned
    required property bool isRunning
    required property bool isFocused
    required property int runningWindowCount
    required property real bounceOffset
    required property real itemScale
    required property real itemX
    required property real itemY
    required property real itemWidth
    required property real itemHeight

    property bool isBeingDragged: false

    x: itemX
    y: itemY
    width: itemWidth
    height: itemHeight
    scale: isBeingDragged ? 1.12 : 1.0
    z: isBeingDragged ? 100 : 1

    // Smooth temporal interpolation across frames
    Behavior on scale {
        NumberAnimation { duration: 120; easing.type: Easing.OutBack }
    }
    Behavior on x {
        NumberAnimation { duration: 50; easing.type: Easing.OutQuad }
    }
    Behavior on y {
        NumberAnimation { duration: 50; easing.type: Easing.OutQuad }
    }
    Behavior on width {
        NumberAnimation { duration: 50; easing.type: Easing.OutQuad }
    }
    Behavior on height {
        NumberAnimation { duration: 50; easing.type: Easing.OutQuad }
    }

    // Check if separator type
    readonly property bool isSeparator: itemType === 3

    // Separator line
    Rectangle {
        visible: root.isSeparator
        anchors.fill: parent
        color: Qt.rgba(1, 1, 1, 0.25)
    }

    // Application Icon Container
    Item {
        id: iconContainer
        visible: !root.isSeparator
        anchors.fill: parent

        transform: [
            Scale {
                id: lampScale
                origin.x: iconContainer.width * 0.5
                origin.y: iconContainer.height
                xScale: 1.0
                yScale: 1.0
            },
            Translate {
                id: lampTranslate
                y: 0.0
            }
        ]

        Connections {
            target: dockController
            function onGenieEffectTriggered(genieIdx, ix, iy, iw, ih, iname, aname) {
                if (genieIdx === root.index) {
                    genieLampAnim.restart();
                }
            }
        }

        SequentialAnimation {
            id: genieLampAnim

            // Phase 1: Magic lamp compresses at base
            ParallelAnimation {
                NumberAnimation { target: lampScale; property: "xScale"; to: 1.30; duration: 85; easing.type: Easing.OutQuad }
                NumberAnimation { target: lampScale; property: "yScale"; to: 0.68; duration: 85; easing.type: Easing.OutQuad }
                NumberAnimation { target: lampTranslate; property: "y"; to: 4.0; duration: 85; easing.type: Easing.OutQuad }
            }

            // Phase 2: Genie erupts upward out of the lamp spout
            ParallelAnimation {
                NumberAnimation { target: lampScale; property: "xScale"; to: 0.76; duration: 150; easing.type: Easing.OutCubic }
                NumberAnimation { target: lampScale; property: "yScale"; to: 1.34; duration: 150; easing.type: Easing.OutCubic }
                NumberAnimation { target: lampTranslate; property: "y"; to: -10.0; duration: 150; easing.type: Easing.OutCubic }
            }

            // Phase 3: Elastic settle back to resting proportions
            ParallelAnimation {
                NumberAnimation { target: lampScale; property: "xScale"; to: 1.0; duration: 220; easing.type: Easing.OutElastic }
                NumberAnimation { target: lampScale; property: "yScale"; to: 1.0; duration: 220; easing.type: Easing.OutElastic }
                NumberAnimation { target: lampTranslate; property: "y"; to: 0.0; duration: 220; easing.type: Easing.OutElastic }
            }
        }

        Image {
            id: iconImage
            anchors.fill: parent
            anchors.margins: Math.max(2, parent.width * 0.04)
            source: root.iconName.length > 0 ? ("image://icon/" + root.iconName) : ""
            sourceSize.width: 128
            sourceSize.height: 128
            smooth: true
            mipmap: true
            asynchronous: false
            fillMode: Image.PreserveAspectFit

            // Fallback generic placeholder if icon not found
            Rectangle {
                visible: iconImage.status !== Image.Ready
                anchors.fill: parent
                radius: width * 0.22
                color: Qt.rgba(0.25, 0.32, 0.45, 0.8)
                border.color: Qt.rgba(1, 1, 1, 0.3)
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: root.name.length > 0 ? root.name.charAt(0).toUpperCase() : "?"
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: parent.width * 0.45
                }
            }
        }

        // macOS Status Indicator Dot / Pill underneath icon
        Rectangle {
            id: statusDot
            visible: root.isRunning
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -6

            width: root.isFocused ? 12 : 4
            height: 4
            radius: 2

            color: root.isFocused ? "#FFFFFF" : Qt.rgba(0.85, 0.90, 0.98, 0.75)

            // Smooth morphing between unfocused dot and focused capsule
            Behavior on width {
                NumberAnimation { duration: 180; easing.type: Easing.OutBack }
            }
            Behavior on color {
                ColorAnimation { duration: 160 }
            }

            // Subtle luminous drop glow
            Rectangle {
                anchors.centerIn: parent
                width: parent.width + 4
                height: parent.height + 4
                radius: parent.radius + 2
                color: Qt.rgba(1, 1, 1, 0.3)
                z: -1
                visible: root.isFocused
            }
        }
    }
}
