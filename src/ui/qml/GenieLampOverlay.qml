import QtQuick

Item {
    id: root
    anchors.fill: parent
    visible: genieAnim.running || progress > 0.001 && progress < 0.999
    z: 95

    property real progress: 0.0
    property real originX: 0.0
    property real originY: 0.0
    property real originWidth: 48.0
    property real originHeight: 48.0
    property string activeIconName: ""
    property string activeAppName: ""

    function triggerGenie(ix, iy, iw, ih, iconName, appName) {
        originX = ix;
        originY = iy;
        originWidth = iw > 0 ? iw : 48;
        originHeight = ih > 0 ? ih : 48;
        activeIconName = iconName;
        activeAppName = appName;
        progress = 0.0;
        genieAnim.restart();
    }

    NumberAnimation {
        id: genieAnim
        target: root
        property: "progress"
        from: 0.0
        to: 1.0
        duration: 440
        easing.type: Easing.OutCubic
        onStopped: {
            genieCanvas.requestPaint();
        }
    }

    onProgressChanged: {
        genieCanvas.requestPaint();
    }

    // S-curve width profile function: narrow genie neck near the lamp (s=0), flaring wide at the top (s=1)
    function genieHalfWidthAt(s, spoutHalfW, topHalfW) {
        // Non-linear S-curve: stays narrow in the bottom 35% (lamp neck) then flares out smoothly
        var curved = Math.pow(Math.max(0.0, Math.min(1.0, s)), 1.75);
        var smooth = curved * curved * (3.0 - 2.0 * curved);
        var blend = curved * 0.65 + smooth * 0.35;
        return spoutHalfW + (topHalfW - spoutHalfW) * blend;
    }

    function genieCenterXAt(s, spoutX, topX) {
        var smooth = s * s * (3.0 - 2.0 * s);
        // Subtle genie wave oscillation as it rises from the lamp
        var wave = Math.sin(s * Math.PI * 2.0 - root.progress * 5.0) * (1.0 - s) * s * 14.0;
        return spoutX + (topX - spoutX) * smooth + wave;
    }

    Canvas {
        id: genieCanvas
        anchors.fill: parent
        renderTarget: Canvas.FramebufferObject
        renderStrategy: Canvas.Cooperative

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.clearRect(0, 0, width, height);

            if (root.progress <= 0.001 || root.progress >= 0.999) {
                return;
            }

            var spoutX = root.originX + root.originWidth * 0.5;
            var spoutY = root.originY + root.originHeight * 0.25;
            var topX = spoutX * 0.88 + (width * 0.5) * 0.12;
            var topY = 4.0;

            var spoutHalfW = Math.max(6.0, root.originWidth * 0.14);
            var topHalfW = Math.max(135.0, root.originWidth * 2.75);

            // Head of the genie shoots from lamp (0) to top (1) in first 62% of animation
            var headS = Math.min(1.0, root.progress / 0.62);
            // Tail detaches from the lamp at 28% and gets pulled up into the window
            var tailS = Math.max(0.0, (root.progress - 0.28) / 0.72);

            if (headS <= tailS + 0.01) {
                return;
            }

            var steps = 22;
            var leftPoints = [];
            var rightPoints = [];

            for (var i = 0; i <= steps; ++i) {
                var frac = i / steps;
                var s = tailS + (headS - tailS) * frac;
                var py = spoutY + (topY - spoutY) * s;
                var cx = root.genieCenterXAt(s, spoutX, topX);
                var hw = root.genieHalfWidthAt(s, spoutHalfW, topHalfW);

                leftPoints.push({ x: cx - hw, y: py, s: s, cx: cx, hw: hw });
                rightPoints.push({ x: cx + hw, y: py, s: s, cx: cx, hw: hw });
            }

            var fadeOut = root.progress > 0.72 ? (1.0 - (root.progress - 0.72) / 0.28) : 1.0;
            fadeOut = Math.max(0.0, Math.min(1.0, fadeOut));

            // 1. Draw the Genie Funnel Body
            ctx.beginPath();
            ctx.moveTo(leftPoints[0].x, leftPoints[0].y);
            for (var li = 1; li < leftPoints.length; ++li) {
                ctx.lineTo(leftPoints[li].x, leftPoints[li].y);
            }
            for (var ri = rightPoints.length - 1; ri >= 0; --ri) {
                ctx.lineTo(rightPoints[ri].x, rightPoints[ri].y);
            }
            ctx.closePath();

            var yBottom = leftPoints[0].y;
            var yTop = leftPoints[leftPoints.length - 1].y;
            var grad = ctx.createLinearGradient(spoutX, yBottom, topX, yTop);
            grad.addColorStop(0.0, Qt.rgba(0.38, 0.78, 1.0, 0.72 * fadeOut));
            grad.addColorStop(0.45, Qt.rgba(0.26, 0.56, 0.96, 0.48 * fadeOut));
            grad.addColorStop(1.0, Qt.rgba(0.18, 0.24, 0.36, 0.68 * fadeOut));
            ctx.fillStyle = grad;
            ctx.fill();

            // 2. Draw horizontal warped window scan-slices inside the Genie funnel
            for (var si = 1; si < leftPoints.length; si += 2) {
                var ptL = leftPoints[si];
                var ptR = rightPoints[si];
                var sliceAlpha = (0.14 + 0.18 * (si / leftPoints.length)) * fadeOut;
                ctx.beginPath();
                ctx.moveTo(ptL.x + 2, ptL.y);
                ctx.lineTo(ptR.x - 2, ptR.y);
                ctx.strokeStyle = Qt.rgba(1.0, 1.0, 1.0, sliceAlpha);
                ctx.lineWidth = 1.2;
                ctx.stroke();
            }

            // 3. Draw luminous S-curve borders on left and right edges of the Genie funnel
            ctx.beginPath();
            ctx.moveTo(leftPoints[0].x, leftPoints[0].y);
            for (var bli = 1; bli < leftPoints.length; ++bli) {
                ctx.lineTo(leftPoints[bli].x, leftPoints[bli].y);
            }
            ctx.strokeStyle = Qt.rgba(0.65, 0.90, 1.0, 0.75 * fadeOut);
            ctx.lineWidth = 1.8;
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(rightPoints[0].x, rightPoints[0].y);
            for (var bri = 1; bri < rightPoints.length; ++bri) {
                ctx.lineTo(rightPoints[bri].x, rightPoints[bri].y);
            }
            ctx.strokeStyle = Qt.rgba(0.65, 0.90, 1.0, 0.75 * fadeOut);
            ctx.lineWidth = 1.8;
            ctx.stroke();

            // 4. Draw expanding window titlebar header at the top of the Genie head
            if (headS > 0.25) {
                var headPtL = leftPoints[leftPoints.length - 1];
                var headPtR = rightPoints[rightPoints.length - 1];
                var headerH = Math.min(18.0, (yBottom - yTop) * 0.25);
                if (headerH > 3.0) {
                    ctx.fillStyle = Qt.rgba(1.0, 1.0, 1.0, 0.25 * fadeOut);
                    ctx.fillRect(headPtL.x, headPtL.y, headPtR.x - headPtL.x, headerH);
                }
            }

            // 5. Lamp spout glow while tail is still attached to the icon
            if (tailS < 0.35) {
                var spoutAlpha = (1.0 - tailS / 0.35) * 0.65;
                var radGrad = ctx.createRadialGradient(spoutX, spoutY, 2.0, spoutX, spoutY, root.originWidth * 0.55);
                radGrad.addColorStop(0.0, Qt.rgba(0.65, 0.92, 1.0, spoutAlpha));
                radGrad.addColorStop(1.0, Qt.rgba(0.35, 0.75, 1.0, 0.0));
                ctx.fillStyle = radGrad;
                ctx.beginPath();
                ctx.arc(spoutX, spoutY, root.originWidth * 0.55, 0, Math.PI * 2.0);
                ctx.fill();
            }
        }
    }

    // Rising & Morphing Application Spirit Icon inside the Genie Funnel
    Item {
        id: risingSpiritContainer
        readonly property real headS: Math.min(1.0, root.progress / 0.62)
        readonly property real tailS: Math.max(0.0, (root.progress - 0.28) / 0.72)
        readonly property real spiritS: tailS * 0.35 + headS * 0.65
        readonly property real spoutX: root.originX + root.originWidth * 0.5
        readonly property real spoutY: root.originY + root.originHeight * 0.25
        readonly property real topX: spoutX * 0.88 + (root.width * 0.5) * 0.12
        readonly property real topY: 4.0

        width: root.originWidth
        height: root.originHeight
        x: root.genieCenterXAt(spiritS, spoutX, topX) - width * 0.5
        y: spoutY + (topY - spoutY) * spiritS - height * 0.5
        opacity: root.progress < 0.72 ? (0.85 * Math.min(1.0, root.progress * 4.0)) : (0.85 * (1.0 - (root.progress - 0.72) / 0.28))

        transform: Scale {
            origin.x: risingSpiritContainer.width * 0.5
            origin.y: risingSpiritContainer.height * 0.5
            // Starts pinched horizontally & stretched vertically like a genie leaving the bottle, then expands wide
            xScale: 0.32 + 1.15 * Math.pow(risingSpiritContainer.spiritS, 1.4)
            yScale: 1.42 - 0.32 * risingSpiritContainer.spiritS
        }

        Image {
            anchors.fill: parent
            source: root.activeIconName.length > 0 ? ("image://icon/" + root.activeIconName) : ""
            sourceSize.width: 128
            sourceSize.height: 128
            smooth: true
            fillMode: Image.PreserveAspectFit
        }
    }
}
