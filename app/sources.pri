SOURCES += \
localprojectsmanager.cpp \
main.cpp \
merginprojectmetadata.cpp \
projectsmodel.cpp \
layersmodel.cpp \
loader.cpp \
digitizingcontroller.cpp \
mapthemesmodel.cpp \
appsettings.cpp \
merginapi.cpp \
merginapistatus.cpp \
merginprojectmodel.cpp \
androidutils.cpp \
inpututils.cpp \
test/testmerginapi.cpp

HEADERS += \
localprojectsmanager.h \
merginprojectmetadata.h \
projectsmodel.h \
layersmodel.h \
loader.h \
digitizingcontroller.h \
mapthemesmodel.h \
appsettings.h \
merginapi.h \
merginapistatus.h \
merginprojectmodel.h \
androidutils.h \
inpututils.h \
test/testmerginapi.h

RESOURCES += \
    img/pics.qrc \
    qml/qml.qrc \
    fonts/fonts.qrc

TRANSLATIONS +=

# this makes the manifest visible from Qt Creator
DISTFILES += \
    qml/MapThemePanel.qml \
    qml/Notification.qml