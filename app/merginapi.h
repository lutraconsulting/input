#ifndef MERGINAPI_H
#define MERGINAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <memory>
#include <QFile>

#include "merginapistatus.h"

enum ProjectStatus
{
  NoVersion,
  UpToDate,
  OutOfDate,
  Modified
};
Q_ENUMS( ProjectStatus )

struct MerginProject
{
  QString name; //projectNamespace + projectName
  QStringList tags;
  QDateTime created;
  QDateTime updated; // local version of project files
  QDateTime serverUpdated; // available version of project files on server
  QDateTime lastSync; // local datetime of download/upload/update project
  bool pending = false; // if there is a pending request for downlaod/update a project
  ProjectStatus status = NoVersion;
  int size;
  int filesCount;
  int creator; // ID of current user
  //QList<int> owners;
  QList<int> writers;
};

struct MerginFile
{
  QString path;
  QString checksum;
  qint64 size;
};


typedef QList<std::shared_ptr<MerginProject>> ProjectList;

class MerginApi: public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString username READ username NOTIFY authChanged )
    Q_PROPERTY( int userId READ userId NOTIFY authChanged )
    Q_PROPERTY( int storageLimit READ storageLimit NOTIFY userInfoChanged )
    Q_PROPERTY( int diskUsage READ diskUsage NOTIFY userInfoChanged )
    Q_PROPERTY( QString apiRoot READ apiRoot WRITE setApiRoot NOTIFY apiRootChanged )
    Q_PROPERTY( int apiVersionStatus READ apiVersionStatus NOTIFY apiVersionStatusChanged )
  public:
    explicit MerginApi( const QString &dataDir, QObject *parent = nullptr );
    ~MerginApi() = default;

    /**
     * Sends non-blocking GET request to the server to listProjects. On listProjectsReplyFinished,
     * when a response is received, parses project json, writes it to a cache text file and sets mMerginProjects.
     * Eventually emits listProjectsFinished on which ProjectPanel (qml component) updates content.
     * If listing has been successful, updates cached merginProjects list.
     * \param searchExpression Search filter on projects name.
     * \param user Mergin username used with flag
     * \param flag If defined, it is used to filter out projects tagged as 'created' or 'shared' with given username
     * \param withFilter If true, applies "input" tag in request.
     */
    Q_INVOKABLE void listProjects( const QString &searchExpression = QStringLiteral(), const QString &user = QStringLiteral(),
                                   const QString &flag = QStringLiteral(), const QString &filterTag = QStringLiteral( "input_use" ) );

    /**
     * Sends non-blocking GET request to the server to download a project with a given name. On downloadProjectReplyFinished,
     * when a response is received, parses data-stream and creates files. Eventually emits syncProjectFinished on which
     * MerginProjectModel updates status of the project item. On syncProjectFinished, ProjectModel adds the project item to the project list.
     * If download has been successful, updates cached merginProjects list.
     * Emits also notify signal with a message for the GUI.
     * \param projectName Name of project to download.
     */
    Q_INVOKABLE void downloadProject( const QString &projectName );

    /**
     * Sends non-blocking POST request to the server to update a project with a given name. On downloadProjectReplyFinished,
     * when a response is received, parses data-stream to files and rewrites local files with them. Extra files which don't match server
     * files are removed. Eventually emits syncProjectFinished on which MerginProjectModel updates status of the project item.
     * If update has been successful, updates cached merginProjects list.
     * Emits also notify signal with a message for the GUI.
     * \param projectName Name of project to update.
     */
    Q_INVOKABLE void updateProject( const QString &projectName );

    /**
     * Sends non-blocking POST request to the server to upload changes in a project with a given name.
     * Firstly updateProject is triggered to fetch new changes. If it was successful, sends update post request with list of local changes
     * and modified/newly added files in JSON. Eventually emits syncProjectFinished on which MerginProjectModel updates status of the project item.
     * Emits also notify signal with a message for the GUI.
     * \param projectName Name of project to upload.
     */
    Q_INVOKABLE void uploadProject( const QString &projectName );

    /**
    * Currently no auth service is used, only "username:password" is encoded and asign to mToken.
    * \param username Login user name to Mergin
    * \param password Password to given username to log in to Mergin
    */
    Q_INVOKABLE void authorize( const QString &username, const QString &password );
    Q_INVOKABLE void getUserInfo( const QString &username );
    Q_INVOKABLE void clearAuth();
    Q_INVOKABLE void resetApiRoot();
    Q_INVOKABLE bool hasAuthData();
    /**
    * Pings Mergin server and checks its version with required version defined in version.pri
    * Accordingly sets mApiVersionStatus variable (reset when mergin url is changed).
    * The function is skipped if version has been checked and passed.
    */
    Q_INVOKABLE void pingMergin();

    // Test functions
    void createProject( const QString &projectName );
    void deleteProject( const QString &projectName );
    void clearTokenData();

    ProjectList projects();

    QString username() const;

    QString apiRoot() const;
    void setApiRoot( const QString &apiRoot );

    //! Disk usage of current logged in user in Mergin instance in Bytes
    int diskUsage() const;

    //! Total storage limit of current logged in user in Mergin instance in Bytes
    int storageLimit() const;

    int userId() const;
    void setUserId( int userId );

    MerginApiStatus::VersionStatus apiVersionStatus() const;
    void setApiVersionStatus( const MerginApiStatus::VersionStatus &apiVersionStatus );

  signals:
    void listProjectsFinished( const ProjectList &merginProjects );
    void syncProjectFinished( const QString &projectDir, const QString &projectName, bool successfully = true );
    void reloadProject( const QString &projectDir );
    void networkErrorOccurred( const QString &message, const QString &additionalInfo );
    void apiIncompatibilityOccured( const QString &message, const QString &additionalInfo );
    void notify( const QString &message );
    void merginProjectsChanged();
    void authRequested();
    void authChanged();
    void authFailed();
    void apiRootChanged();
    void apiVersionStatusChanged();
    void projectCreated( const QString &projectName );
    void serverProjectDeleted( const QString &projectName );
    void userInfoChanged();
    void pingMerginFinished( const QString &apiVersion, const QString &msg );

  public slots:
    void projectDeleted( const QString &projectName );

  private slots:
    void listProjectsReplyFinished();
    void downloadProjectReplyFinished(); // download + update
    void uploadProjectReplyFinished();
    void updateInfoReplyFinished();
    void uploadInfoReplyFinished();
    void getUserInfoFinished();
    void cacheProjects();
    void continueWithUpload( const QString &projectDir, const QString &projectName, bool successfully = true );
    void setUpdateToProject( const QString &projectDir, const QString &projectName, bool successfully );
    void saveAuthData();
    void createProjectFinished();
    void deleteProjectFinished();
    void authorizeFinished();
    void pingMerginReplyFinished();

  private:
    ProjectList parseProjectsData( const QByteArray &data, bool dataFromServer = false );
    bool cacheProjectsData( const QByteArray &data );
    void handleDataStream( QNetworkReply *r, const QString &projectDir, bool overwrite );
    bool saveFile( const QByteArray &data, QFile &file, bool closeFile );
    void createPathIfNotExists( const QString &filePath );
    /**
    *
    * \param localUpdated Timestamp version of local copy of the project
    * \param updated Timestamp version of latest project on a server
    * \param lastSync Timestamp of last successfull sync with a server
    * \param lastMod Timestamp of last modification on local copy of the project
    */
    ProjectStatus getProjectStatus( const QDateTime &localUpdated, const QDateTime &updated, const QDateTime &lastSync, const QDateTime &lastMod );
    QDateTime getLastModifiedFileDateTime( const QString &path );
    QByteArray getChecksum( const QString &filePath );
    QSet<QString> listFiles( const QString &projectPath );
    void downloadProjectFiles( const QString &projectName, const QByteArray &json );
    void uploadProjectFiles( const QString &projectName, const QByteArray &json, const QList<MerginFile> &files );
    QHash<QString, QList<MerginFile>> parseAndCompareProjectFiles( QNetworkReply *r, bool isForUpdate );
    ProjectList updateMerginProjectList( const ProjectList &serverProjects );
    void deleteObsoleteFiles( const QString &projectName );
    QByteArray generateToken();
    void loadAuthData();
    static QString defaultApiRoot() { return "https://public.cloudmergin.com/"; }
    bool validateAuthAndContinute();
    void checkMerginVersion( QString apiVersion, QString msg = QStringLiteral() );

    QNetworkAccessManager mManager;
    QString mApiRoot;
    ProjectList mMerginProjects;
    QString mDataDir; // dir with all projects
    QString mCacheFile;
    QString mUsername;
    QString mPassword;
    int mUserId = -1;
    QByteArray mAuthToken;
    QDateTime mTokenExpiration;
    int mDiskUsage = 0; // in Bytes
    int mStorageLimit = 0; // in Bytes
    QHash<QUrl, QString >mPendingRequests; // projectNamespace/projectName
    QSet<QString> mWaitingForUpload; // projectNamespace/projectName
    QHash<QString, QSet<QString>> mObsoleteFiles;
    QSet<QString> mIgnoreFiles = QSet<QString>() << "gpkg-shm" << "gpkg-wal" << "qgs~" << "qgz~";
    QEventLoop mAuthLoopEvent;
    MerginApiStatus::VersionStatus mApiVersionStatus = MerginApiStatus::VersionStatus::UNKNOWN;

    const int CHUNK_SIZE = 65536;
};

#endif // MERGINAPI_H
