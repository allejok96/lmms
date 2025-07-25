"""
<!DOCTYPE TS>
<TS>
<context>
    <name>Additional information for the translator</name>
    <message>
        <source>Email address</source>
        <translation>Email address</translation>
    </message>
    <message numerus="yes">
        <location filename="../transifex/src/core.py" line="512"/>
        <source>%s minute</source>
        <translation>
        <numerusform>%s minute</numerusform><numerusform>%s minutes</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>Additional information for the translator: This appears on the login page</name>
    <message>
        <source>Please provide your username</source>
        <comment>field for username</comment>
        <extracomment>Please translate as close to the original as possible</extracomment>
        <translation>Please provide your username</translation>
    </message>
  </context>
</TS>
"""
import os
from dataclasses import dataclass
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import NamedTuple, ItemsView
from collections import defaultdict
from xml.dom.minidom import Element

Language = str
ClassName = str
EnglishString = str

StringID = tuple[ClassName, EnglishString]


@dataclass
class TranslationData:
    language: Language
    class_name: ClassName
    english: EnglishString
    translation_node: ET.Element
    finished: bool

    @property
    def id(self) -> StringID:
        return self.class_name, self.english

    def update(self, other: 'TranslationData') -> None:
        """Copy translation from other to this"""
        self.translation_node.text = other.translation_node.text
        try:
            del self.translation_node.attrib['type']
        except KeyError:
            pass
        self.finished = other.finished




class DonorReceiver(NamedTuple):
    donor: ClassName
    receiver: ClassName


def get_tr_data(file: Path) -> list[TranslationData]:
    """Get all source strings and translations from a TS file"""

    strings: list[TranslationData] = []

    doc = ET.parse(str(file))
    root = doc.getroot()

    language = root.attrib.get('language', 'en')

    for context in root.findall('context'):
        name_node = context.find('name')
        assert name_node is not None, "Missing <name>"
        name = name_node.text

        for message_node in context.findall('message'):
            source_node = message_node.find('source')
            assert source_node is not None, "Missing <source>"
            source = source_node.text

            comment_node = message_node.find('comment')
            if comment_node is not None:
                source += f' ({comment_node.text})'

            translation_node = message_node.find('translation')
            if translation_node is None:
                translation_node = ET.SubElement(message_node, 'translation')
                translation_node.attrib['type'] = "unfinished"

            # TODO support numerus forms
            if translation_node.find("numerusform") is not None:
                continue

            finished = translation_node.get("type", "") != "unfinished"

            strings.append(
                TranslationData(
                    language=language,
                    class_name=name,
                    english=source,
                    translation_node=translation_node,
                    finished=finished,
                )
            )
    return strings


def count_unfinished(string_pairs: StringPairList) -> int:
    return sum(not string_pair.receiver.finished for string_pair in string_pairs)


def string_pair_filter(string_pair: TranslationPair) -> bool:
    return not string_pair.receiver.finished


def main():
    # - Pull down latest translations from transifex
    # - Run lupdate to update the translations (keep obsolete)
    # - Find missing translations and existing translations
    # - For every string that's missing a translation, list all possible translations
    # - Check which new strings are missing translations
    #

    translations: dict[Language, dict[EnglishString, list[TranslationData]]] = defaultdict(lambda: defaultdict(list))
    donations: dict[tuple[ClassName, ClassName], set[EnglishString]] = defaultdict(set)

    for file in Path().glob('data/locale/*.ts'):
        # fetch_latest_translation(file)
        # run_lupdate(file)

        for tr in get_tr_data(file):
            translations[tr.language][tr.english].append(tr)

    for lang in translations:
        for string in translations[lang]:
            for receiver in translations[lang][string]:
                if not receiver.finished:
                    for donor in translations[lang][string]:
                        if donor.finished:
                            donations[(donor.class_name, receiver.class_name)].add(string)

    while donations:
        donation = max(donations, key=lambda x: len(donations[x]))
        for lang in translations:
            for string in donations[donation]:
                if string in translations[lang]:
                    for



        string_pairs = context_list.pop()
        filtered_pairs: list[TranslationPair] = list(filter(string_pair_filter, string_pairs))

        if not filtered_pairs:
            continue

        first_pair = filtered_pairs[0]
        old_context = first_pair.donor.class_name
        new_context = first_pair.receiver.class_name

        print()
        for old_string, new_string in filtered_pairs:
            print(f'{old_string.english} -> {old_string.translation}')
        print()

        reply = input(f'{old_context} -> {new_context}: [y/N] ')

        if reply in 'Yy':
            for old_string, new_string in filtered_pairs:
                new_string.update(old_string)

    new_file = f'{lang}_output.ts'
    new_file_tmp = f'{lang}_output.ts.tmp'
    new_document.write(new_file, encoding="unicode")

    with open(new_file, 'r', encoding='utf-8') as infile, \
            open(new_file_tmp, 'w', encoding='utf-8') as outfile:
        outfile.write('<?xml version="1.0" ?><!DOCTYPE TS>')
        for line in infile:
            # Replace only occurrences in this line
            new_line = line.replace(' />', '/>')
            outfile.write(new_line)

    os.replace(new_file_tmp, new_file)


if __name__ == '__main__':
    main()
