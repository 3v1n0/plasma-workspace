/*
 * Copyright 2013 Heena Mahour <heena393@gmail.com>
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 * Copyright 2013 Martin Klapetek <mklapetek@kde.org>
 * Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components

Item {
    id: main

    Layout.minimumWidth: vertical ? 0 : sizehelper.paintedWidth + (units.smallSpacing * 2)
    Layout.maximumWidth: vertical ? Infinity : Layout.minimumWidth
    Layout.preferredWidth: vertical ? undefined : Layout.minimumWidth

    Layout.minimumHeight: vertical ? sizehelper.paintedHeight + (units.smallSpacing * 2) : 0
    Layout.maximumHeight: vertical ? Layout.minimumHeight : Infinity
    Layout.preferredHeight: vertical ? Layout.minimumHeight : theme.mSize(theme.defaultFont).height * 2

    property int formFactor: plasmoid.formFactor
    property int timePixelSize: theme.defaultFont.pixelSize
    property int timezonePixelSize: theme.smallestFont.pixelSize

    property bool constrained: formFactor == PlasmaCore.Types.Vertical || formFactor == PlasmaCore.Types.Horizontal

    property bool vertical: plasmoid.formFactor == PlasmaCore.Types.Vertical

    property bool showSeconds: plasmoid.configuration.showSeconds
    property bool showTimezone: plasmoid.configuration.showTimezone
    property bool showDate: plasmoid.configuration.showDate
    property int dateFormat: plasmoid.configuration.dateFormat == "longDate" ? Locale.LongFormat :
                             plasmoid.configuration.dateFormat == "shortDate" ? Locale.ShortFormat :
                             Locale.NarrowFormat
    property string lastDate: ""
    property string timeFormat
    property int tzOffset

    property int tzIndex: plasmoid.configuration.lastSelectedIndex >= plasmoid.configuration.selectedTimeZones.length ? 0 : plasmoid.configuration.lastSelectedIndex

    onShowSecondsChanged: {
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat))
    }
    onShowTimezoneChanged: {
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat))
    }

    Components.Label  {
        id: timeLabel
        font {
            weight: plasmoid.configuration.boldText ? Font.Bold : Font.Normal
            italic: plasmoid.configuration.italicText
            pixelSize: 1024
            pointSize: 0 // we need to unset pointSize otherwise it breaks the Text.Fit size mode
        }
        minimumPixelSize: theme.mSize(theme.smallestFont).height
        fontSizeMode: Text.Fit
        text: {
            var returnString = "";

            var now = dataSource.data[plasmoid.configuration.selectedTimeZones[tzIndex]]["DateTime"];
            // get current UTC time
            var msUTC = now.getTime() + (now.getTimezoneOffset() * 60000);
            // add the TZ offset to it
            var dateTime = new Date(msUTC + (dataSource.data[plasmoid.configuration.selectedTimeZones[tzIndex]]["Offset"] * 1000));

            returnString += Qt.formatTime(dateTime, main.timeFormat);

            if (plasmoid.configuration.selectedTimeZones[tzIndex] != "Local" || main.showTimezone) {
                var timezoneString = plasmoid.configuration.displayTimezoneAsCode ? dataSource.data[plasmoid.configuration.selectedTimeZones[tzIndex]]["Timezone Abbreviation"]
                                                                                  : dataSource.data[plasmoid.configuration.selectedTimeZones[tzIndex]]["Timezone City"];

                returnString += (showDate ? " (" + timezoneString + ")" : "<br/>" + timezoneString);
            }

            if (showDate) {
                returnString += "<br/>" + Qt.formatDate(dateTime, Qt.locale().dateFormat(main.dateFormat));
            }

            return returnString;
        }

        wrapMode: plasmoid.formFactor != PlasmaCore.Types.Horizontal ? Text.WordWrap : Text.NoWrap
        horizontalAlignment: vertical ? Text.AlignHCenter : Text.AlignLeft // we want left align when horizontal to avoid re-aligning when seconds are visible
        verticalAlignment: Text.AlignVCenter
        height: 0
        width: 0
        anchors {
            fill: parent
            leftMargin: units.smallSpacing
            rightMargin: units.smallSpacing
        }

        MouseArea {
            anchors.fill: parent
            enabled: plasmoid.configuration.wheelChangesTimezone
            onWheel: {
                var delta = wheel.angleDelta.y || wheel.angleDelta.x
                var newIndex = tzIndex;

                if (delta < 0) {
                    newIndex--;
                } else if (delta > 1) {
                    newIndex++;
                }

                if (newIndex >= plasmoid.configuration.selectedTimeZones.length) {
                    newIndex = 0;
                } else if (newIndex < 0) {
                    newIndex = plasmoid.configuration.selectedTimeZones.length - 1;
                }

                plasmoid.configuration.lastSelectedIndex = newIndex;

                dataSource.dataChanged();
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: plasmoid.expanded = !plasmoid.expanded
    }

    Text {
        id: sizehelper
        font.weight: timeLabel.font.weight
        font.italic: timeLabel.font.italic
        font.pixelSize: vertical ? theme.mSize(theme.defaultFont).height * 2 : 1024 // random "big enough" size - this is used as a max pixelSize by the fontSizeMode
        minimumPixelSize: theme.mSize(theme.smallestFont).height
        // when seconds are visible, align to left to avoid text jumping with non-proportional font
        // when seconds are disabled, align to right to avoid big gap on the right side when the hours are just one number (like "9:00")
        horizontalAlignment: vertical ? Text.AlignHCenter : (main.showSeconds ? Text.AlignLeft : Text.AlignRight)
        verticalAlignment: Text.AlignVCenter

        // with this property we want to get the font to fit into applet's height when in horizontal panel
        // or applet's width when vertical - we scale the font to fit the height and then see how much
        // is the paintedWidth and set that in the Layout properties. Same with vertical panels but width
        // and paintedHeight instead
        fontSizeMode: vertical ? Text.HorizontalFit : Text.VerticalFit

        wrapMode: plasmoid.formFactor != PlasmaCore.Types.Horizontal ? Text.WordWrap : Text.NoWrap
        visible: false
        anchors {
            fill: parent
            leftMargin: units.smallSpacing
            rightMargin: units.smallSpacing
        }
    }

    // Qt's QLocale does not offer any modular time creating like Klocale did
    // eg. no "gimme time with seconds" or "gimme time without seconds and with timezone".
    // QLocale supports only two formats - Long and Short. Long is unusable in many situations
    // and Short does not provide seconds. So if seconds are enabled, we need to add it here.
    //
    // What happens here is that it looks for the delimiter between "h" and "m", takes it
    // and appends it after "mm" and then appends "ss" for the seconds. Also it checks
    // if the format string already does not contain the seconds part.
    //
    // It can happen that Qt uses the 'C' locale (it's a fallback) and that locale
    // has always ":ss" part in ShortFormat, so we need to remove it.
    function timeFormatCorrection(timeFormatString) {
        if (main.showSeconds && timeFormatString.indexOf('s') == -1) {
            timeFormatString = timeFormatString.replace(/^(hh*)(.+)(mm)(.*?)/i,
                                                        function(match, firstPart, delimiter, secondPart, rest, offset, original) {
                return firstPart + delimiter + secondPart + delimiter + "ss" + rest
            });
        } else if (!main.showSeconds && timeFormatString.indexOf('s') != -1) {
            timeFormatString = timeFormatString.replace(/.ss?/i, "");
        }

        var st = Qt.formatTime(new Date(2000, 0, 1, 20, 0, 0), timeFormatString);
        if (main.showTimezone || plasmoid.configuration.selectedTimeZones[tzIndex] != "Local") {
            var timezoneString = plasmoid.configuration.displayTimezoneAsCode ? dataSource.data[plasmoid.configuration.selectedTimeZones[tzIndex]]["Timezone Abbreviation"]
                                                                              : dataSource.data[plasmoid.configuration.selectedTimeZones[tzIndex]]["Timezone City"];

            st += (showDate ? " (" + timezoneString + ")" : "<br/>" + timezoneString);
        }

        if (main.showDate) {
            st += "<br/>" + Qt.formatDate(dataSource.data[plasmoid.configuration.selectedTimeZones[tzIndex]]["DateTime"], Qt.locale().dateFormat(main.dateFormat));
        }

        if (sizehelper.text != st) {
            sizehelper.text = st;
        }

        main.timeFormat = timeFormatString;
    }

    function dateTimeChanged()
    {
        //console.log("Date/time changed!");
        var doCorrections = false;

        if (main.showDate) {
            // If the date has changed, force size recalculation, because the day name
            // or the month name can now be longer/shorter, so we need to adjust applet size
            var currentDate = Qt.formatDateTime(dataSource.data["Local"]["DateTime"], "yyyy-mm-dd");
            if (main.lastDate != currentDate) {
                doCorrections = true;
                main.lastDate = currentDate
            }
        }

        var currentTZOffset = dataSource.data["Local"]["Offset"] / 60;
        if (currentTZOffset != tzOffset) {
            doCorrections = true;
            tzOffset = currentTZOffset;
            //console.log("TZ offset changed: " + tzOffset);
            Date.timeZoneUpdated(); // inform the QML JS engine about TZ change
        }

        if (doCorrections) {
            timeFormatCorrection(main.timeFormat);
        }
    }

    Component.onCompleted: {
        tzOffset = new Date().getTimezoneOffset();
        //console.log("Initial TZ offset: " + tzOffset);
        dateTimeChanged();
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat));
        dataSource.onDataChanged.connect(dateTimeChanged);
    }
}
