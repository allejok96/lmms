#include "scanner.h"

#include <vector>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QSet>


// Digits with optional decimals
static QRegularExpression FLOAT("\\d+(.\\d+)*");

// Letters and underscore
static QRegularExpression WORD("\\w+");

// Two letters followed by letters, digits and underscore
static QRegularExpression VARIABLE_NAME("[a-zA-Z]{2}[\\w\\d]*");
static QRegularExpression PADDED_EQUAL_SIGN("[ \t]*=[ \t]*");
static QRegularExpression VARIABLE_DECLARATION(VARIABLE_NAME.pattern() + PADDED_EQUAL_SIGN.pattern());

// Same as variable name but may include hyphen
static QRegularExpression PART_NAME("[a-zA-Z]{2}[\\w\\d-]*");

// Part name + whitespace + / (start of next name) or " (start of alias) or : (end of definition)
static QRegularExpression PART_DECLARATION(PART_NAME.pattern() + "\\s*[/\":]");



bool isDigit(char c)
{
	return '0' <= c && c <= '9';
}


bool isNoteLetter(char c)
{
	return 'a' <= c && c <= 'g';
}


void AldaParser::error(QString msg)
{
	m_error = QString("%1:%2 %3").arg(m_line).arg(m_column).arg(msg);
}


void AldaParser::unexpectedCharError(QString context)
{
	QString error("Unexpected ");
	char c = currentChar();
	switch (c)
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
		if (QChar(c).category() == QChar::Other_Control)
		{
			error += QString("control character (%1)").arg(c);
		}
		else
		{
			error += QString(c);
		}
	}

	if (!context.isEmpty())
	{
		error += " " + context;
	}

	m_error = error;
}


bool AldaParser::reachedEOF()
{
	return m_pos >= m_string.length();
}


char AldaParser::currentChar()
{
	if (reachedEOF()) {
		return 0;
	}

	return m_string[m_pos].toLatin1();
}


char AldaParser::readChar()
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


bool AldaParser::readCharIf(char c)
{
	if (currentChar() == c)
	{
		readChar();
		return true;
	}
	return false;
}


bool AldaParser::matchString(QRegularExpression regex)
{
	return regex.match(
		m_string,
		m_pos, // begin from here
		QRegularExpression::NormalMatch,
		QRegularExpression::AnchoredMatchOption
	).hasMatch();
}


QString AldaParser::readString(QRegularExpression regex)
{
	auto match = regex.match(
		m_string,
		m_pos, // begin from here
		QRegularExpression::NormalMatch,
		QRegularExpression::AnchoredMatchOption
	);

	if (!match.hasMatch()) { return ""; }

	int matchStart = m_pos;
	int matchLength = match.capturedLength();

	// Move the cursor to the end of the matching string
	// We do this step by step to keep track of line and column
	for (int i = 0; i < matchLength; i++) { readChar(); }

	return m_string.mid(matchStart, matchLength);
}


