/**
 * AnytizerParser.cpp
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

/*
(?<NAME>PATTERN)

instrument ^[a-zA-Z0-9]+ ("[^"]*")? :
setOctave o\d+
octaveUp <
octaveDown >
note [a-gA-G]
accidentals [_+-]+
duration \d+[.]*
extend ~
chordDelim /
rest r
voice V\d+:
repeat [*]\d+
groupBegin \[
groupEnd \]
alternative '\d+(-\d+|,\d+)*
variable [a-ZA-Z}{2}[a-zA-Z0-9_]+ = .*
multiLineVar [a-ZA-Z}{2}[a-zA-Z0-9_]+ = [.*]
tupleBegin {
tupleEnd }(\d+)?[.]*



(key-signature "f+ c+")
(key-sig f (sharp) c (sharp))
(key-sig (a major))
(octave 'up)
(octave 'down)
(octave N)
(pan 0-100)
(quantization 0-100) (quantize 0-100) (quant 0-100)
(tempo NNN)
(track-volume NNN.N)
(transposition N) (transpose N)
(volume 0-100)
(pppppp) (mp) (mf) (ffffff)
(midi-channel NN)
*/

#include "AnytizerParser.h"

#include <QString>
#include <QList>
#include <QRegularExpression>

namespace PLUGIN_NAME
{

constexpr int DEFAULT_NOTE_LENGTH = 48;
constexpr int DEFAULT_OCTAVE = 5;

const QRegularExpression SPLITTER_REGEX("(o\\d+)|");


//! Convert lower-case letter to a piano key
int pianoKey(const char c)
{
	switch (c)
	{
	case 'c': return 0;
	case 'd': return 2;
	case 'e': return 4;
	case 'f': return 5;
	case 'g': return 7;
	case 'a': return 9;
	case 'b':
	case 'h': return 11;
	default: return -1;
	}
}




//! Find the shortest note length in a chord
int lengthOfShortestNote(NoteList& chord)
{
	if (chord.empty()) { return 0; }

	int shortest = chord.back().length();
	for (const auto& note: chord)
	{
		shortest = std::min<int>(shortest, note.length());
	}
	return shortest;
}




//! Replace a bunch of sub-strings and return a new string
QString applyReplacements(QString text, const std::vector<std::pair<QString, QString>> replacements)
{
	for (const auto& [string, replacement]: replacements)
	{
		text.replace(string, replacement);
	}
	return text;
}




AnytizerParser::AnytizerParser(const AnytizerMapping& info)
: ParserBase(info.name, info.icon)
, m_replacementMap(info.replacements)
{
	// Extend the mapping with the default replacements
	m_replacementMap.insert(m_replacementMap.end(), DEFAULT_REPLACEMENTS.begin(), DEFAULT_REPLACEMENTS.end());
}



//! Fresh start
void AnytizerParser::reset()
{
	m_notes.clear();
	m_chord.clear();

	m_mode = Mode::None;
	m_cursorPos = 0;
	m_key = -1;
	m_octave = DEFAULT_OCTAVE;
	m_length = 0;
	m_insideChord = false;
}




NoteList AnytizerParser::parse(const QString& text, QString& error)
{
	reset();

	std::string rawText = applyReplacements(text, m_replacementMap).toLower().toStdString();

	int line = 1;
	int col = 0;
	QString errorMessage;

	for (auto character : rawText)
	{
		if (character == '\n')
		{
			line += 1;
			col = 0;
		}

		processChar(character, error);

		if (!error.isEmpty())
		{
			error.insert(0, QString("line %1 col %2: ").arg(line).arg(col));
			return {};
		}

		col += 1;
	}

	// Close open chords and process the last note
	endCurrentNote();
	m_insideChord = false;
	endCurrentNote();

	return m_notes;
}




void AnytizerParser::processChar(const char character, QString& error)
{
	// Skip everything if we are in a inline comment, until we hit the newline
	if (m_mode == Mode::Comment)
	{
		if (character == '\n') { m_mode = Mode::None; }
		else { return; }
	}

	switch (character)
	{	
	case '\0':
	case '\t':
	case ' ':
		if (m_mode == Mode::NoteName) { m_mode = Mode::NoteAttrib; }
		break;

	case '\n':
	case '|':
	case '/':
		endCurrentNote();
		break;

	case 'x':
		endCurrentNote();
		m_mode = Mode::NoteAttrib;
		m_length = DEFAULT_NOTE_LENGTH;
		break;

	case '#':
		// Raise current note, or begin comment
		if (m_mode == Mode::NoteName)
		{
			m_key += 1;
		}
		else
		{
			endCurrentNote();
			m_mode = Mode::Comment;
		}
		break;

	case '*':
	case '.':
		// Raise or lower octave
		if (m_mode == Mode::NoteName) { m_mode = Mode::NoteAttrib; }
		m_octave += character == '*' ? 1 : -1;
		if (m_key >= 0)
		{
			m_key += character == '*' ? 12 : -12;
		}
		break;

	case '[':
		// Begin chord
		if (m_insideChord) { error = "Cannot nest chords"; }
		endCurrentNote();
		m_insideChord = true;
		break;

	case ']':
		// End chord
		if (!m_insideChord) { error = "Unexpected ]"; }
		endCurrentNote();
		m_insideChord = false; // important to have this after endCurrentNote
		m_length = 0;
		m_mode = Mode::ChordLength;
		break;

	case '-':
		// Extend chord or note
		switch (m_mode)
		{
		case Mode::NoteName:
		case Mode::NoteAttrib:
		case Mode::ChordLength:
			// The default length of a chord group is zero
			// (meaning it has no length, it will be determined by the notes in the chord)
			// so if we assign the chord group a length, the first step is doubled for convenience
			m_length += m_length == 0 ? 2 * DEFAULT_NOTE_LENGTH : DEFAULT_NOTE_LENGTH;
			break;
		default:
			error = "Missing note name";
		}
		break;

	default:
		// Add new note, or lower the current if it is a 'b'
		int key = pianoKey(character);
		if (key < 0)
		{
			error = QString("Unknown character: %1 (%2)").arg(character).arg(static_cast<int>(character));
		}
		else if (m_mode == Mode::NoteName)
		{
			if (character == 'b')
			{
				m_key -= 1;
			}
			else
			{
				error = "Missing separator between notes";
			}
		}
		else
		{
			endCurrentNote();
			m_mode = Mode::NoteName;
			m_key = key + 12 * m_octave;
			m_length = DEFAULT_NOTE_LENGTH;
		}
	}
}




void AnytizerParser::endCurrentNote()
{
	auto& noteList = m_insideChord ? m_chord : m_notes;

	// If we have set chord length, apply it to all notes in the chord
	if (m_mode == Mode::ChordLength && m_length > 0)
	{
		for (auto& note: m_chord)
		{
			note.setLength(m_length);
		}
	}

	// Dump content of chord if it has ended
	if (!m_insideChord && !m_chord.empty())
	{
		for (auto& note: m_chord)
		{
			noteList.emplace_back(note);
		}
		m_length = lengthOfShortestNote(m_chord);
		m_chord.clear();
	}

	// Write note if any
	if (m_key >= 0)
	{
		noteList.emplace_back(lmms::Note{m_length, m_cursorPos, m_key});
	}

	// Advance the cursor
	if (!m_insideChord)
	{
		m_cursorPos += m_length;
	}

	// Reset note values
	m_mode = Mode::None;
	m_key = -1;
	m_length = 0;
}


} // namespace PLUGIN_NAME
