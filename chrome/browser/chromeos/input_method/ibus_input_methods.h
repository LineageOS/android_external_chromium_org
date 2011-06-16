// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file is automatically generated by gen_engines.py in libcros.
// TODO(satorux): Move the generation process to Chrome. crosbug.com/16630.

#ifndef CHROME_BROWSER_CHROMEOS_INPUT_METHOD_IBUS_INPUT_METHODS_H_
#define CHROME_BROWSER_CHROMEOS_INPUT_METHOD_IBUS_INPUT_METHODS_H_
#pragma once

namespace chromeos {
namespace input_method {

struct IBusEngineInfo {
  const char* id;
  const char* longname;
  const char* layout;
  const char* language;
};
const IBusEngineInfo kIBusEngines[] = {
{"xkb:nl::nld", "Netherlands", "nl", "nld"},
{"xkb:be::nld", "Belgium", "be", "nld"},
{"xkb:fr::fra", "France", "fr", "fra"},
{"xkb:be::fra", "Belgium", "be", "fra"},
{"xkb:ca::fra", "Canada", "ca", "fra"},
{"xkb:ch:fr:fra", "Switzerland - French", "ch(fr)", "fra"},
{"xkb:de::ger", "Germany", "de", "ger"},
{"xkb:de:neo:ger", "Germany - Neo 2", "de(neo)", "ger"},
{"xkb:be::ger", "Belgium", "be", "ger"},
{"xkb:ch::ger", "Switzerland", "ch", "ger"},
{"mozc", "Mozc (US keyboard layout)", "us", "ja"},
{"mozc-jp", "Mozc (Japanese keyboard layout)", "jp", "ja"},
{"mozc-dv", "Mozc (US Dvorak keyboard layout)", "us(dvorak)", "ja"},
{"xkb:jp::jpn", "Japan", "jp", "jpn"},
{"xkb:ru::rus", "Russia", "ru", "rus"},
{"xkb:ru:phonetic:rus", "Russia - Phonetic", "ru(phonetic)", "rus"},
{"m17n:th:kesmanee", "kesmanee (m17n)", "us", "th"},
{"m17n:th:pattachote", "pattachote (m17n)", "us", "th"},
{"m17n:th:tis820", "tis820 (m17n)", "us", "th"},
{"pinyin", "Pinyin", "us", "zh-CN"},
{"pinyin-dv", "Pinyin (for US Dvorak keyboard)", "us(dvorak)", "zh-CN"},
{"mozc-chewing", "Mozc Chewing (Chewing)", "us", "zh_TW"},
{"m17n:zh:cangjie", "cangjie (m17n)", "us", "zh"},
{"m17n:zh:quick", "quick (m17n)", "us", "zh"},
{"m17n:vi:tcvn", "tcvn (m17n)", "us", "vi"},
{"m17n:vi:telex", "telex (m17n)", "us", "vi"},
{"m17n:vi:viqr", "viqr (m17n)", "us", "vi"},
{"m17n:vi:vni", "vni (m17n)", "us", "vi"},
{"xkb:us::eng", "USA", "us", "eng"},
{"xkb:us:intl:eng", "USA - International (with dead keys)", "us(intl)", "eng"},
{"xkb:us:altgr-intl:eng", "USA - International (AltGr dead keys)", "us(altgr-intl)", "eng"},
{"xkb:us:dvorak:eng", "USA - Dvorak", "us(dvorak)", "eng"},
{"xkb:us:colemak:eng", "USA - Colemak", "us(colemak)", "eng"},
{"hangul", "Korean", "kr(kr104)", "ko"},
{"m17n:ar:kbd", "kbd (m17n)", "us", "ar"},
{"m17n:hi:itrans", "itrans (m17n)", "us", "hi"},
{"m17n:fa:isiri", "isiri (m17n)", "us", "fa"},
{"xkb:br::por", "Brazil", "br", "por"},
{"xkb:bg::bul", "Bulgaria", "bg", "bul"},
{"xkb:bg:phonetic:bul", "Bulgaria - Traditional phonetic", "bg(phonetic)", "bul"},
{"xkb:ca:eng:eng", "Canada - English", "ca(eng)", "eng"},
{"xkb:cz::cze", "Czechia", "cz", "cze"},
{"xkb:ee::est", "Estonia", "ee", "est"},
{"xkb:es::spa", "Spain", "es", "spa"},
{"xkb:es:cat:cat", "Spain - Catalan variant with middle-dot L", "es(cat)", "cat"},
{"xkb:dk::dan", "Denmark", "dk", "dan"},
{"xkb:gr::gre", "Greece", "gr", "gre"},
{"xkb:il::heb", "Israel", "il", "heb"},
{"xkb:kr:kr104:kor", "Korea, Republic of - 101/104 key Compatible", "kr(kr104)", "kor"},
{"xkb:latam::spa", "Latin American", "latam", "spa"},
{"xkb:lt::lit", "Lithuania", "lt", "lit"},
{"xkb:lv:apostrophe:lav", "Latvia - Apostrophe (') variant", "lv(apostrophe)", "lav"},
{"xkb:hr::scr", "Croatia", "hr", "scr"},
{"xkb:gb:extd:eng", "United Kingdom - Extended - Winkeys", "gb(extd)", "eng"},
{"xkb:gb:dvorak:eng", "United Kingdom - Dvorak", "gb(dvorak)", "eng"},
{"xkb:fi::fin", "Finland", "fi", "fin"},
{"xkb:hu::hun", "Hungary", "hu", "hun"},
{"xkb:it::ita", "Italy", "it", "ita"},
{"xkb:no::nob", "Norway", "no", "nob"},
{"xkb:pl::pol", "Poland", "pl", "pol"},
{"xkb:pt::por", "Portugal", "pt", "por"},
{"xkb:ro::rum", "Romania", "ro", "rum"},
{"xkb:se::swe", "Sweden", "se", "swe"},
{"xkb:sk::slo", "Slovakia", "sk", "slo"},
{"xkb:si::slv", "Slovenia", "si", "slv"},
{"xkb:rs::srp", "Serbia", "rs", "srp"},
{"xkb:tr::tur", "Turkey", "tr", "tur"},
{"xkb:ua::ukr", "Ukraine", "ua", "ukr"},

};

}  // namespace input_method
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_INPUT_METHOD_IBUS_INPUT_METHODS_H_
