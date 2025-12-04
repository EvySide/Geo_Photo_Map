#include "mainwindow.h"

#include <QApplication>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QIcon>
#include <QFrame>
#include <QLinearGradient>
#include <QPalette>
#include <QSizePolicy>
#include <QFile>
#include <QUrl>
#include <QDir>
#include <QCoreApplication>
#include <QFileDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QStatusBar>
#include <QtMath>
#include <QWebEngineView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>
#include <cmath>

// ---------------------------
// Реализация MainWindow
// ---------------------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    applyDarkTheme();
    setupUi();
    loadSampleData();
    createMap();
    populateTree();

    statusBar()->showMessage(
        tr("Всего фотографий: %1").arg(m_photos.size())
        );
}

void MainWindow::applyDarkTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(18, 21, 27));
    palette.setColor(QPalette::WindowText, QColor("#e9ecf2"));
    palette.setColor(QPalette::Base, QColor(20, 23, 30));
    palette.setColor(QPalette::AlternateBase, QColor(24, 28, 36));
    palette.setColor(QPalette::Text, QColor("#e9ecf2"));
    palette.setColor(QPalette::Button, QColor(24, 28, 34));
    palette.setColor(QPalette::ButtonText, QColor("#e9ecf2"));
    palette.setColor(QPalette::Highlight, QColor("#3ba9ff"));
    palette.setColor(QPalette::HighlightedText, QColor("#0b1016"));
    palette.setColor(QPalette::ToolTipBase, QColor(34, 38, 46));
    palette.setColor(QPalette::ToolTipText, QColor("#f2f4f7"));
    palette.setColor(QPalette::PlaceholderText, QColor("#7c8696"));
    qApp->setPalette(palette);

    const QString css = QStringLiteral(
        "QWidget { background-color: #12151c; color: #e9ecf2; }"
        "QToolTip { color: #e9ecf2; background-color: #20242c; border: 1px solid #3ba9ff; }"
        "QTreeWidget { background-color: #141821; border: 1px solid #2b303d; }"
        "QTreeWidget::item { padding: 6px; }"
        "QTreeWidget::item:selected { background-color: #243245; }"
        "QComboBox, QAbstractItemView { background-color: #1c2029; border: 1px solid #303544; padding: 4px; }"
        "QLabel#PreviewFrame { background-color: #0f131b; border: 1px solid #2e3442; border-radius: 8px; }"
        "QLabel#PreviewCaption { color: #b6bdc9; }"
        "QWebEngineView { background: #0f131b; border: 1px solid #1f2532; }"
        "QStatusBar { background-color: #0f1116; border-top: 1px solid #262b36; }"
    );

    qApp->setStyleSheet(css);
}

void MainWindow::setupUi()
{
    resize(1280, 760);

    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // Левая часть: сортировка + список
    auto *leftLayout = new QVBoxLayout;
    leftLayout->setSpacing(10);
    auto *controlsLayout = new QHBoxLayout;

    m_openButton = new QPushButton(tr("Open"), central);
    m_openButton->setCursor(Qt::PointingHandCursor);
    m_openButton->setStyleSheet("font-weight: 600;");

    auto *sortLabel = new QLabel(tr("Сортировка:"), central);
    m_sortCombo = new QComboBox(central);
    m_sortCombo->addItem(tr("По времени"));
    m_sortCombo->addItem(tr("По месту (название)"));

    controlsLayout->addWidget(m_openButton);
    controlsLayout->addWidget(sortLabel);
    controlsLayout->addWidget(m_sortCombo);
    controlsLayout->addStretch();

    m_tree = new QTreeWidget(central);
    m_tree->setHeaderHidden(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setIconSize(QSize(96, 72));
    m_tree->setMinimumWidth(340);
    m_tree->setExpandsOnDoubleClick(true);
    m_tree->setAnimated(true);

    leftLayout->addLayout(controlsLayout);
    leftLayout->addWidget(m_tree, 1);

    auto *previewTitle = new QLabel(tr("Предпросмотр"), central);
    previewTitle->setStyleSheet("font-weight: 600; letter-spacing: 0.2px;");

    m_previewImage = new QLabel(central);
    m_previewImage->setObjectName("PreviewFrame");
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setMinimumHeight(220);
    m_previewImage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_previewCaption = new QLabel(tr("Выберите снимок"), central);
    m_previewCaption->setObjectName("PreviewCaption");
    m_previewCaption->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_previewCaption->setWordWrap(true);

    leftLayout->addWidget(previewTitle);
    leftLayout->addWidget(m_previewImage);
    leftLayout->addWidget(m_previewCaption);

    // Правая часть: карта
    m_mapView = new QWebEngineView(central);
    m_mapView->setContextMenuPolicy(Qt::NoContextMenu);
    m_mapView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_mapView->setZoomFactor(1.0);

    mainLayout->addLayout(leftLayout, 0);
    mainLayout->addWidget(m_mapView, 1);
    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 3);

    setCentralWidget(central);
    setWindowTitle(tr("GeoPhotoMap - карта снимков"));

    connect(m_openButton, &QPushButton::clicked,
            this, &MainWindow::openDirectory);
    connect(m_sortCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::resortList);
    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onTreeSelectionChanged);
}

