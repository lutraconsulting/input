#include "layerfeaturesmodel.h"
#include <QDebug>

LayerFeaturesModel::LayerFeaturesModel( QObject *parent, LayersModel *lm )
  : QAbstractListModel( parent ), p_layerModel(lm)
{
}

int LayerFeaturesModel::rowCount( const QModelIndex &parent ) const
{
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if ( parent.isValid() )
    return 0;

  return m_features.count();
}

QVariant LayerFeaturesModel::data( const QModelIndex &index, int role ) const
{
  int row = index.row();
  if ( row < 0 || row >= m_features.count() )
    return QVariant();

  if (role < roleNames::id || role > roleNames::displayName )
    return QVariant();

  if ( !index.isValid() )
    return QVariant();

  QgsFeature feat = m_features.at(index.row());

  return QVariant( "Test" );
}

void LayerFeaturesModel::reloadDataFromLayerName( const QString &layerName )
{
  Q_UNUSED( layerName );

  // We mock layerName because it is not yet implemented
  QgsMapLayer *mockedLayer = p_layerModel->activeLayer();

  if ( mockedLayer->type() == QgsMapLayerType::VectorLayer )
    this->reloadDataFromLayer( qobject_cast<QgsVectorLayer *>( mockedLayer ) );
}

void LayerFeaturesModel::reloadDataFromLayer( const QgsVectorLayer *layer )
{
  beginResetModel();
  m_features.clear();

  if ( layer )
  {
    QgsFeatureRequest req;

    req.setLimit( FEATURES_LIMIT );

    QgsFeatureIterator it = layer->getFeatures( req );
    QgsFeature f;

    while( it.nextFeature( f ) )
      m_features << f;
  }

  endResetModel();
}

QHash<int, QByteArray> LayerFeaturesModel::roleNames() const
{
  QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
  roleNames[id] = "id";
  roleNames[displayName] = "displayName";
  return roleNames;
}

bool LayerFeaturesModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  // Mocked method - for future when attributes will be editable (it changes data)
  Q_UNUSED(index);
  Q_UNUSED(value);
  Q_UNUSED(role);
  return false;
}

Qt::ItemFlags LayerFeaturesModel::flags( const QModelIndex &index ) const
{
  // Mocked method - for future when attributes will be editable (it checks if data is editable)
  if ( !index.isValid() )
    return Qt::NoItemFlags;

  return Qt::ItemIsEditable;
}
