INCLUDE(BuildPlugin)

BUILD_PLUGIN(
  melodyeditor

  MelodyEditor.cpp
  gui/MelodyEditorView.cpp
  gui/TextArea.cpp
  parsers/AnytizerParser.cpp
  alda/parser/parser.cpp


  MOCFILES
    alda/parser/parser.h
    MelodyEditor.h
    gui/MelodyEditorView.h
    gui/TextArea.h
    parsers/AnytizerParser.h
 
  EMBEDDED_RESOURCES
    assets/*.svg
)
