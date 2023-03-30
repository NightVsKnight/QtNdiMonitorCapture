import QtQuick
import QtQuickNdi
import QtMultimedia

Rectangle {
    id: window
    width: 800
    height: 600
    visible: true

    Rectangle {
        id: viewPort
        anchors.fill: parent

        QtNdiItem {
            id: ndi
            ndiSource: "BUGATTI (c922 Pro Stream Webcam)"
            videoOutput: videoWindow1
        }

        VideoOutput {
            id: videoWindow1
            anchors.fill: parent
            visible: ndi.active
        }

        MouseArea {
            anchors.fill: parent
            onClicked: ndi.active = !ndi.active
        }
    }


}
