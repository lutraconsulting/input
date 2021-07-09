/***************************************************************************
  iosimagepicker.h
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

#ifndef IOSIMAGEPICKER_H
#define IOSIMAGEPICKER_H

#include <QObject>
#include <QVariantMap>

class PositionKit;
class Compass;

/**
 * The class suppose to be used in QML to invoke iOS image picker and postprocess the image if any has been choosen.
*/
class IOSImagePicker : public QObject
{
    Q_OBJECT
  public:
    ~IOSImagePicker() = default;
    /**
    * Method suppose to be used in QML and calls IOSImagePicker::showImagePickerDirect which invokes IOSViewDelegate and image picker.
    * \param targetDir - String representing directory path where captured photo suppose to be saved.
    */
    Q_INVOKABLE void showImagePicker( const QString  &targetDir );

    /**
    * Method suppose to be used in QML and calls IOSImagePicker::showImagePickerDirect which invokes IOSViewDelegate and image picker.
    * \param targetDir - String representing directory path where captured photo suppose to be saved.
    * \param position - object to get GPS EXIF data from
    * \param compass - object to get GPS direction for EXIF data
    */
    Q_INVOKABLE void callCamera( const QString  &targetDir, PositionKit *positionKit, Compass *compass );

    /**
     * Calls the objective-c function to read EXIF metadata.
     */
    static QString readExifDirect( const QString &filepath, const QString &tag );

    QString targetDir() const;
    void setTargetDir( const QString &targetDir );
    void setPositionKit( PositionKit *positionKit );
    PositionKit *positionKit() const;

    Compass *compass() const;
    void setCompass( Compass *compass );

  signals:
    void targetDirChanged();
    void positionKitChanged();
    void compassChanged();
    void imageCaptured( const QString &absoluteImagePath );
    void notify( const QString &message );

  public slots:
    /**
     * Callback after succesfuly captured photo - saves image at targetDir location.
     * After successful image selection from a gallery, only emits a singal with final location from result data.
     */
    void onImagePickerFinished( bool successful, const QVariantMap &data );

  private:
    QString mTargetDir;
    PositionKit *mPositionKit = nullptr;
    Compass *mCompass = nullptr;

    /**
     * Calls the objective-c function to show image picker.
     */
    void showImagePickerDirect( IOSImagePicker *handler );
    /**
     * Calls the objective-c function to open camera.
     */
    void callCameraDirect( IOSImagePicker *handler );
};
#endif // IOSIMAGEPICKER_H
