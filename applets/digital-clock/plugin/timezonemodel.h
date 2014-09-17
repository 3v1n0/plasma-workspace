/***************************************************************************
 *   Copyright (C) 2014 Kai Uwe Broulik <kde@privat.broulik.de>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef TIMEZONEMODEL_H
#define TIMEZONEMODEL_H

#include <QAbstractListModel>

#include "timezonedata.h"

class TimeZoneModel : public QAbstractListModel
{

Q_OBJECT

public:
    explicit TimeZoneModel(QObject *parent = 0);
    ~TimeZoneModel();

    enum Roles {
        TimeZoneIdRole = Qt::UserRole + 1,
        RegionRole,
        CityRole,
        CommentRole,
        CheckedRole
    };

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    void update();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    QList<TimeZoneData> m_data;

};

#endif // TIMEZONEMODEL_H
