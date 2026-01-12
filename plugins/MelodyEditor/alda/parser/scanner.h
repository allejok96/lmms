#ifndef SCANNER_H
#define SCANNER_H

#include <map>
#include <vector>

#include <QRegularExpression>
#include <QString>

constexpr int DEFAULT_OCTAVE = 4;
constexpr float DEFAULT_DURATION = 1/4;


struct Range
{
	int start;
	int end;
};

using Selectivity = std::vector<Range>;


struct Event
{
	Event() {}
	virtual ~Event() = default;

	// Prevent copy assinment, like event = anotherEvent
	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;

	// Allow move assignment, like event = functionThatReturnEvent()
	Event(Event&&) = default;
	Event& operator=(Event&&) = default;

	virtual bool supportsDuration() { return false; }
	virtual bool supportsSelectivity() { return false; }
	virtual bool supportsSlur() { return false; }
	virtual bool supportsRepeat() { return false; }

	float duration = 0;
	int repeats = 0;
	Selectivity selectivity = {};
	bool slur = false;
};

using EventPtr = std::shared_ptr<Event>;


struct AtMarker : public Event
{
	AtMarker(QString n) : name(n) {}
	QString name;
};


struct Chord : public Event
{
	bool supportsSelectivity() override { return true; }
	bool supportsRepeat() override { return true; }

	std::vector<EventPtr> events = {};
};


struct Cram : public Event
{
	bool supportsDuration() override { return true; }
	bool supportsRepeat() override { return true; }

	std::vector<EventPtr> events = {};
};


struct EventSequence : public Event
{
	bool supportsSelectivity() override { return true; }
	bool supportsRepeat() override { return true; }

	std::vector<EventPtr> events = {};
};


struct LispExpression : public Event
{

};


struct Note : public Event
{
	bool supportsDuration() override { return true; }
	bool supportsRepeat() override { return true; }
	bool supportsSlur() override { return true; }
	bool supportsSelectivity() override { return true; }

	enum class Accent
	{
		Natural,
		Flat,
		Sharp
	};

	Note(int _key) : key(_key) {}

	int key = 0;

	//! When (quant) is used, the note plays for only a fraction of the duration
	//! but when slur is true it plays for the whole duration
	bool slur = false;
};


struct Marker : public Event
{
	Marker(QString n) : name(n) {}
	QString name;
};


struct OctaveChange : public Event
{
	// Yeah [c <'5]*10 is a valid alda expression
	bool supportsSelectivity() override { return true; }

	OctaveChange(int _value, bool _relative = false) : value(_value), relative(_relative) {}
	int value;
	bool relative = false;
};


struct Rest : public Event
{
	bool supportsDuration() override { return true; }
	bool supportsSelectivity() override { return true; }
	bool supportsRepeat() override { return true; }
};


struct VariableReference : public Event
{
	bool supportsRepeat() override { return true; }

	VariableReference(QString n) : name(n) {}
	QString name;
};


struct VoiceChange : public Event
{
	VoiceChange(int v) : voice(v) {}
	int voice;
};


struct Part
{
	QString instrument;
	QString alias;
	std::vector<EventPtr> events = {};
};


struct Score
{
	std::vector<Part> parts;
	std::map<QString, EventPtr> variables;
};





class Parser
{
public:
	static Score parse(QString input, QString* error);

private:
	Parser(QString string) : m_string(string) {}

	// Error handling

	void error(QString msg);
	void unexpectedCharError(QString context);

	// Moving the cursor

	char currentChar();
	bool consumeChar(char c);
	char readChar();

	void skipWhitespaceAndComments();
	bool reachedEOF();

	// Data parsing

	bool matchString(QRegularExpression regex);
	QString readString(QRegularExpression regex);
	int readInt(QString errorContext);

	// Event parsing
	EventPtr readEvent();
	EventPtr readBasicEvent();
	EventPtr readLispExpr();
	EventPtr readVariableReference();
	EventPtr readNote();
	EventPtr readRest();
	EventPtr readChord();
	EventPtr readEventSequence(char endChar);
	EventPtr readCramExpression();

	// Event attribute parsing

	void readEventAttributes(EventPtr event);
	float readDuration(QString errorContext);
	Selectivity readSelectivity();

	// Score parsing

	void readPartDeclaration();
	void readScore();

	// Data members

	QString m_string;
	int m_pos = 0;
	int m_line = 1;
	int m_column = 1;

	Score m_score = {};
	std::vector<Part*> m_currentParts;
	std::map<QString, std::vector<Part*>> m_partNames;

	QString m_error;
};

struct MidiNote
{
	int pos;
	int key;
	float duration;
};

using Voice = std::vector<MidiNote>;

struct Track
{
};

class Composer
{
	std::map<QString, std::vector<>>
};


#endif // SCANNER_H
