/* ============================================================
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 *         Gilles Caulier <caulier dot gilles at free.fr>
 * Date  : 2004-02-12
 * Description :
 *
 * Copyright 2004 by Renchi Raju, Gilles Caulier
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

// Qt includes.

#include <qpopupmenu.h>
#include <qstatusbar.h>
#include <qcursor.h>
#include <qtimer.h>
#include <qlabel.h>
#include <qimage.h>

// KDE includes.

#include <klocale.h>
#include <kaction.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kpropertiesdialog.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kprinter.h>
#include <ktempfile.h>
#include <kimageio.h>
#include <kfiledialog.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kaccel.h>

// LibKexif includes.

#include <libkexif/kexif.h>
#include <libkexif/kexifdata.h>
#include <libkexif/kexifutils.h>

// Local includes.

#include "exifrestorer.h"
#include "guifactory.h"
#include "canvas.h"
#include "imageguiclient.h"
#include "imageplugin.h"
#include "imagepluginloader.h"
#include "imageresizedlg.h"
#include "imagerotatedlg.h"
#include "imageprint.h"
#include "albummanager.h"
#include "album.h"
#include "albumdb.h"
#include "albumsettings.h"
#include "syncjob.h"
#include "imagewindow.h"
#include "albumiconview.h"
#include "albumiconitem.h"
#include "imageproperties.h"
#include "imagedescedit.h"


ImageWindow* ImageWindow::instance()
{
    if (!m_instance)
        new ImageWindow();

    return m_instance;
}

ImageWindow* ImageWindow::m_instance = 0;

ImageWindow::ImageWindow()
           : QMainWindow(0,0,WType_TopLevel|WDestructiveClose)
{
    m_instance              = this;
    m_rotatedOrFlipped      = false;
    m_allowSaving           = true;
    m_fullScreen            = false;
    m_fullScreenHideToolBar = false;
    m_view                  = 0L;
    
    // -- build the gui -------------------------------------

    m_guiFactory = new Digikam::GUIFactory();
    m_guiClient  = new ImageGUIClient(this);
    m_guiFactory->insertClient(m_guiClient);

    ImagePluginLoader* loader = ImagePluginLoader::instance();
    for (Digikam::ImagePlugin* plugin = loader->pluginList().first();
         plugin; plugin = loader->pluginList().next()) {
        if (plugin) {
            m_guiFactory->insertClient(plugin);
            plugin->setParentWidget(this);
            plugin->setEnabledSelectionActions(false);
        }
    }

    m_contextMenu = new QPopupMenu(this);
    m_guiFactory->buildGUI(this);
    m_guiFactory->buildGUI(m_contextMenu);

    // -- construct the view ---------------------------------

    m_canvas    = new Canvas(this);
    setCentralWidget(m_canvas);

    statusBar()->setSizeGripEnabled(false);
    m_nameLabel = new QLabel(statusBar());
    m_nameLabel->setAlignment(Qt::AlignCenter);
    statusBar()->addWidget(m_nameLabel,1);
    m_zoomLabel = new QLabel(statusBar());
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    statusBar()->addWidget(m_zoomLabel,1);
    m_resLabel  = new QLabel(statusBar());
    m_resLabel->setAlignment(Qt::AlignCenter);
    statusBar()->addWidget(m_resLabel,1);

    // -- Some Accels not available from actions -------------

    m_accel = new KAccel(this);
    m_accel->insert("Exit fullscreen", i18n("Exit Fullscreen"),
                    i18n("Exit out of the fullscreen mode"),
                    Key_Escape, this, SLOT(slotEscapePressed()),
                    false, true);

    // -- setup connections ---------------------------

    connect(m_guiClient, SIGNAL(signalNext()),
            SLOT(slotLoadNext()));
    connect(m_guiClient, SIGNAL(signalPrev()),
            SLOT(slotLoadPrev()));
    connect(m_guiClient, SIGNAL(signalFirst()),
            SLOT(slotLoadFirst()));
    connect(m_guiClient, SIGNAL(signalLast()),
            SLOT(slotLoadLast()));
    connect(m_guiClient, SIGNAL(signalExit()),
            SLOT(close()));

    connect(m_guiClient, SIGNAL(signalSave()),
            SLOT(slotSave()));
    connect(m_guiClient, SIGNAL(signalSaveAs()),
            SLOT(slotSaveAs()));

    connect(m_guiClient, SIGNAL(signalRestore()),
            m_canvas, SLOT(slotRestore()));

    connect(m_guiClient, SIGNAL(signalFilePrint()),
            SLOT(slotFilePrint()));
    connect(m_guiClient, SIGNAL(signalFileProperties()),
            SLOT(slotFileProperties()));
    connect(m_guiClient, SIGNAL(signalDeleteCurrentItem()),
            SLOT(slotDeleteCurrentItem()));
    connect(m_guiClient, SIGNAL(signalCommentsEdit()),
            SLOT(slotCommentsEdit()));


    connect(m_guiClient, SIGNAL(signalZoomPlus()),
            m_canvas, SLOT(slotIncreaseZoom()));
    connect(m_guiClient, SIGNAL(signalZoomMinus()),
            m_canvas, SLOT(slotDecreaseZoom()));
    connect(m_guiClient, SIGNAL(signalZoomFit()),
            SLOT(slotToggleAutoZoom()));
    connect(m_guiClient, SIGNAL(signalToggleFullScreen()),
            SLOT(slotToggleFullScreen()));

    connect(m_guiClient, SIGNAL(signalRotate90()),
            m_canvas, SLOT(slotRotate90()));
    connect(m_guiClient, SIGNAL(signalRotate180()),
            m_canvas, SLOT(slotRotate180()));
    connect(m_guiClient, SIGNAL(signalRotate270()),
            m_canvas, SLOT(slotRotate270()));

    connect(m_guiClient, SIGNAL(signalFlipHoriz()),
            m_canvas, SLOT(slotFlipHoriz()));
    connect(m_guiClient, SIGNAL(signalFlipVert()),
            m_canvas, SLOT(slotFlipVert()));

    // if rotating/flipping set the rotatedflipped flag to true
    connect(m_guiClient, SIGNAL(signalRotate90()),
            SLOT(slotRotatedOrFlipped()));
    connect(m_guiClient, SIGNAL(signalRotate180()),
            SLOT(slotRotatedOrFlipped()));
    connect(m_guiClient, SIGNAL(signalRotate270()),
            SLOT(slotRotatedOrFlipped()));

    connect(m_guiClient, SIGNAL(signalFlipHoriz()),
            SLOT(slotRotatedOrFlipped()));
    connect(m_guiClient, SIGNAL(signalFlipVert()),
            SLOT(slotRotatedOrFlipped()));

    connect(m_guiClient, SIGNAL(signalCrop()),
            m_canvas, SLOT(slotCrop()));
    connect(m_guiClient, SIGNAL(signalResize()),
            SLOT(slotResize()));
    connect(m_guiClient, SIGNAL(signalRotate()),
            SLOT(slotRotate()));

    connect(m_canvas, SIGNAL(signalRightButtonClicked()),
            SLOT(slotContextMenu()));
    connect(m_canvas, SIGNAL(signalZoomChanged(float)),
            SLOT(slotZoomChanged(float)));
    connect(m_canvas, SIGNAL(signalSelected(bool)),
            SLOT(slotSelected(bool)));
    connect(m_canvas, SIGNAL(signalChanged(bool)),
            SLOT(slotChanged(bool)));
    connect(m_canvas, SIGNAL(signalShowNextImage()),
            SLOT(slotLoadNext()));
    connect(m_canvas, SIGNAL(signalShowPrevImage()),
            SLOT(slotLoadPrev()));

    connect(m_guiClient, SIGNAL(signalShowImagePluginsHelp()),
            SLOT(slotImagePluginsHelp()));

    // -- read settings --------------------------------
    readSettings();
    applySettings();
}


ImageWindow::~ImageWindow()
{
    m_instance = 0;

    saveSettings();
    delete m_guiClient;
    delete m_guiFactory;

    ImagePluginLoader* loader = ImagePluginLoader::instance();
    for (Digikam::ImagePlugin* plugin = loader->pluginList().first();
         plugin; plugin = loader->pluginList().next()) {
        if (plugin) {
            plugin->setParentWidget(0);
            plugin->setEnabledSelectionActions(false);
        }
    }
}

void ImageWindow::loadURL(const KURL::List& urlList,
                          const KURL& urlCurrent,
                          const QString& caption,
                          bool  allowSaving,
                          AlbumIconView* view)
{
    setCaption(i18n("Digikam Image Editor - Album \"%1\"").arg(caption));

    m_view        = view;
    m_urlList     = urlList;
    m_urlCurrent  = urlCurrent;
    m_allowSaving = allowSaving;
    
    m_guiClient->m_saveAction->setEnabled(false);
    m_guiClient->m_restoreAction->setEnabled(false);

    QTimer::singleShot(0, this, SLOT(slotLoadCurrent()));
}

void ImageWindow::applySettings()
{
    KConfig* config = kapp->config();
    config->setGroup("ImageViewer Settings");

    // Background color.
    QColor bgColor(Qt::black);
    m_canvas->setBackgroundColor(config->readColorEntry("BackgroundColor",
                                                        &bgColor));
    m_canvas->update();

    // JPEG compression value.
    m_JPEGCompression = config->readNumEntry("JPEGCompression", 75);

    // PNG compression value.
    m_PNGCompression = config->readNumEntry("PNGCompression", 1);

    // TIFF compression.
    m_TIFFCompression = config->readBoolEntry("TIFFCompression", false);

    AlbumSettings *settings = AlbumSettings::instance();
    if (settings->getUseTrash())
    {
        m_guiClient->m_fileDelete->setIcon("edittrash");
        m_guiClient->m_fileDelete->setText(i18n("Move to Trash"));
    }
    else
    {
        m_guiClient->m_fileDelete->setIcon("editdelete");
        m_guiClient->m_fileDelete->setText(i18n("Delete File"));
    }

    m_canvas->setExifOrient(settings->getExifRotate());
}

void ImageWindow::readSettings()
{
    KConfig* config = kapp->config();

    int width, height;
    bool autoZoom;

    config->setGroup("ImageViewer Settings");

    // GUI options.
    width = config->readNumEntry("Width", 500);
    height = config->readNumEntry("Height", 500);
    autoZoom = config->readBoolEntry("AutoZoom", true);
    m_fullScreen = config->readBoolEntry("FullScreen", false);
    m_fullScreenHideToolBar = config->readBoolEntry("FullScreen Hide ToolBar",
                                                    false);
    resize(width, height);

    if (autoZoom) {
        m_guiClient->m_zoomFitAction->activate();
        m_guiClient->m_zoomPlusAction->setEnabled(false);
        m_guiClient->m_zoomMinusAction->setEnabled(false);
    }

    if (m_fullScreen)
    {
        m_fullScreen = false;
        m_guiClient->m_fullScreenAction->activate();
    }
}

void ImageWindow::saveSettings()
{
    KConfig* config = kapp->config();

    config->setGroup("ImageViewer Settings");
    config->writeEntry("Width", width());
    config->writeEntry("Height", height());
    config->writeEntry("AutoZoom", m_guiClient->m_zoomFitAction->isChecked());
    config->writeEntry("FullScreen", m_fullScreen);
    config->sync();
}

void ImageWindow::slotLoadCurrent()
{
    KURL::List::iterator it = m_urlList.find(m_urlCurrent);
    uint index = m_urlList.findIndex(m_urlCurrent);

    if (it != m_urlList.end()) {

        QApplication::setOverrideCursor(Qt::WaitCursor);

        m_canvas->load(m_urlCurrent.path());
        m_rotatedOrFlipped = false;

        QString text = m_urlCurrent.filename() +
                       i18n(" (%2 of %3)")
                       .arg(QString::number(index+1))
                       .arg(QString::number(m_urlList.count()));
        m_nameLabel->setText(text);

        ++it;
        if (it != m_urlList.end())
            m_canvas->preload((*it).path());


        QApplication::restoreOverrideCursor();
    }

    if (m_urlList.count() == 1) {
        m_guiClient->m_navPrevAction->setEnabled(false);
        m_guiClient->m_navNextAction->setEnabled(false);
        m_guiClient->m_navFirstAction->setEnabled(false);
        m_guiClient->m_navLastAction->setEnabled(false);
    }
    else {
        m_guiClient->m_navPrevAction->setEnabled(true);
        m_guiClient->m_navNextAction->setEnabled(true);
        m_guiClient->m_navFirstAction->setEnabled(true);
        m_guiClient->m_navLastAction->setEnabled(true);
    }

    if (index == 0) {
        m_guiClient->m_navPrevAction->setEnabled(false);
        m_guiClient->m_navFirstAction->setEnabled(false);
    }

    if (index == m_urlList.count()-1) {
        m_guiClient->m_navNextAction->setEnabled(false);
        m_guiClient->m_navLastAction->setEnabled(false);
    }

    // Disable some menu actions if the current root image URL
    // isn't include in the Digikam Albums library database.
    // This is necessary when ImageEditor is opened from cameraclient.

    KURL u(m_urlCurrent.directory());
    PAlbum *palbum = AlbumManager::instance()->findPAlbum(u);

    if (!palbum)
    {
       m_guiClient->m_fileDelete->setEnabled(false);
       m_guiClient->m_commentedit->setEnabled(false);
    }
    else
    {
       m_guiClient->m_fileDelete->setEnabled(true);
       m_guiClient->m_commentedit->setEnabled(true);
    }
}

void ImageWindow::slotLoadNext()
{
    promptUserSave();

    KURL::List::iterator it = m_urlList.find(m_urlCurrent);

    if (it != m_urlList.end()) {

        if (m_urlCurrent != m_urlList.last()) {
           KURL urlNext = *(++it);
           m_urlCurrent = urlNext;
           slotLoadCurrent();
        }
    }
}

void ImageWindow::slotLoadPrev()
{
    promptUserSave();

    KURL::List::iterator it = m_urlList.find(m_urlCurrent);

    if (it != m_urlList.begin()) {

        if (m_urlCurrent != m_urlList.first())
        {
            KURL urlPrev = *(--it);
            m_urlCurrent = urlPrev;
            slotLoadCurrent();
        }
    }
}

void ImageWindow::slotLoadFirst()
{
    promptUserSave();
    m_urlCurrent = m_urlList.first();
    slotLoadCurrent();
}

void ImageWindow::slotLoadLast()
{
    promptUserSave();
    m_urlCurrent = m_urlList.last();
    slotLoadCurrent();
}

void ImageWindow::slotToggleAutoZoom()
{
    bool checked = m_guiClient->m_zoomFitAction->isChecked();

    m_guiClient->m_zoomPlusAction->setEnabled(!checked);
    m_guiClient->m_zoomMinusAction->setEnabled(!checked);

    m_canvas->slotToggleAutoZoom();
}

void ImageWindow::slotResize()
{
    int width  = m_canvas->imageWidth();
    int height = m_canvas->imageHeight();

    ImageResizeDlg dlg(this, &width, &height);
    if (dlg.exec() == QDialog::Accepted &&
        (width != m_canvas->imageWidth() ||
        height != m_canvas->imageHeight()))
        m_canvas->resizeImage(width, height);
}

void ImageWindow::slotRotate()
{
    double angle = 0;

    ImageRotateDlg dlg(this, &angle);

    if (dlg.exec() == QDialog::Accepted )
        m_canvas->rotateImage(angle);
}

void ImageWindow::slotContextMenu()
{
    m_contextMenu->exec(QCursor::pos());
}

void ImageWindow::slotZoomChanged(float zoom)
{
    m_zoomLabel->setText(i18n("Zoom: ") +
                         QString::number(zoom*100, 'f', 2) +
                         QString("%"));

    m_guiClient->m_zoomPlusAction->
        setEnabled(!m_canvas->maxZoom() &&
                   !m_guiClient->m_zoomFitAction->isChecked());
    m_guiClient->m_zoomMinusAction->
        setEnabled(!m_canvas->minZoom() &&
                   !m_guiClient->m_zoomFitAction->isChecked());
}

void ImageWindow::slotChanged(bool val)
{
    m_resLabel->setText(QString::number(m_canvas->imageWidth())  +
                        QString("x") +
                        QString::number(m_canvas->imageHeight()) +
                        QString(" ") +
                        i18n("pixels"));

    m_guiClient->m_restoreAction->setEnabled(val);
    if (m_allowSaving)
    {
        m_guiClient->m_saveAction->setEnabled(val);
    }

    if (!val)
        m_rotatedOrFlipped = false;
}

void ImageWindow::slotSelected(bool val)
{
    m_guiClient->m_cropAction->setEnabled(val);

    ImagePluginLoader* loader = ImagePluginLoader::instance();
    for (Digikam::ImagePlugin* plugin = loader->pluginList().first();
         plugin; plugin = loader->pluginList().next()) {
        if (plugin) {
            m_guiFactory->insertClient(plugin);
            plugin->setEnabledSelectionActions(val);
        }
    }
}

void ImageWindow::slotRotatedOrFlipped()
{
    m_rotatedOrFlipped = true;
}

void ImageWindow::slotFileProperties()
{
    if (!m_view)
        (void) new KPropertiesDialog( m_urlCurrent, this, "props dialog", true );
    else
    {
        AlbumIconItem *iconItem = m_view->findItem(m_urlCurrent.url());
        
        if (iconItem)
        {
            ImageProperties properties(m_view, iconItem, this);
            properties.exec();
        }
    }
}

void ImageWindow::slotCommentsEdit()
{
    if (m_view)
    {
        AlbumIconItem *iconItem = m_view->findItem(m_urlCurrent.url());

        if (iconItem)
        {
            ImageDescEdit descEdit(m_view, iconItem, this);
            descEdit.exec();
        }
    }
}

void ImageWindow::slotDeleteCurrentItem()
{
    KURL u(m_urlCurrent.directory());
    PAlbum *palbum = AlbumManager::instance()->findPAlbum(u);

    if (!palbum)
        return;


    AlbumSettings* settings = AlbumSettings::instance();

    if (!settings->getUseTrash())
    {
        QString warnMsg(i18n("About to Delete File \"%1\"\nAre you sure?")
                        .arg(m_urlCurrent.filename()));
        if (KMessageBox::warningContinueCancel(this,
                                               warnMsg,
                                               i18n("Warning"),
                                               i18n("Delete"))
            !=  KMessageBox::Continue)
        {
            return;
        }
    }

    if (!SyncJob::userDelete(m_urlCurrent))
    {
        QString errMsg(SyncJob::lastErrorMsg());
        KMessageBox::error(this, errMsg, errMsg);
        return;
    }

    emit signalFileDeleted(m_urlCurrent);

    KURL CurrentToRemove = m_urlCurrent;
    KURL::List::iterator it = m_urlList.find(m_urlCurrent);

    if (it != m_urlList.end())
    {
        if (m_urlCurrent != m_urlList.last())
        {
            // Try to get the next image in the current Album...

            KURL urlNext = *(++it);
            m_urlCurrent = urlNext;
            m_urlList.remove(CurrentToRemove);
            slotLoadCurrent();
            return;
        }
        else if (m_urlCurrent != m_urlList.first())
        {
            // Try to get the previous image in the current Album...

            KURL urlPrev = *(--it);
            m_urlCurrent = urlPrev;
            m_urlList.remove(CurrentToRemove);
            slotLoadCurrent();
            return;
        }
    }

    // No image in the current Album -> Quit ImageEditor...

    KMessageBox::information(this,
                             i18n("There is no image to show in the current album.\n"
                                  "The image editor will be closed."),
                             i18n("No Image in Current Album"));

    close();
}

void ImageWindow::slotFilePrint()
{
    KPrinter printer;
    printer.setDocName( m_urlCurrent.filename() );
    printer.setCreator( "Digikam-ImageEditor");
#if KDE_IS_VERSION(3,2,0)
    printer.setUsePrinterResolution(true);
#endif

    KPrinter::addDialogPage( new ImageEditorPrintDialogPage( this, "ImageEditor page"));

    if ( printer.setup( this, i18n("Print %1").arg(printer.docName().section('/', -1)) ) )
    {
        bool ok = false;
        KTempFile tmpFile( "digikam_imageeditor", ".png" );

        if ( tmpFile.status() == 0 )
        {
            ok = true;
            tmpFile.setAutoDelete( true );

            if ( m_canvas->saveAsTmpFile(tmpFile.name(), m_JPEGCompression, m_PNGCompression,
                                         m_TIFFCompression, "png") )
            {
                ImagePrint *printOperations = new ImagePrint(tmpFile.name(),
                                                             printer,
                                                             m_urlCurrent.filename());

                if ( printOperations->printImageWithQt() == false )
                    ok = false;

                delete printOperations;
            }
            else
                ok = false;
        }

        if ( ok == false )
            KMessageBox::error(this, i18n("Failed to print file\n\"%1\"")
                               .arg(m_urlCurrent.filename()));
    }
}

void ImageWindow::slotSave()
{
    QString tmpFile = locateLocal("tmp", m_urlCurrent.filename());

    bool result = m_canvas->saveAsTmpFile(tmpFile, m_JPEGCompression, 
                                          m_PNGCompression, m_TIFFCompression);

    if (result == false)
    {
        KMessageBox::error(this, i18n("Failed to save file\n\"%1\" to album\n\"%2\".")
                           .arg(m_urlCurrent.filename())
                           .arg(m_urlCurrent.path().section('/', -2, -2)));
        return;
    }

    ExifRestorer exifHolder;
    exifHolder.readFile(m_urlCurrent.path(), ExifRestorer::ExifOnly);

    if (exifHolder.hasExif())
    {
        ExifRestorer restorer;
        restorer.readFile(tmpFile, ExifRestorer::EntireImage);
        restorer.insertExifData(exifHolder.exifData());
        restorer.writeFile(tmpFile);
    }
    else
        kdWarning() << ("slotSave::No Exif Data Found") << endl;

    if( m_rotatedOrFlipped || m_canvas->exifRotated() )
       KExifUtils::writeOrientation(tmpFile, KExifData::NORMAL);

    if(!SyncJob::file_move(KURL(tmpFile), m_urlCurrent))
    {
        QString errMsg(SyncJob::lastErrorMsg());
        KMessageBox::error(this, errMsg, errMsg);
    }
    else
    {
        emit signalFileModified(m_urlCurrent);
        QTimer::singleShot(0, this, SLOT(slotLoadCurrent()));
    }
}

void ImageWindow::slotSaveAs()
 {
    // Get the new filename.

    // The typemines listed is the base imagefiles supported by imlib2.

    QStringList mimetypes;
    mimetypes << "image/jpeg" << "image/png" << "image/tiff" << "image/gif"
              << "image/x-tga" << "image/x-bmp" <<  "image/x-xpm"
              << "image/x-portable-anymap";

    KFileDialog *imageFileSaveDialog = new KFileDialog(m_urlCurrent.directory(),
                                                       QString::null,
                                                       this,
                                                       "imageFileSaveDialog",
                                                       false);

    imageFileSaveDialog->setOperationMode( KFileDialog::Saving );
    imageFileSaveDialog->setMode( KFile::File );
    imageFileSaveDialog->setCaption( i18n("New Image File Name") );
    imageFileSaveDialog->setMimeFilter(mimetypes);

    // Check for cancel.
    if ( imageFileSaveDialog->exec() != KFileDialog::Accepted )
    {
       delete imageFileSaveDialog;
       return;
    }

    m_newFile = imageFileSaveDialog->selectedURL();
    QString format = KImageIO::typeForMime(imageFileSaveDialog->currentMimeFilter());
    delete imageFileSaveDialog;

    if (!m_newFile.isValid())
    {
        KMessageBox::error(this, i18n("Failed to save file\n\"%1\" to Album\n\"%2\"")
                           .arg(m_newFile.filename())
                           .arg(m_newFile.path().section('/', -2, -2)));
        kdWarning() << ("slotSaveAs:: target URL isn't valid !") << endl;
        return;
    }

    KURL currURL(m_urlCurrent);
    currURL.cleanPath();
    m_newFile.cleanPath();

    if (currURL.equals(m_newFile))
    {
        slotSave();
        return;
    }

    QFileInfo fi(m_newFile.path());
    if ( fi.exists() )
    {
        int result =
            KMessageBox::warningYesNo( this, i18n("About to overwrite file %1. "
                                                  "Are you sure you want to continue?")
                                       .arg(m_newFile.filename()) );

        if (result != KMessageBox::Yes)
            return;
    }

    QString tmpFile = locateLocal("tmp", m_newFile.filename());

    int result = m_canvas->saveAsTmpFile(tmpFile, m_JPEGCompression, m_PNGCompression,
                                         m_TIFFCompression, format.lower());

    if (result == false)
    {
        KMessageBox::error(this, i18n("Failed to save file\n\"%1\" to album\n\"%2\".")
                           .arg(m_newFile.filename())
                           .arg(m_newFile.path().section('/', -2, -2)));

        return;
    }

    // only try to write exif if both src and destination are jpeg files
    if (QString(QImageIO::imageFormat(m_urlCurrent.path())).upper() == "JPEG" &&
        format.upper() == "JPEG")
    {
        ExifRestorer exifHolder;
        exifHolder.readFile(m_urlCurrent.path(), ExifRestorer::ExifOnly);

        if (exifHolder.hasExif())
        {
            ExifRestorer restorer;
            restorer.readFile(tmpFile, ExifRestorer::EntireImage);
            restorer.insertExifData(exifHolder.exifData());
            restorer.writeFile(tmpFile);
        }
        else
            kdWarning() << ("slotSaveAs::No Exif Data Found") << endl;

        if( m_rotatedOrFlipped )
            KExifUtils::writeOrientation(tmpFile, KExifData::NORMAL);
    }

    KIO::FileCopyJob* job = KIO::file_move(KURL(tmpFile), m_newFile,
                                           -1, true, false, false);

    connect(job, SIGNAL(result(KIO::Job *) ),
            this, SLOT(slotSaveAsResult(KIO::Job *)));
}

void ImageWindow::slotSaveAsResult(KIO::Job *job)
{
    if (job->error())
    {
      job->showErrorDialog(this);
      return;
    }

    // Added new file URL into list if the new file has been added in the current Album

    KURL su(m_urlCurrent.directory());
    PAlbum *sourcepAlbum = AlbumManager::instance()->findPAlbum(su);

    if (!sourcepAlbum)
    {
        kdWarning() << ("slotSaveAsResult::Cannot found the source album!") << endl;
        return;
    }

    KURL tu(m_newFile.directory());
    PAlbum *targetpAlbum = AlbumManager::instance()->findPAlbum(tu);

    if (!targetpAlbum)
    {
        kdWarning() << ("slotSaveAsResult::Cannot found the target album!") << endl;
        return;
    }

    // Copy the metadata from the original image to the target image.

    AlbumDB* db = AlbumManager::instance()->albumDB();
    db->copyItem(sourcepAlbum, m_urlCurrent.fileName(),
                 targetpAlbum, m_newFile.fileName());

    if ( sourcepAlbum == targetpAlbum &&                       // Target Album = current Album ?
         m_urlList.find(m_newFile) == m_urlList.end() )        // The image file not already exist
    {                                                          // in the list.
        KURL::List::iterator it = m_urlList.find(m_urlCurrent);
        m_urlList.insert(it, m_newFile);
        m_urlCurrent = m_newFile;
    }

    emit signalFileAdded(m_newFile);

    QTimer::singleShot(0, this, SLOT(slotLoadCurrent()));      // Load the new target image.
}

void ImageWindow::slotToggleFullScreen()
{
    if (m_fullScreen)
    {

#if QT_VERSION >= 0x030300
        setWindowState( windowState() & ~WindowFullScreen );
#else
        showNormal();
#endif
        menuBar()->show();
        statusBar()->show();

        QObject* obj = child("toolbar","KToolBar");
        if (obj)
        {
            KToolBar* toolBar = static_cast<KToolBar*>(obj);
            if (m_guiClient->m_fullScreenAction->isPlugged(toolBar))
                m_guiClient->m_fullScreenAction->unplug(toolBar);
            if (toolBar->isHidden())
                toolBar->show();
        }

        // -- remove the imageguiclient action accels ----

        unplugActionAccel(m_guiClient->m_navNextAction);
        unplugActionAccel(m_guiClient->m_navPrevAction);
        unplugActionAccel(m_guiClient->m_navFirstAction);
        unplugActionAccel(m_guiClient->m_navLastAction);
        unplugActionAccel(m_guiClient->m_saveAction);
        unplugActionAccel(m_guiClient->m_saveAsAction);
        unplugActionAccel(m_guiClient->m_zoomPlusAction);
        unplugActionAccel(m_guiClient->m_zoomMinusAction);
        unplugActionAccel(m_guiClient->m_zoomFitAction);
        unplugActionAccel(m_guiClient->m_cropAction);
        unplugActionAccel(m_guiClient->m_fileprint);
        unplugActionAccel(m_guiClient->m_fileproperties);
        unplugActionAccel(m_guiClient->m_fileDelete);
        unplugActionAccel(m_guiClient->m_commentedit);

        m_fullScreen = false;
    }
    else
    {
        // hide the menubar and the statusbar
        menuBar()->hide();
        statusBar()->hide();

        QObject* obj = child("toolbar","KToolBar");
        if (obj)
        {
            KToolBar* toolBar = static_cast<KToolBar*>(obj);
            if (m_fullScreenHideToolBar)
                toolBar->hide();
            else
                m_guiClient->m_fullScreenAction->plug(toolBar);
        }

        // -- Insert all the imageguiclient actions into the accel --

        plugActionAccel(m_guiClient->m_navNextAction);
        plugActionAccel(m_guiClient->m_navPrevAction);
        plugActionAccel(m_guiClient->m_navFirstAction);
        plugActionAccel(m_guiClient->m_navLastAction);
        plugActionAccel(m_guiClient->m_saveAction);
        plugActionAccel(m_guiClient->m_saveAsAction);
        plugActionAccel(m_guiClient->m_zoomPlusAction);
        plugActionAccel(m_guiClient->m_zoomMinusAction);
        plugActionAccel(m_guiClient->m_zoomFitAction);
        plugActionAccel(m_guiClient->m_cropAction);
        plugActionAccel(m_guiClient->m_fileprint);
        plugActionAccel(m_guiClient->m_fileproperties);
        plugActionAccel(m_guiClient->m_fileDelete);
        plugActionAccel(m_guiClient->m_commentedit);

        showFullScreen();
        m_fullScreen = true;
    }
}

void ImageWindow::slotEscapePressed()
{
    if (m_fullScreen)
    {
        m_guiClient->m_fullScreenAction->activate();
    }
}

void ImageWindow::promptUserSave()
{
    if (m_guiClient->m_saveAction->isEnabled())
    {
        int result =
            KMessageBox::warningYesNo(this,
                                      i18n("The image \"%1\" has been modified.\n"
                                           "Do you want to save it?")
                                      .arg(m_urlCurrent.filename()));
        if (result == KMessageBox::Yes)
            slotSave();
    }
}

void ImageWindow::closeEvent(QCloseEvent *e)
{
    if (!e) return;

    promptUserSave();
    e->accept();
}

void ImageWindow::plugActionAccel(KAction* action)
{
    if (!action)
        return;

    m_accel->insert(action->text(),
                    action->text(),
                    action->whatsThis(),
                    action->shortcut(),
                    action,
                    SLOT(activate()));
}

void ImageWindow::unplugActionAccel(KAction* action)
{
    m_accel->remove(action->text());
}

void ImageWindow::slotImagePluginsHelp()
{
    KApplication::kApplication()->invokeHelp( QString::null, "digikamimageplugins" );
}

#include "imagewindow.moc"

