/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "imageviewer.h"
#include "lammpswrapper.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPoint>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidgetAction>

static const QString blank(" ");

ImageViewer::ImageViewer(const QString &fileName, LammpsWrapper *_lammps, QWidget *parent) :
    QDialog(parent), imageLabel(new QLabel), scrollArea(new QScrollArea), menuBar(new QMenuBar),
    lammps(_lammps), group("all"), filename(fileName)
{
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);
    imageLabel->minimumSizeHint();

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setVisible(false);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    auto *zoomin   = new QPushButton("Zoom In");
    auto *zoomout  = new QPushButton("Zoom Out");
    auto *rotleft  = new QPushButton("Rotate Left");
    auto *rotright = new QPushButton("Rotate Right");
    auto *rotup    = new QPushButton("Rotate Up");
    auto *rotdown  = new QPushButton("Rotate Down");
    auto *combo    = new QComboBox;
    combo->setObjectName("group");
    int ngroup = lammps->id_count("group");
    char gname[64];
    for (int i = 0; i < ngroup; ++i) {
        lammps->id_name("group", i, gname, 64);
        combo->addItem(gname);
    }

    QHBoxLayout *menuLayout = new QHBoxLayout;
    menuLayout->addWidget(menuBar);
    menuLayout->addWidget(zoomin);
    menuLayout->addWidget(zoomout);
    menuLayout->addWidget(rotleft);
    menuLayout->addWidget(rotright);
    menuLayout->addWidget(rotup);
    menuLayout->addWidget(rotdown);
    menuLayout->addWidget(new QLabel(" Group: "));
    menuLayout->addWidget(combo);

    connect(zoomin, &QPushButton::released, this, &ImageViewer::do_zoom_in);
    connect(zoomout, &QPushButton::released, this, &ImageViewer::do_zoom_out);
    connect(rotleft, &QPushButton::released, this, &ImageViewer::do_rot_left);
    connect(rotright, &QPushButton::released, this, &ImageViewer::do_rot_right);
    connect(rotup, &QPushButton::released, this, &ImageViewer::do_rot_up);
    connect(rotdown, &QPushButton::released, this, &ImageViewer::do_rot_down);
    connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(change_group(int)));

    mainLayout->addLayout(menuLayout);
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(buttonBox);
    setWindowTitle(QString("Image Viewer: ") + QFileInfo(fileName).completeBaseName());

    QSettings settings;
    settings.beginGroup("snapshot");
    zoom = settings.value("zoom", 1.0).toDouble();
    hrot = settings.value("hrot", 60).toInt();
    vrot = settings.value("vrot", 30).toInt();
    settings.endGroup();

    createActions();
    createImage();

    scaleFactor = 1.0;
    resize(image.width() + 20, image.height() + 50);

    scrollArea->setVisible(true);
    fitToWindowAct->setEnabled(true);
    updateActions();
    if (!fitToWindowAct->isChecked()) imageLabel->adjustSize();
    setLayout(mainLayout);
}

void ImageViewer::do_zoom_in()
{
    zoom = zoom * 1.1;
    if (zoom > 5.0) zoom = 5.0;
    createImage();
}

void ImageViewer::do_zoom_out()
{
    zoom = zoom / 1.1;
    if (zoom < 0.5) zoom = 0.5;
    createImage();
}

void ImageViewer::do_rot_right()
{
    vrot -= 15;
    if (vrot < 0) vrot += 360;
    createImage();
}

void ImageViewer::do_rot_left()
{
    vrot += 15;
    if (vrot > 360) vrot -= 360;
    createImage();
}

void ImageViewer::do_rot_down()
{
    hrot -= 15;
    if (hrot < 0) hrot += 360;
    createImage();
}

void ImageViewer::do_rot_up()
{
    hrot += 15;
    if (hrot > 360) hrot -= 360;
    createImage();
}

void ImageViewer::change_group(int idx)
{
    QComboBox *box = findChild<QComboBox *>("group");
    if (box) group = box->currentText();
    createImage();
}