int AldaParser::readInt(QString errorContext)
{
	QString digits;
	for (char c = readChar(); isDigit(c); c = readChar())
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
		switch (currentChar()) {
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


float AldaParser::readDuration(QString errorContext)
{
	bool ok;
	float denominator = readString(FLOAT).toFloat(&ok);

	if (!ok) { unexpectedCharError(errorContext); return 0; }
	if (denominator <= 0.f) { error("duration must be positive"); return 0; }

	float duration = 1/denominator;

	while (readChar() == '.') { duration += duration/2; }

	return duration;
}




Selectivity AldaParser::readSelectivity()
{
	Selectivity repetitions;
	while (true)
	{
		int start = readInt("in repetitions");
		int end = matchAndConsumeChar('-') ? readInt("in repetitions") : start;
		repetitions.push_back({start, end});

		if (matchAndConsumeChar(',')) { continue; }
		break;
	}
	return repetitions;
}


EventPtr AldaParser::readLispExpr()
{
	while (!reachedEOF())
	{
		skipWhitespace();
		if (readChar() == ')') { return {}; }
	}

	unexpectedCharError("in paranthesis expression");
	return {};
}


EventPtr AldaParser::readChord()
{
	return {};
}


EventPtr AldaParser::readEventSequence(char endChar)
{
	auto sequence = std::make_shared<EventSequence>();

	skipWhitespace();
	while (!matchAndConsumeChar(endChar))
	{
		if (auto event = readEvent())
		{
			readEventAttributes(event);
			sequence->events.push_back(event);
		}
		skipWhitespace();
	}
	return sequence;
}


EventPtr AldaParser::readCramExpression()
{
	return {};
}


EventPtr AldaParser::readBasicEvent()
{
	if (auto name = readString(VARIABLE_NAME); !name.isEmpty())
	{
		return std::make_shared<VariableReference>(name);
	}

	if ('a' <= currentChar() && currentChar() <= 'g')
	{
		return readChord();
	}

	switch (readChar())
	{
	case '(': return readLispExpr();
	case '[': return readEventSequence(']');
	case '{': return readCramExpression();
	case '@': return std::make_shared<AtMarker>(readString(WORD));
	case '%': return std::make_shared<Marker>(readString(WORD));
	case '>': return std::make_shared<OctaveChange>(1, true);
	case '<': return std::make_shared<OctaveChange>(-1, true);
	case 'o': return std::make_shared<OctaveChange>(readInt("in octave change"));
	case 'V': return std::make_shared<VoiceChange>(readInt("in voice marker"));
	default:
		unexpectedCharError("in event sequence");
		return nullptr;
	}
}

void AldaParser::readEventAttributes(EventPtr event)
{
	while (event->supportsDuration() && matchAndConsumeChar('~'))
	{
		if (isDigit(currentChar()))
		{
			event->duration += readDuration("following ~");
		}
		else if (event->supportsSlur())
		{
			// Notes can temporarily override (quant) by ending with a tilde, for example:
			// (quant 10) c d e f~g~a
			event->slur = true;
			break;
		}
		else
		{
			unexpectedCharError("following ~");
		}
		skipWhitespace();
	}

	if (event->supportsRepeat() && matchAndConsumeChar('*'))
	{
		readChar();
		skipWhitespace();
		event->repeats = readInt("following *");
		skipWhitespace();
	}

	if (event->supportsSelectivity() && matchAndConsumeChar('\''))
	{
		event->selectivity = readSelectivity();
	}

}

EventPtr AldaParser::readEvent()
{
	auto event = readBasicEvent();
	if (event) { readEventAttributes(event); }
	return event;
}


void AldaParser::readPartDeclaration()
{
	// From the alda source code:
	//
	// It's possible to refer to two existing named groups where the parts covered
	// by each group overlap.
	//
	// For example:
	//
	// piano "foo":
	// trumpet "bar":
	// bassoon "baz":
	// foo/bar "group1":
	// foo/baz "group2":
	// group1/group2 "groups1and2":
	//
	// In this contrived example, `groups1and2` refers to 3 parts, by way of
	// referring to 2 groups of 2 parts that have 1 part in common.
	//
	// In order to ensure that there are no duplicate parts, we use a "set" here
	// to keep track of which parts we've already added.

	QSet<QString> names;
	while (true)
	{
		names.insert(readString(WORD));

		skipWhitespace();
		if (!matchAndConsumeChar('/')) { break; }
		skipWhitespace();
	}

	m_currentParts.clear();

	for (const QString& name : names)
	{
		// Add a new part to the score if we don't recognize the name
		if (!m_partNames.contains(name))
		{
			auto newPart = m_score.parts.emplace_back();
			m_partNames[name].push_back(&newPart);
		}

		// Update the current parts with whatever parts the name points to
		m_currentParts.insert(
			m_currentParts.end(),
			m_partNames.at(name).begin(),
			m_partNames.at(name).begin()
		);
	}

	skipWhitespace();

	if (matchAndConsumeChar('\"'))
	{
		auto alias = readString(PART_NAME);

		// Don't allow an alias to overwrite an existing name
		if (m_partNames.contains(alias)) { return error(alias + " is already defined"); }

		m_partNames[alias] = m_currentParts;

		// Consume end of alias
		if (!matchAndConsumeChar('\"')) { return unexpectedCharError("in alias"); }
	}

	skipWhitespace();

	// Consume end of part declaration
	if (!matchAndConsumeChar(':')) { return unexpectedCharError("at the end of part declaration (should not happen)"); }
}


void AldaParser::readScore()
{
	// Create the implicit part
	auto implicitPart = m_score.parts.emplace_back();
	m_currentParts.push_back(&implicitPart);

	while (!reachedEOF())
	{
		skipWhitespace();

		if (matchString(PART_DECLARATION))
		{
			readPartDeclaration();
		}
		else if (matchString(VARIABLE_DECLARATION))
		{
			auto name = readString(VARIABLE_NAME);
			readString(PADDED_EQUAL_SIGN); // consume the whitespace and equal sign
			m_score.variables[name] = readEventSequence('\n');
		}
		else
		{
			if (auto event = readEvent())
			{
				for (auto part : m_currentParts)
				{
					part->events.push_back(event);
				}
			}
		}
	}
}


Score AldaParser::parse(QString string, QString* error)
{
	auto p = AldaParser(string);
	p.readScore();
	*error = p.m_error;
	return p.m_score;
}

