import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 900
    height: 660
    minimumWidth: 600
    minimumHeight: 440
    visible: true
    title: qsTr("pacui - Package Manager")
    color: "#0e0e10"
    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false

    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "Ctrl+R"; context: Qt.ApplicationShortcut; onActivated: backend.refreshInstalled() }
    Shortcut { sequence: "Ctrl+U"; context: Qt.ApplicationShortcut; enabled: !backend.busy && backend.canElevate; onActivated: backend.upgrade() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; enabled: !field.activeFocus && !backend.busy && win.activeFocusItem === null; onActivated: backend.search() }

    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("pacui")
                    font.pixelSize: 24
                    font.bold: true
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("Arch Linux package manager")
                    color: "#888888"
                    font.pixelSize: 12
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                visible: !backend.supported
                color: "#3a1c1c"
                radius: theme.radius
                Label {
                    anchors.centerIn: parent
                    text: qsTr("Unsupported system: an Arch-based distribution is required")
                    color: "#ff8a8a"
                    font.pixelSize: 13
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: field
                    Layout.fillWidth: true
                    enabled: backend.supported
                    placeholderText: qsTr("Search packages...")
                    onAccepted: backend.search()
                    onTextChanged: backend.query = text
                }
                Button {
                    text: qsTr("Search")
                    enabled: backend.supported && !backend.busy
                    onClicked: backend.search()
                }
                Button {
                    text: qsTr("Refresh")
                    enabled: backend.supported && !backend.busy
                    onClicked: backend.refreshInstalled()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                BusyIndicator {
                    running: backend.busy
                    visible: backend.busy
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                }
                Label {
                    text: backend.status
                    color: "#bbbbbb"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Button {
                    text: qsTr("Update system")
                    enabled: backend.supported && !backend.busy && (backend.omarchy || backend.canElevate)
                    onClicked: backend.upgrade()
                    ToolTip.visible: hovered && (backend.omarchy || !backend.canElevate)
                    ToolTip.text: backend.omarchy ? qsTr("Runs omarchy update")
                                                  : qsTr("pkexec is required for privileged operations")
                }
                Button {
                    text: qsTr("Clear log")
                    flat: true
                    enabled: backend.output.length > 0
                    onClicked: backend.clearOutput()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#17171a"
                radius: theme.radius
                border.color: "#2a2a2f"
                border.width: 1
                clip: true

                ListView {
                    id: list
                    anchors.fill: parent
                    anchors.margins: 6
                    model: backend.packages
                    clip: true
                    spacing: 2
                    delegate: Item {
                        width: ListView.view.width
                        height: 70

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Label {
                                    text: modelData.name
                                    font.pixelSize: 15
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: modelData.description || qsTr("No description")
                                    color: "#9a9a9a"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 140
                                spacing: 2
                                Label {
                                    text: modelData.version
                                    color: "#888888"
                                    font.pixelSize: 12
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignRight
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: modelData.repo
                                    color: Material.accent
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignRight
                                    elide: Text.ElideRight
                                }
                            }

                            Button {
                                text: modelData.installed ? qsTr("Remove") : qsTr("Install")
                                enabled: backend.supported && !backend.busy && backend.canElevate
                                Layout.preferredWidth: 104
                                onClicked: modelData.installed ? backend.remove(modelData.name) : backend.install(modelData.name)
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: list.count === 0 && !backend.busy
                    text: qsTr("Search for a package to get started.")
                    color: "#777777"
                    font.pixelSize: 14
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                visible: backend.output.length > 0
                color: "#0a0a0c"
                radius: theme.radius
                border.color: "#2a2a2f"
                border.width: 1
                clip: true

                Flickable {
                    id: logFlick
                    anchors.fill: parent
                    anchors.margins: 8
                    contentWidth: width
                    contentHeight: logText.height
                    clip: true

                    Text {
                        id: logText
                        width: logFlick.width
                        text: backend.output
                        color: "#cfcfcf"
                        font.family: "monospace"
                        font.pixelSize: 11
                        wrapMode: Text.WrapAnywhere
                        onTextChanged: Qt.callLater(function() {
                            logFlick.contentY = Math.max(0, logFlick.contentHeight - logFlick.height)
                        })
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "#aa000000"
            visible: win.helpVisible
            z: 10
            MouseArea {
                anchors.fill: parent
                onClicked: win.helpVisible = false
            }
        }

        Pane {
            anchors.centerIn: parent
            width: Math.min(420, parent.width - 40)
            visible: win.helpVisible
            z: 11
            Material.elevation: 6
            Material.roundedScale: theme.radius

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Label {
                    text: qsTr("Shortcuts")
                    font.pixelSize: 18
                    font.bold: true
                }

                Label {
                    text: qsTr("Enter or Space - search\nCtrl+R - refresh installed packages\nCtrl+U - update system\n? - toggle this help\nQ - quit")
                    color: "#cccccc"
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Close")
                    Layout.alignment: Qt.AlignRight
                    onClicked: win.helpVisible = false
                }
            }
        }
    }
}
