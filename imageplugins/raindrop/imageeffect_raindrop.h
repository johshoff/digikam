/* ============================================================
 * File  : imageeffect_raindrop.h
 * Author: Gilles Caulier <caulier dot gilles at free.fr>
 * Date  : 2004-09-30
 * Description : a Digikam image plugin for to simulate 
 *               a rain droppping on an image.
 * 
 * Copyright 2004-2005 by Gilles Caulier
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

#ifndef IMAGEEFFECT_RAINDROP_H
#define IMAGEEFFECT_RAINDROP_H

// KDE includes.

#include <kdialogbase.h>

class QPushButton;
class QSpinBox;
class QSlider;
class QTimer;

class KProgress;
class KIntNumInput;

namespace Digikam
{
class ImageWidget;
}

namespace DigikamRainDropImagesPlugin
{

class ImageEffect_RainDrop : public KDialogBase
{
    Q_OBJECT
    
public:

    ImageEffect_RainDrop(QWidget *parent);
    ~ImageEffect_RainDrop();

protected:

    void closeEvent(QCloseEvent *e);
    void rainDrops(uint *data, int Width, int Height, int DropSize, int Amount, int Coeff);
    
private:
    
    bool                  m_cancel;
    bool                  m_dirty;
    
    QWidget              *m_parent;
    
    QPushButton          *m_helpButton;

    QTimer               *m_timerDrop;
    QTimer               *m_timerAmount;
    QTimer               *m_timerCoeff;
            
    KIntNumInput         *m_dropInput;
    KIntNumInput         *m_amountInput;
    KIntNumInput         *m_coeffInput;    
    
    KProgress            *m_progressBar;
    
    Digikam::ImageWidget *m_previewWidget;

    inline bool** CreateBoolArray (uint Columns, uint Rows);
    inline void FreeBoolArray (bool** lpbArray, uint Columns);
    inline uchar LimitValues (int ColorValue);
    
private slots:

    void slotHelp();
    void slotEffect();
    void slotOk();
    void slotCancel();
    void slotUser1();
    void slotTimerDrop();
    void slotTimerAmount();    
    void slotTimerCoeff();    
};

}  // NameSpace DigikamRainDropImagesPlugin

#endif /* IMAGEEFFECT_RAINDROP_H */
