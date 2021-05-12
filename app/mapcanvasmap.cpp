/***************************************************************************
  mapcanvasmap.cpp
  --------------------------------------
  Date                 : 10.12.2014
  Copyright            : (C) 2014 by Matthias Kuhn
  Email                : matthias (at) opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QQuickWindow>
#include <QScreen>
#include <QSGSimpleTextureNode>
#include <QtConcurrent>

#include "qgsmaprendererparalleljob.h"
#include "qgsmessagelog.h"
#include "qgspallabeling.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgis.h"

#include "mapcanvasmap.h"
#include "mapsettings.h"
#include "qgsexpressioncontextutils.h"


MapCanvasMap::MapCanvasMap( QQuickItem *parent )
  : QQuickItem( parent )
  , mMapSettings( new MapSettings() )
{
  connect( this, &QQuickItem::windowChanged, this, &MapCanvasMap::onWindowChanged );
  connect( &mRefreshTimer, &QTimer::timeout, this, &MapCanvasMap::refreshMap );
  connect( &mMapUpdateTimer, &QTimer::timeout, this, &MapCanvasMap::renderJobUpdated );

  connect( mMapSettings.get(), &MapSettings::extentChanged, this, &MapCanvasMap::onExtentChanged );
  connect( mMapSettings.get(), &MapSettings::layersChanged, this, &MapCanvasMap::onLayersChanged );

  connect( this, &MapCanvasMap::renderStarting, this, &MapCanvasMap::isRenderingChanged );
  connect( this, &MapCanvasMap::mapCanvasRefreshed, this, &MapCanvasMap::isRenderingChanged );

  mMapUpdateTimer.setSingleShot( false );
  mMapUpdateTimer.setInterval( 250 );
  mRefreshTimer.setSingleShot( true );
  setTransformOrigin( QQuickItem::TopLeft );
  setFlags( QQuickItem::ItemHasContents );
}

MapSettings *MapCanvasMap::mapSettings() const
{
  return mMapSettings.get();
}

void MapCanvasMap::zoom( QPointF center, qreal scale )
{
  QgsRectangle extent = mMapSettings->extent();
  QgsPoint oldCenter( extent.center() );
  QgsPoint mousePos( mMapSettings->screenToCoordinate( center ) );
  QgsPointXY newCenter( mousePos.x() + ( ( oldCenter.x() - mousePos.x() ) * scale ),
                        mousePos.y() + ( ( oldCenter.y() - mousePos.y() ) * scale ) );

  // same as zoomWithCenter (no coordinate transformations are needed)
  extent.scale( scale, &newCenter );
  mMapSettings->setExtent( extent );
  mNeedsRefresh = true;
}

void MapCanvasMap::pan( QPointF oldPos, QPointF newPos )
{
  QgsPoint start = mMapSettings->screenToCoordinate( oldPos.toPoint() );
  QgsPoint end = mMapSettings->screenToCoordinate( newPos.toPoint() );

  double dx = end.x() - start.x();
  double dy = end.y() - start.y();

  // modify the extent
  QgsRectangle extent = mMapSettings->extent();

  extent.setXMinimum( extent.xMinimum() + dx );
  extent.setXMaximum( extent.xMaximum() + dx );
  extent.setYMaximum( extent.yMaximum() + dy );
  extent.setYMinimum( extent.yMinimum() + dy );

  mMapSettings->setExtent( extent );
  mNeedsRefresh = true;
}

void MapCanvasMap::refreshMap()
{
  stopRendering(); // if any...

  QgsMapSettings mapSettings = mMapSettings->mapSettings();

  //build the expression context
  QgsExpressionContext expressionContext;
  expressionContext << QgsExpressionContextUtils::globalScope()
                    << QgsExpressionContextUtils::mapSettingsScope( mapSettings );

  QgsProject *project = mMapSettings->project();
  if ( project )
  {
    expressionContext << QgsExpressionContextUtils::projectScope( project );

    mapSettings.setLabelingEngineSettings( project->labelingEngineSettings() );
  }

  mapSettings.setExpressionContext( expressionContext );

  // enables on-the-fly simplification of geometries to spend less time rendering
  mapSettings.setFlag( QgsMapSettings::UseRenderingOptimization );
  // with incremental rendering - enables updates of partially rendered layers (good for WMTS, XYZ layers)
  mapSettings.setFlag( QgsMapSettings::RenderPartialOutput, mIncrementalRendering );

  // create the renderer job
  Q_ASSERT( !mJob );
  mJob = new QgsMapRendererParallelJob( mapSettings );

  if ( mIncrementalRendering )
    mMapUpdateTimer.start();

  connect( mJob, &QgsMapRendererJob::renderingLayersFinished, this, &MapCanvasMap::renderJobUpdated );
  connect( mJob, &QgsMapRendererJob::finished, this, &MapCanvasMap::renderJobFinished );
  mJob->setCache( mCache );

  mJob->start();

  emit renderStarting();
}

void MapCanvasMap::renderJobUpdated()
{
  mImage = mJob->renderedImage();
  mImageMapSettings = mJob->mapSettings();
  mDirty = true;
  // Temporarily freeze the canvas, we only need to reset the geometry but not trigger a repaint
  bool freeze = mFreeze;
  mFreeze = true;
  updateTransform();
  mFreeze = freeze;

  update();
  emit mapCanvasRefreshed();
}

void MapCanvasMap::renderJobFinished()
{
  const QgsMapRendererJob::Errors errors = mJob->errors();
  for ( const QgsMapRendererJob::Error &error : errors )
  {
    QgsMessageLog::logMessage( QStringLiteral( "%1 :: %2" ).arg( error.layerID, error.message ), tr( "Rendering" ) );
  }

  // take labeling results before emitting renderComplete, so labeling map tools
  // connected to signal work with correct results
  delete mLabelingResults;
  mLabelingResults = mJob->takeLabelingResults();

  mImage = mJob->renderedImage();
  mImageMapSettings = mJob->mapSettings();

  // now we are in a slot called from mJob - do not delete it immediately
  // so the class is still valid when the execution returns to the class
  mJob->deleteLater();
  mJob = nullptr;
  mDirty = true;
  mMapUpdateTimer.stop();

  // Temporarily freeze the canvas, we only need to reset the geometry but not trigger a repaint
  bool freeze = mFreeze;
  mFreeze = true;
  updateTransform();
  mFreeze = freeze;

  update();
  emit mapCanvasRefreshed();
}

void MapCanvasMap::onWindowChanged( QQuickWindow *window )
{
  // FIXME? the above disconnect is done potentially on a nullptr
  // cppcheck-suppress nullPointerRedundantCheck
  disconnect( window, &QQuickWindow::screenChanged, this, &MapCanvasMap::onScreenChanged );
  if ( window )
  {
    connect( window, &QQuickWindow::screenChanged, this, &MapCanvasMap::onScreenChanged );
    onScreenChanged( window->screen() );
  }
}

void MapCanvasMap::onScreenChanged( QScreen *screen )
{
  if ( screen )
    mMapSettings->setOutputDpi( screen->physicalDotsPerInch() );
}

void MapCanvasMap::onExtentChanged()
{
  updateTransform();

  // And trigger a new rendering job
  refresh();
}

void MapCanvasMap::updateTransform()
{
  QgsMapSettings currentMapSettings = mMapSettings->mapSettings();
  QgsMapToPixel mtp = currentMapSettings.mapToPixel();

  QgsRectangle imageExtent = mImageMapSettings.visibleExtent();
  QgsRectangle newExtent = currentMapSettings.visibleExtent();
  QgsPointXY pixelPt = mtp.transform( imageExtent.xMinimum(), imageExtent.yMaximum() );
  setScale( imageExtent.width() / newExtent.width() );

  setX( pixelPt.x() );
  setY( pixelPt.y() );
}

int MapCanvasMap::mapUpdateInterval() const
{
  return mMapUpdateTimer.interval();
}

void MapCanvasMap::setMapUpdateInterval( int mapUpdateInterval )
{
  if ( mMapUpdateTimer.interval() == mapUpdateInterval )
    return;

  mMapUpdateTimer.setInterval( mapUpdateInterval );

  emit mapUpdateIntervalChanged();
}

bool MapCanvasMap::incrementalRendering() const
{
  return mIncrementalRendering;
}

void MapCanvasMap::setIncrementalRendering( bool incrementalRendering )
{
  if ( incrementalRendering == mIncrementalRendering )
    return;

  mIncrementalRendering = incrementalRendering;
  emit incrementalRenderingChanged();
}

bool MapCanvasMap::freeze() const
{
  return mFreeze;
}

void MapCanvasMap::setFreeze( bool freeze )
{
  if ( freeze == mFreeze )
    return;

  mFreeze = freeze;

  if ( !mFreeze && mNeedsRefresh )
  {
    refresh();
  }

  // we are freezing or unfreezing - either way we can reset "needs refresh"
  mNeedsRefresh = false;

  emit freezeChanged();
}

bool MapCanvasMap::isRendering() const
{
  return mJob;
}

QSGNode *MapCanvasMap::updatePaintNode( QSGNode *oldNode, QQuickItem::UpdatePaintNodeData * )
{
  if ( mDirty )
  {
    delete oldNode;
    oldNode = nullptr;
    mDirty = false;
  }

  QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>( oldNode );
  if ( !node )
  {
    node = new QSGSimpleTextureNode();
    QSGTexture *texture = window()->createTextureFromImage( mImage );
    node->setTexture( texture );
    node->setOwnsTexture( true );
  }

  QRectF rect( boundingRect() );

  // Check for resizes that change the w/h ratio
  if ( !rect.isEmpty() &&
       !mImage.size().isEmpty() &&
       !qgsDoubleNear( rect.width() / rect.height(), mImage.width() / mImage.height() ) )
  {
    if ( qgsDoubleNear( rect.height(), mImage.height() ) )
    {
      rect.setHeight( rect.width() / mImage.width() * mImage.height() );
    }
    else
    {
      rect.setWidth( rect.height() / mImage.height() * mImage.width() );
    }
  }

  node->setRect( rect );

  return node;
}

void MapCanvasMap::geometryChanged( const QRectF &newGeometry, const QRectF &oldGeometry )
{
  Q_UNUSED( oldGeometry )
  // The Qt documentation advices to call the base method here.
  // However, this introduces instabilities and heavy performance impacts on Android.
  // It seems on desktop disabling it prevents us from downsizing the window...
  // Be careful when re-enabling it.
  // QQuickItem::geometryChanged( newGeometry, oldGeometry );

  mMapSettings->setOutputSize( newGeometry.size().toSize() );
  refresh();
}

void MapCanvasMap::onLayersChanged()
{
  if ( mMapSettings->extent().isEmpty() )
    zoomToFullExtent();

  for ( const QMetaObject::Connection &conn : qgis::as_const( mLayerConnections ) )
  {
    disconnect( conn );
  }
  mLayerConnections.clear();

  const QList<QgsMapLayer *> layers = mMapSettings->layers();
  for ( QgsMapLayer *layer : layers )
  {
    mLayerConnections << connect( layer, &QgsMapLayer::repaintRequested, this, &MapCanvasMap::refresh );
  }

  refresh();
}

void MapCanvasMap::destroyJob( QgsMapRendererJob *job )
{
  job->cancel();
  job->deleteLater();
}

void MapCanvasMap::stopRendering()
{
  if ( mJob )
  {
    disconnect( mJob, &QgsMapRendererJob::renderingLayersFinished, this, &MapCanvasMap::renderJobUpdated );
    disconnect( mJob, &QgsMapRendererJob::finished, this, &MapCanvasMap::renderJobFinished );

    mJob->cancelWithoutBlocking();
    mJob = nullptr;
  }
}

void MapCanvasMap::zoomToFullExtent()
{
  QgsRectangle extent;
  const QList<QgsMapLayer *> layers = mMapSettings->layers();
  for ( QgsMapLayer *layer : layers )
  {
    if ( mMapSettings->destinationCrs() != layer->crs() )
    {
      QgsCoordinateTransform transform( layer->crs(), mMapSettings->destinationCrs(), mMapSettings->transformContext() );
      extent.combineExtentWith( transform.transformBoundingBox( layer->extent() ) );
    }
    else
    {
      extent.combineExtentWith( layer->extent() );
    }
  }
  mMapSettings->setExtent( extent );

  refresh();
}

void MapCanvasMap::refresh()
{
  if ( mMapSettings->outputSize().isNull() )
    return;  // the map image size has not been set yet

  if ( !mFreeze )
    mRefreshTimer.start( 1 );
}
