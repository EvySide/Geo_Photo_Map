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
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QStatusBar>
#include <QtMath>
#include <QStack>
#include <QWebEngineView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QByteArray>
#include <QRegularExpression>
#include <QtGlobal>
#include <QMessageBox>
#include <sstream>
#include <algorithm>
#include <cmath>

#ifdef HAVE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

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

    m_editGpsButton = new QPushButton(tr("GPS"), central);
    m_editGpsButton->setCursor(Qt::PointingHandCursor);

    auto *sortLabel = new QLabel(tr("Сортировка:"), central);
    m_sortCombo = new QComboBox(central);
    m_sortCombo->addItem(tr("По времени"));
    m_sortCombo->addItem(tr("По месту (название)"));

    controlsLayout->addWidget(m_openButton);
    controlsLayout->addWidget(m_editGpsButton);
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
    connect(m_editGpsButton, &QPushButton::clicked,
            this, &MainWindow::editGpsForSelected);
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
    m_photos.clear(); // без демонстрационных данных
    m_currentRoot = QString();
}

void MainWindow::scanDirectory(const QString &path)
{
    QVector<PhotoInfo> loaded;

    QDirIterator it(path,
                    QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif" << "*.webp",
                    QDir::Files,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString file = it.next();
        QFileInfo fi(file);
        PhotoInfo info;
        info.filePath = file;
        info.timestamp = fi.lastModified();
        info.locationName = fi.absoluteDir().dirName();
        info.latitude = 0.0;
        info.longitude = 0.0;
        double lat = 0.0, lng = 0.0;
        info.hasGps = extractGpsFromExif(file, lat, lng);
        if (info.hasGps) {
            info.latitude = lat;
            info.longitude = lng;
        }

        loaded.append(info);
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
        tr("Загружено %1 фото, GPS найдено: %2 (%3)")
            .arg(m_photos.size())
            .arg(std::count_if(m_photos.begin(), m_photos.end(), [](const PhotoInfo &p){ return p.hasGps; }))
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

void MainWindow::editGpsForSelected()
{
    if (!m_tree || m_photos.isEmpty())
        return;

    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) {
        statusBar()->showMessage(tr("Выберите фотографию в списке слева"), 3000);
        return;
    }

    QVariant data = item->data(0, Qt::UserRole);
    if (!data.isValid()) {
        statusBar()->showMessage(tr("Выберите файл, а не папку"), 3000);
        return;
    }

    const int idx = data.toInt();
    if (idx < 0 || idx >= m_photos.size())
        return;

    PhotoInfo info = m_photos[idx];
    const QFileInfo fi(info.filePath);
    const QString ext = fi.suffix().toLower();
    if (ext != "jpg" && ext != "jpeg" && ext != "tif" && ext != "tiff") {
        QMessageBox::warning(this, tr("Формат не поддерживается"),
                             tr("Запись GPS в EXIF поддерживается только для JPEG/TIFF."));
        return;
    }
    if (!fi.isWritable()) {
        QMessageBox::warning(this, tr("Файл только для чтения"),
                             tr("Нет прав на запись. Снимите атрибут \"только чтение\"."));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("GPS координаты"));
    auto *form = new QFormLayout(&dlg);

    auto *pathLabel = new QLabel(QDir::toNativeSeparators(info.filePath), &dlg);
    pathLabel->setWordWrap(true);

    auto *latSpin = new QDoubleSpinBox(&dlg);
    latSpin->setRange(-90.0, 90.0);
    latSpin->setDecimals(6);
    latSpin->setValue(info.latitude);

    auto *lngSpin = new QDoubleSpinBox(&dlg);
    lngSpin->setRange(-180.0, 180.0);
    lngSpin->setDecimals(6);
    lngSpin->setValue(info.longitude);

    auto *nameEdit = new QLineEdit(info.locationName, &dlg);

    form->addRow(tr("Файл:"), pathLabel);
    form->addRow(tr("Широта:"), latSpin);
    form->addRow(tr("Долгота:"), lngSpin);
    form->addRow(tr("Место:"), nameEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    const double newLat = latSpin->value();
    const double newLng = lngSpin->value();

    QString error;
    if (!writeGpsToExif(info.filePath, newLat, newLng, &error)) {
        QMessageBox::warning(this, tr("Не удалось записать EXIF"),
                             error.isEmpty() ? tr("Запись GPS в EXIF недоступна.") : error);
        return;
    }

    double verifyLat = 0.0, verifyLng = 0.0;
    if (!extractGpsFromExif(info.filePath, verifyLat, verifyLng)) {
        QMessageBox::warning(this, tr("EXIF не подтверждён"),
                             tr("Записать координаты не удалось. Проверьте файл и попробуйте снова."));
        return;
    }

    info.latitude = verifyLat;
    info.longitude = verifyLng;
    info.locationName = nameEdit->text().trimmed();
    info.hasGps = true;
    m_photos[idx] = info;

    createMap();
    populateTree();

    // найти и выделить текущий элемент заново
    if (m_tree) {
        QStack<QTreeWidgetItem*> stack;
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
            stack.push(m_tree->topLevelItem(i));
        while (!stack.isEmpty()) {
            QTreeWidgetItem *it = stack.pop();
            if (it->data(0, Qt::UserRole).toInt() == idx) {
                m_tree->setCurrentItem(it);
                break;
            }
            for (int i = 0; i < it->childCount(); ++i)
                stack.push(it->child(i));
        }
    }

    centerOnMarker(idx);
    statusBar()->showMessage(tr("GPS записан в EXIF"), 2000);
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
            const QString tooltip = info.hasGps
                    ? tr("%1\n%2\nШирота: %3\nДолгота: %4")
                          .arg(info.locationName.isEmpty() ? part : info.locationName)
                          .arg(info.timestamp.toString("yyyy-MM-dd hh:mm"))
                          .arg(info.latitude, 0, 'f', 4)
                          .arg(info.longitude, 0, 'f', 4)
                    : tr("%1\n%2\nGPS: не найден")
                          .arg(info.locationName.isEmpty() ? part : info.locationName)
                          .arg(info.timestamp.toString("yyyy-MM-dd hh:mm"));
            fileItem->setToolTip(0, tooltip);

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
    const QString coords = info.hasGps
            ? tr("Широта: %1, Долгота: %2")
                  .arg(info.latitude, 0, 'f', 4)
                  .arg(info.longitude, 0, 'f', 4)
            : tr("GPS не найден");

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
        if (!info.hasGps)
            continue; // пропускаем только реально без координат

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
      if (typeof m.lat !== 'number' || typeof m.lng !== 'number') return;
      const popupHtml = `<b>${m.title || 'Фото'}</b><br>${m.subtitle || ''}` +
        (m.image ? `<br><img class="popup-img" src="${m.image}" />` : '');
      const marker = L.marker([m.lat, m.lng]).addTo(map).bindPopup(popupHtml);
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

bool MainWindow::parseExifCoord(const QString &value, const QString &ref, double &result)
{
    // Поддержка форматов: "55/1 45/1 1234/100", "55 45 12.34", "55°45'12.34\"", "55.7522"
    QRegularExpression re(QStringLiteral("([+-]?\\d+(?:[\\.,]\\d+)?(?:/\\d+(?:[\\.,]\\d+)?)?)"));
    QRegularExpressionMatchIterator it = re.globalMatch(value);
    QStringList tokens;
    while (it.hasNext()) {
        const auto m = it.next();
        tokens << m.captured(1);
    }

    double raw = 0.0;
    if (tokens.size() == 1) {
        // Уже в виде десятичной координаты
        raw = tokens.first().replace(',', '.').toDouble();
    } else if (tokens.size() >= 2) {
        auto toDouble = [](const QString &token) -> double {
            QString cleaned = token;
            cleaned.replace(',', '.');
            const QStringList frac = cleaned.split('/');
            if (frac.size() == 2 && frac[1].toDouble() != 0.0)
                return frac[0].toDouble() / frac[1].toDouble();
            return cleaned.toDouble();
        };

        double deg = toDouble(tokens.value(0));
        const double sign = (deg < 0.0) ? -1.0 : 1.0;
        deg = std::abs(deg);
        const double min = std::abs(toDouble(tokens.value(1)));
        const double sec = std::abs(toDouble(tokens.value(2, QStringLiteral("0"))));
        raw = sign * (deg + (min / 60.0) + (sec / 3600.0));
    } else {
        return false;
    }

    const QString refUp = ref.trimmed().toUpper();
    const int refSign = (refUp == "S" || refUp == "W") ? -1 : 1;
    const double magnitude = std::abs(raw);
    result = (refSign == -1) ? -magnitude : (raw < 0.0 ? -magnitude : magnitude);

    const double maxAbs = (refUp == "E" || refUp == "W") ? 180.0 : 90.0;
    if (std::abs(result) > maxAbs)
        return false;
    return true;
}

bool MainWindow::extractGpsFromExif(const QString &path, double &lat, double &lng) const
{
#ifdef HAVE_EXIV2
    try {
        auto image = Exiv2::ImageFactory::open(path.toStdString());
        if (!image.get())
            return false;

        image->readMetadata();
        Exiv2::ExifData &exif = image->exifData();
        if (exif.empty())
            return false;

        auto itLat = exif.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitude"));
        auto itLatRef = exif.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitudeRef"));
        auto itLng = exif.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitude"));
        auto itLngRef = exif.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitudeRef"));
        if (itLat == exif.end() || itLng == exif.end())
            return false;

        const QString latRef = (itLatRef != exif.end())
                ? QString::fromStdString(itLatRef->value().toString())
                : QStringLiteral("N");
        const QString lngRef = (itLngRef != exif.end())
                ? QString::fromStdString(itLngRef->value().toString())
                : QStringLiteral("E");

        const int latCount = itLat->value().count();
        const int lngCount = itLng->value().count();
        auto rationalToDouble = [](const Exiv2::Rational &r) -> double {
            return r.second != 0 ? double(r.first) / double(r.second) : 0.0;
        };

        double outLat = 0.0, outLng = 0.0;
        if (latCount >= 3 && lngCount >= 3) {
            const double dLat = rationalToDouble(itLat->value().toRational(0));
            const double mLat = rationalToDouble(itLat->value().toRational(1));
            const double sLat = rationalToDouble(itLat->value().toRational(2));
            const double dLng = rationalToDouble(itLng->value().toRational(0));
            const double mLng = rationalToDouble(itLng->value().toRational(1));
            const double sLng = rationalToDouble(itLng->value().toRational(2));
            outLat = dLat + (mLat / 60.0) + (sLat / 3600.0);
            outLng = dLng + (mLng / 60.0) + (sLng / 3600.0);
            if (latRef.trimmed().toUpper() == "S")
                outLat = -outLat;
            if (lngRef.trimmed().toUpper() == "W")
                outLng = -outLng;
        } else {
            if (!parseExifCoord(QString::fromStdString(itLat->value().toString()), latRef, outLat))
                return false;
            if (!parseExifCoord(QString::fromStdString(itLng->value().toString()), lngRef, outLng))
                return false;
        }

        lat = outLat;
        lng = outLng;
        return true;
    } catch (...) {
        // fallthrough to QImageReader below
    }
#endif

    // Через QImageReader::text (если Qt собран с поддержкой Exif)
    QImageReader reader(path);
    const QStringList keys = reader.textKeys();
    if (keys.isEmpty())
        return false;

    auto findKey = [&](const QStringList &names) -> QString {
        for (const QString &k : keys) {
            for (const QString &n : names) {
                if (k.compare(n, Qt::CaseInsensitive) == 0)
                    return reader.text(k);
            }
        }
        return QString();
    };

    QString latRef = findKey({QStringLiteral("Exif.GPSInfo.GPSLatitudeRef"),
                              QStringLiteral("GPSLatitudeRef"),
                              QStringLiteral("Exif.GPSLatitudeRef")});
    QString latVal = findKey({QStringLiteral("Exif.GPSInfo.GPSLatitude"),
                              QStringLiteral("GPSLatitude"),
                              QStringLiteral("Exif.GPSLatitude")});
    QString lngRef = findKey({QStringLiteral("Exif.GPSInfo.GPSLongitudeRef"),
                              QStringLiteral("GPSLongitudeRef"),
                              QStringLiteral("Exif.GPSLongitudeRef")});
    QString lngVal = findKey({QStringLiteral("Exif.GPSInfo.GPSLongitude"),
                              QStringLiteral("GPSLongitude"),
                              QStringLiteral("Exif.GPSLongitude")});

    if (!latVal.isEmpty() && !lngVal.isEmpty()) {
        if (latRef.isEmpty()) latRef = QStringLiteral("N");
        if (lngRef.isEmpty()) lngRef = QStringLiteral("E");

        double outLat = 0.0, outLng = 0.0;
        if (parseExifCoord(latVal, latRef, outLat) && parseExifCoord(lngVal, lngRef, outLng)) {
            lat = outLat;
            lng = outLng;
            return true;
        }
    }

    return false;
}

bool MainWindow::writeGpsToExif(const QString &path, double lat, double lng, QString *error)
{
#ifdef HAVE_EXIV2
    try {
        auto image = Exiv2::ImageFactory::open(path.toStdString());
        if (!image.get()) {
            if (error) *error = tr("Не удалось открыть файл.");
            return false;
        }

        image->readMetadata();
        Exiv2::ExifData &exif = image->exifData();

        auto makeRationalString = [](double value) -> std::string {
            double absVal = std::abs(value);
            int deg = int(std::floor(absVal));
            double minFloat = (absVal - deg) * 60.0;
            int min = int(std::floor(minFloat));
            double secFloat = (minFloat - min) * 60.0;
            int secDen = 10000;
            int secNum = int(std::round(secFloat * secDen));
            if (secNum >= secDen * 60) {
                secNum = 0;
                min += 1;
            }
            if (min >= 60) {
                min = 0;
                deg += 1;
            }

            std::ostringstream oss;
            oss << deg << "/1 " << min << "/1 " << secNum << "/" << secDen;
            return oss.str();
        };

        exif["Exif.GPSInfo.GPSVersionID"] = "2 3 0 0";
        exif["Exif.GPSInfo.GPSMapDatum"] = "WGS-84";
        exif["Exif.GPSInfo.GPSLatitudeRef"] = (lat < 0.0 ? "S" : "N");
        exif["Exif.GPSInfo.GPSLongitudeRef"] = (lng < 0.0 ? "W" : "E");
        auto latValue = Exiv2::Value::create(Exiv2::unsignedRational);
        latValue->read(makeRationalString(lat));
        exif["Exif.GPSInfo.GPSLatitude"].setValue(latValue.get());

        auto lngValue = Exiv2::Value::create(Exiv2::unsignedRational);
        lngValue->read(makeRationalString(lng));
        exif["Exif.GPSInfo.GPSLongitude"].setValue(lngValue.get());

        image->setExifData(exif);
        image->writeMetadata();
        return true;
    } catch (const std::exception &e) {
        if (error) *error = QString::fromUtf8(e.what());
        return false;
    }
#else
    Q_UNUSED(path);
    Q_UNUSED(lat);
    Q_UNUSED(lng);
    if (error) *error = tr("Exiv2 не подключён. Установите библиотеку Exiv2.");
    return false;
#endif
}
