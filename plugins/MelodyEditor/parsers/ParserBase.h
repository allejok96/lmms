/**
 * ParserBase.h
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#ifndef MELODY_EDITOR_PARSER_BASE_H
#define MELODY_EDITOR_PARSER_BASE_H

#include "Note.h"


namespace PLUGIN_NAME
{

typedef std::vector<::lmms::Note> NoteList;

class ParserBase
{
public:
	ParserBase(const QString& name, const std::string iconName)
		: m_name(name)
		, m_icon(iconName)
	{
	}

	const QString& name() const { return m_name; }
	std::string icon() const { return m_icon; }

	//! Convert text to notes
	virtual NoteList parse(const QString& text, QString& error) = 0;

	//! Convert notes to text
	virtual QString generate(lmms::NoteVector notes, QString& error)
	{
		error = "This parser does not support loading notes from the editor";
		return {};
	}

private:
	QString m_name;
	std::string m_icon;

};

}

#endif // MELODY_EDITOR_PARSER_BASE_H
