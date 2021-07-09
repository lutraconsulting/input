/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "iosutils.h"
#include "positionkit.h"

IosUtils::IosUtils( QObject *parent ): QObject( parent )
{
#ifdef Q_OS_IOS
  setIdleTimerDisabled();
#endif
  mImagePicker = new IOSImagePicker();
  QObject::connect( mImagePicker, &IOSImagePicker::imageCaptured, this, &IosUtils::imageSelected );
  QObject::connect( mImagePicker, &IOSImagePicker::notify, this, &IosUtils::showToast );
}

bool IosUtils::isIos() const
{
#ifdef Q_OS_IOS
  return true;
#else
  return false;
#endif
}

void IosUtils::callImagePicker( const QString &targetPath )
{
  mImagePicker->showImagePicker( targetPath );
}

void IosUtils::callCamera( const QString &targetPath )
{
  mImagePicker->callCamera( targetPath, mPositionKit, mCompass );
}

IOSImagePicker *IosUtils::imagePicker() const
{
  return mImagePicker;
}

QString IosUtils::readExif( const QString &filepath, const QString &tag )
{
#ifdef Q_OS_IOS
  return IOSImagePicker::readExifDirect( filepath, tag );
#else
  Q_UNUSED( filepath )
  Q_UNUSED( tag )
  return QString();
#endif
}
