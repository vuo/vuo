import QtQuick 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import "../common"

ColumnLayout {
	id: layout

	property alias logsModel: scrollView.logsModel
	property alias selectedIndices: scrollView.selectedIndices

	property bool isInterfaceDark: false
	readonly property color listBackgroundColor: isInterfaceDark ? "#262626" : "#ffffff"  // same as VuoNodeLibrary
	readonly property color rowAlternateColor: isInterfaceDark ? "#2b2b2b" : "#f8f8f8"  // same as VuoNodeClassListItemDelegate
	readonly property color rowSelectedColor: isInterfaceDark ? "#1d6ae5" : "#74acec"   //
	readonly property color textNormalColor: isInterfaceDark ? "#a0a0a0" : "#606060"    //
	readonly property color textSelectedColor: isInterfaceDark ? "#dadada" : "#ffffff"  //

	ScrollView {
		id: scrollView
		Layout.fillWidth: true
		Layout.fillHeight: true
		clip: true

		property alias logsModel: logsView.model
		property alias selectedIndices: logsView.selectedIndices

		Rectangle {
			id: scrollBackground
			color: listBackgroundColor
			width: parent.width
			height: parent.height
		}

		ListView {
			id: logsView
			width: parent.width
			height: parent.height
			clip: true
			z: scrollBackground.z + 0.1

			readonly property real horizontalMargin: 10

			delegate: Rectangle {
				id: logBackground
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.leftMargin: logsView.horizontalMargin
				anchors.rightMargin: logsView.horizontalMargin
				radius: 5

				readonly property int textPadding: 5
				height: logText.contentHeight + 2*textPadding

				property alias textColor: logText.color

				Text {
					id: logText
					text: modelData
					anchors.fill: parent
					anchors.margins: logBackground.textPadding
					font.family: "Menlo"  // same as VuoCodeEditor
					font.pointSize: 11    //
					textFormat: Text.PlainText
					wrapMode: Text.Wrap
				}

				TapHandler {
					onTapped: {
						logsView.modifySelection(index, point.modifiers)
					}
				}

				onParentChanged: {
					logsView.registerItem(this, index)
					logsView.setItemBackgroundColor(index, logsView.selectedIndices.indexOf(index) !== -1)
				}
			}

			// Selecting

			property var selectedIndices: []  // in no particular order
			property var singlySelectedIndices: []  // ordered least to most recent
			property var backgroundAtIndex: []  // workaround since ListView.itemAtIndex not available until Qt 5.13

			function registerItem(item, index) {
				while (index >= backgroundAtIndex.length)
					backgroundAtIndex.push(null)
				backgroundAtIndex.length = Math.max(backgroundAtIndex.length, index+1)
				backgroundAtIndex[index] = item
			}

			function setItemBackgroundColor(index, selected) {
				if (backgroundAtIndex[index] !== null) {
					backgroundAtIndex[index].color = selected ? layout.rowSelectedColor :
																index % 2 ? layout.rowAlternateColor :
																			layout.listBackgroundColor
					backgroundAtIndex[index].textColor = selected ? layout.textSelectedColor :
																	layout.textNormalColor
				}
			}

			function modifySelection(clickedIndex, modifiers) {
				if (modifiers === Qt.NoModifier) {
					// Select the clicked item. Deselect everything else.
					selectedIndices.forEach(function(index) {
						setItemBackgroundColor(index, false)
					})
					selectedIndices = [clickedIndex]
					singlySelectedIndices = [clickedIndex]
					setItemBackgroundColor(clickedIndex, true)

				} else if (modifiers & Qt.ControlModifier) {
					// Toggle the clicked item's selected status.
					var found = selectedIndices.indexOf(clickedIndex);
					if (found === -1) {
						selectedIndices.push(clickedIndex)
						singlySelectedIndices.push(clickedIndex)
						setItemBackgroundColor(clickedIndex, true)
					} else {
						selectedIndices.splice(found, 1)
						var found2 = singlySelectedIndices.indexOf(clickedIndex);
						if (found2 !== -1)
							singlySelectedIndices.splice(found2, 1)
						setItemBackgroundColor(clickedIndex, false)
					}

				} else if (modifiers & Qt.ShiftModifier) {
					// Add the clicked item and all items between it and the anchor item to the selection.
					var lastSinglySelectedIndex = singlySelectedIndices.length == 0 ? 0 : singlySelectedIndices[singlySelectedIndices.length-1]
					var start = Math.min(lastSinglySelectedIndex, clickedIndex)
					var end = Math.max(lastSinglySelectedIndex, clickedIndex)
					for (var i = start; i <= end; ++i) {
						if (selectedIndices.indexOf(i) === -1) {
							selectedIndices.push(i)
							setItemBackgroundColor(i, true)
						}
					}
				}
			}

			function selectAll() {
				selectedIndices = []
				for (var i = 0; i < count; ++i) {
					selectedIndices.push(i)
					setItemBackgroundColor(i, true)
				}
			}

			function adjustSelectedIndices(oldLogsDeleted) {
				if (oldLogsDeleted > 0) {
					selectedIndices.forEach(function(index, i, a) { a[i] -= oldLogsDeleted; })
					singlySelectedIndices.forEach(function(index, i, a) { a[i] -= oldLogsDeleted; })
					selectedIndices = selectedIndices.filter(function(index) { return index >= 0; })
					singlySelectedIndices = singlySelectedIndices.filter(function(index) { return index >= 0; })
					backgroundAtIndex = backgroundAtIndex.slice(oldLogsDeleted)
				}
			}

			focus: true

			// Scrolling

			property bool scrolledUp: false
			property int previousIndex: 0

			function checkScroll(oldLogsDeleted) {
				// Check if the user has scrolled up by seeing if the bottom-most log in the window is the
				// last log in the list. (Flickable.atYEnd would be a simpler way to check for this, except
				// that it doesn't handle the fact that ListView.positionViewAtEnd() only scrolls down to
				// the first line, not the end, of multi-line log messages.)
				let firstVisibleIndex = indexAt(horizontalMargin, contentY)
				let lastY = contentY + height - 1  // account for 1px margin (or something?) at end of list
				let lastVisibleIndex = indexAt(horizontalMargin, lastY)
				scrolledUp = (0 <= lastVisibleIndex && lastVisibleIndex < count - 1)
				previousIndex = Math.max(firstVisibleIndex - oldLogsDeleted, 0)
			}

			Connections {
				target: modelProvider

				onModelAboutToChange: {
					logsView.checkScroll(oldLogsDeleted)
					logsView.adjustSelectedIndices(oldLogsDeleted)
				}

				onSelectedAllLogs: {
					logsView.selectAll()
				}
			}

			onModelChanged: {
				if (count == 0) {
					selectedIndices = [];
					singlySelectedIndices = [];
					backgroundAtIndex = [];
					scrolledUp = false;
					previousIndex = 0;
				}

				if (!scrolledUp)
					// Auto-scroll to the last log in the list.
					positionViewAtEnd()
				else
					// Stay at the spot that the user has scrolled to.
					positionViewAtIndex(previousIndex, ListView.Beginning)
			}
		}
	}
}
