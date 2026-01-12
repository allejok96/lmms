/**
 * MelodyEditor.h
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#ifndef LMMS_MELODY_EDITOR_H
#define LMMS_MELODY_EDITOR_H

#include <QPointer>
#include "ToolPlugin.h"
#include "MidiClip.h"


namespace PLUGIN_NAME
{
class ParserBase;
}


namespace lmms::gui
{
class MelodyEditorView;
}


namespace lmms
{

class ComboBoxModel;


class MelodyEditor : public ToolPlugin
{
public:
	MelodyEditor();
	virtual ~MelodyEditor() override = default;

	QString nodeName() const override;
	void saveSettings(QDomDocument&, QDomElement&) override {}
	void loadSettings(const QDomElement&) override {}

	gui::PluginView* instantiateView(QWidget*) override;

	void setMidiClip(MidiClip* midiClip) { m_midiClip = midiClip; }

	QString readFromClip(QString& errors);
	void writeToClip(const QString& text, QString& errors);


private:
	const std::vector<::PLUGIN_NAME::ParserBase*> m_parsers;
	ComboBoxModel* m_parserModel;

	QPointer<MidiClip> m_midiClip;

friend class gui::MelodyEditorView;
};


} // namespace lmms

#endif // LMMS_MELODY_EDITOR_H
