pragma Singleton
import QtQuick 2.0

QtObject {
	property font headerFont: Qt.font({
		family: 'PT Sans',
		pointSize: 19
	})
	property font bodyFont: Qt.font({
		family: 'PT Sans',
		pointSize: 14
	})
	property color teal: '#0099a9'
	property color error: '#772200'
}
