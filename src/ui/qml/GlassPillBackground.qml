import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property real pillX: 0
    property real pillY: 0
    property real pillWidth: 0
    property real pillHeight: 0
    property real pillRadius: pillHeight / 2
    property real dockOpacity: 0.42

    x: pillX
    y: pillY
    width: pillWidth
    height: pillHeight

    // Smooth temporal animation on container expansion
    Behavior on x {
        NumberAnimation { duration: 50; easing.type: Easing.OutQuad }
    }
    Behavior on width {
        NumberAnimation { duration: 50; easing.type: Easing.OutQuad }
    }

    // Outer drop shadow & ambient glow
    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        radius: root.pillRadius + 1
        color: "transparent"
        border.color: Qt.rgba(0, 0, 0, 0.45)
        border.width: 1
        opacity: 0.8
    }

    // Frosted glass base container (KWin handles the real-time background blur behind this)
    Rectangle {
        id: glassBase
        anchors.fill: parent
        radius: root.pillRadius
        // Deep translucent tinted glass with dynamic opacity level
        color: Qt.rgba(0.08, 0.10, 0.14, root.dockOpacity)
        clip: true

        Behavior on color {
            ColorAnimation { duration: 100 }
        }

        // Inner specular top-light gradient (simulates refraction on beveled glass rim)
        Rectangle {
            anchors.fill: parent
            radius: root.pillRadius
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.28)

            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.35) }
                GradientStop { position: 0.4; color: Qt.rgba(1, 1, 1, 0.12) }
                GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.04) }
            }
        }

        // Horizontal subtle highlight reflection bar across the top half of the pill
        Rectangle {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.88
            height: parent.height * 0.45
            radius: Math.min(height / 2, root.pillRadius)
            opacity: 0.06
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }
}
