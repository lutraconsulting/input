/***************************************************************************
 attributedata.h
  --------------------------------------
  Date                 : 22.4.2021
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

#ifndef ATTRIBUTEDATA_H
#define ATTRIBUTEDATA_H


#include <QString>
#include <QVariant>
#include <QVector>
#include <QUuid>
#include <QVariantMap>
#include <QObject>
#include <memory>

#include "qgsfieldconstraints.h"
#include "qgseditorwidgetsetup.h"
#include "qgsexpression.h"
#include "qgsrelation.h"
#include "qgsfield.h"

class FormItem
{
    Q_GADGET

  public:
    enum FormItemType
    {
      Invalid = 1,
      Container,
      Relation,
      Field,
    };
    Q_ENUMS( FormItemType )

    enum ValueState
    {
      ValidValue = 1,
      InvalidValue,     // did not pass convertCompatible check
      ValueOutOfRange  // number is out of min/max range
    };
    Q_ENUMS( ValueState )

    FormItem(
      const QUuid &id,
      const QgsField &field,
      const QString &groupName,
      int parentTabId,
      FormItem::FormItemType type,
      const QString &name,
      bool isEditable,
      const QgsEditorWidgetSetup &editorWidgetSetup,
      int fieldIndex,
      const QgsFieldConstraints &contraints,
      const QgsExpression &visibilityExpression,
      const QgsRelation &relation = QgsRelation()
    );

    static FormItem *createFieldItem(
      const QUuid &id,
      const QgsField &field,
      const QString &groupName,
      int parentTabId,
      FormItem::FormItemType type,
      const QString &name,
      bool isEditable,
      const QgsEditorWidgetSetup &editorWidgetSetup,
      int fieldIndex,
      const QgsFieldConstraints &contraints,
      const QgsExpression &visibilityExpression
    );

    static FormItem *createRelationItem(
      const QUuid &id,
      const QString &groupName,
      int parentTabId,
      FormItem::FormItemType type,
      const QString &name,
      const QgsRelation &relation
    );

    FormItem::FormItemType type() const;
    QString name() const;
    bool isEditable() const;
    QString editorWidgetType() const;
    QVariantMap editorWidgetConfig() const;
    int fieldIndex() const;

    bool constraintSoftValid() const;
    void setConstraintSoftValid( bool constraintSoftValid );

    bool constraintHardValid() const;
    void setConstraintHardValid( bool constraintHardValid );

    ValueState valueState() const;
    void setState( ValueState state );

    bool isVisible() const;
    void setVisible( bool visible );

    QString constraintDescription() const;

    QUuid id() const;

    int parentTabId() const;

    QgsExpression visibilityExpression() const;

    bool visible() const;

    QgsField field() const;

    QString groupName() const;

    QVariant originalValue() const;
    void setOriginalValue( const QVariant &originalValue );

    QgsRelation relation() const;
    QString fieldError() const;

  private:

    const QUuid mId;
    const QgsField mField;
    const QString mGroupName; //empty for no group, group/tab name if widget is in container
    const int mParentTabId;
    const FormItem::FormItemType mType;
    const QString mName;
    const bool mIsEditable;
    const QgsEditorWidgetSetup mEditorWidgetSetup;
    const int mFieldIndex;
    const QgsFieldConstraints mConstraints;
    const QgsExpression mVisibilityExpression;

    bool mConstraintSoftValid = false;
    bool mConstraintHardValid = false;
    ValueState mState = ValueState::ValidValue;
    bool mVisible = false;
    QVariant mOriginalValue; // original unmodified value

    const QgsRelation mRelation; // empty if type is field
};

class  TabItem
{
  public:
    TabItem( const int &id,
             const QString &name,
             const QVector<QUuid> &formItems,
             const QgsExpression &visibilityExpression
           );

    QString name() const;
    const QVector<QUuid> formItems() const;

    bool isVisible() const;
    void setVisible( bool visible );

    int tabIndex() const;

    QgsExpression visibilityExpression() const;

  private:
    const int mTabIndex;
    const QString mName;
    const QVector<QUuid> mFormItems;
    const QgsExpression mVisibilityExpression;
    bool mVisible = false;
};

#endif // ATTRIBUTEDATA_H
