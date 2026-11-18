#include "i18n.h"

#include <cstring>
#include <string_view>
#include <unordered_map>

namespace rdoom {

namespace {

struct Entry { const char *en, *fr, *pt; };

const Entry kStrings[] = {
#include "i18n_strings.inc"
};

Lang g_lang = Lang::En;

const std::unordered_map<std::string_view, const Entry *> &Table()
{
	static const std::unordered_map<std::string_view, const Entry *> table = [] {
		std::unordered_map<std::string_view, const Entry *> t;
		for (const Entry &e : kStrings)
			t.emplace(e.en, &e);
		return t;
	}();
	return table;
}

} // namespace

void SetLanguage(Lang lang) { g_lang = lang; }
Lang GetLanguage() { return g_lang; }

const char *LangCode(Lang lang)
{
	switch (lang)
	{
	case Lang::Fr: return "fr";
	case Lang::Pt: return "pt";
	default: return "en";
	}
}

Lang LangFromCode(const std::string &code)
{
	if (code == "fr") return Lang::Fr;
	if (code == "pt") return Lang::Pt;
	return Lang::En;
}

const char *LangName(Lang lang)
{
	switch (lang)
	{
	case Lang::Fr: return "Français";
	case Lang::Pt: return "Português";
	default: return "English";
	}
}

const char *EngineLanguage(Lang lang)
{
	switch (lang)
	{
	case Lang::Fr: return "fr";
	case Lang::Pt: return "pt_BR";
	default: return "default";
	}
}

const char *L(const char *english)
{
	if (g_lang == Lang::En || !english)
		return english;
	auto it = Table().find(english);
	if (it == Table().end())
		return english;
	return g_lang == Lang::Fr ? it->second->fr : it->second->pt;
}

} // namespace rdoom
