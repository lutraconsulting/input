/***************************************************************************
 qgsquickattributeformmodel.h
  --------------------------------------
  Date                 : 20.4.2021
  Copyright            : (C) 2021 by Peter Petrik
  Email                : zilolv@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSQUICKATTRIBUTEFORMMODEL_H
#define QGSQUICKATTRIBUTEFORMMODEL_H

#include <QAbstractListModel>
#include <QVariant>
#include <QUuid>

#include "qgis_quick.h"

class QgsQuickAttributeController;

/**
 * \ingroup quick
 *
 * This is a model implementation of attribute form of a feature from a vector layer
 * for a SINGLE tab in case of tab layout, or a WHOLE form in case there are no tabs at all.
 * Groups are flattened into a list.
 *
 * Items can be widgets for editing attributes, widgets for relations and containers (groups and tabs) flattened to separators.
 *
 * \note QML Type: AttributeFormModel
 *
 * \since QGIS 3.22
 */
class QUICK_EXPORT QgsQuickAttributeFormModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    QgsQuickAttributeFormModel( QObject *parent,
                                QgsQuickAttributeController *controller,
                                const QVector<QUuid> &data );
    ~QgsQuickAttributeFormModel() override;

    enum AttributeFormRoles
    {
      Type = Qt::UserRole + 1, //!< User role used to identify either "field" or "container" type of item
      Name, //!< Field Name
      AttributeValue, //!< Field Value
      AttributeEditable,  //!< Whether is field editable
      EditorWidget, //!< Widget type to represent the data (text field, value map, ...)
      EditorWidgetConfig, //!< Widget configuration
      RememberValue, //!< Remember value (whether to remember the value)
      Field, //!< Field
      FieldIndex, //!< Index
      Group, //!< Group name
      //AttributeEditorElement, //!< Attribute editor element
      Visible, //!< Field visible
      ConstraintSoftValid, //! Constraint soft valid
      ConstraintHardValid, //! Constraint hard valid
      ConstraintDescription //!< Constraint description
    };

    Q_ENUM( AttributeFormRoles )

    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags( const QModelIndex &index ) const override;
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) override;
    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;

  public slots:
    void onFormDataChanged( const QUuid id );
    void onFeatureChanged();

  private:
    bool rowIsValid( int row ) const;

    QgsQuickAttributeController *mController = nullptr; // not owned
    const QVector<QUuid> mData;
};

#endif // QGSQUICKATTRIBUTEFORMMODEL_H
