/**
 * MelodyEditor.cpp
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#include <QWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <QDomDocument>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTextStream>
#include <QEvent>
#include <QDebug>

#include "MelodyEditor.h"


#include "embed.h"
#include "plugin_export.h"
#include "ComboBoxModel.h"

#include "gui/MelodyEditorView.h"
#include "parsers/ParserBase.h"
#include "parsers/AnytizerParser.h"
#include "alda/parser/parser.h"


namespace lmms
{

extern "C"
{

	Plugin::Descriptor PLUGIN_EXPORT melodyeditor_plugin_descriptor = {
		LMMS_STRINGIFY(PLUGIN_NAME),
		"Melody Editor",
		QT_TRANSLATE_NOOP("PluginBrowser", "Melody Editor"),
		"Bimal Poudel <anytizer@users.noreply.github.com>",
		0x0100,
		Plugin::Type::Tool,
		new PluginPixmapLoader("logo"),
		"txt", // nullptr,
		nullptr,
	};

	PLUGIN_EXPORT Plugin* lmms_plugin_main(Model*, void*)
	{
		return new MelodyEditor();
	}

} // extern "C"




MelodyEditor::MelodyEditor()
	: ToolPlugin(&melodyeditor_plugin_descriptor, nullptr)
	, m_parsers{
		new ::PLUGIN_NAME::AldaParser(),
		new ::PLUGIN_NAME::AnytizerParser(::PLUGIN_NAME::ENGLISH_MAPPING),
		new ::PLUGIN_NAME::AnytizerParser(::PLUGIN_NAME::HINDUSTANI_MAPPING),
		new ::PLUGIN_NAME::AnytizerParser(::PLUGIN_NAME::CARNATIC_MAPPING)
	}
	, m_parserModel(new ComboBoxModel(this, "Parser"))
{	
	for(auto parser : m_parsers)
	{
		auto icon = std::make_unique<PluginPixmapLoader>(parser->icon());
		m_parserModel->addItem(parser->name(), std::move(icon));
	}
}




QString MelodyEditor::nodeName() const
{
	return melodyeditor_plugin_descriptor.name;
}




gui::PluginView* MelodyEditor::instantiateView(QWidget*)
{
	return new gui::MelodyEditorView(this);
}




QString MelodyEditor::readFromClip(QString& error)
{
	if (!m_midiClip)
	{
		error = "Open a Piano-Roll Window first.";
		return {};
	}

	auto parser = m_parsers.at(m_parserModel->value());
	return parser->generate(m_midiClip->notes(), error);
}




void MelodyEditor::writeToClip(const QString& text, QString& error)
{
	if (!m_midiClip)
	{
		error = "Open a Piano-Roll Window first.";
		return;
	}

	auto parser = m_parsers.at(m_parserModel->value());
	auto notes = parser->parse(text, error);

	if (notes.empty()) { return; }

	m_midiClip->setJournalling(false);
	m_midiClip->clear();
	for (auto note: notes)
	{
		m_midiClip->addNote(note);
	}
	m_midiClip->setJournalling(true);
}


} // namespace lmms
