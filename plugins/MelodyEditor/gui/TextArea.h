/**
 * TextArea.h
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#ifndef MELODY_EDITOR_GUI_TEXTAREA_H
#define MELODY_EDITOR_GUI_TEXTAREA_H

#include <QPlainTextEdit>

class QWheelEvent;


namespace PLUGIN_NAME::gui
{


class TextArea: public QPlainTextEdit
{
	Q_OBJECT
public:
	TextArea();
	void loadFile(const QString& filename);

protected:
	void wheelEvent(QWheelEvent *event) override;
};


}

#endif // MELODY_EDITOR_TEXTAREA_H
