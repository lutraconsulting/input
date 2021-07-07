/***************************************************************************
  featureslistmodel.cpp
 ---------------------------
  Date                 : Sep 2020
  Copyright            : (C) 2020 by Tomas Mizera
  Email                : tomas.mizera2 at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "featureslistmodel.h"
#include "qgsexpressioncontextutils.h"
#include "qgslogger.h"
#include "coreutils.h"

FeaturesListModel::FeaturesListModel( QObject *parent )
  : QAbstractListModel( parent ),
    mCurrentLayer( nullptr )
{
  // avoid dangling pointers to mCurrentLayer/mCurrentFeature when switching projects
  QObject::connect( QgsProject::instance(), &QgsProject::cleared, this, &FeaturesListModel::emptyData );
}

FeaturesListModel::~FeaturesListModel() = default;

int FeaturesListModel::rowCount( const QModelIndex &parent ) const
{
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if ( parent.isValid() )
    return 0;

  return mFeatures.count();
}

QVariant FeaturesListModel::featureTitle( const FeatureLayerPair &featurePair ) const
{
  QString title;

  if ( !mFeatureTitleField.isEmpty() )
  {
    title = featurePair.feature().attribute( mFeatureTitleField ).toString();
    if ( !title.isEmpty() )
      return title;
  }

  QgsExpressionContext context( QgsExpressionContextUtils::globalProjectLayerScopes( featurePair.layer() ) );
  context.setFeature( featurePair.feature() );
  QgsExpression expr( featurePair.layer()->displayExpression() );
  title = expr.evaluate( &context ).toString();

  if ( title.isEmpty() )
    return featurePair.feature().id();

  return title;
}

QVariant FeaturesListModel::data( const QModelIndex &index, int role ) const
{
  int row = index.row();
  if ( row < 0 || row >= mFeatures.count() )
    return QVariant();

  if ( !index.isValid() )
    return QVariant();

  const FeatureLayerPair pair = mFeatures.at( index.row() );

  switch ( role )
  {
    case FeatureTitle: return featureTitle( pair );
    case FeatureId: return QVariant( pair.feature().id() );
    case Feature: return QVariant::fromValue<QgsFeature>( pair.feature() );
    case FeaturePair: return QVariant::fromValue<FeatureLayerPair>( pair );
    case Description: return QVariant( QString( "Feature ID %1" ).arg( pair.feature().id() ) );
    case KeyColumn: return mKeyField.isEmpty() ? QVariant() : pair.feature().attribute( mKeyField );
    case FoundPair: return foundPair( pair );
    case Qt::DisplayRole: return featureTitle( pair );
  }

  return QVariant();
}

QString FeaturesListModel::foundPair( const FeatureLayerPair &pair ) const
{
  if ( mSearchExpression.isEmpty() )
    return QString();

  QgsFields fields = pair.feature().fields();
  QStringList words = mSearchExpression.split( ' ', QString::SplitBehavior::SkipEmptyParts );
  QStringList foundPairs;

  for ( const QString &word : words )
  {
    for ( const QgsField &field : fields )
    {
      if ( field.configurationFlags().testFlag( QgsField::ConfigurationFlag::NotSearchable ) )
        continue;

      QString attrValue = pair.feature().attribute( field.name() ).toString();

      if ( attrValue.toLower().indexOf( word.toLower() ) != -1 )
      {
        foundPairs << field.name() + ": " + attrValue;

        // remove found field from list of fields to not select it more than once
        fields.remove( fields.lookupField( field.name() ) );
      }
    }
  }

  return foundPairs.join( ", " );
}

QString FeaturesListModel::buildSearchExpression()
{
  if ( mSearchExpression.isEmpty() || !mCurrentLayer )
    return QString();

  const QgsFields fields = mCurrentLayer->fields();
  QStringList expressionParts;
  QStringList wordExpressions;

  QStringList words = mSearchExpression.split( ' ', QString::SplitBehavior::SkipEmptyParts );

  for ( const QString &word : words )
  {
    bool searchExpressionIsNumeric;
    int filterInt = word.toInt( &searchExpressionIsNumeric );
    Q_UNUSED( filterInt ); // we only need to know if expression is numeric, int value is not used


    for ( const QgsField &field : fields )
    {
      if ( field.configurationFlags().testFlag( QgsField::ConfigurationFlag::NotSearchable ) )
        continue;

      if ( field.isNumeric() && searchExpressionIsNumeric )
        expressionParts << QStringLiteral( "%1 ~ '%2.*'" ).arg( QgsExpression::quotedColumnRef( field.name() ), word );
      else if ( field.type() == QVariant::String )
        expressionParts << QStringLiteral( "%1 ILIKE '%%2%'" ).arg( QgsExpression::quotedColumnRef( field.name() ), word );
    }
    wordExpressions << QStringLiteral( "(%1)" ).arg( expressionParts.join( QLatin1String( " ) OR ( " ) ) );
    expressionParts.clear();
  }

  QString expression = QStringLiteral( "(%1)" ).arg( wordExpressions.join( QLatin1String( " ) AND ( " ) ) );

  return expression;
}

void FeaturesListModel::setupFeatureRequest( QgsFeatureRequest &request )
{
  if ( !mFilterExpression.isEmpty() && !mSearchExpression.isEmpty() )
  {
    request.setFilterExpression( buildSearchExpression() );
    request.combineFilterExpression( mFilterExpression );
  }
  else if ( !mSearchExpression.isEmpty() )
  {
    request.setFilterExpression( buildSearchExpression() );
  }
  else if ( !mFilterExpression.isEmpty() )
  {
    request.setFilterExpression( mFilterExpression );
  }

  request.setLimit( FEATURES_LIMIT );

  // create context for filter expression
  if ( !mFilterExpression.isEmpty() && QgsValueRelationFieldFormatter::expressionIsUsable( mFilterExpression, mCurrentFeature ) )
  {
    QgsExpression exp( mFilterExpression );
    QgsExpressionContext filterContext = QgsExpressionContext( QgsExpressionContextUtils::globalProjectLayerScopes( mCurrentLayer ) );

    if ( mCurrentFeature.isValid() && QgsValueRelationFieldFormatter::expressionRequiresFormScope( mFilterExpression ) )
      filterContext.appendScope( QgsExpressionContextUtils::formScope( mCurrentFeature ) );

    request.setExpressionContext( filterContext );
  }
}

void FeaturesListModel::loadFeaturesFromLayer( QgsVectorLayer *layer )
{
  if ( layer && layer->isValid() )
    mCurrentLayer = layer;

  if ( mCurrentLayer )
  {
    beginResetModel();
    mFeatures.clear();

    QgsFeatureRequest req;
    setupFeatureRequest( req );

    QgsFeatureIterator it = mCurrentLayer->getFeatures( req );
    QgsFeature f;

    while ( it.nextFeature( f ) )
    {
      mFeatures << FeatureLayerPair( f, mCurrentLayer );
    }

    emit featuresCountChanged( featuresCount() );
    endResetModel();
  }
}

void FeaturesListModel::setupValueRelation( const QVariantMap &config )
{
  beginResetModel();
  emptyData();

  QgsVectorLayer *layer = QgsValueRelationFieldFormatter::resolveLayer( config, QgsProject::instance() );

  if ( layer && layer->fields().size() != 0 )
  {
    // save key and value field
    QgsFields fields = layer->fields();

    QString keyFieldName = config.value( QStringLiteral( "Key" ) ).toString();
    QString valueFieldName = config.value( QStringLiteral( "Value" ) ).toString();

    if ( fields.indexOf( keyFieldName ) >= 0 && fields.indexOf( valueFieldName ) >= 0 )
    {
      setKeyField( keyFieldName );
      setFeatureTitleField( valueFieldName );

      // store value relation filter expression
      setFilterExpression( config.value( QStringLiteral( "FilterExpression" ) ).toString() );

      loadFeaturesFromLayer( layer );
    }
    else
      CoreUtils::log( QStringLiteral( "FeaturesListModel" ), QStringLiteral( "Missing referenced fields for value relations." ) );
  }
  else
    CoreUtils::log( QStringLiteral( "FeaturesListModel" ), QStringLiteral( "Missing referenced table for value relations." ) );

  endResetModel();
}

void FeaturesListModel::populateFromLayer( QgsVectorLayer *layer )
{
  beginResetModel();
  emptyData();

  loadFeaturesFromLayer( layer );
  endResetModel();
}

void FeaturesListModel::reloadFeatures()
{
  loadFeaturesFromLayer();
}

void FeaturesListModel::emptyData()
{
  mFeatures.clear();
  mCurrentLayer = nullptr;
  mKeyField.clear();
  mFeatureTitleField.clear();
  mFilterExpression.clear();
  mSearchExpression.clear();
  mCurrentFeature = QgsFeature();
}

QHash<int, QByteArray> FeaturesListModel::roleNames() const
{
  QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
  roleNames[FeatureTitle] = QStringLiteral( "FeatureTitle" ).toLatin1();
  roleNames[FeatureId] = QStringLiteral( "FeatureId" ).toLatin1();
  roleNames[Feature] = QStringLiteral( "Feature" ).toLatin1();
  roleNames[FeaturePair] = QStringLiteral( "FeaturePair" ).toLatin1();
  roleNames[Description] = QStringLiteral( "Description" ).toLatin1();
  roleNames[FoundPair] = QStringLiteral( "FoundPair" ).toLatin1();
  roleNames[KeyColumn] = QStringLiteral( "KeyColumn" ).toLatin1();
  return roleNames;
}

int FeaturesListModel::featuresCount() const
{
  if ( mCurrentLayer && mCurrentLayer->isValid() )
    return mCurrentLayer->featureCount();
  return 0;
}

QString FeaturesListModel::searchExpression() const
{
  return mSearchExpression;
}

void FeaturesListModel::setSearchExpression( const QString &searchExpression )
{
  mSearchExpression = searchExpression;
  emit searchExpressionChanged( mSearchExpression );

  loadFeaturesFromLayer();
}

void FeaturesListModel::setFeatureTitleField( const QString &attribute )
{
  mFeatureTitleField = attribute;
}

void FeaturesListModel::setKeyField( const QString &attribute )
{
  mKeyField = attribute;
}

void FeaturesListModel::setFilterExpression( const QString &filterExpression )
{
  mFilterExpression = filterExpression;
}

void FeaturesListModel::setCurrentFeature( QgsFeature feature )
{
  if ( mCurrentFeature == feature )
    return;

  mCurrentFeature = feature;
  reloadFeatures();
  emit currentFeatureChanged( mCurrentFeature );
}

QgsFeature FeaturesListModel::currentFeature() const
{
  return mCurrentFeature;
}

int FeaturesListModel::featuresLimit() const
{
  return FEATURES_LIMIT;
}

int FeaturesListModel::rowFromAttribute( const int role, const QVariant &value ) const
{
  for ( int i = 0; i < mFeatures.count(); ++i )
  {
    QVariant d = data( index( i, 0 ), role );
    if ( d == value )
    {
      return i;
    }
  }
  return -1;
}

QVariant FeaturesListModel::attributeFromValue( const int role, const QVariant &value, const int requestedRole ) const
{
  for ( int i = 0; i < mFeatures.count(); ++i )
  {
    QVariant d = data( index( i, 0 ), role );
    if ( d.toString().trimmed() == value.toString().trimmed() )
    {
      QVariant key = data( index( i, 0 ), requestedRole );
      return key;
    }
  }
  return QVariant();
}

QVariant FeaturesListModel::convertMultivalueFormat( const QVariant &multivalue, const int role )
{
  QStringList list = QgsValueRelationFieldFormatter::valueToStringList( multivalue );
  QList<QVariant> retList;

  for ( const QString &i : list )
  {
    QVariant var = attributeFromValue( KeyColumn, i, role );
    if ( !var.isNull() )
      retList.append( var );
  }

  return retList;
}

FeatureLayerPair FeaturesListModel::featureLayerPair( const int &featureId )
{
  for ( const FeatureLayerPair &i : mFeatures )
  {
    if ( i.feature().id() == featureId )
      return i;
  }
  return FeatureLayerPair();
}
