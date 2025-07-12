/*
 * ProjectNotes.cpp - implementation of project-notes-editor
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "ProjectNotes.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QFontDatabase>
#include <QLineEdit>
#include <QMdiArea>
#include <QTextEdit>
#include <QToolBar>
#include <QDomCDATASection>

#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "KeyboardShortcuts.h"
#include "MainWindow.h"
#include "Song.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QFont>
#include <QScrollArea>
#include <QTextCursor>
#include <QSplitter>
#include <QScrollBar>

namespace lmms::gui
{


bool WheelEventFilter::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Wheel) {
		QWheelEvent *ev = static_cast<QWheelEvent *>(event);
		QString mods;
		if (ev->modifiers() & Qt::SHIFT) { mods.append("shift;"); }
		if (ev->modifiers() & Qt::ALT) { mods.append("alt;"); }
		if (ev->modifiers() & Qt::CTRL) { mods.append("ctrl;"); }
		if (ev->modifiers() & Qt::MetaModifier) { mods.append("meta;"); }
		
		m_edit->textCursor().insertText(QString("\n %8 angle=%1:%2 px=%3:%4 mod=%5 phase=%6 inv=%7 ")
				.arg(ev->angleDelta().x())
				.arg(ev->angleDelta().y())
				.arg(ev->pixelDelta().x())
				.arg(ev->pixelDelta().y())
				.arg(mods)
				.arg(ev->phase())
				.arg(ev->inverted())
				.arg(obj->metaObject()->className()));
		m_edit->textCursor().movePosition(QTextCursor::End);
		m_edit->ensureCursorVisible();
	}
	return false; // Don't consume the event
}


ProjectNotes::ProjectNotes() :
	QMainWindow( getGUI()->mainWindow()->workspace() )
{
	m_edit = new QTextEdit( this );
	m_edit->setAutoFillBackground( true );
	
	auto filter = new WheelEventFilter(this, m_edit);
	
	auto font = QFont("Sans", 500, QFont::Bold);
	auto label = new QLabel("You're gonna need\na bigger box");
	label->setFont(font);
	
	auto scrollArea = new QScrollArea;
	scrollArea->setWidget(label);
	scrollArea->horizontalScrollBar()->installEventFilter(filter);
	scrollArea->verticalScrollBar()->installEventFilter(filter);
	
	m_horSlider = new QSlider(Qt::Horizontal);
	m_horSlider->installEventFilter(filter);
	m_horSlider->setMinimumWidth(100);
	m_horSlider->setMaximum(1000);
	connect(m_horSlider, &QSlider::valueChanged, this, [this]{
		m_edit->textCursor().insertText(QString("sliderx=%1 ").arg(m_horSlider->value()));
		m_edit->textCursor().movePosition(QTextCursor::End);
		m_edit->ensureCursorVisible();
	});
	
	m_verSlider = new QSlider(Qt::Vertical);
	m_verSlider->installEventFilter(filter);
	m_verSlider->setMinimumHeight(100);
	m_verSlider->setMaximum(1000);
	connect(m_verSlider, &QSlider::valueChanged, this, [this]{
		m_edit->textCursor().insertText(QString("slidery=%1 ").arg(m_verSlider->value()));
		m_edit->textCursor().movePosition(QTextCursor::End);
		m_edit->ensureCursorVisible();
	});
	
	auto toolsWidget = new QWidget;
	auto toolsLayout = new QHBoxLayout(toolsWidget);
	toolsLayout->addWidget(scrollArea);
	toolsLayout->addWidget(m_horSlider);
	toolsLayout->addWidget(m_verSlider);
	
	auto mainWidget = new QSplitter(Qt::Vertical);
	mainWidget->addWidget(toolsWidget);
	mainWidget->addWidget(m_edit);

	setCentralWidget(mainWidget);
	setWindowTitle( tr( "Project Notes" ) );
	setWindowIcon( embed::getIconPixmap( "project_notes" ) );

	getGUI()->mainWindow()->addWindowedWidget( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 700, 10 );
	parentWidget()->resize( 400, 300 );
	parentWidget()->show();
}




void ProjectNotes::clear()
{
}




void ProjectNotes::setText( const QString & _text )
{
}




void ProjectNotes::setupActions()
{
}




void ProjectNotes::textBold()
{
}




void ProjectNotes::textUnderline()
{
}




void ProjectNotes::textItalic()
{
}




void ProjectNotes::textFamily( const QString & _f )
{
}




void ProjectNotes::textSize( const QString & _p )
{
}




void ProjectNotes::textColor()
{
}




void ProjectNotes::textAlign( QAction * _a )
{
}




void ProjectNotes::formatChanged( const QTextCharFormat & _f )
{
}




void ProjectNotes::alignmentChanged( int _a )
{
}




void ProjectNotes::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
}




void ProjectNotes::loadSettings( const QDomElement & _this )
{
}




void ProjectNotes::closeEvent( QCloseEvent * _ce )
{
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
 }

} // namespace lmms::gui
