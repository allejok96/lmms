/**
 * MelodyEditorView.h
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#ifndef LMMS_GUI_MELODY_EDITOR_VIEW_H
#define LMMS_GUI_MELODY_EDITOR_VIEW_H

#include "ToolPluginView.h"


class QLineEdit;


namespace PLUGIN_NAME::gui
{
class TextArea;
}


namespace lmms
{
class MelodyEditor;
}


namespace lmms::gui
{




class MelodyEditorView : public ToolPluginView
{
	Q_OBJECT
public:
	MelodyEditorView(MelodyEditor* plugin);

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;

private slots:
	void setClipFromPianoRoll();
	void openFileSelector();
	void readFromClip();
	void writeToClip();


private:
	MelodyEditor* m_plugin;
	::PLUGIN_NAME::gui::TextArea* m_textArea;
	QLineEdit* m_errorBox;
	QString m_file = "";


};


} // namespace lmms::gui

#endif // LMMS_GUI_MELODY_EDITOR_VIEW_H

