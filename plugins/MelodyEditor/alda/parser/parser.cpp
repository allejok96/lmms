#include "parser.h"

#include <vector>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QSet>

namespace PLUGIN_NAME
{


void AldaParser::error(QString msg)
{
	m_error += QString("%1:%2 %3; ").arg(m_line).arg(m_column).arg(msg);
}


void AldaParser::unexpectedCharError(QString context)
{
	QString error("Unexpected ");
	switch (currentChar().toLatin1())
	{
	case '\0':
		error += "end of file";
		break;
	case '\t':
		error += "tab";
		break;
	case '\n':
		error += "carriage return";
		break;
	default:
		if (currentChar().category() == QChar::Other_Control)
		{
			error += QString("control character (%1)").arg(currentChar().toLatin1());
		}
		else
		{
			error += currentChar();
		}
	}

	if (!context.isEmpty())
	{
		error += " " + context;
	}

	m_error += error + "; ";
}


bool AldaParser::reachedEOF()
{
	return m_pos >= m_string.length();
}


QChar AldaParser::currentChar()
{
	if (reachedEOF()) {
		return '\0';
	}

	return m_string.at(m_pos);
}


QChar AldaParser::readChar()
{
	auto c = currentChar();
	m_pos++;

	if (c == '\n') {
		m_line++;
		m_column = 1;
	} else {
		m_column++;
	}

	return c;
}


bool AldaParser::consumeChar(char c)
{
	if (currentChar() == c)
	{
		readChar();
		return true;
	}
	return false;
}


QString AldaParser::readWord(QString errorContext)
{
	QString string;
	while (m_string.at(m_pos).isLetterOrNumber())
	{
		string += readChar();
	}
	if (string.isNull())
	{
		unexpectedCharError(errorContext);
	}
	return string;
}


int AldaParser::readInt(QString errorContext)
{
	QString digits;
	for (QChar c = currentChar(); c.isDigit(); readChar())
	{
		digits += c;
	}
	if (digits.isNull())
	{
		unexpectedCharError(errorContext);
		return 0;
	}
	return digits.toInt();
}


void AldaParser::skipWhitespace()
{
	while (!reachedEOF())
	{
		switch (currentChar().toLatin1()) {
		case ' ':
		case '\r':
		case '\n':
		case '\t':
		case '|':
			readChar();
			break;
		case '#':
			// Skip comments
			while (currentChar() != '\n' && !reachedEOF()) { readChar(); }
			break;
		default:
			return;
		}
	}
}


int AldaParser::readDuration(QString errorContext)
{
	char c = currentChar().toLatin1();
	if (c < '1' || '9' < c)
	{
		unexpectedCharError(errorContext);
		return 0;
	}

	int denominator = readInt(errorContext);
	int duration = lmms::DefaultTicksPerBar / denominator;

	// TODO warning instead of error?
	if (lmms::DefaultTicksPerBar % denominator) { error(QString("note length not supported")); }

	while (consumeChar('.'))
	{
		if (duration % 2) { error(QString("note length not supported")); }
		duration += duration/2;
	}

	if (consumeChar('~')) { duration += readDuration("following ~"); }

	return duration;
}


void AldaParser::readNote()
{
	int key = -1;
	char noteLetter = readChar().toLatin1();

	if ('a' <= noteLetter && noteLetter <= 'g')
	{
		int relativeKey = static_cast<int>(noteLetter - 'c');
		if (relativeKey < 0) { relativeKey += lmms::KeysPerOctave; }
		key = m_currentVoice->octave * lmms::KeysPerOctave + relativeKey;
	}
	else if (noteLetter != 'r')
	{
		unexpectedCharError();
	}

	if (currentChar().isDigit())
	{
		m_currentVoice->lastNoteDuration = readDuration("following note or rest");
	}

	if (key >= 0)
	{
		// Create Note
		m_notes.emplace_back(m_currentVoice->lastNoteDuration, m_currentVoice->timePos, key);
	}

	m_currentVoice->timePos += m_currentVoice->lastNoteDuration;
}


void AldaParser::readChord()
{
	TimePos startOfChord = m_currentVoice->timePos;

	while (true)
	{
		readNote();
		skipWhitespace();
		if (!consumeChar('/'))	{ break; }
		m_currentVoice->timePos = startOfChord;
		skipWhitespace();
	}
}


void AldaParser::readSequence(char end)
{
	int startPos = m_pos;
	int repetitions = 1;

	for (int rep = 0; rep < repetitions; rep++)
	{
		// Move the cursor back to the beginning for every repetition
		m_pos = startPos;

		skipWhitespace();
		while (!consumeChar(end))
		{
			readEvent();
			skipWhitespace();
		}

		// On the first iteration we read the * marker at the end
		// to see if we should repeat more
		if (rep == 0 && consumeChar('*'))
		{
			skipWhitespace();
			repetitions = readInt("following *");
			if (repetitions == 0) { error("cannot repeat zero times"); }
		}
	}
}


void AldaParser::readCram()
{
	TimePos startPos = m_currentVoice->timePos;
	int defaultDuration = m_currentVoice->lastNoteDuration;

	// Disable voice change or jumping position
	m_readingCram = true;

	// Read notes into a new vector
	std::vector<lmms::Note> cramNotes;
	std::swap(cramNotes, m_notes);

	// Read the cram expression
	readSequence('}');

	// Restore the regular note vector
	std::swap(cramNotes, m_notes);
	m_readingCram = false;

	// Get wanted cram duration, and move the time cursor there
	int wantedDuration = currentChar().isDigit()
		? readDuration("after cram expression")
		: defaultDuration;

	m_currentVoice->lastNoteDuration = wantedDuration;
	m_currentVoice->timePos = startPos + wantedDuration;

	// Compare the wanted duration with the actual duration
	float scaleFactor = wantedDuration / (m_currentVoice->timePos - startPos);

	// Resize all notes and move them into the regular note vector
	for (auto note : cramNotes)
	{
		note.setLength(note.length() * scaleFactor);
		note.setPos((note.pos() - startPos) * scaleFactor + startPos);
		m_notes.push_back(std::move(note));
	}
}


void AldaParser::readVoiceMarker()
{
	if (m_readingCram) { error("cannot change voice inside {}"); return; }

	int number = readInt("as voice number");
	if (!m_voices.contains(number))
	{
		m_voices[number] = Voice{m_voices[0]}; // copy values from V0
	}

	// Moving to 0 from another voice moves the cursor to the last voice to finish
	if (number == 0 && m_currentVoice != &m_voices[0])
	{
		// Get the voice with the largest timePos
		// If they are the same, the one with the largest voice number
		auto lastVoiceToFinish = std::max_element(m_voices.begin(), m_voices.end(),
			[](auto& v1, auto& v2) { return v1.second.timePos < v2.second.timePos && v1.first > v2.first; }
		);

		m_voices[0] = lastVoiceToFinish->second;
	}

	m_currentVoice = &m_voices[number];

	if (!consumeChar(':')) { error("Expected `:` after voice change"); }
}

void AldaParser::readEvent()
{
	switch (readChar().toLatin1())
	{
	case '[': readSequence(']'); break;
	case '{': readCram(); break;
	case '@': moveToMarker(readWord("after @ marker")); break;
	case '%': saveMarker(readWord("after % marker")); break;
	case 'V': readVoiceMarker(); break;
	case '>': m_currentVoice->octave += 1; break;
	case '<': m_currentVoice->octave += 1; break;
	case 'o': m_currentVoice->octave = readInt("after octave marker"); break;
	default:
		m_pos--; // let the chord read that char again
		readChord();
	}
}


void AldaParser::saveMarker(QString name)
{
	m_markers[name] = m_currentVoice->timePos;
}


void AldaParser::moveToMarker(QString name)
{
	if (m_readingCram) { error("cannot use a @marker inside {}"); return; }

	if (!m_markers.contains(name)) { error(QString("%1: marker not found").arg(name)); return; }

	m_currentVoice->timePos = m_markers[name];
}


NoteList AldaParser::parse(const QString& text, QString& error)
{
	m_string = text;
	m_pos = 0;
	m_line = 1;
	m_column = 1;

	m_notes.clear();
	m_voices.clear();
	m_currentVoice = &m_voices[0];
	m_markers.clear();
	m_readingCram = false;

	m_error.clear();

	skipWhitespace();

	while (!reachedEOF())
	{
		readEvent();
		skipWhitespace();
	}

	error = m_error;
	return m_notes;
}

}
