/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2005-03-10
 * Description : a plugin to apply texture over an image
 * 
 * Copyright (C) 2005-2007 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

// KDE includes.
  
#include <klocale.h>
#include <kgenericfactory.h>
#include <klibloader.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kcursor.h>

// Local includes.

#include "ddebug.h"
#include "imageeffect_texture.h"
#include "imageplugin_texture.h"
#include "imageplugin_texture.moc"

K_EXPORT_COMPONENT_FACTORY(digikamimageplugin_texture,
                           KGenericFactory<ImagePlugin_Texture>("digikamimageplugin_texture"));

ImagePlugin_Texture::ImagePlugin_Texture(QObject *parent, const QStringList &)
                   : Digikam::ImagePlugin(parent, "ImagePlugin_Texture")
{
                
    m_textureAction  = new KAction(KIcon("texture"), i18n("Apply Texture..."), this);
    actionCollection()->addAction("imageplugin_texture", m_textureAction );
    connect(m_textureAction, SIGNAL(triggered(bool) ), SLOT(slotTexture()));
    setXMLFile( "digikamimageplugin_texture_ui.rc" );                                
    
    DDebug() << "ImagePlugin_Texture plugin loaded" << endl;
}

ImagePlugin_Texture::~ImagePlugin_Texture()
{
}

void ImagePlugin_Texture::setEnabledActions(bool enable)
{
    m_textureAction->setEnabled(enable);
}

void ImagePlugin_Texture::slotTexture()
{
    DigikamTextureImagesPlugin::ImageEffect_Texture dlg(parentWidget());
    dlg.exec();
}

