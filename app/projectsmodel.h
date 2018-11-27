/***************************************************************************
  qgsquicklayertreemodel.h
  --------------------------------------
  Date                 : Nov 2017
  Copyright            : (C) 2017 by Peter Petrik
  Email                : zilolv at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef PROJECTSMODEL_H
#define PROJECTSMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QModelIndex>

/*
 * Given data directory, find all QGIS projects (*.qgs) in the directory and subdirectories
 * and create list model from them. Available are full path to the file, name of the project
 * and short name of the project (clipped to N chars)
 */
class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY( QString dataDir READ dataDir) // never changes
    Q_PROPERTY( int defaultIndex READ defaultIndex WRITE setDefaultIndex NOTIFY defaultIndexChanged )
    Q_PROPERTY( QString defaultPath READ defaultProjectPath WRITE setDefaultProjectPath NOTIFY defaultProjectPathChanged )


  public:
    enum Roles
    {
      Name = Qt::UserRole + 1,
      Path,
      ShortName, // name shortened to maxShortNameChars
      ProjectInfo,
      Size
    };
    Q_ENUMS( Roles )

    explicit ProjectModel(const QString& dataDir, QObject* parent = nullptr);
    ~ProjectModel();

    Q_INVOKABLE QVariant data( const QModelIndex& index, int role ) const override;
    Q_INVOKABLE QModelIndex index( int row ) const;

    QHash<int, QByteArray> roleNames() const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QString dataDir() const;

    int defaultIndex() const;
    void setDefaultIndex(int index);

    Q_INVOKABLE int defaultIndexAccordingPath( QString path ) const;

    QString defaultProjectPath() const;
    void setDefaultProjectPath(QString path);

  signals:
    void defaultIndexChanged();
    void defaultProjectPathChanged();

  private:
    void findProjectFiles();

    struct ProjectFile {
        QString name;
        QString path;
        QString info;

        bool operator < (const ProjectFile& str) const
        {
            return (name < str.name);
        }
    };
    QList<ProjectFile> mProjectFiles;
    QString mDataDir;
    const int mMaxShortNameChars = 10;
    QString mDefaultProjectPath = QString("");
    int mDefaultIndex = 0; // set to "none" project

};

#endif // PROJECTSMODEL_H
