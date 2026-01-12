/**
 * AnytizerParser.h
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#ifndef MELODY_EDITOR_ANYTIZER_PARSER_H
#define MELODY_EDITOR_ANYTIZER_PARSER_H

#include "AnytizerMappings.h"
#include "ParserBase.h"

namespace PLUGIN_NAME
{


class AnytizerParser : public ParserBase
{
public:
	AnytizerParser(const AnytizerMapping& info);

	NoteList parse(const QString& text, QString& error) override;
	void reset();

private:
	//! Read mode - keeps track of what the parser is currently doing
	enum class Mode
	{
		None,
		Comment,
		NoteName,
		NoteAttrib,
		ChordLength,
	};

	void processChar(char character, QString& error);
	void endCurrentNote();

	ReplacementMapping m_replacementMap {};

	NoteList m_notes {};
	NoteList m_chord {};

	Mode m_mode;
	int m_cursorPos;
	int m_key;
	int m_octave;
	int m_length;
	bool m_insideChord;
};

}

#endif // MELODY_EDITOR_ANYTIZER_PARSER_H
