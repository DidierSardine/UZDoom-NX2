// UZDoom NX2 launcher: user interface language (English, French, Portuguese).
//
// Every user-visible string in the launcher is written in English in the
// source and goes through L(); i18n_strings.inc holds the translations.
// A string without a translation is shown in English.
#pragma once
#include <string>

namespace rdoom {

enum class Lang { En, Fr, Pt };

void SetLanguage(Lang lang);
Lang GetLanguage();
// "en", "fr", "pt" (launcher.json) <-> Lang; unknown codes give English
const char *LangCode(Lang lang);
Lang LangFromCode(const std::string &code);
// Name of a language in that language ("English", "Français", "Português")
const char *LangName(Lang lang);
// The value of UZDoom's "language" CVAR for a launcher language
const char *EngineLanguage(Lang lang);

// Translation of an English string into the current language.
const char *L(const char *english);

} // namespace rdoom
