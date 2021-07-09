/***************************************************************************
  iosimagepicker.cpp
  --------------------------------------
  Date                 : Nov 2019
  Copyright            : (C) 2019 by Viktor Sklencar
  Email                : viktor.sklencar@lutraconsulting.co.uk
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "iosimagepicker.h"
#include "qdatetime.h"

#include <QDebug>
#include <QImage>
#include <QImageWriter>
#include <QUrl>

void IOSImagePicker::showImagePicker( const QString  &targetDir )
{
#ifdef Q_OS_IOS
  setTargetDir( targetDir );
  showImagePickerDirect( this );
#else
  Q_UNUSED( targetDir )
#endif
}

void IOSImagePicker::callCamera( const QString &targetDir, PositionKit *positionKit, Compass *compass )
{
#ifdef Q_OS_IOS
  setTargetDir( targetDir );
  setPositionKit( positionKit );
  setCompass( compass );
  callCameraDirect( this );
#else
  Q_UNUSED( targetDir )
  Q_UNUSED( positionKit )
  Q_UNUSED( compass )
#endif
}

QString IOSImagePicker::targetDir() const
{
  return mTargetDir;
}

void IOSImagePicker::setTargetDir( const QString &targetDir )
{
  mTargetDir = targetDir;
  emit targetDirChanged();
}

void IOSImagePicker::setPositionKit( PositionKit *positionKit )
{
  mPositionKit = positionKit;
  emit positionKitChanged();
}

void IOSImagePicker::onImagePickerFinished( bool successful, const QVariantMap &data )
{
  if ( successful )
  {
    QString imagePath = data["imagePath"].value<QString>();
    emit imageCaptured( imagePath );
  }
  else
  {
    QString msg = QString( "Error:%1" ).arg( data["error"].value<QString>() );
    qWarning() << msg;
    emit notify( msg );
  }
}

Compass *IOSImagePicker::compass() const
{
  return mCompass;
}

void IOSImagePicker::setCompass( Compass *compass )
{
  mCompass = compass;
  emit compassChanged();
}

PositionKit *IOSImagePicker::positionKit() const
{
  return mPositionKit;
}
