/*
 * Copyright 2018 Richard Liebscher <richard.liebscher@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import SailfishConnect.UI 0.3

Page {
    id: page

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView
        model: JobsModel { }
        anchors.fill: parent
        header: PageHeader {
            title: qsTr("Transfers")
        }

        delegate: ListItem {
            id: listItem

            width: page.width
            height: Theme.itemSizeMedium

            ProgressCircle {
                id: progress

                // inAlternateCycle: true

                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSizeMedium
                height: Theme.iconSizeMedium

                value: processedBytes / totalBytes

                Image {
                    source: "image://theme/icon-s-device-download" // TODO
                    anchors { centerIn: progress }
                }
            }

            Label {
                id: jobTitle
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                                + Theme.iconSizeMedium + Theme.paddingMedium
                    right: parent.right
                    top: parent.top
                }

                text: title
                color: listItem.highlighted
                       ? Theme.highlightColor
                       : Theme.primaryColor
                width: parent.width - Theme.iconSizeMedium
                truncationMode: TruncationMode.Fade
                textFormat: Text.PlainText
            }

            Label {
                id: jobDescription
                anchors {
                    left: jobTitle.left
                    top: jobTitle.bottom
                    right: parent.right
                }

                text: description
                color: listItem.highlighted
                       ? Theme.secondaryHighlightColor
                       : Theme.secondaryColor
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                width: parent.width - Theme.iconSizeMedium
                textFormat: Text.PlainText
            }
        }

        VerticalScrollDecorator { flickable: listView }
    }

}
