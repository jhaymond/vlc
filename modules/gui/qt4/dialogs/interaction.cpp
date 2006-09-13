/*****************************************************************************
 * interaction.cpp : Interaction stuff
 ****************************************************************************
 * Copyright (C) 2006 the VideoLAN team
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA. *****************************************************************************/

#include "qt4.hpp"
#include "dialogs/errors.hpp"
#include "dialogs/interaction.hpp"
#include "util/qvlcframe.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QMessageBox>

#include <assert.h>

InteractionDialog::InteractionDialog( intf_thread_t *_p_intf,
                         interaction_dialog_t *_p_dialog ) : QWidget( 0 ),
                          p_intf( _p_intf), p_dialog( _p_dialog )
{
    QVBoxLayout *layout = new QVBoxLayout( this );
    int i_ret = -1;
    panel = NULL;

    if( p_dialog->i_flags & DIALOG_BLOCKING_ERROR )
    {
        i_ret = QMessageBox::critical( this, qfu( p_dialog->psz_title ),
                                       qfu( p_dialog->psz_description ),
                                       QMessageBox::Ok, 0, 0 );
    }
    else if( p_dialog->i_flags & DIALOG_NONBLOCKING_ERROR )
    {
        if( config_GetInt( p_intf, "errors-dialog" ) != 0 )
            ErrorsDialog::getInstance( p_intf )->addError(
                 qfu( p_dialog->psz_title ), qfu( p_dialog->psz_description ) );
        i_ret = 0;
        //  QApplication::style()->standardPixmap(QStyle::SP_MessageBoxCritical)
    }
    else if( p_dialog->i_flags & DIALOG_WARNING )
    {
        if( config_GetInt( p_intf, "errors-dialog" ) != 0 )
            ErrorsDialog::getInstance( p_intf )->addWarning(
                qfu( p_dialog->psz_title ),qfu( p_dialog->psz_description ) );
        i_ret = 0;
    }
    else if( p_dialog->i_flags & DIALOG_YES_NO_CANCEL )
    {
        i_ret = QMessageBox::question( this,
              qfu( p_dialog->psz_title), qfu( p_dialog->psz_description ),
              p_dialog->psz_default_button ?
                    qfu( p_dialog->psz_default_button ) : QString::null,
              p_dialog->psz_alternate_button ?
                    qfu( p_dialog->psz_alternate_button ) : QString::null,
              p_dialog->psz_other_button ?
                    qfu( p_dialog->psz_other_button ) : QString::null, 0,
              p_dialog->psz_other_button ? 2 : -1 );
    }
    else if( p_dialog->i_flags & DIALOG_LOGIN_PW_OK_CANCEL )
    {
        panel = new QWidget( 0 );
        QGridLayout *grid = new QGridLayout;

        description = new QLabel( qfu( p_dialog->psz_description ) );
        grid->addWidget( description, 0, 0, 1, 2 );

        grid->addWidget( new QLabel( qtr( "Login") ), 1, 0 );
        loginEdit = new QLineEdit;
        grid->addWidget( loginEdit, 1, 1 );

        grid->addWidget( new QLabel( qtr("Password") ), 2, 0);
        passwordEdit = new QLineEdit;
        grid->addWidget( passwordEdit, 2, 1 );

        panel->setLayout( grid );
        layout->addWidget( panel );
    }
    else if( p_dialog->i_flags & DIALOG_USER_PROGRESS )
    {
        description = new QLabel( qfu( p_dialog->psz_description ) );
        layout->addWidget( description );

        progressBar = new QProgressBar;
        progressBar->setMaximum( 1000 );
        progressBar->setTextVisible( true );
        progressBar->setOrientation(Qt::Horizontal);
        layout->addWidget( progressBar );
    }
    else if( p_dialog->i_flags & DIALOG_PSZ_INPUT_OK_CANCEL )
    {
        description = new QLabel( qfu( p_dialog->psz_description ) );
        layout->addWidget( description );

        inputEdit = new QLineEdit;
        layout->addWidget( inputEdit );
    }
    else
        msg_Err( p_intf, "unknown dialog type" );

    /* We used a message box */
    if( i_ret != -1 )
    {
        if( i_ret == 0 ) Finish( DIALOG_OK_YES );
        else if ( i_ret == 1 ) Finish( DIALOG_NO );
        else Finish( DIALOG_CANCELLED );
    }
    else
    /* Custom box, finish it */
    {
        QVLCFrame::doButtons( this, layout,
                              &defaultButton, p_dialog->psz_default_button,
                              &altButton, p_dialog->psz_alternate_button,
                              &otherButton, p_dialog->psz_other_button );
        if( p_dialog->psz_default_button )
            BUTTONACT( defaultButton, defaultB );
        if( p_dialog->psz_alternate_button )
            BUTTONACT( altButton, altB );
        if( p_dialog->psz_other_button )
            BUTTONACT( otherButton, otherB );
        setLayout( layout );
        setWindowTitle( qfu( p_dialog->psz_title ) );
    }
}

void InteractionDialog::Update()
{
    if( p_dialog->i_flags & DIALOG_USER_PROGRESS )
    {
        assert( progressBar );
        progressBar->setValue( (int)(p_dialog->val.f_float*1000) );
    }
}

InteractionDialog::~InteractionDialog()
{
    if( panel ) delete panel;
}

void InteractionDialog::defaultB()
{
    Finish( DIALOG_OK_YES );
}
void InteractionDialog::altB()
{
    Finish( DIALOG_NO );
}
void InteractionDialog::otherB()
{
    Finish( DIALOG_CANCELLED );
}

void InteractionDialog::Finish( int i_ret )
{
    vlc_mutex_lock( &p_dialog->p_interaction->object_lock );

    if( p_dialog->i_flags & DIALOG_LOGIN_PW_OK_CANCEL )
    {
        p_dialog->psz_returned[0] = strdup( qtu( loginEdit->text() ) );
        p_dialog->psz_returned[1] = strdup( qtu( passwordEdit->text() ) );
    }
    else if( p_dialog->i_flags & DIALOG_PSZ_INPUT_OK_CANCEL )
    {
        p_dialog->psz_returned[0] = strdup( qtu( inputEdit->text() ) );
    }
    p_dialog->i_status = ANSWERED_DIALOG;
    p_dialog->i_return = i_ret;
    hide();
    vlc_mutex_unlock( &p_dialog->p_interaction->object_lock );
}