void MainWindow::createMap()
{
    if (!m_mapView)
        return;

    const QString html = buildMapHtml();
    m_mapView->setHtml(html, QUrl("https://local.map/"));
}

// Простейшие тестовые данные
void MainWindow::loadSampleData()
{
    m_photos.clear();

    const QString baseDir = QCoreApplication::applicationDirPath();
    m_currentRoot = baseDir;

    PhotoInfo p1;
    p1.filePath = baseDir + "/sample1.jpg";  // можешь положить сюда реальное фото
    p1.latitude = 55.7558;
    p1.longitude = 37.6173;
    p1.timestamp = QDateTime(QDate(2024, 5, 10), QTime(14, 30));
    p1.locationName = tr("Москва");
    m_photos.append(p1);

    PhotoInfo p2;
    p2.filePath = baseDir + "/sample2.jpg";
    p2.latitude = 59.9375;
    p2.longitude = 30.3086;
    p2.timestamp = QDateTime(QDate(2023, 8, 20), QTime(18, 15));
    p2.locationName = tr("Санкт-Петербург");
    m_photos.append(p2);

    PhotoInfo p3;
    p3.filePath = baseDir + "/sample3.jpg";
    p3.latitude = 48.8566;
    p3.longitude = 2.3522;
    p3.timestamp = QDateTime(QDate(2022, 7, 3), QTime(12, 5));
    p3.locationName = tr("Париж");
    m_photos.append(p3);

    // Здесь в реальном приложении можно читать EXIF
    // и подставлять реальные координаты и время
}

void MainWindow::assignCoordinatesIfMissing(PhotoInfo &info, int index) const
{
    if (info.latitude != 0.0 || info.longitude != 0.0)
        return;

    const double lon = std::fmod(index * 42.0, 360.0) - 180.0;
    const double lat = qBound(-80.0, 80.0, 60.0 * std::sin(index * 0.7));
    info.latitude = lat;
    info.longitude = lon;
}

void MainWindow::scanDirectory(const QString &path)
{
    QVector<PhotoInfo> loaded;

    QDirIterator it(path,
                    QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif" << "*.webp",
                    QDir::Files,
                    QDirIterator::Subdirectories);

    int counter = 0;
    while (it.hasNext()) {
        const QString file = it.next();
        QFileInfo fi(file);
        PhotoInfo info;
        info.filePath = file;
        info.timestamp = fi.lastModified();
        info.locationName = fi.absoluteDir().dirName();
        info.latitude = 0.0;
        info.longitude = 0.0;
        assignCoordinatesIfMissing(info, counter);
        loaded.append(info);
        ++counter;
    }

    if (loaded.isEmpty()) {
        statusBar()->showMessage(tr("В папке нет поддерживаемых изображений"), 4000);
        return;
    }

    m_currentRoot = path;
    m_photos = loaded;
    createMap();
    populateTree();
    statusBar()->showMessage(
        tr("Загружено %1 фото из \"%2\"")
            .arg(m_photos.size())
            .arg(QDir(path).dirName()),
        4000);
}

void MainWindow::openDirectory()
{
    const QString startPath = !m_currentRoot.isEmpty()
            ? m_currentRoot
            : QDir::homePath();
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Выберите папку с фотографиями"),
        startPath);

    if (dir.isEmpty())
        return;

    scanDirectory(dir);
}

