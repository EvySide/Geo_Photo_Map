#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTreeWidget>
#include <QComboBox>
#include <QPushButton>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QPixmap>
#include <QWebEngineView>
#include <QImageReader>

// Информация об одной фотографии
struct PhotoInfo
{
    QString filePath;      // путь к файлу фото (может быть пустой)
    double latitude;       // широта  (-90..90)
    double longitude;      // долгота (-180..180)
    QDateTime timestamp;   // время съёмки
    QString locationName;  // название места
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void resortList();                 // пересортировать список (по времени/месту)
    void onTreeSelectionChanged();     // подсветить соответствующий маркер
    void openDirectory();              // выбрать директорию с фото

private:
    QWebEngineView *m_mapView = nullptr;
    QTreeWidget *m_tree = nullptr;
    QPushButton *m_openButton = nullptr;
    QComboBox *m_sortCombo = nullptr;
    QLabel *m_previewImage = nullptr;
    QLabel *m_previewCaption = nullptr;
    QString m_currentRoot;

    QVector<PhotoInfo> m_photos;

    void setupUi();
    void applyDarkTheme();
    void loadSampleData();
    void createMap();
    void populateTree();
    void updatePreview(int photoIndex);
    QPixmap loadThumbnail(const QString &path, const QSize &size,
                          const QString &fallbackText = QString()) const;
    QPixmap placeholderThumbnail(const QSize &size, const QString &text) const;
    void scanDirectory(const QString &path);
    QTreeWidgetItem* ensureTreePath(const QStringList &parts, QMap<QString, QTreeWidgetItem*> &cache,
                                    QTreeWidgetItem *root, int photoIndex, const QString &fileName);
    QString buildMapHtml() const;
    void centerOnMarker(int index);
    void assignFallbackCoords(PhotoInfo &info, int seed) const;
    bool extractGpsFromExif(const QString &path, double &lat, double &lng) const;
    static bool parseExifCoord(const QString &value, const QString &ref, double &result);
};

#endif // MAINWINDOW_H
