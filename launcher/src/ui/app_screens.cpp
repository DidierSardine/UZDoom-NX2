// RDoom launcher - editor, libraries, settings and diagnostics screens.
#include "app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "fsutil.h"
#include "log.h"
#include "text.h"

namespace rdoom {

static const uint32_t kText = Rgba(242, 234, 226);
static const uint32_t kMuted = Rgba(176, 156, 146);
static const uint32_t kDim = Rgba(118, 100, 94);
static const uint32_t kAccent = Rgba(255, 106, 28);
static const uint32_t kAmber = Rgba(255, 196, 90);
static const uint32_t kRed = Rgba(232, 48, 36);
static const uint32_t kGreen = Rgba(92, 214, 128);
static const uint32_t kVulkan = Rgba(196, 28, 44);

static std::string Up(const std::string &s)
{
	std::string o = s;
	for (size_t i = 0; i < o.size(); i++)
	{
		char &c = o[i];
		if (c >= 'a' && c <= 'z')
			c = char(c - 'a' + 'A');
		// Latin-1 lower case in UTF-8 (U+00E0..U+00FE except U+00F7)
		else if ((unsigned char)c == 0xC3 && i + 1 < o.size())
		{
			unsigned char n = (unsigned char)o[i + 1];
			if (n >= 0xA0 && n <= 0xBE && n != 0xB7)
				o[i + 1] = (char)(n - 0x20);
			i++;
		}
	}
	return o;
}

static std::string Size(uint64_t b)
{
	char buf[32];
	if (b >= (1ull << 30)) snprintf(buf, sizeof(buf), "%.1f GB", b / 1073741824.0);
	else if (b >= (1ull << 20)) snprintf(buf, sizeof(buf), "%.1f MB", b / 1048576.0);
	else snprintf(buf, sizeof(buf), "%llu KB", (unsigned long long)(b / 1024));
	return buf;
}

static const char *CategoryPt(ModCategory c)
{
	switch (c)
	{
	case ModCategory::MapPack: return "Pack de cartes";
	case ModCategory::Gameplay: return "Gameplay";
	case ModCategory::GameplayAndMaps: return "Gameplay + cartes";
	case ModCategory::Resources: return "Ressources";
	case ModCategory::Dehacked: return "Patch DeHackEd";
	default: return "Inconnu";
	}
}

// Load-order badge: the position among the enabled mods (the order of -file),
// "OFF" for a disabled one.
static void OrderBadge(Canvas &c, int x, int y, int size, int order)
{
	if (order > 0)
	{
		c.Fill(x, y, size, size, kAccent);
		DrawTextCentered(c, Font::Display, size - 12, x + size / 2, y + 5, Rgba(255, 255, 255), std::to_string(order));
	}
	else
	{
		c.Frame(x, y, size, size, kDim, 2);
		DrawTextCentered(c, Font::Bold, 14, x + size / 2, y + size / 2 - 9, kDim, "OFF");
	}
}

static void SelRow(Canvas &c, int x, int y, int w, int h, bool sel, bool moving = false)
{
	if (!sel)
		return;
	c.GradientH(x, y, w, h, WithAlpha(moving ? kAmber : kAccent, 150), WithAlpha(kAccent, 10));
	c.Fill(x, y, 5, h, moving ? Rgba(255, 255, 255) : kAmber);
}

// ---------------------------------------------------------------------------
// Profile editor
// ---------------------------------------------------------------------------

namespace {

enum EditRow
{
	ER_Name, ER_Desc, ER_Iwad, ER_Mods, ER_Renderer, ER_Quality, ER_Scale, ER_Fps, ER_Vsync, ER_Lights, ER_Filter,
	ER_Sens, ER_Dead, ER_Invert, ER_Autoaim, ER_Run, ER_Cross, ER_Skill, ER_Warp, ER_Fast, ER_NoMon, ER_Respawn,
	ER_Reset, ER_Count
};

const int kFpsSteps[] = { 30, 35, 60, 0 };
const int kFilters[] = { 0, 4, 5 };

const char *FilterName(int f)
{
	switch (f)
	{
	case 0: return "Net (sans filtre)";
	case 4: return "Bilinear";
	case 5: return "Trilinear";
	default: return "Personnalisé";
	}
}

} // namespace

void LauncherApp::UpdateEditor(const InputFrame &in)
{
	State &st = states_[Screen::Editor];
	Profile *p = store_.FindMutable(editId_);
	if (!p)
	{
		Pop();
		return;
	}
	MoveList(st.sel, ER_Count, in, 8);
	if (in.down & BtnB)
	{
		Pop();
		return;
	}
	if (in.down & BtnPlus)
	{
		SetCurrent(p->id);
		TryLaunch();
		return;
	}
	const int dir = (in.repeat & BtnRight) ? 1 : (in.repeat & BtnLeft) ? -1 : 0;
	const bool a = (in.down & BtnA) != 0;
	bool changed = false;
	auto cycle = [&](int &v, const int *steps, int n) {
		int idx = 0;
		for (int i = 0; i < n; i++)
			if (steps[i] == v)
				idx = i;
		idx = (idx + (dir ? dir : 1) + n) % n;
		v = steps[idx];
		changed = true;
	};
	auto toggle = [&](bool &b) {
		b = !b;
		changed = true;
	};
	auto step = [&](float &v, float d, float lo, float hi) {
		int s = dir ? dir : 1;
		v = std::round((v + s * d) / d) * d;
		if (v > hi + 1e-4f) v = lo;
		if (v < lo - 1e-4f) v = hi;
		changed = true;
	};
	if (!dir && !a)
		return;
	switch (st.sel)
	{
	case ER_Name:
		if (a)
		{
			std::string name = p->name, err;
			if (Keyboard("Nom du profil", p->name, 48, name) && !name.empty())
			{
				if (!store_.Rename(p->id, name, err))
					ToastMsg(err, true);
			}
		}
		break;
	case ER_Desc:
		if (a)
		{
			std::string d = p->description;
			if (Keyboard("Description", p->description, 200, d))
			{
				p->description = d;
				changed = true;
			}
		}
		break;
	case ER_Iwad:
		if (a)
		{
			pickIwad_ = true;
			Push(Screen::Iwads);
		}
		break;
	case ER_Mods:
		if (a)
		{
			// materialise the template choice so the list can be edited
			if (p->mods.empty() && !p->modPatterns.empty())
			{
				ResolvedProfile r = ResolveProfile(*p, lib_, defs_);
				p->mods = r.filled.mods;
				changed = true;
			}
			states_[Screen::Mods] = State();
			Push(Screen::Mods);
		}
		break;
	case ER_Renderer:
		ToastMsg("UZDoom NX2 utilise uniquement Vulkan (OunceVK).");
		break;
	case ER_Quality: {
		const char *q[] = { "low", "stock", "high" };
		int idx = 1;
		for (int i = 0; i < 3; i++)
			if (p->video.quality == q[i])
				idx = i;
		idx = (idx + (dir ? dir : 1) + 3) % 3;
		p->video.quality = q[idx];
		changed = true;
		break;
	}
	case ER_Scale: step(p->video.internalScale, 0.05f, 0.5f, 1.0f); break;
	case ER_Fps: cycle(p->video.fpsLimit, kFpsSteps, 4); break;
	case ER_Vsync: toggle(p->video.vsync); break;
	case ER_Lights: toggle(p->video.dynamicLights); break;
	case ER_Filter: cycle(p->video.textureFilter, kFilters, 3); break;
	case ER_Sens: step(p->input.lookSensitivity, 0.25f, 0.25f, 3.0f); break;
	case ER_Dead: step(p->input.deadzone, 0.05f, 0.0f, 0.4f); break;
	case ER_Invert: toggle(p->input.invertLook); break;
	case ER_Autoaim:
		p->gameplay.autoaim = p->gameplay.autoaim > 0 ? 0.0f : 35.0f;
		changed = true;
		break;
	case ER_Run: toggle(p->gameplay.alwaysRun); break;
	case ER_Cross:
		p->gameplay.crosshair = p->gameplay.crosshair ? 0 : 1;
		changed = true;
		break;
	case ER_Skill:
		p->gameplay.skill = (p->gameplay.skill + (dir ? dir : 1) + 6) % 6;
		changed = true;
		break;
	case ER_Warp:
		if (a)
		{
			std::string w = p->gameplay.warp;
			if (Keyboard("Carte de départ (ex. : MAP01, E1M1 ; vide = menu)", p->gameplay.warp, 8, w))
			{
				if (IsValidWarp(w))
				{
					p->gameplay.warp = Up(w);
					changed = true;
				}
				else
					ToastMsg("Nom de carte invalide (lettres et chiffres uniquement)", true);
			}
		}
		break;
	case ER_Fast: toggle(p->gameplay.fast); break;
	case ER_NoMon: toggle(p->gameplay.noMonsters); break;
	case ER_Respawn: toggle(p->gameplay.respawn); break;
	case ER_Reset:
		if (a)
		{
			const std::string id = p->id;
			Ask("Valeurs par défaut", "Toutes les options reviennent aux valeurs du modèle. L'IWAD et les mods sont conservés.",
			    { "Annuler", "Restaurer" }, [this, id](int b) {
				std::string err;
				if (b == 1)
				{
					if (store_.RestoreDefaults(id, err)) ToastMsg("Valeurs par défaut restaurées");
					else ToastMsg(err, true);
				}
			});
		}
		break;
	}
	if (changed)
	{
		std::string err;
		if (!store_.Save(*p, err))
			ToastMsg("Erreur à l'enregistrement : " + err, true);
	}
}

void LauncherApp::DrawEditor(Canvas &c)
{
	State &st = states_[Screen::Editor];
	const Profile *p = store_.Find(editId_);
	if (!p)
		return;
	DrawHeader(c, "Modifier le profil", p->name + "  •  les changements sont enregistrés immédiatement");
	ResolvedProfile r = ResolveProfile(*p, lib_, defs_);

	struct Line { const char *section; std::string label, value; uint32_t col; };
	std::vector<Line> rows(ER_Count);
	auto onoff = [](bool b) { return std::string(b ? "Activé" : "Désactivé"); };
	char buf[64];
	rows[ER_Name] = { "PROFIL", "Nom", p->name, kText };
	rows[ER_Desc] = { nullptr, "Description", p->description.empty() ? std::string("—") : p->description, kMuted };
	rows[ER_Iwad] = { nullptr, "IWAD", r.iwad ? r.iwad->info.title + " (" + PathFileName(r.iwad->ref) + ")" : (p->iwad.empty() ? std::string("Choisir…") : p->iwad + " [absent]"), r.iwad ? kText : kRed };
	int en = 0;
	for (auto &m : (p->mods.empty() ? r.filled.mods : p->mods))
		if (m.enabled)
			en++;
	rows[ER_Mods] = { nullptr, "Mods et ordre de chargement", std::to_string(en) + " actif(s)  ›", kText };
	rows[ER_Renderer] = { "VIDÉO", "Rendu", RendererLabel(p->video.renderer), p->video.renderer == Renderer::Vulkan ? kAmber : kText };
	rows[ER_Quality] = { nullptr, "Qualité", p->video.quality == "low" ? "Basse" : p->video.quality == "high" ? "Haute" : "Stock (recommandé)", kText };
	snprintf(buf, sizeof(buf), "%d%% (%d×%d)", (int)std::lround(p->video.internalScale * 100), (int)std::lround(1920 * p->video.internalScale), (int)std::lround(1080 * p->video.internalScale));
	rows[ER_Scale] = { nullptr, "Résolution interne", buf, kText };
	rows[ER_Fps] = { nullptr, "Limite d'IPS", p->video.fpsLimit ? std::to_string(p->video.fpsLimit) + " FPS" : std::string("Sans limite"), kText };
	rows[ER_Vsync] = { nullptr, "VSync", onoff(p->video.vsync), kText };
	rows[ER_Lights] = { nullptr, "Lumières dynamiques", onoff(p->video.dynamicLights), kText };
	rows[ER_Filter] = { nullptr, "Filtrage des textures", FilterName(p->video.textureFilter), kText };
	snprintf(buf, sizeof(buf), "%.2fx", p->input.lookSensitivity);
	rows[ER_Sens] = { "COMMANDES", "Sensibilité de visée", buf, kText };
	snprintf(buf, sizeof(buf), "%d%%", (int)std::lround(p->input.deadzone * 100));
	rows[ER_Dead] = { nullptr, "Zone morte des sticks", buf, kText };
	rows[ER_Invert] = { nullptr, "Inverser la visée verticale", onoff(p->input.invertLook), kText };
	rows[ER_Autoaim] = { "JEU", "Visée automatique", onoff(p->gameplay.autoaim > 0), kText };
	rows[ER_Run] = { nullptr, "Toujours courir", onoff(p->gameplay.alwaysRun), kText };
	rows[ER_Cross] = { nullptr, "Réticule", onoff(p->gameplay.crosshair > 0), kText };
	const char *skills[] = { "Choisir dans le menu", "1 — Très facile", "2 — Facile", "3 — Normal", "4 — Difficile", "5 — Cauchemar" };
	rows[ER_Skill] = { nullptr, "Difficulté de départ", skills[std::max(0, std::min(5, p->gameplay.skill))], kText };
	rows[ER_Warp] = { nullptr, "Carte de départ", p->gameplay.warp.empty() ? std::string("Menu du jeu") : p->gameplay.warp, kText };
	rows[ER_Fast] = { nullptr, "Monstres rapides (-fast)", onoff(p->gameplay.fast), kText };
	rows[ER_NoMon] = { nullptr, "Sans monstres (-nomonsters)", onoff(p->gameplay.noMonsters), kText };
	rows[ER_Respawn] = { nullptr, "Réapparition (-respawn)", onoff(p->gameplay.respawn), kText };
	rows[ER_Reset] = { "MAINTENANCE", "Valeurs par défaut du profil", "›", kAmber };

	const int x = 48, y0 = 136, w = 760, rh = 42, visible = 12;
	KeepVisible(st, ER_Count, visible);
	DrawPanel(c, x - 12, y0 - 12, w + 24, visible * rh + 24);
	for (int i = 0; i < visible && st.scroll + i < ER_Count; i++)
	{
		const int idx = st.scroll + i;
		const int y = y0 + i * rh;
		SelRow(c, x, y, w, rh - 4, idx == st.sel);
		DrawText(c, Font::Body, 20, x + 22, y + 6, idx == st.sel ? Rgba(255, 255, 255) : kMuted, rows[idx].label);
		const bool arrows = idx >= ER_Renderer && idx <= ER_Respawn && idx != ER_Warp;
		std::string v = arrows ? "‹  " + rows[idx].value + "  ›" : rows[idx].value;
		DrawTextRight(c, Font::Bold, 20, x + w - 18, y + 6, rows[idx].col, Ellipsize(Font::Bold, 20, v, 400));
	}
	DrawScrollbar(c, x + w + 4, y0, visible * rh, ER_Count, visible, st.scroll);

	// help panel for the selected row
	const int hx = 846, hw = 386;
	DrawPanel(c, hx, y0 - 12, hw, visible * rh + 24);
	const char *section = nullptr;
	for (int i = st.sel; i >= 0 && !section; i--)
		section = rows[i].section;
	DrawText(c, Font::Display, 30, hx + 24, y0 + 4, kAccent, section ? section : "");
	std::string help;
	switch (st.sel)
	{
	case ER_Iwad: help = "L'IWAD est le jeu de base (DOOM II, Freedoom, WadFusion…). Placez les fichiers dans switch/uzdoom-nx2/iwads/."; break;
	case ER_Mods: help = "Activez, désactivez et ordonnez les mods. L'ordre compte : les fichiers chargés après remplacent les précédents."; break;
	case ER_Renderer: help = "UZDoom NX2 affiche uniquement avec Vulkan (OunceVK, le pilote NVK de Mesa pour Switch 2)."; break;
	case ER_Quality: help = "Stock : sans SSAO, bloom, lens flare ni MSAA. Basse coupe aussi les lumières dynamiques et limite l'échelle à 75 %. Haute active le bloom et les ombres des lumières."; break;
	case ER_Scale: help = "Résolution de rendu avant la mise à l'échelle à l'écran. Des valeurs plus basses soulagent les mods lourds."; break;
	case ER_Fps: help = "30 ou 35 IPS donnent des images plus régulières avec les mods lourds ; 60 IPS pour le contenu léger. Les fréquences d'origine de la console ne sont jamais modifiées."; break;
	case ER_Sens: case ER_Dead: case ER_Invert: help = "Valeurs appliquées aux Joy-Con et à la manette Pro au lancement. Les attributions de boutons faites dans le menu du jeu sont enregistrées dans ce profil."; break;
	case ER_Warp: help = "Démarre directement sur la carte indiquée (ex. : MAP01). Laissez vide pour ouvrir le menu du jeu."; break;
	default: help = "‹ › pour changer une valeur, A pour ouvrir. Chaque profil a sa configuration et ses sauvegardes."; break;
	}
	DrawTextWrapped(c, Font::Body, 19, hx + 24, y0 + 50, hw - 48, kText, help, 12);

	// problems of this profile
	int py = y0 + visible * rh - 120;
	for (auto &is : r.issues)
	{
		if (is.level == IssueLevel::Info || py > y0 + visible * rh - 20)
			continue;
		c.FillCircle(hx + 30, py + 11, 5, is.level == IssueLevel::Error ? kRed : kAmber);
		py += DrawTextWrapped(c, Font::Body, 16, hx + 44, py, hw - 64, is.level == IssueLevel::Error ? Rgba(255, 150, 140) : kAmber, is.text, 2);
	}
	DrawHints(c, { { "A", "Ouvrir/Changer" }, { "‹ ›", "Valeur" }, { "+", "Lancer" }, { "B", "Retour" } });
}

// ---------------------------------------------------------------------------
// IWAD library
// ---------------------------------------------------------------------------

void LauncherApp::UpdateIwads(const InputFrame &in)
{
	State &st = states_[Screen::Iwads];
	const auto &items = lib_.Iwads();
	MoveList(st.sel, (int)items.size(), in);
	if (in.down & BtnB)
	{
		pickIwad_ = false;
		Pop();
		return;
	}
	if (in.down & BtnY)
	{
		Rescan();
		ToastMsg("Bibliothèque actualisée : " + std::to_string(lib_.Iwads().size()) + " IWAD(s)");
		return;
	}
	if (items.empty() || !(in.down & BtnA))
		return;
	const LibraryItem &it = items[st.sel];
	if (it.info.status != FileStatus::Ok || !it.info.isIwad)
	{
		Message("Utilisation impossible", it.info.status != FileStatus::Ok
			? it.info.fileName + " : fichier invalide (" + it.info.error + ")."
			: it.info.fileName + " n'est pas une IWAD. Déplacez-le dans mods/ si c'est un mod.");
		return;
	}
	const std::string target = pickIwad_ ? editId_ : currentId_;
	Profile *p = store_.FindMutable(target);
	if (!p)
		return;
	p->iwad = it.ref;
	std::string err;
	if (store_.Save(*p, err))
		ToastMsg("IWAD de \"" + p->name + "\": " + it.info.title);
	else
		ToastMsg(err, true);
	if (pickIwad_)
	{
		pickIwad_ = false;
		Pop();
	}
}

void LauncherApp::DrawIwads(Canvas &c)
{
	State &st = states_[Screen::Iwads];
	const auto &items = lib_.Iwads();
	DrawHeader(c, "Bibliothèque d'IWAD", pickIwad_ ? "Choisissez l'IWAD du profil" : std::to_string(items.size()) + " fichier(s) dans " + paths_.Iwads());
	const int x = 48, y0 = 136, w = 700, rh = 82, visible = 6;
	KeepVisible(st, (int)items.size(), visible);
	DrawPanel(c, x - 12, y0 - 12, w + 24, visible * rh + 24);
	if (items.empty())
		DrawTextWrapped(c, Font::Body, 21, x + 16, y0 + 20, w - 40, kText,
		                "Aucune IWAD trouvée. Copiez par exemple freedoom2.wad ou doom2.wad dans switch/uzdoom-nx2/iwads/ sur la carte SD, puis appuyez sur Y.");
	for (int i = 0; i < visible && st.scroll + i < (int)items.size(); i++)
	{
		const int idx = st.scroll + i;
		const LibraryItem &it = items[idx];
		const int y = y0 + i * rh;
		SelRow(c, x, y, w, rh - 6, idx == st.sel);
		c.Fill(x + 16, y + 6, 104, 65, Rgba(0, 0, 0, 160));
		if (const Image *t = thumbs_.Get(it, 104, 65))
			c.Blit(*t, x + 16, y + 6);
		else
			DrawTextCentered(c, Font::Display, 22, x + 68, y + 24, kDim, "IWAD");
		DrawText(c, Font::Display, 30, x + 136, y + 4, kText, Up(it.info.title), w - 340);
		DrawText(c, Font::Body, 16, x + 136, y + 44, kMuted, it.ref + "  •  " + Size(it.info.size), w - 340);
		if (it.info.status != FileStatus::Ok)
			DrawChip(c, x + w - 170, y + 22, "INVALIDE", Rgba(130, 20, 16), kText);
		else if (!it.info.isIwad)
			DrawChip(c, x + w - 170, y + 22, "PAS UNE IWAD", Rgba(120, 80, 10), kText);
		else
			DrawChip(c, x + w - 170, y + 22, "DISPONIBLE", Rgba(24, 96, 52), kText);
	}
	DrawScrollbar(c, x + w + 4, y0, visible * rh, (int)items.size(), visible, st.scroll);

	// details of the selection
	const int dx = 786, dw = 446;
	DrawPanel(c, dx, y0 - 12, dw, visible * rh + 24);
	if (!items.empty())
	{
		const LibraryItem &it = items[st.sel];
		c.Fill(dx + 22, y0 + 6, 400, 250, Rgba(0, 0, 0, 180));
		if (const Image *t = thumbs_.Get(it, 400, 250))
			c.Blit(*t, dx + 22, y0 + 6);
		else
			DrawTextCentered(c, Font::Body, 18, dx + 222, y0 + 118, kDim, it.info.hasTitlePic ? "création de la miniature…" : "pas de TITLEPIC");
		int y = y0 + 270;
		auto kv = [&](const std::string &k, const std::string &v, uint32_t col = kText) {
			DrawText(c, Font::Body, 15, dx + 24, y + 2, kDim, Up(k));
			DrawText(c, Font::Body, 18, dx + 140, y, col, v, dw - 160);
			y += 28;
		};
		std::string type = it.info.archive == ArchiveType::Wad ? "WAD" : it.info.archive == ArchiveType::Zip ? "PK3/IPK3 (zip)" : it.info.archive == ArchiveType::SevenZip ? "PK7 (7z)" : "—";
		kv("Jeu", it.info.iwadDef >= 0 ? defs_[it.info.iwadDef].name : (it.info.isIwad ? it.info.title : "—"));
		kv("Type", type + (it.info.mapCount ? "  •  " + std::to_string(it.info.mapCount) + " cartes" : ""));
		kv("État", it.info.status != FileStatus::Ok ? "Invalide : " + it.info.error : it.info.isIwad ? "Valide" : "PWAD/mod, pas une IWAD",
		   it.info.status != FileStatus::Ok ? kRed : it.info.isIwad ? kGreen : kAmber);
		auto users = ProfilesUsing(it.ref, true);
		std::string u;
		for (auto &s : users)
			u += (u.empty() ? "" : ", ") + s;
		kv("Profils", u.empty() ? "aucun" : u, kMuted);
		// compatibility with the current profile's mods
		if (const Profile *p = store_.Find(pickIwad_ ? editId_ : currentId_))
		{
			ResolvedProfile r = ResolveProfile(*p, lib_, defs_);
			for (auto *m : r.mods)
				if (!m->info.gameinfoIwad.empty() && ToLower(PathFileName(m->info.gameinfoIwad)) != ToLower(PathFileName(it.ref)))
				{
					kv("Avertissement", PathFileName(m->ref) + " attend " + m->info.gameinfoIwad, kAmber);
					break;
				}
		}
	}
	DrawHints(c, { { "A", pickIwad_ ? "Choisir" : "Utiliser dans le profil actif" }, { "Y", "Actualiser" }, { "B", "Retour" } });
}

// ---------------------------------------------------------------------------
// Profile mod list (load order)
// ---------------------------------------------------------------------------

void LauncherApp::UpdateMods(const InputFrame &in)
{
	State &st = states_[Screen::Mods];
	Profile *p = store_.FindMutable(editId_);
	if (!p)
	{
		Pop();
		return;
	}
	auto &mods = p->mods;
	const int n = (int)mods.size();
	bool changed = false;
	if (moving_)
	{
		if ((in.repeat & BtnUp) && st.sel > 0)
		{
			std::swap(mods[st.sel], mods[st.sel - 1]);
			st.sel--;
			changed = true;
		}
		if ((in.repeat & BtnDown) && st.sel < n - 1)
		{
			std::swap(mods[st.sel], mods[st.sel + 1]);
			st.sel++;
			changed = true;
		}
		if (in.down & (BtnA | BtnB | BtnY))
			moving_ = false;
	}
	else
	{
		MoveList(st.sel, n, in);
		if (in.down & BtnB)
		{
			Pop();
			return;
		}
		if (in.down & (BtnPlus | BtnR))
		{
			Push(Screen::ModPicker);
			return;
		}
		if (n > 0 && (in.down & BtnA))
		{
			mods[st.sel].enabled = !mods[st.sel].enabled;
			changed = true;
		}
		if (n > 0 && (in.down & BtnY))
			moving_ = true;
		if (n > 0 && (in.down & BtnX))
		{
			const std::string f = mods[st.sel].file;
			const std::string id = p->id;
			Ask("Retirer du profil", "Retirer " + f + " de ce profil ? Le fichier reste sur la carte SD.", { "Annuler", "Retirer" },
			    [this, id, f](int b) {
				Profile *q = store_.FindMutable(id);
				if (b != 1 || !q)
					return;
				q->mods.erase(std::remove_if(q->mods.begin(), q->mods.end(), [&](const ModRef &m) { return m.file == f; }), q->mods.end());
				std::string err;
				if (!store_.Save(*q, err))
					ToastMsg(err, true);
				states_[Screen::Mods].sel = std::max(0, std::min(states_[Screen::Mods].sel, (int)q->mods.size() - 1));
			});
		}
	}
	if (changed)
	{
		std::string err;
		if (!store_.Save(*p, err))
			ToastMsg(err, true);
	}
}

void LauncherApp::DrawMods(Canvas &c)
{
	State &st = states_[Screen::Mods];
	const Profile *p = store_.Find(editId_);
	if (!p)
		return;
	DrawHeader(c, "Mods du profil", p->name + "  •  ordre de chargement : 1 est chargé en premier, le dernier remplace les précédents");
	const int x = 48, y0 = 136, w = 1184, rh = 58, visible = 8;
	KeepVisible(st, (int)p->mods.size(), visible);
	DrawPanel(c, x - 12, y0 - 12, w + 24, visible * rh + 24);
	if (p->mods.empty())
		DrawTextWrapped(c, Font::Body, 21, x + 16, y0 + 20, w - 40, kText,
		                "Aucun mod dans ce profil. Appuyez sur + pour en ajouter depuis la bibliothèque (switch/uzdoom-nx2/mods/).");
	// load order = position among the enabled mods
	std::vector<int> order(p->mods.size(), 0);
	for (size_t k = 0, n = 0; k < p->mods.size(); k++)
		if (p->mods[k].enabled)
			order[k] = (int)++n;
	for (int i = 0; i < visible && st.scroll + i < (int)p->mods.size(); i++)
	{
		const int idx = st.scroll + i;
		const ModRef &m = p->mods[idx];
		const LibraryItem *it = lib_.FindMod(m.file);
		const int y = y0 + i * rh;
		SelRow(c, x, y, w, rh - 6, idx == st.sel, moving_ && idx == st.sel);
		OrderBadge(c, x + 10, y + 4, 44, order[idx]);
		// checkbox
		c.Frame(x + 68, y + 14, 24, 24, m.enabled ? kAccent : kDim, 2);
		if (m.enabled)
			c.Fill(x + 74, y + 20, 12, 12, kAccent);
		DrawText(c, Font::Bold, 21, x + 106, y + 4, m.enabled ? kText : kDim, PathFileName(m.file), 520);
		std::string sub = it && it->info.title != PathStem(m.file) ? it->info.title : m.file;
		if (m.optional)
			sub += "  (facultatif)";
		if (!m.enabled)
			sub += "  •  désactivé, non chargé";
		DrawText(c, Font::Body, 15, x + 106, y + 32, kMuted, sub, 520);
		if (!it)
			DrawChip(c, x + 660, y + 14, "ABSENT", Rgba(130, 20, 16), kText);
		else if (it->info.status != FileStatus::Ok)
			DrawChip(c, x + 660, y + 14, "INVALIDE", Rgba(130, 20, 16), kText);
		else
		{
			DrawChip(c, x + 660, y + 14, Up(CategoryPt(it->info.category)), Rgba(60, 40, 34), kText);
			std::string info = Size(it->info.size);
			if (it->info.mapCount)
				info += "  •  " + std::to_string(it->info.mapCount) + " cartes";
			if (it->info.hasZScript)
				info += "  •  ZScript";
			DrawTextRight(c, Font::Body, 16, x + w - 20, y + 18, kMuted, info);
		}
	}
	DrawScrollbar(c, x + w + 4, y0, visible * rh, (int)p->mods.size(), visible, st.scroll);
	if (moving_)
		DrawHints(c, { { "D-Pad", "Déplacer" }, { "A", "Terminer" } });
	else
		DrawHints(c, { { "A", "Activer/Désactiver" }, { "Y", "Déplacer" }, { "X", "Retirer" }, { "+", "Ajouter" }, { "B", "Retour" } });
}

// ---------------------------------------------------------------------------
// Mod library (browse / add to profile)
// ---------------------------------------------------------------------------

void LauncherApp::UpdateModPicker(const InputFrame &in)
{
	State &st = states_[Screen::ModPicker];
	const auto &items = lib_.Mods();
	MoveList(st.sel, (int)items.size(), in);
	if (in.down & BtnB)
	{
		Pop();
		return;
	}
	if (in.down & BtnY)
	{
		Rescan();
		ToastMsg("Bibliothèque actualisée : " + std::to_string(lib_.Mods().size()) + " mod(s)");
		return;
	}
	if (items.empty() || !(in.down & BtnA))
		return;
	const LibraryItem &it = items[st.sel];
	if (it.info.status != FileStatus::Ok)
	{
		Message("Fichier invalide", it.info.fileName + ": " + it.info.error);
		return;
	}
	if (it.info.isIwad)
	{
		Message("C'est une IWAD", it.info.fileName + " contient IWADINFO et doit servir d'IWAD. Déplacez-le dans iwads/.");
		return;
	}
	const std::string target = editId_.empty() ? currentId_ : editId_;
	Profile *p = store_.FindMutable(target);
	if (!p)
		return;
	for (auto &m : p->mods)
		if (ToLower(m.file) == ToLower(it.ref))
		{
			ToastMsg("Déjà dans le profil \"" + p->name + "\"");
			return;
		}
	if (p->mods.empty() && !p->modPatterns.empty())
		p->mods = ResolveProfile(*p, lib_, defs_).filled.mods;
	ModRef ref;
	ref.file = it.ref;
	p->mods.push_back(ref);
	std::string err;
	if (store_.Save(*p, err))
		ToastMsg("Ajouté à \"" + p->name + "\" (position " + std::to_string(p->mods.size()) + ")");
	else
		ToastMsg(err, true);
}

void LauncherApp::DrawModPicker(Canvas &c)
{
	State &st = states_[Screen::ModPicker];
	const auto &items = lib_.Mods();
	const std::string target = editId_.empty() ? currentId_ : editId_;
	const Profile *p = store_.Find(target);
	DrawHeader(c, "Bibliothèque de mods", std::to_string(items.size()) + " fichier(s)  •  A ajoute au profil \"" + (p ? p->name : "") + "\"");
	const int x = 48, y0 = 136, w = 700, rh = 62, visible = 8;
	KeepVisible(st, (int)items.size(), visible);
	DrawPanel(c, x - 12, y0 - 12, w + 24, visible * rh + 24);
	if (items.empty())
		DrawTextWrapped(c, Font::Body, 21, x + 16, y0 + 20, w - 40, kText,
		                "Aucun mod trouvé. Copiez des fichiers .pk3, .wad, .zip, .pk7 ou .deh dans switch/uzdoom-nx2/mods/, puis appuyez sur Y.");
	for (int i = 0; i < visible && st.scroll + i < (int)items.size(); i++)
	{
		const int idx = st.scroll + i;
		const LibraryItem &it = items[idx];
		const int y = y0 + i * rh;
		SelRow(c, x, y, w, rh - 6, idx == st.sel);
		int inProfile = 0;  // 1-based position in the profile's list
		if (p)
			for (size_t k = 0; k < p->mods.size(); k++)
				if (ToLower(p->mods[k].file) == ToLower(it.ref))
					inProfile = (int)k + 1;
		// the file name is what people copied to the SD card; the title comes from inside the file
		DrawText(c, Font::Bold, 21, x + 20, y + 4, kText, PathFileName(it.ref), w - 260);
		std::string sub = it.info.title != PathStem(it.ref) ? it.info.title + "  •  " : std::string();
		if (it.ref != PathFileName(it.ref))
			sub += it.ref + "  •  ";
		DrawText(c, Font::Body, 15, x + 20, y + 33, kMuted, sub + Size(it.info.size), w - 260);
		if (it.info.status != FileStatus::Ok)
			DrawChip(c, x + w - 200, y + 14, "INVALIDE", Rgba(130, 20, 16), kText);
		else if (it.info.isIwad)
			DrawChip(c, x + w - 200, y + 14, "C'EST UNE IWAD", Rgba(120, 80, 10), kText);
		else if (inProfile)
			DrawChip(c, x + w - 200, y + 14, "DANS LE PROFIL #" + std::to_string(inProfile), Rgba(24, 96, 52), kText);
		else
			DrawChip(c, x + w - 200, y + 14, Up(CategoryPt(it.info.category)), Rgba(60, 40, 34), kText);
	}
	DrawScrollbar(c, x + w + 4, y0, visible * rh, (int)items.size(), visible, st.scroll);

	const int dx = 786, dw = 446;
	DrawPanel(c, dx, y0 - 12, dw, visible * rh + 24);
	if (!items.empty())
	{
		const LibraryItem &it = items[st.sel];
		int y = y0 + 6;
		y += DrawTextWrapped(c, Font::Display, 34, dx + 24, y, dw - 48, kText, Up(it.info.title), 2, -4) + 8;
		auto kv = [&](const std::string &k, const std::string &v, uint32_t col = kText) {
			DrawText(c, Font::Body, 15, dx + 24, y + 2, kDim, Up(k));
			y += DrawTextWrapped(c, Font::Body, 18, dx + 150, y, dw - 174, col, v, 2) + 4;
		};
		std::string type = it.info.archive == ArchiveType::Wad ? "WAD" : it.info.archive == ArchiveType::Zip ? "PK3 (zip)"
			: it.info.archive == ArchiveType::SevenZip ? "PK7 (7z)" : (it.info.hasDehacked ? "DeHackEd" : "—");
		kv("Fichier", it.ref, kMuted);
		kv("Format", type + "  •  " + Size(it.info.size));
		kv("Catégorie", CategoryPt(it.info.category));
		std::string content;
		if (it.info.mapCount) content += std::to_string(it.info.mapCount) + " cartes ";
		if (it.info.hasZScript) content += "ZScript ";
		if (it.info.hasDecorate) content += "DECORATE ";
		if (it.info.hasDehacked) content += "DEHACKED ";
		if (it.info.hasMapinfo) content += "MAPINFO";
		kv("Contenu", content.empty() ? (it.info.archive == ArchiveType::SevenZip ? "7z : lu par le moteur" : "recursos") : content);
		if (!it.info.gameinfoIwad.empty())
			kv("IWAD demandée", it.info.gameinfoIwad, kAmber);
		if (!it.info.gameinfoLoad.empty())
		{
			std::string deps;
			for (auto &d : it.info.gameinfoLoad)
				deps += (deps.empty() ? "" : ", ") + d + (lib_.FindMod(d) ? "" : " (absente)");
			kv("Dépendances", deps, kAmber);
		}
		else
			kv("Dépendances", "aucune déclarée (GAMEINFO)", kMuted);
		if (it.info.status != FileStatus::Ok)
			kv("Erreur", it.info.error, kRed);
		auto users = ProfilesUsing(it.ref, false);
		std::string u;
		for (auto &s : users)
			u += (u.empty() ? "" : ", ") + s;
		kv("Profils", u.empty() ? "aucun" : u, kMuted);
	}
	DrawHints(c, { { "A", "Ajouter au profil" }, { "Y", "Actualiser" }, { "B", "Retour" } });
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

void LauncherApp::UpdateSettings(const InputFrame &in)
{
	State &st = states_[Screen::Settings];
	const int n = 7;
	MoveList(st.sel, n, in);
	if (in.down & BtnB)
	{
		Pop();
		return;
	}
	if (!(in.down & BtnA) && !(in.repeat & (BtnLeft | BtnRight)))
		return;
	std::string err;
	auto editDirs = [this](std::vector<std::string> &dirs, const std::string &what) {
		std::vector<std::string> items = { "Ajouter un dossier…" };
		for (auto &d : dirs)
			items.push_back("Retirer : " + d);
		Choose("Dossiers supplémentaires : " + what, items, [this, &dirs, what](int i) {
			if (i < 0)
				return;
			if (i == 0)
			{
				// nx2: read-only path (an extra library folder is only scanned and read)
				std::string path = "sdmc:/switch/gzdoom/" + std::string(what == "IWADs" ? "iwads" : "pwads"); // nx2: read-only path
				if (!Keyboard("Chemin du dossier", path, 200, path))
					return;
				if (!DirExists(path))
				{
					Message("Dossier introuvable", path + " n'existe pas sur la carte SD.");
					return;
				}
				dirs.push_back(path);
			}
			else
				dirs.erase(dirs.begin() + (i - 1));
			std::string e;
			if (!SaveLauncherSettings(paths_, settings_, e))
				ToastMsg(e, true);
			Rescan();
			ToastMsg("Bibliothèque actualisée");
		});
	};
	switch (st.sel)
	{
	case 0: editDirs(settings_.extraIwadDirs, "IWADs"); break;
	case 1: editDirs(settings_.extraModDirs, "mods"); break;
	case 2:
		settings_.animations = !settings_.animations;
		if (!SaveLauncherSettings(paths_, settings_, err))
			ToastMsg(err, true);
		break;
	case 3:
		Rescan();
		ToastMsg("Bibliothèque actualisée : " + std::to_string(lib_.Iwads().size()) + " IWAD(s), " + std::to_string(lib_.Mods().size()) + " mod(s)");
		break;
	case 4:
		Message("Fréquences d'origine",
		        "UZDoom NX2 fonctionne uniquement avec la gestion d'énergie normale de la Nintendo Switch 2. Il n'a aucune option d'overclock, "
		        "ne modifie pas les fréquences CPU, GPU ou mémoire et ne dépend d'aucun module système. "
		        "Les performances viennent des réglages de résolution, de limite d'IPS et de qualité.");
		break;
	case 5:
		Message("À propos d'UZDoom NX2",
		        info_.version + "\n" + info_.engineVersion + "\n" + info_.gfxVersion + "\n\n"
		        "Portage non officiel d'UZDoom pour Nintendo Switch 2 (kaiHEN), rendu Vulkan via OunceVK. Basé sur RDoom NX de Renan (Æon) (github.com/aeon1911/RDoom-NX). "
		        "Ce n'est pas un produit d'id Software ni des mainteneurs d'UZDoom.\n\n"
		        "Écritures limitées à sdmc:/switch/uzdoom-nx2 (pare-feu SD), aucune écriture NAND.\n"
		        "\n"
		        "Licence GPLv3 ; mentions des tiers dans switch/uzdoom-nx2/licenses/.");
		break;
	case 6:
		Ask("Réinitialiser le launcher", "Remettre les réglages globaux du launcher par défaut ? Les profils ne sont pas touchés.", { "Annuler", "Restaurer" },
		    [this](int b) {
			if (b != 1)
				return;
			std::string last = settings_.lastProfile, e;
			settings_ = LauncherSettings();
			settings_.lastProfile = last;
			if (!SaveLauncherSettings(paths_, settings_, e))
				ToastMsg(e, true);
			Rescan();
		});
		break;
	}
}

void LauncherApp::DrawSettings(Canvas &c)
{
	State &st = states_[Screen::Settings];
	DrawHeader(c, "Réglages", "Options globales du launcher (chaque profil garde les siennes)");
	auto list = [](const std::vector<std::string> &v) {
		std::string s;
		for (auto &d : v)
			s += (s.empty() ? "" : ", ") + d;
		return s.empty() ? std::string("nenhuma") : s;
	};
	struct Row { std::string label, value; };
	std::vector<Row> rows = {
		{ "Dossiers d'IWAD supplémentaires", list(settings_.extraIwadDirs) },
		{ "Dossiers de mods supplémentaires", list(settings_.extraModDirs) },
		{ "Animations de l'interface", settings_.animations ? "Activées" : "Désactivées" },
		{ "Actualiser les bibliothèques", "›" },
		{ "Fréquences", "D'origine uniquement" },
		{ "À propos et licences", "›" },
		{ "Réinitialiser les réglages du launcher", "›" },
	};
	const int x = 48, y0 = 136, w = 1184, rh = 60;
	DrawPanel(c, x - 12, y0 - 12, w + 24, (int)rows.size() * rh + 24);
	for (int i = 0; i < (int)rows.size(); i++)
	{
		const int y = y0 + i * rh;
		SelRow(c, x, y, w, rh - 6, i == st.sel);
		DrawText(c, Font::Body, 22, x + 22, y + 12, i == st.sel ? Rgba(255, 255, 255) : kMuted, rows[i].label);
		DrawTextRight(c, Font::Bold, 20, x + w - 22, y + 14, i == 4 ? kGreen : kText, Ellipsize(Font::Bold, 20, rows[i].value, 640));
	}
	DrawText(c, Font::Body, 17, 52, y0 + (int)rows.size() * rh + 30, kDim,
	         "Les dossiers supplémentaires permettent d'utiliser des bibliothèques existantes (lecture seule), comme sdmc:/switch/gzdoom/iwads, sans copier de gros fichiers.");
	DrawHints(c, { { "A", "Ouvrir/Changer" }, { "B", "Retour" } });
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

void LauncherApp::UpdateDiagnostics(const InputFrame &in)
{
	State &st = states_[Screen::Diagnostics];
	if (in.repeat & BtnUp) st.scroll = std::max(0, st.scroll - 1);
	if (in.repeat & BtnDown) st.scroll++;
	if (in.down & BtnB)
		Pop();
	if (in.down & BtnY)
	{
		ReadFile(info_.lastSessionFile, lastSession_);
		ToastMsg("Rapport de la dernière session rechargé");
	}
	if (in.down & BtnX)
	{
		Ask("Vider le cache des shaders",
		    "Efface le cache de pipelines Vulkan (switch/uzdoom-nx2/cache). "
		    "À utiliser si le jeu se fige ou se ferme avec \"device lost\". La partie suivante "
		    "recompile les shaders (petites pauses). IWAD, mods, profils et sauvegardes ne sont pas touchés.",
		    { "Annuler", "Vider" }, [this](int b) {
			if (b != 1)
				return;
			// only the program's own cache: nothing outside the sandbox is touched
			int n = RemoveFile(PathJoin(paths_.Cache(), "uzdoom/vulkanpipelinecache")) ? 1 : 0;
			RLOG_I("diag", "shader caches cleared by the user: %d files", n);
			ToastMsg("Cache des shaders vidé (" + std::to_string(n) + " fichier(s))");
		});
	}
}

void LauncherApp::DrawDiagnostics(Canvas &c)
{
	State &st = states_[Screen::Diagnostics];
	DrawHeader(c, "Diagnostic", "Versions, mémoire, rendu de la dernière session et problèmes détectés");
	SystemInfo si = plat_.Info();
	std::vector<std::pair<std::string, uint32_t>> lines;
	auto add = [&](const std::string &s, uint32_t col = kText) { lines.push_back({ s, col }); };
	add("[VERSIONS]", kAccent);
	add(info_.version + "  •  compilé le " + info_.buildDate);
	add(info_.engineVersion);
	add(info_.gfxVersion);
	add("[SYSTÈME]", kAccent);
	add(std::string("Mode d'exécution : ") + (si.appletMode ? "applet (mémoire limitée !)" : "application (titre, mémoire complète)"), si.appletMode ? kAmber : kGreen);
	if (si.memTotal)
		add("Mémoire du processus : " + Size(si.memUsed) + " utilisés sur " + Size(si.memTotal));
	add("Espace libre sur la SD : " + (si.sdFree == UINT64_MAX ? std::string("desconhecido") : Size(si.sdFree)));
	add(std::string("Console : ") + (si.docked ? "sur le dock (TV)" : "portable") + (si.firmware.empty() ? "" : "  •  firmware " + si.firmware));
	if (!si.emulator.empty())
		add("Environnement : " + si.emulator, kAmber);
	add("Fréquences : d'origine (CPU/GPU/mémoire jamais modifiés)", kGreen);
	add("[DERNIÈRE SESSION DU MOTEUR]", kAccent);
	if (lastSession_.empty())
		add("Aucune session enregistrée pour l'instant (" + info_.lastSessionFile + ")", kMuted);
	else
	{
		size_t s = 0;
		while (s < lastSession_.size())
		{
			size_t e = lastSession_.find('\n', s);
			std::string l = lastSession_.substr(s, e == std::string::npos ? std::string::npos : e - s);
			if (!l.empty())
				add(l, l.find("FAIL") != std::string::npos || l.find("error") != std::string::npos ? kRed : kText);
			if (e == std::string::npos)
				break;
			s = e + 1;
		}
	}
	{
		std::string lost;
		if (ReadFile(PathJoin(paths_.Logs(), "gpu-lost-previous.txt"), lost) && !lost.empty())
		{
			add("[DERNIÈRE PANNE GPU (device lost)]", kAccent);
			add("Le cache des shaders a été vidé automatiquement après.", kAmber);
			size_t s0 = 0;
			for (int k = 0; k < 14 && s0 < lost.size(); k++)
			{
				size_t e = lost.find('\n', s0);
				std::string l = lost.substr(s0, e == std::string::npos ? std::string::npos : e - s0);
				if (!l.empty())
					add(l, k == 0 ? kRed : kMuted);
				if (e == std::string::npos)
					break;
				s0 = e + 1;
			}
		}
	}
	add("[BIBLIOTHÈQUE]", kAccent);
	add(std::to_string(lib_.Iwads().size()) + " IWAD(s), " + std::to_string(lib_.Mods().size()) + " mod(s), " + std::to_string(store_.All().size()) + " profil(s), " + std::to_string(defs_.size()) + " définitions d'IWAD");
	add("[PROBLÈMES]", kAccent);
	if (loadWarnings_.empty())
		add("Aucun problème au chargement des profils et réglages.", kGreen);
	for (auto &w : loadWarnings_)
		add(w, kAmber);
	add("[FICHIERS DE LOG]", kAccent);
	add(PathJoin(paths_.Logs(), "rdoom.log") + "  (launcher)", kMuted);
	add(PathJoin(paths_.Logs(), "engine.log") + "  (moteur)", kMuted);
	add(PathJoin(paths_.Logs(), "last-launch.txt") + "  (dernière commande de lancement)", kMuted);

	const int x = 48, y0 = 136, rh = 30, visible = 17;
	st.scroll = std::max(0, std::min(st.scroll, std::max(0, (int)lines.size() - visible)));
	DrawPanel(c, x - 12, y0 - 12, 1208, visible * rh + 24);
	for (int i = 0; i < visible && st.scroll + i < (int)lines.size(); i++)
	{
		const auto &l = lines[st.scroll + i];
		const bool head = !l.first.empty() && l.first[0] == '[';
		DrawText(c, head ? Font::Display : Font::Body, head ? 26 : 19, x + 16, y0 + i * rh + (head ? -1 : 3), l.second, l.first, 1150);
	}
	DrawScrollbar(c, x + 1188, y0, visible * rh, (int)lines.size(), visible, st.scroll);
	DrawHints(c, { { "D-Pad", "Défiler" }, { "X", "Vider le cache des shaders" }, { "Y", "Recharger" }, { "B", "Retour" } });
}

} // namespace rdoom