void MainWindow::populateTree()
{
    if (!m_tree)
        return;

    m_tree->clear();

    if (m_photos.isEmpty()) {
        updatePreview(-1);
        return;
    }

    QVector<int> indices;
    indices.reserve(m_photos.size());
    for (int i = 0; i < m_photos.size(); ++i)
        indices.append(i);

    if (m_sortCombo->currentIndex() == 0) {
        // Сортировка по времени
        std::sort(indices.begin(), indices.end(),
                  [&](int a, int b) {
                      return m_photos[a].timestamp < m_photos[b].timestamp;
                  });
    } else {
        // Сортировка по названию места
        std::sort(indices.begin(), indices.end(),
                  [&](int a, int b) {
                      return QString::localeAwareCompare(
                                 m_photos[a].locationName,
                                 m_photos[b].locationName) < 0;
                  });
    }

    const QString basePath = m_currentRoot.isEmpty()
            ? QCoreApplication::applicationDirPath()
            : m_currentRoot;
    QDir baseDir(basePath);

    auto *rootItem = new QTreeWidgetItem(QStringList(baseDir.dirName().isEmpty()
                                                     ? tr("Фото")
                                                     : baseDir.dirName()));
    rootItem->setData(0, Qt::UserRole, QVariant());
    m_tree->addTopLevelItem(rootItem);

    QMap<QString, QTreeWidgetItem*> cache;
    cache.insert(QString(), rootItem);
    QTreeWidgetItem *firstPhotoItem = nullptr;

    for (int idx : indices) {
        const PhotoInfo &info = m_photos[idx];
        const QString rel = baseDir.relativeFilePath(info.filePath);
        const QStringList parts = rel.split(QDir::separator(), Qt::SkipEmptyParts);
        QTreeWidgetItem *fileItem = ensureTreePath(parts, cache, rootItem, idx, QFileInfo(info.filePath).fileName());

        if (!firstPhotoItem && fileItem) {
            firstPhotoItem = fileItem;
        }
    }

    m_tree->expandItem(rootItem);
    if (firstPhotoItem) {
        m_tree->setCurrentItem(firstPhotoItem);
    } else {
        updatePreview(-1);
    }
}

void MainWindow::resortList()
{
    populateTree();
}

QTreeWidgetItem* MainWindow::ensureTreePath(const QStringList &parts,
                                QMap<QString, QTreeWidgetItem*> &cache,
                                QTreeWidgetItem *root,
                                int photoIndex,
                                const QString &fileName)
{
    QTreeWidgetItem *parent = root;
    QString currentKey;
    QTreeWidgetItem *fileItem = nullptr;

    if (parts.isEmpty())
        return root;

    for (int i = 0; i < parts.size(); ++i) {
        const bool isLast = (i == parts.size() - 1);
        const QString part = parts[i];
        currentKey = currentKey.isEmpty() ? part : currentKey + "/" + part;

        if (!isLast) {
            if (!cache.contains(currentKey)) {
                auto *dirItem = new QTreeWidgetItem(parent, QStringList(part));
                dirItem->setData(0, Qt::UserRole, QVariant());
                cache.insert(currentKey, dirItem);
            }
            parent = cache.value(currentKey);
        } else {
            fileItem = new QTreeWidgetItem(parent, QStringList(part));
            fileItem->setData(0, Qt::UserRole, photoIndex);

            const PhotoInfo &info = m_photos[photoIndex];
            QPixmap icon = loadThumbnail(info.filePath, m_tree->iconSize(), fileName);
            fileItem->setIcon(0, QIcon(icon));
            fileItem->setToolTip(0, tr("%1\n%2\nШирота: %3\nДолгота: %4")
                                    .arg(info.locationName.isEmpty() ? part : info.locationName)
                                    .arg(info.timestamp.toString("yyyy-MM-dd hh:mm"))
                                    .arg(info.latitude, 0, 'f', 4)
                                    .arg(info.longitude, 0, 'f', 4));

            cache.insert(currentKey, fileItem);
        }
    }

    return fileItem;
}

void MainWindow::onTreeSelectionChanged()
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) {
        updatePreview(-1);
        return;
    }

    QVariant data = item->data(0, Qt::UserRole);
    if (!data.isValid()) {
        updatePreview(-1);
        return;
    }

    const int photoIndex = data.toInt();
    centerOnMarker(photoIndex);
    updatePreview(photoIndex);
}

void MainWindow::updatePreview(int photoIndex)
{
    if (!m_previewImage || !m_previewCaption) {
        return;
    }

    if (photoIndex < 0 || photoIndex >= m_photos.size()) {
        m_previewImage->setPixmap(placeholderThumbnail(QSize(520, 320), tr("Нет фото")));
        m_previewCaption->setText(tr("Выберите снимок в списке слева, чтобы увидеть превью."));
        return;
    }

    const PhotoInfo &info = m_photos[photoIndex];
    const QString name = info.locationName.isEmpty()
            ? tr("Без названия")
            : info.locationName;
    const QString coords = tr("Широта: %1, Долгота: %2")
                               .arg(info.latitude, 0, 'f', 4)
                               .arg(info.longitude, 0, 'f', 4);

    QPixmap pix = loadThumbnail(info.filePath, QSize(520, 320), name);
    m_previewImage->setPixmap(pix);
    m_previewCaption->setText(QString("%1\n%2\n%3")
                                  .arg(name)
                                  .arg(info.timestamp.toString("yyyy-MM-dd hh:mm"))
                                  .arg(coords));
}

