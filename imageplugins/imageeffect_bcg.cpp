/* ============================================================
 * File  : imageeffect_bcg.cpp
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2004-06-05
 * Description : 
 * 
 * Copyright 2004 by Renchi Raju

 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published bythe Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * ============================================================ */

#include <qlayout.h>
#include <qframe.h>
#include <qvgroupbox.h>
#include <qlabel.h>

#include <knuminput.h>
#include <klocale.h>

#include <imageiface.h>
#include <imagewidget.h>

#include "imageeffect_bcg.h"

ImageEffect_BCG::ImageEffect_BCG(QWidget* parent)
    : KDialogBase(Plain, i18n("Brightness/Contrast/Gamma"),
                  Ok|Cancel, Ok,
                  parent, 0, true, true)
{
    QVBoxLayout *topLayout = new QVBoxLayout( plainPage(),
                                              0, spacingHint());

    QVGroupBox *gbox = new QVGroupBox(i18n("Adjust Brightness/Contrast/Gamma"),
                                      plainPage());
    QFrame *frame = new QFrame(gbox);
    frame->setFrameStyle(QFrame::Panel|QFrame::Sunken);
    QVBoxLayout* l  = new QVBoxLayout(frame, 5, 0);
    m_previewWidget = new Digikam::ImageWidget(480,320,frame);
    l->addWidget(m_previewWidget, 0, Qt::AlignCenter);
    topLayout->addWidget(gbox);

    QHBoxLayout *hlay  = 0;
    QLabel      *label = 0;

    hlay     = new QHBoxLayout(topLayout);
    label    = new QLabel(i18n("Brightness :"), plainPage());
    m_bInput = new KDoubleNumInput(plainPage());
    m_bInput->setPrecision(2);
    m_bInput->setRange(-1.0, 1.0, 0.01, true);
    hlay->addWidget(label,1);
    hlay->addWidget(m_bInput,5);

    hlay     = new QHBoxLayout(topLayout);
    label    = new QLabel(i18n("Contrast :"), plainPage());
    m_cInput = new KDoubleNumInput(plainPage());
    m_cInput->setPrecision(2);
    m_cInput->setRange(0.0, 2.0, 0.01, true);
    hlay->addWidget(label,1);
    hlay->addWidget(m_cInput,5);

    hlay     = new QHBoxLayout(topLayout);
    label    = new QLabel(i18n("Gamma :"), plainPage());
    m_gInput = new KDoubleNumInput(plainPage());
    m_gInput->setPrecision(2);
    m_gInput->setRange(0.0, 2.0, 0.01, true);
    hlay->addWidget(label,1);
    hlay->addWidget(m_gInput,5);

    m_bInput->setValue(0.0);
    m_cInput->setValue(1.0);
    m_gInput->setValue(1.0);

    connect(m_bInput, SIGNAL(valueChanged (double)),
            SLOT(slotEffect()));
    connect(m_cInput, SIGNAL(valueChanged (double)),
            SLOT(slotEffect()));
    connect(m_gInput, SIGNAL(valueChanged (double)),
            SLOT(slotEffect()));

    adjustSize();
}

ImageEffect_BCG::~ImageEffect_BCG()
{
    
}

void ImageEffect_BCG::slotEffect()
{
    Digikam::ImageIface* iface =
        m_previewWidget->imageIface();

    double b = m_bInput->value();
    double c = m_cInput->value();    
    double g = m_gInput->value();

    iface->setPreviewBCG(b, c, g);
    m_previewWidget->update();
}

void ImageEffect_BCG::slotOk()
{
    Digikam::ImageIface* iface =
        m_previewWidget->imageIface();

    double b = m_bInput->value();
    double c = m_cInput->value();    
    double g = m_gInput->value();

    iface->setOriginalBCG(b, c, g);

    accept();
}

#include "imageeffect_bcg.moc"