void ImageViewer::createImage()
{
    QSettings settings;
    QString dumpcmd = QString("write_dump ") + group + " image ";
    QDir dumpdir    = settings.value("tempdir").toString();
    QFile dumpfile(dumpdir.absoluteFilePath(filename + ".ppm"));
    dumpcmd += dumpfile.fileName();

    settings.beginGroup("snapshot");
    int aa    = settings.value("antialias", 0).toInt() + 1;
    int xsize = settings.value("xsize", 800).toInt() * aa;
    int ysize = settings.value("ysize", 600).toInt() * aa;

    dumpcmd += blank + settings.value("color", "type").toString();
    dumpcmd += blank + settings.value("diameter", "type").toString();
    dumpcmd += QString(" size ") + QString::number(xsize) + blank + QString::number(ysize);
    dumpcmd += QString(" zoom ") + QString::number(zoom);
    lammps->command(dumpcmd.toLocal8Bit());
    if (lammps->extract_setting("dimension") == 3) {
        dumpcmd += QString(" view ") + QString::number(hrot) + blank + QString::number(vrot);
    }
    if (settings.value("ssao", false).toBool()) dumpcmd += QString(" ssao yes 453983 0.6");
    settings.endGroup();

    lammps->command(dumpcmd.toLocal8Bit());

    QImageReader reader(dumpfile.fileName());
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();

    if (newImage.isNull()) {
        QMessageBox::warning(
            this, QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2").arg(dumpfile.fileName(), reader.errorString()));
        return;
    }
    dumpfile.remove();

    settings.beginGroup("snapshot");
    xsize = settings.value("xsize", 800).toInt();
    ysize = settings.value("ysize", 600).toInt();
    settings.endGroup();
    // scale back to achieve antialiasing
    image = newImage.scaled(xsize, ysize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(QPixmap::fromImage(image));
}

void ImageViewer::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image File As", QString(),
                                                    "Image Files (*.jpg *.png *.bmp *.ppm)");
    saveFile(fileName);
}

void ImageViewer::copy() {}

void ImageViewer::zoomIn()
{
    scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
    scaleImage(0.8);
}

void ImageViewer::normalSize()
{
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}

void ImageViewer::fitToWindow()
{
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow) normalSize();
    updateActions();
}

void ImageViewer::saveFile(const QString &fileName)
{
    if (!fileName.isEmpty()) image.save(fileName);
}

void ImageViewer::createActions()
{
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));

    saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &ImageViewer::saveAs);
    saveAsAct->setEnabled(false);
    fileMenu->addSeparator();
    copyAct = fileMenu->addAction(tr("&Copy"), this, &ImageViewer::copy);
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);
    fileMenu->addSeparator();
    QAction *exitAct = fileMenu->addAction(tr("&Close"), this, &QWidget::close);
    exitAct->setShortcut(tr("Ctrl+W"));

    QMenu *viewMenu = menuBar->addMenu(tr("&View"));

    zoomInAct = viewMenu->addAction(tr("Image Zoom &In (25%)"), this, &ImageViewer::zoomIn);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);

    zoomOutAct = viewMenu->addAction(tr("Image Zoom &Out (25%)"), this, &ImageViewer::zoomOut);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);

    normalSizeAct = viewMenu->addAction(tr("&Reset Image Size"), this, &ImageViewer::normalSize);
    normalSizeAct->setShortcut(tr("Ctrl+0"));
    normalSizeAct->setEnabled(false);

    viewMenu->addSeparator();

    fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &ImageViewer::fitToWindow);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+="));
}

void ImageViewer::updateActions()
{
    saveAsAct->setEnabled(!image.isNull());
    copyAct->setEnabled(!image.isNull());
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void ImageViewer::scaleImage(double factor)
{
    scaleFactor *= factor;
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());
#else
    imageLabel->resize(scaleFactor * imageLabel->pixmap(Qt::ReturnByValue).size());
#endif

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);
    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(
        int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2)));
}

// Local Variables:
// c-basic-offset: 4
// End:
