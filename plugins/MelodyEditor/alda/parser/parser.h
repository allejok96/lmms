#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <vector>

#include <QRegularExpression>
#include <QString>

#include "Note.h"
#include "../../parsers/ParserBase.h"

constexpr int DEFAULT_OCTAVE = 5; // why?
constexpr int DEFAULT_DURATION = lmms::DefaultTicksPerBar/4;

namespace PLUGIN_NAME
{


using lmms::TimePos;

struct Voice
{
	TimePos timePos;
	int octave = DEFAULT_OCTAVE;
	int lastNoteDuration = DEFAULT_DURATION;
};


class AldaParser : public ParserBase
{
public:
	AldaParser()
		: ParserBase("Alda Parser", "")
	{
	}

	//! Convert text to notes
	NoteList parse(const QString& text, QString& error) override;

private:
	void error(QString msg);
	void unexpectedCharError(QString context = "");

	bool reachedEOF();
	QChar currentChar();
	QChar readChar();

	bool consumeChar(char c);
	void skipWhitespace();

	QString readWord(QString errorContext);
	int readInt(QString errorContext);
	int readDuration(QString errorContext);

	// Event parsing
	void readEvent();
	void readNote();
	void readChord();
	void saveMarker(QString name);
	void moveToMarker(QString name);
	void readVoiceMarker();
	void readSequence(char end);
	void readCram();

   // Data members

	QString m_string;
	int m_pos = 0;
	int m_line = 1;
	int m_column = 1;

	NoteList m_notes;
	std::map<int, Voice> m_voices;
	Voice* m_currentVoice;
	std::map<QString, TimePos> m_markers;
	bool m_readingCram = false;

	QString m_error;
};

}

#endif // PARSER_H
