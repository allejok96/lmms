/**
 * MelodyEditorTextArea.cpp
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#ifndef LMMS_GUI_MELODY_EDITOR_TEXTAREA_CPP
#define LMMS_GUI_MELODY_EDITOR_TEXTAREA_CPP

#include "TextArea.h"

#include <QMimeData>
#include <QTextStream>
#include <QString>
#include <QFont>
#include <QFileInfo>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QWheelEvent>
#include <QMessageBox>


namespace PLUGIN_NAME::gui
{


/**
 * 20kb notations gives around 2000 to 5000 measures.
 * This is nearly 1 hour long play time, @160 bpm.
 * Processing too large text may cause crash or delay.
 */
const int MAX_FILE_SIZE = 20 * 1024;

/**
 * Zoom factors on notations editor
 */
const int ZOOM_FACTOR = 90; // 80 to 120 | 120 default
const int MIN_FONTSIZE = 10; // min: readable font size: 10
const int MAX_FONTSIZE = 28; // max 36




QString readFile(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { return {}; }
	return QTextStream(&file).readAll();
}




TextArea::TextArea()
{
	setAcceptDrops(true);

	// Try to be OS neutral ~~and printer friendly~~.
	// Make a room for zoom out also.
	QFont font("Consolas", MIN_FONTSIZE+4); // Consolas | sans-serif @ 14 points
	setFont(font);
	setStyleSheet("font-family: Menlo, Consolas, 'Ubuntu Mono', 'Roboto Mono', 'DejaVu Sans Mono', monospace;");

	setCursor(Qt::IBeamCursor);

	// Prepare to enforce scrollbars on zooming
	setLineWrapMode(QPlainTextEdit::NoWrap);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}



void TextArea::loadFile(const QString& filename)
{
	QFile file(filename);
	if (file.size() > MAX_FILE_SIZE)
	{
		QMessageBox::critical(this, "Error", "The file you are trying to load is too large");
		return;
	}
	setPlainText(readFile(filename));
}


/**
 * Custom zoom logic on ctrl+wheel
 */
void TextArea::wheelEvent(QWheelEvent *event)
{
	static int angleDeltaRemainder = 0;

	if (event->modifiers() & Qt::ControlModifier)
	{
		QFont newFont = font();
		angleDeltaRemainder += event->angleDelta().y();

		const int pointsize = std::clamp(
			newFont.pointSize() + angleDeltaRemainder / ZOOM_FACTOR,
			MIN_FONTSIZE,
			MAX_FONTSIZE
		);

		angleDeltaRemainder %= ZOOM_FACTOR;

		newFont.setPointSize(pointsize);
		setFont(newFont);
	}

	// @see https://doc.qt.io/qt-6/qml-qtquick-controls-scrollview.html
	event->accept();
	QPlainTextEdit::wheelEvent(event); // make scrollable with wheel
}



} // lmms::gui

#endif // LMMS_GUI_MELODY_EDITOR_TEXTAREA_CPP
