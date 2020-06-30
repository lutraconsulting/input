#ifndef LAYERFEATURESMODEL_H
#define LAYERFEATURESMODEL_H

#include <QAbstractListModel>
#include "layersmodel.h"

#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgsfeaturerequest.h"
#include "qgsfeatureiterator.h"

class LayerFeaturesModel : public QAbstractListModel
{
    Q_OBJECT

  enum roleNames {
    id = Qt::UserRole + 1,
    displayName
  };

  public:
    explicit LayerFeaturesModel( LayersModel *layersModel, QObject *parent = nullptr );

    // Basic functionality:
    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;

    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;

    QHash<int, QByteArray> roleNames() const override;

    // Editable:
    bool setData( const QModelIndex &index, const QVariant &value,
                  int role = Qt::EditRole ) override;

    Qt::ItemFlags flags( const QModelIndex &index ) const override;

public slots:
    void reloadDataFromLayerName( const QString &layerName ); // mock
    void reloadDataFromLayer( const QgsVectorLayer *layer );


  private:
    QList<QgsFeature> m_features;
    LayersModel *p_layerModel;

    const char FEATURES_LIMIT = 100;
};

#endif // LAYERFEATURESMODEL_H
