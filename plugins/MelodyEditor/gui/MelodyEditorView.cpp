/**
 * MelodyEditorView.cpp
 *
 * Copyright (c) 2025 - 2025 Bimal Poudel <anytizer@users.noreply.github.com>
 */

#include "MelodyEditorView.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QTextStream>
#include <QLineEdit>

#include "FileDialog.h"
#include "ComboBox.h"
#include "Song.h"
#include "GuiApplication.h"
#include "PianoRoll.h"
#include "PatternStore.h"

#include "../MelodyEditor.h"
#include "TextArea.h"



namespace lmms::gui
{

using ::PLUGIN_NAME::gui::TextArea;


//! File name extension for drag and drop
const QString MELODY_EXTENSION = "txt";


QString fileContents(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { return {}; }
	return QTextStream(&file).readAll();
}


MelodyEditorView::MelodyEditorView(MelodyEditor* plugin)
	: ToolPluginView(plugin)
	, m_plugin(plugin)
	, m_textArea(new TextArea())
	, m_errorBox(new QLineEdit(this))
{
	setAcceptDrops(true);

	connect(getGUI()->pianoRoll(), &PianoRollWindow::currentMidiClipChanged, this, &MelodyEditorView::setClipFromPianoRoll);
	setClipFromPianoRoll();

	connect(m_textArea, &TextArea::textChanged, this, &MelodyEditorView::writeToClip);

	auto parserComboBox = new ComboBox(this, "Select Notation System");
	parserComboBox->setModel(m_plugin->m_parserModel);
	connect(m_plugin->m_parserModel, &Model::dataChanged, this, &MelodyEditorView::writeToClip);

	m_errorBox->setDisabled(true);

	auto loadButton = new QPushButton("Load", this);
	loadButton->setToolTip("Load notes from Piano Roll");
	connect(loadButton, &QPushButton::clicked, this, &MelodyEditorView::readFromClip);

	auto writeButton = new QPushButton("Write", this);
	writeButton->setToolTip("Write notes to Piano Roll");
	connect(loadButton, &QPushButton::clicked, this, &MelodyEditorView::readFromClip);

	auto mainLayout = new QVBoxLayout(this);
	auto buttonLayout = new QHBoxLayout(this);
	mainLayout->addWidget(m_textArea);
	mainLayout->addWidget(m_errorBox);
	buttonLayout->addWidget(parserComboBox);
	buttonLayout->addWidget(loadButton);
	buttonLayout->addWidget(writeButton);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	if (QWidget* pw = parentWidget())
	{
		//pw->hide(); // default hidden

		Qt::WindowFlags flags = pw->windowFlags();
		flags &= ~Qt::WindowMaximizeButtonHint;
		flags |= Qt::WindowStaysOnTopHint;
		pw->setWindowFlags(flags);

		pw->adjustSize();
	}
}




QString pathFromMimeData(const QMimeData* mimeData)
{
	for (const QUrl& url : mimeData->urls())
	{
		if (!url.isLocalFile()) { continue; }
		QString path = url.toLocalFile();
		if (QFileInfo(path).suffix().toLower() == MELODY_EXTENSION)
		{
			return path;
		}
	}
	return "";
}




void MelodyEditorView::dragEnterEvent(QDragEnterEvent *event)
{
	if (!pathFromMimeData(event->mimeData()).isEmpty())
	{
		event->acceptProposedAction();
	}
}




void MelodyEditorView::dropEvent(QDropEvent *event)
{
	auto path = pathFromMimeData(event->mimeData());
	if (!path.isEmpty())
	{
		m_textArea->loadFile(path);
		event->acceptProposedAction();
	}
}




//! Hacky way to get a writable MidiClip pointer from PianoRollWindow
void MelodyEditorView::setClipFromPianoRoll()
{
	const MidiClip* currentMidiClip = getGUI()->pianoRoll()->currentMidiClip();

	const auto trackContainers = std::initializer_list<TrackContainer*>{
		Engine::getSong(),
		Engine::patternStore(),
	};

	for (auto* trackContainer: trackContainers)
	{
		for (auto* track: trackContainer->tracks())
		{
			if (track->type() != Track::Type::Instrument) { continue; }
			for (auto* clip: track->getClips())
			{
				if (static_cast<const MidiClip*>(clip) == currentMidiClip)
				{
					m_plugin->setMidiClip(static_cast<MidiClip*>(clip));
					return;
				}
			}
		}
	}
	m_plugin->setMidiClip(nullptr);
	m_errorBox->clear();
}




void MelodyEditorView::readFromClip()
{
	QString error;
	m_plugin->readFromClip(error);
	m_errorBox->setText(error);
}




void MelodyEditorView::writeToClip()
{
	QString error;
	m_plugin->writeToClip(m_textArea->toPlainText(), error);
	m_errorBox->setText(error);
}




void MelodyEditorView::openFileSelector()
{
	QString dir = m_file.isEmpty() ? "" : QFileInfo(m_file).dir().path();
	FileDialog ofd(this, "Open melody notations", dir, "Melodies (*.txt)");
	ofd.setFileMode(FileDialog::ExistingFiles);
	if (ofd.exec() == QDialog::Accepted && !ofd.selectedFiles().isEmpty())
	{
		m_file = ofd.selectedFiles()[0];
		m_textArea->loadFile(m_file);
	}
}


} // namespace lmms::gui
