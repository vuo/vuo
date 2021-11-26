import QtQuick 2.6
import QtQuick.Controls.Styles 1.1

ButtonStyle {
	padding { top: 10; left: 10; right: 10; bottom: 10 }
	label: Text {
		color: control.isDefault ? '#ffffff' : '#202020'
		text: control.text
		font: VuoStyle.bodyFont
		renderType: Text.QtRendering
		horizontalAlignment: Text.AlignHCenter

		wrapMode: Text.Wrap
		elide: Text.ElideNone

		// Causes Qt to respect the img tag's width and height attributes
		// (enabling us to render Retina-resolution images).
		textFormat: Text.RichText
	}
	background: Rectangle {
		color: control.isDefault ? '#404040' : '#f8f8f8'
		border.width: control.isDefault ? 0 : 1
		border.color: '#a0a0a0'
		radius: 4
	}
}