QPixmap MainWindow::loadThumbnail(const QString &path, const QSize &size,
                                  const QString &fallbackText) const
{
    const QSize target(qMax(64, size.width()), qMax(48, size.height()));

    QPixmap pixmap;
    if (!path.isEmpty() && QFile::exists(path) && pixmap.load(path)) {
        return pixmap.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    const QString text = fallbackText.isEmpty() ? tr("Нет файла") : fallbackText;
    return placeholderThumbnail(target, text);
}

QPixmap MainWindow::placeholderThumbnail(const QSize &size, const QString &text) const
{
    const QSize target(qMax(64, size.width()), qMax(48, size.height()));
    QPixmap placeholder(target);
    placeholder.fill(Qt::transparent);

    QPainter painter(&placeholder);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient grad(0, 0, target.width(), target.height());
    grad.setColorAt(0.0, QColor(30, 36, 48));
    grad.setColorAt(1.0, QColor(18, 22, 30));
    painter.fillRect(placeholder.rect(), grad);

    painter.setPen(QPen(QColor("#3ba9ff"), 2));
    painter.drawRoundedRect(placeholder.rect().adjusted(2, 2, -2, -2), 6, 6);

    painter.setPen(QColor("#c3ccda"));
    painter.setFont(QFont("Segoe UI", 10, QFont::Medium));
    painter.drawText(placeholder.rect().adjusted(6, 6, -6, -6), Qt::AlignCenter, text);

    return placeholder;
}

QString MainWindow::buildMapHtml() const
{
    QJsonArray arr;
    for (int i = 0; i < m_photos.size(); ++i) {
        const PhotoInfo &info = m_photos[i];
        QJsonObject obj;
        obj["id"] = i;
        obj["lat"] = info.latitude;
        obj["lng"] = info.longitude;
        obj["title"] = info.locationName.isEmpty()
                ? QFileInfo(info.filePath).fileName()
                : info.locationName;
        obj["subtitle"] = info.timestamp.toString("yyyy-MM-dd hh:mm");
        if (!info.filePath.isEmpty() && QFile::exists(info.filePath)) {
            obj["image"] = QUrl::fromLocalFile(info.filePath).toString();
        }
        arr.append(obj);
    }

    const QString data = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));

    const QString html = QStringLiteral(R"(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>GeoPhotoMap</title>
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
  <style>
    html, body, #map { margin: 0; padding: 0; width: 100%; height: 100%; background: #0f131b; }
    .leaflet-container { background: #0f131b; color: #e9ecf2; }
    .popup-img { max-width: 220px; border-radius: 8px; margin-top: 6px; }
    .leaflet-popup-content { color: #e9ecf2; }
    .leaflet-popup-content-wrapper { background: #161c26; color: #e9ecf2; border: 1px solid #2a3242; }
  </style>
</head>
<body>
  <div id="map"></div>
  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  <script>
    const map = L.map('map', { worldCopyJump: true }).setView([20, 0], 2);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      maxZoom: 19,
      attribution: '&copy; OpenStreetMap'
    }).addTo(map);

    const markersData = %1;
    const markers = [];

    markersData.forEach(m => {
      const popupHtml = `<b>${m.title || 'Фото'}</b><br>${m.subtitle || ''}` +
        (m.image ? `<br><img class="popup-img" src="${m.image}" />` : '');
      const marker = L.marker([m.lat || 0, m.lng || 0]).addTo(map).bindPopup(popupHtml);
      marker.photoId = m.id;
      markers.push(marker);
    });

    window.centerOn = function(id) {
      const marker = markers.find(m => m.photoId === id);
      if (!marker) return;
      const pos = marker.getLatLng();
      map.flyTo(pos, Math.max(map.getZoom(), 5), { duration: 0.6 });
      marker.openPopup();
    };
  </script>
</body>
</html>
)").arg(data);

    return html;
}

void MainWindow::centerOnMarker(int index)
{
    if (!m_mapView || index < 0)
        return;

    m_mapView->page()->runJavaScript(QStringLiteral("centerOn(%1);").arg(index));
}
