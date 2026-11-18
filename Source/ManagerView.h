#pragma once
#include "Catalog.h"
#include "NineHandsMenu.h"
#include <deque>
#include <thread>

//==============================================================================
// Fenetre de Plugin manager, au style du catalogue de firmwares de FM-9 :
// fiches par famille, filtres (etat + type), installation / mise a jour une par une ou toutes d'un coup.
//==============================================================================
namespace pm
{
    using namespace juce;
    using namespace NineHandsMenu;

    // tailles en points (comme la maquette), police Poppins integree
    inline Font pf (const Typeface::Ptr& tf, float h) { return tf != nullptr ? Font (FontOptions (tf).withPointHeight (h)) : Font (FontOptions().withPointHeight (h)); }

    const Colour kOrange (0xffffa726), kRed (0xffff5a52), kGreen (0xff4ccf78), kInk (0xff0b1520);

    //------------------------------------------------------------------ icones
    namespace Ico
    {
        inline Path svg (const char* d, bool evenOdd = false) { auto p = Drawable::parseSVGPath (d); p.setUsingNonZeroWinding (! evenOdd); return p; }
        inline void place (Graphics& g, const Path& p, Rectangle<float> r, Colour c, float stroke, bool fill = false)
        {
            auto q = p; q.applyTransform (AffineTransform::scale (r.getWidth() / 24.0f, r.getHeight() / 24.0f).translated (r.getX(), r.getY()));
            g.setColour (c);
            if (fill) g.fillPath (q);
            else g.strokePath (q, PathStrokeType (stroke * r.getWidth() / 24.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        // guitare, faders (studio), clavier (instrument) : memes icones que la page de telechargement
        inline void type (Graphics& g, const String& t, Rectangle<float> r, Colour c)
        {
            if (t == "studio")
            {
                place (g, svg ("M6 3v18M12 3v18M18 3v18"), r, c, 1.8f);
                const Path caps = svg ("M3.5 13h5v3h-5zM9.5 6h5v3h-5zM15.5 10h5v3h-5z");
                place (g, caps, r, Colours::black, 0, true);
                place (g, caps, r, c, 1.8f);
            }
            else if (t == "instrument")
                place (g, svg ("M4 5h16a1.5 1.5 0 0 1 1.5 1.5v11a1.5 1.5 0 0 1-1.5 1.5H4a1.5 1.5 0 0 1-1.5-1.5v-11A1.5 1.5 0 0 1 4 5zM5.6 5h2.6v9h-2.6zM10.7 5h2.6v9h-2.6zM15.8 5h2.6v9h-2.6zM6.8 14h.9v5h-.9zM11.55 14h.9v5h-.9zM16.3 14h.9v5h-.9z", true), r, c, 0, true);
            else
                place (g, svg ("M9 2.5h6l1 9-1.5 2h-5L8 11.5zM10.5 13.5v8M13.5 13.5v8M8.3 4.5H6.5M8.5 7H6.7M8.8 9.5H7M15.7 4.5h1.8M15.5 7h1.8M15.2 9.5H17"), r, c, 1.8f);
        }
        inline void arrow (Graphics& g, Point<float> c, float r, Colour col, bool filled)
        {
            if (filled) { g.setColour (col); g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r); }
            Path p;
            p.startNewSubPath (c.x, c.y - r * 0.5f); p.lineTo (c.x, c.y + r * 0.41f);
            p.startNewSubPath (c.x - r * 0.38f, c.y + 0.05f * r); p.lineTo (c.x, c.y + 0.43f * r); p.lineTo (c.x + 0.38f * r, c.y + 0.05f * r);
            g.setColour (filled ? kBg : col);
            g.strokePath (p, PathStrokeType (r * 0.18f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        inline void check (Graphics& g, Point<float> c, float r, bool on)
        {
            if (on) { g.setColour (kGreen); g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r); }
            else { g.setColour (kFaint); g.drawEllipse (c.x - r + 0.7f, c.y - r + 0.7f, 2 * r - 1.4f, 2 * r - 1.4f, 1.4f); }
            Path p; p.startNewSubPath (c.x - 0.41f * r, c.y + 0.03f * r); p.lineTo (c.x - 0.14f * r, c.y + 0.3f * r); p.lineTo (c.x + 0.41f * r, c.y - 0.26f * r);
            g.setColour (on ? kBg : kFaint);
            g.strokePath (p, PathStrokeType (r * 0.18f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        inline void trash (Graphics& g, Point<float> c, float r, Colour col)
        {
            place (g, svg ("M5.4 7.2h13.2M9.6 7.2V5.2h4.8v2M7.2 7.2l1 12h7.6l1-12M10.3 10.3v6M13.7 10.3v6"), Rectangle<float> (2 * r, 2 * r).withCentre (c), col, 1.8f);
        }
        inline void ring (Graphics& g, Point<float> c, float r, float f, Colour col)
        {
            g.setColour (kFaint); g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 2.0f);
            Path a; a.addCentredArc (c.x, c.y, r, r, 0, 0, MathConstants<float>::twoPi * jlimit (0.02f, 1.0f, f), true);
            g.setColour (col); g.strokePath (a, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        inline void star (Graphics& g, Rectangle<float> r, bool on, bool hot)
        {
            g.setColour (Colours::black.withAlpha (on || hot ? 0.55f : 0.45f)); g.fillEllipse (r);
            Path st; st.addStar (r.getCentre(), 5, r.getWidth() * 0.15f, r.getWidth() * 0.33f, 0.0f);
            const Colour gold (0xffffc83d);
            if (on) { g.setColour (gold); g.fillPath (st); }
            else { g.setColour (hot ? gold : Colours::white.withAlpha (0.85f)); g.strokePath (st, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded)); }
        }
        inline void folder (Graphics& g, Rectangle<float> r, Colour c)
        {
            place (g, svg ("M1.5 3.75A2.25 2.25 0 0 1 3.75 1.5h4.8l2.4 2.7h9.3a2.25 2.25 0 0 1 2.25 2.25v10.8a2.25 2.25 0 0 1-2.25 2.25H3.75a2.25 2.25 0 0 1-2.25-2.25z"), r, c, 2.0f);
        }
        inline void search (Graphics& g, Rectangle<float> r, Colour c)
        {
            place (g, svg ("M10.2 3a7.2 7.2 0 1 1 0 14.4a7.2 7.2 0 1 1 0-14.4zM15.6 15.6l6 6"), r, c, 2.1f);
        }
    }

    //------------------------------------------------------------------ boite de dialogue au style de l'application
    struct Dialog : public Component
    {
        String title, text; StringArray buttons; int primary = -1, danger = -1;
        std::function<void (int)> onChoice;      // index du bouton, -1 = fermee
        Fonts f; int hot = -1;

        Rectangle<float> box() const { return Rectangle<float> (500.0f, 210.0f).withCentre (getLocalBounds().toFloat().getCentre()); }
        Rectangle<float> button (int i) const
        {
            auto b = box().reduced (22.0f).removeFromBottom (36.0f);
            float x = b.getRight();
            for (int k = buttons.size() - 1; k >= 0; --k)
            {
                const float w = GlyphArrangement::getStringWidth (pf (f.med, 13.5f), buttons[k]) + 34.0f;
                x -= w;
                if (k == i) return { x, b.getY(), w, b.getHeight() };
                x -= 10.0f;
            }
            return {};
        }
        void paint (Graphics& g) override
        {
            g.fillAll (Colours::black.withAlpha (0.55f));
            const auto b = box();
            g.setColour (kBg); g.fillRoundedRectangle (b, 12.0f);
            g.setColour (kBorder); g.drawRoundedRectangle (b, 12.0f, 1.0f);
            auto r = b.reduced (24.0f, 20.0f);
            g.setColour (kText); g.setFont (pf (f.bold, 18.0f));
            g.drawText (title, r.removeFromTop (28.0f), Justification::centredLeft, true);
            g.setColour (kDim); g.setFont (pf (f.reg, 13.0f));
            g.drawFittedText (text, r.withTrimmedTop (8.0f).withTrimmedBottom (46.0f).toNearestInt(), Justification::topLeft, 5, 1.0f);
            for (int i = 0; i < buttons.size(); ++i)
            {
                const auto br = button (i);
                const bool pri = i == primary, dg = i == danger;
                const auto fill = dg ? kRed : pri ? kAccent : kHover;
                g.setColour (i == hot ? fill.brighter (0.12f) : fill); g.fillRoundedRectangle (br, 7.0f);
                if (! pri && ! dg) { g.setColour (kBorder); g.drawRoundedRectangle (br.reduced (0.5f), 7.0f, 1.0f); }
                g.setColour (pri || dg ? kInk : kText); g.setFont (pf (f.med, 13.5f));
                g.drawText (buttons[i], br, Justification::centred);
            }
        }
        int hit (Point<float> p) const { for (int i = 0; i < buttons.size(); ++i) if (button (i).contains (p)) return i; return -1; }
        void mouseMove (const MouseEvent& e) override { const int h = hit (e.position); if (h != hot) { hot = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); } }
        void mouseUp (const MouseEvent& e) override
        {
            const int h = hit (e.position);
            if (h < 0 && box().contains (e.position)) return;
            auto cb = onChoice; const int choice = h;
            MessageManager::callAsync ([cb, choice] { if (cb) cb (choice); });
        }
    };

    //==============================================================================
    class ManagerView : public Component, private Timer
    {
    public:
        ManagerView()
        {
            int n = 0;
            if (auto* d = resource ("Poppins-Regular.ttf", n)) fo.reg = Typeface::createSystemTypefaceFor (d, (size_t) n);
            if (auto* d = resource ("Poppins-Medium.ttf", n)) fo.med = Typeface::createSystemTypefaceFor (d, (size_t) n);
            if (auto* d = resource ("Poppins-Bold.ttf", n)) fo.bold = Typeface::createSystemTypefaceFor (d, (size_t) n);
            if (auto* d = resource ("logo_white.png", n)) logo = ImageFileFormat::loadFrom (d, (size_t) n);
            menuLnf.f = fo; menuLnf.textScale = 13.0f / 15.0f;

            cat.loadLocal();
            folder = File (settings().getValue ("folder"));
            statusFilter = jlimit (0, 5, settings().getIntValue ("statusFilter", 0));
            typeFilter = jlimit (0, 3, settings().getIntValue ("typeFilter", 0));
            for (auto& s : StringArray::fromLines (dataDir().getChildFile ("Favorites.txt").loadFileAsString())) if (s.isNotEmpty()) favorites.add (s);

            search.setFont (pf (fo.reg, 13.0f));
            search.setTextToShowWhenEmpty ("Search plugins", kFaint);
            search.setIndents (38, 0);
            search.setJustification (Justification::centredLeft);
            search.setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
            search.setColour (TextEditor::outlineColourId, Colours::transparentBlack);
            search.setColour (TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            search.setColour (TextEditor::textColourId, kText);
            search.setColour (TextEditor::highlightColourId, kAccent.withAlpha (0.35f));
            search.setColour (CaretComponent::caretColourId, kAccent);
            search.onTextChange = [this] { layoutList(); viewport.setViewPosition (0, 0); repaint(); };
            search.onEscapeKey = [this] { search.clear(); layoutList(); repaint(); };
            addAndMakeVisible (search);

            addAndMakeVisible (viewport);
            viewport.setViewedComponent (&list, false);
            viewport.setScrollBarsShown (true, false);
            viewport.setScrollBarThickness (8);
            viewport.getVerticalScrollBar().setColour (ScrollBar::thumbColourId, kFaint);
            list.owner = this;

            cleanAfterUpdate();
            if (JUCEApplicationBase::getCommandLineParameters().contains ("--after-update"))
                status = "Plugin manager updated to v" APP_VERSION;
            setSize (1440, 900);
            if (folder.getFullPathName().isNotEmpty()) { scan (cat, folder); refresh(); }
            else MessageManager::callAsync ([sp = SafePointer<ManagerView> (this)] { if (sp != nullptr) sp->askFolder (true); });
            startTimerHz (20);
        }

        ~ManagerView() override
        {
            stopFlag = true;
            if (checker.joinable()) checker.join();
            if (installer.joinable()) installer.join();
            if (updater.joinable()) updater.join();
        }

        //------------------------------------------------------------------ dessin
        void paint (Graphics& g) override
        {
            g.fillAll (kBg);
            // marque : logo en grand, PLUGIN MANAGER en petit dessous
            if (logo.isValid())
                g.drawImage (logo, logoRect(), RectanglePlacement::xLeft | RectanglePlacement::yTop);
            g.setColour (kDim); g.setFont (pf (fo.med, 12.5f).withExtraKerningFactor (0.33f));
            g.drawText ("PLUGIN MANAGER", Rectangle<float> (logoRect().getX(), logoRect().getBottom() + 6.0f, 300.0f, 16.0f), Justification::centredLeft);

            // recherche
            const auto sr = searchRect().toFloat();
            g.setColour (kHover); g.fillRoundedRectangle (sr, 7.0f);
            g.setColour (search.hasKeyboardFocus (true) ? kAccent : kBorder); g.drawRoundedRectangle (sr.reduced (0.5f), 7.0f, 1.0f);
            Ico::search (g, Rectangle<float> (17.0f, 17.0f).withPosition (sr.getX() + 12.0f, sr.getCentreY() - 8.5f), search.hasKeyboardFocus (true) ? kAccent : kDim);

            // filtres
            drawSegments (g, statusSegs(), statusFilter, true);
            drawSegments (g, typeSegs(), typeFilter, false);

            // dossier
            const auto fr = folderRect().toFloat();
            g.setColour (kHover); g.fillRoundedRectangle (fr, 7.0f);
            g.setColour (kBorder); g.drawRoundedRectangle (fr.reduced (0.5f), 7.0f, 1.0f);
            Ico::folder (g, Rectangle<float> (16.0f, 16.0f).withPosition (fr.getX() + 11.0f, fr.getCentreY() - 8.0f), kAccent);
            const auto cr = changeRect().toFloat();
            g.setColour (hotHeader == 100 ? kHover.brighter (0.15f) : Colour (0xff232a34)); g.fillRoundedRectangle (cr, 11.0f);
            g.setColour (hotHeader == 100 ? kAccent.withAlpha (0.6f) : kBorder); g.drawRoundedRectangle (cr.reduced (0.5f), 11.0f, 1.0f);
            g.setColour (kText); g.setFont (pf (fo.reg, 11.5f)); g.drawText ("Change folder...", cr, Justification::centred);
            g.setFont (pf (fo.reg, 12.5f)); g.setColour (folder.isDirectory() ? kText : kOrange);
            g.drawText (folder.getFullPathName().isEmpty() ? String ("No folder selected") : folder.getFullPathName(),
                        Rectangle<float> (fr.getX() + 36.0f, fr.getY(), cr.getX() - fr.getX() - 44.0f, fr.getHeight()), Justification::centredLeft, true);

            // resume
            int inst = 0; for (auto& p : cat.plugins) inst += p.isInstalled() ? 1 : 0;
            const auto ts = typeSegs();
            const float sx = ts.back().second.getRight() + 14.0f;
            g.setColour (kDim); g.setFont (pf (fo.reg, 13.0f));
            g.drawText (String (inst) + " installed / " + String ((int) cat.plugins.size()), Rectangle<float> (sx, (float) row2Y, 200.0f, 30.0f), Justification::centredLeft);

            // systeme
            const auto osr = osRect().toFloat();
            g.setColour (kAccent); g.drawRoundedRectangle (osr.reduced (0.5f), 12.0f, 1.0f);
            g.setFont (pf (fo.reg, 11.5f));
            g.drawText (String (isMac ? "macOS" : "Windows") + "  -  VST3", osr, Justification::centred);

            // pied
            g.setColour (kSep); g.fillRect (0, getHeight() - footerH, getWidth(), 1);
            drawFooter (g);
        }

        void resized() override
        {
            search.setBounds (searchRect().reduced (2));
            viewport.setBounds (getLocalBounds().withTrimmedTop (listTop).withTrimmedBottom (footerH).withTrimmedLeft (margin).withTrimmedRight (6));
            layoutList();
        }

        void useFolder (const File& f) { dialog.reset(); setFolder (f); }      // tests
        void testInstall (const String& repo) { for (int i = 0; i < (int) cat.plugins.size(); ++i) if (cat.plugins[(size_t) i].repo == repo) enqueue ({ i }); }

    private:
        friend struct List;
        static constexpr int margin = 18, row2Y = 76, listTop = 116, footerH = 62;

        Fonts fo; Image logo;
        NineHandsMenu::MenuLnf menuLnf;
        Catalog cat;
        File folder;
        int statusFilter = 0, typeFilter = 0;
        StringArray favorites;
        TextEditor search;
        Viewport viewport;

        // verification GitHub
        std::thread checker;
        std::atomic<bool> stopFlag { false }, checking { false };
        Time checkedAt; String appLatest;
        // mise a jour de l'application elle-meme
        std::thread updater;
        std::atomic<bool> appUpdating { false };
        std::atomic<float> appProgress { 0.0f };
        bool appUpdateFailed = false;
        // installation (file d'attente)
        std::thread installer;
        CriticalSection qLock;
        std::deque<Plugin> queue;
        String currentRepo;          // protege par qLock
        String current() const { const ScopedLock sl (qLock); return currentRepo; }
        int queuedCount() const { const ScopedLock sl (qLock); return (int) queue.size(); }
        std::atomic<float> progress { 0.0f };
        std::atomic<bool> installerRunning { false };
        String status;               // commence par ! : erreur
        std::unique_ptr<FileChooser> chooser;
        std::unique_ptr<Dialog> dialog;
        int hotHeader = -1;

        //------------------------------------------------------------------ geometrie de l'en-tete
        Rectangle<float> logoRect() const
        {
            const float w = 300.0f, h = logo.isValid() ? w * (float) logo.getHeight() / (float) logo.getWidth() : 27.0f;
            return { (float) margin + 2.0f, 22.0f, w, h };
        }
        using Seg = std::pair<String, Rectangle<float>>;
        std::vector<Seg> makeSegs (const StringArray& names, const Array<int>& counts, float x, float y, float h, bool icons) const
        {
            std::vector<Seg> out;
            const auto f1 = pf (fo.reg, 13.0f);
            x += 3.0f;
            for (int i = 0; i < names.size(); ++i)
            {
                const float w = GlyphArrangement::getStringWidth (f1, names[i]) + GlyphArrangement::getStringWidth (f1, String (counts[i])) + 27.0f
                              + (icons && i > 0 ? 21.0f : 0.0f);
                out.push_back ({ names[i] + "|" + String (counts[i]), { x, y + 3.0f, w, h - 6.0f } });
                x += w;
            }
            return out;
        }
        std::vector<Seg> statusSegs() const
        {
            const StringArray names { "All", "Installed", "Updates", "Not installed", "Favorites", "New" };
            Array<int> c; for (int i = 0; i < 6; ++i) c.add (countStatus (i));
            auto s = makeSegs (names, c, 0, 18.0f, 34.0f, false);
            const float w = s.back().second.getRight() + 3.0f, x0 = (float) getWidth() - margin - w;
            for (auto& e : s) e.second.translate (x0, 0);
            return s;
        }
        std::vector<Seg> typeSegs() const
        {
            const StringArray names { "All types", "Guitar", "Studio", "Instrument" };
            Array<int> c; for (int i = 0; i < 4; ++i) c.add (countType (i));
            return makeSegs (names, c, (float) folderRect().getRight() + 14.0f, (float) row2Y, 30.0f, true);
        }
        Rectangle<int> searchRect() const
        {
            const auto s = statusSegs();
            const int right = (int) s.front().second.getX() - 3 - 14;
            return { jmax (margin + 340, right - 300), 18, jmin (300, right - margin - 340), 34 };
        }
        Rectangle<int> folderRect() const
        {
            const auto t = folder.getFullPathName().isEmpty() ? String ("No folder selected") : folder.getFullPathName();
            const int w = jlimit (260, 520, (int) GlyphArrangement::getStringWidth (pf (fo.reg, 12.5f), t) + 36 + 130);
            return { margin, row2Y, w, 30 };
        }
        Rectangle<int> changeRect() const { auto f = folderRect(); return { f.getRight() - 6 - 108, f.getY() + 4, 108, 22 }; }
        Rectangle<int> osRect() const { return { getWidth() - margin - 140, row2Y + 3, 140, 24 }; }

        void drawSegments (Graphics& g, const std::vector<Seg>& segs, int sel, bool updOrange)
        {
            auto all = segs.front().second.getUnion (segs.back().second).expanded (3.0f);
            g.setColour (kHover); g.fillRoundedRectangle (all, 6.0f);
            g.setColour (kBorder); g.drawRoundedRectangle (all.reduced (0.5f), 6.0f, 1.0f);
            const auto f1 = pf (fo.reg, 13.0f);
            for (int i = 0; i < (int) segs.size(); ++i)
            {
                const auto r = segs[(size_t) i].second;
                const bool on = i == sel;
                const auto nm = segs[(size_t) i].first.upToFirstOccurrenceOf ("|", false, false), cnt = segs[(size_t) i].first.fromFirstOccurrenceOf ("|", false, false);
                if (on) { g.setColour (kAccent); g.fillRoundedRectangle (r, 4.0f); }
                float x = r.getX() + 10.0f;
                if (! updOrange && i > 0)
                {
                    static const char* types[] = { "", "guitare", "studio", "instrument" };
                    Ico::type (g, types[i], Rectangle<float> (15.0f, 15.0f).withPosition (x, r.getCentreY() - 7.5f), on ? kInk : kDim);
                    x += 21.0f;
                }
                g.setFont (f1); g.setColour (on ? kInk : kText);
                const float wn = GlyphArrangement::getStringWidth (f1, nm);
                g.drawText (nm, Rectangle<float> (x, r.getY(), wn + 2.0f, r.getHeight()), Justification::centredLeft, false);
                g.setColour (on ? kInk.withAlpha (0.65f) : (updOrange && i == 2 && cnt != "0" ? kOrange : kDim));
                g.drawText (cnt, Rectangle<float> (x + wn + 7.0f, r.getY(), 40.0f, r.getHeight()), Justification::centredLeft, false);
            }
        }

        //------------------------------------------------------------------ pied
        struct Btn { String text; Rectangle<float> r; int id; Colour fill; bool on; };
        std::vector<Btn> footerButtons() const
        {
            const int nInst = (int) visibleWith ([] (const Plugin& p) { return ! p.isInstalled(); }).size();
            const int nUpd = (int) visibleWith ([] (const Plugin& p) { return p.hasUpdate(); }).size();
            std::vector<Btn> b {
                { "Install all  (" + String (nInst) + ")", {}, 1, kAccent.withAlpha (0.9f), nInst > 0 && folder.isDirectory() },
                { "Update all  (" + String (nUpd) + ")", {}, 2, kOrange, nUpd > 0 },
                { "Refresh", {}, 3, kHover, ! checking },
                { "Close", {}, 4, kHover, true } };
            float x = (float) getWidth() - margin;
            const float y = (float) getHeight() - footerH + 14.0f;
            for (int i = (int) b.size() - 1; i >= 0; --i)
            {
                const float w = GlyphArrangement::getStringWidth (pf (fo.med, 13.5f), b[(size_t) i].text) + 34.0f;
                x -= w; b[(size_t) i].r = { x, y, w, 34.0f }; x -= 10.0f;
            }
            return b;
        }
        String appChipText() const
        {
            if (appUpdating) return "Updating Plugin manager to v" + appLatest + "  " + String (roundToInt (appProgress.load() * 100.0f)) + " %";
            return "Plugin manager v" + appLatest + (appUpdateFailed ? "  -  retry the update" : "  available");
        }

        // nouvelle version de l'application : telechargement, remplacement, puis fermeture et reouverture automatiques
        void maybeSelfUpdate()
        {
            if (appLatest.isEmpty() || compareVersions (APP_VERSION, appLatest) >= 0 || appUpdating || appUpdateFailed) return;
            if (current().isNotEmpty() || queuedCount() > 0) return;      // attend la fin des installations en cours
            if (updater.joinable()) updater.join();
            appUpdating = true; appProgress = 0.0f; status = {};
            SafePointer<ManagerView> self (this);
            updater = std::thread ([this, self]
            {
                String err; File target;
                const bool ok = pm::selfUpdate ([this] (float f) { appProgress = f; }, stopFlag, err, target);
                if (stopFlag) return;
                MessageManager::callAsync ([self, ok, err, target]
                {
                    if (self == nullptr) return;
                    if (ok && relaunchApp (target)) { JUCEApplication::quit(); return; }
                    self->appUpdating = false;
                    self->appUpdateFailed = true;
                    self->status = "!" + (err.isNotEmpty() ? err : String ("Plugin manager update: cannot restart the application."));
                    self->repaint();
                });
            });
            repaint();
        }

        Rectangle<float> appUpdateRect() const
        {
            if (appLatest.isEmpty() || compareVersions (APP_VERSION, appLatest) >= 0) return {};
            const auto t = appChipText();
            const float w = GlyphArrangement::getStringWidth (pf (fo.med, 12.0f), t) + 28.0f;
            return { footerButtons().front().r.getX() - 14.0f - w, (float) getHeight() - footerH + 18.0f, w, 26.0f };
        }
        void drawFooter (Graphics& g)
        {
            for (auto& b : footerButtons())
            {
                const bool pri = b.id <= 2, hot = hotHeader == 200 + b.id && b.on;
                auto fill = pri && b.on ? b.fill : kHover;
                g.setColour (hot ? fill.brighter (0.1f) : fill); g.fillRoundedRectangle (b.r, 7.0f);
                if (! (pri && b.on)) { g.setColour (kBorder); g.drawRoundedRectangle (b.r.reduced (0.5f), 7.0f, 1.0f); }
                g.setColour (pri && b.on ? (b.id == 2 ? Colour (0xff1a1206) : kInk) : (b.on ? kText : kFaint));
                g.setFont (pf (fo.med, 13.5f)); g.drawText (b.text, b.r, Justification::centred);
            }
            if (auto ar = appUpdateRect(); ! ar.isEmpty())
            {
                g.setColour (kOrange.withAlpha (hotHeader == 300 ? 0.25f : 0.12f)); g.fillRoundedRectangle (ar, 13.0f);
                g.setColour (kOrange); g.drawRoundedRectangle (ar.reduced (0.5f), 13.0f, 1.0f);
                g.setFont (pf (fo.med, 12.0f)); g.drawText (appChipText(), ar, Justification::centred);
            }
            // etat
            auto sr = Rectangle<float> ((float) margin, (float) getHeight() - footerH, 640.0f, (float) footerH);
            String s = status; bool err = s.startsWith ("!");
            if (err) s = s.substring (1);
            if (s.isEmpty())
            {
                if (current().isNotEmpty()) s = "Installing " + nameOf (current()) + "  " + String (roundToInt (progress.load() * 100.0f)) + " %"
                                                  + (queuedCount() == 0 ? String() : "  -  " + String (queuedCount()) + " queued");
                else if (checking) s = "Checking GitHub...";
                else if (checkedAt.toMilliseconds() > 0) s = "GitHub checked " + checkedAt.formatted ("%d/%m/%Y %H:%M");
            }
            const auto dot = Rectangle<float> (14.0f, 14.0f).withPosition (sr.getX(), sr.getCentreY() - 7.0f);
            if (err) { g.setColour (kRed); g.fillEllipse (dot); }
            else if (checking || current().isNotEmpty()) Ico::ring (g, dot.getCentre(), 6.0f, (float) std::fmod (Time::getMillisecondCounterHiRes() / 900.0, 1.0), kAccent);
            else if (checkedAt.toMilliseconds() > 0) Ico::check (g, dot.getCentre(), 7.0f, true);
            g.setColour (err ? Colour (0xffff8a80) : kDim); g.setFont (pf (fo.reg, 13.0f));
            g.drawText (s, sr.withTrimmedLeft (22.0f), Justification::centredLeft, true);
        }

        //------------------------------------------------------------------ filtres
        bool matchesType (const Plugin& p, int t) const
        {
            static const char* types[] = { "", "guitare", "studio", "instrument" };
            return t == 0 || p.type == types[t];
        }
        bool matchesStatus (const Plugin& p, int s) const
        {
            switch (s)
            {
                case 1: return p.isInstalled();
                case 2: return p.hasUpdate();
                case 3: return ! p.isInstalled();
                case 4: return favorites.contains (p.repo);
                case 5: return p.isNew();
                default: return true;
            }
        }
        bool matchesSearch (const Plugin& p) const
        {
            const auto q = search.getText().trim().toLowerCase();
            return q.isEmpty() || p.name.toLowerCase().contains (q) || p.desc.toLowerCase().contains (q);
        }
        int countStatus (int s) const { int n = 0; for (auto& p : cat.plugins) n += (matchesStatus (p, s) && matchesType (p, typeFilter)) ? 1 : 0; return n; }
        int countType (int t) const { int n = 0; for (auto& p : cat.plugins) n += matchesType (p, t) ? 1 : 0; return n; }
        bool visible (const Plugin& p) const { return matchesStatus (p, statusFilter) && matchesType (p, typeFilter) && matchesSearch (p); }
        template <typename F> std::vector<int> visibleWith (F f) const
        {
            std::vector<int> v;
            for (int i = 0; i < (int) cat.plugins.size(); ++i) if (visible (cat.plugins[(size_t) i]) && f (cat.plugins[(size_t) i])) v.push_back (i);
            return v;
        }
        String nameOf (const String& repo) const { for (auto& p : cat.plugins) if (p.repo == repo) return p.name; return repo; }
        bool isQueued (const String& repo) const { const ScopedLock sl (qLock); for (auto& q : queue) if (q.repo == repo) return true; return false; }

        //------------------------------------------------------------------ liste des fiches
        struct Item { Rectangle<int> r; int plugin = -1; String family; };
        std::vector<Item> items;
        static constexpr int gap = 10, minCard = 136, pad = 7;
        static int cardH (int cw) { return (cw - 2 * pad) + pad + 106; }
        static Rectangle<int> screenRect (Rectangle<int> c) { const int s = c.getWidth() - 2 * pad; return { c.getX() + pad, c.getY() + pad, s, s }; }
        static Point<float> btnCentre (Rectangle<int> c, int k) { return { c.getX() + pad + 13.0f + k * 30.0f, c.getBottom() - 21.0f }; }
        static Rectangle<float> pillRect (Rectangle<int> c) { const auto b = btnCentre (c, 2); return { b.x + 16.0f, b.y - 11.0f, (float) c.getRight() - pad - 2.0f - (b.x + 16.0f), 22.0f }; }
        static Rectangle<float> starRect (Rectangle<int> c) { auto s = screenRect (c).toFloat(); return { s.getX() + 5.0f, s.getY() + 5.0f, 20.0f, 20.0f }; }

        void layoutList()
        {
            items.clear();
            const int w = jmax (300, viewport.getWidth() - 14);
            const int cols = jmax (1, (w + gap) / (minCard + gap));
            const int cw = (w - (cols - 1) * gap) / cols, ch = cardH (cw);
            int y = 0;
            StringArray order { "out" };
            for (auto& f : cat.families) order.addIfNotAlreadyThere (f.id);
            for (auto& p : cat.plugins) order.addIfNotAlreadyThere (p.family);
            for (auto& fid : order)
            {
                std::vector<int> idx;
                for (int i = 0; i < (int) cat.plugins.size(); ++i) if (cat.plugins[(size_t) i].family == fid && visible (cat.plugins[(size_t) i])) idx.push_back (i);
                if (idx.empty()) continue;
                items.push_back ({ { 0, y, w, 38 }, -1, fid });
                y += 38;
                for (size_t k = 0; k < idx.size(); ++k)
                {
                    const int c = (int) k % cols;
                    if (c == 0 && k > 0) y += ch + gap;
                    items.push_back ({ { c * (cw + gap), y, cw, ch }, idx[k], fid });
                }
                y += ch + 4;
            }
            list.setSize (w, jmax (y + 16, 10));
            list.repaint();
        }

        struct List : public Component
        {
            ManagerView* owner = nullptr;
            int hover = -1, hotBtn = -1;      // hotBtn : 0 fleche, 1 coche, 2 corbeille, 3 notice, 4 etoile

            void paint (Graphics& g) override
            {
                auto& o = *owner;
                const auto clip = g.getClipBounds();
                if (o.items.empty())
                {
                    g.setColour (kDim); g.setFont (pf (o.fo.reg, 14.0f));
                    g.drawText (o.cat.plugins.empty() ? "No plugin list." : "No plugin matches these filters.", getLocalBounds().withHeight (120), Justification::centred);
                }
                for (auto& it : o.items)
                {
                    if (! it.r.intersects (clip)) continue;
                    if (it.plugin < 0)
                    {
                        const auto* f = o.cat.family (it.family);
                        const auto title = (f != nullptr ? f->title : it.family).toUpperCase();
                        int n = 0; for (auto& x : o.items) n += (x.plugin >= 0 && x.family == it.family) ? 1 : 0;
                        const float cy = it.r.getY() + 23.0f;
                        g.setColour (f != nullptr ? f->colour : kDim); g.fillEllipse (6.0f, cy - 3.5f, 7.0f, 7.0f);
                        const auto hf = pf (o.fo.bold, 11.5f).withExtraKerningFactor (0.14f);
                        g.setFont (hf); g.setColour (kDim);
                        const float tw = GlyphArrangement::getStringWidth (hf, title);
                        g.drawText (title, Rectangle<float> (20.0f, cy - 9.0f, tw + 4.0f, 18.0f), Justification::centredLeft);
                        g.setFont (pf (o.fo.reg, 11.5f)); g.setColour (kFaint);
                        g.drawText (String (n), Rectangle<float> (26.0f + tw, cy - 9.0f, 40.0f, 18.0f), Justification::centredLeft);
                        continue;
                    }
                    const auto& p = o.cat.plugins[(size_t) it.plugin];
                    const bool inst = p.isInstalled(), upd = p.hasUpdate(), doing = o.current() == p.repo, queued = o.isQueued (p.repo);
                    const bool hot = hover == it.plugin;
                    const auto c = it.r.toFloat();
                    g.setColour (hot ? kHover.brighter (0.06f) : kHover); g.fillRoundedRectangle (c, 10.0f);
                    g.setColour (upd || doing ? kOrange : kBorder); g.drawRoundedRectangle (c.reduced (upd ? 0.9f : 0.5f), 10.0f, upd ? 1.8f : 1.0f);

                    // vignette
                    const auto sr = screenRect (it.r).toFloat();
                    {
                        Graphics::ScopedSaveState ss (g);
                        Path clipP; clipP.addRoundedRectangle (sr, 7.0f); g.reduceClipRegion (clipP);
                        g.setColour (Colour (0xff06080b)); g.fillRect (sr);
                        if (p.image.isValid())
                        {
                            g.setImageResamplingQuality (Graphics::highResamplingQuality);
                            g.setOpacity (inst ? 1.0f : 0.4f);
                            g.drawImage (p.image, sr.reduced (sr.getWidth() * 0.06f), RectanglePlacement::centred);
                            g.setOpacity (1.0f);
                        }
                    }
                    Ico::star (g, starRect (it.r), o.favorites.contains (p.repo), hot && hotBtn == 4);
                    if (upd)
                    {
                        const auto lab = "UPDATE v" + p.latestVersion();
                        const auto ft = pf (o.fo.bold, 8.0f);
                        const float tw = GlyphArrangement::getStringWidth (ft, lab) + 18.0f;
                        const auto b = Rectangle<float> (sr.getRight() - tw - 6.0f, sr.getY() + 6.0f, tw, 22.0f);
                        g.setColour (kOrange); g.fillRoundedRectangle (b, 11.0f);
                        g.setColour (kBg); g.setFont (ft); g.drawText (lab, b, Justification::centred);
                    }
                    if (p.isNew())
                    {
                        const auto nf = pf (o.fo.bold, 8.0f);
                        const float nw = GlyphArrangement::getStringWidth (nf, "NEW") + 8.0f;
                        const auto nb = Rectangle<float> (sr.getRight() - nw - 3.0f, sr.getBottom() - 17.0f, nw, 14.0f);
                        g.setColour (Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (nb, 3.0f);
                        g.setColour (Colour (0xff5be08a)); g.setFont (nf); g.drawText ("NEW", nb, Justification::centred);
                    }
                    {   // type : icone + nom
                        static const std::map<String, String> labels { { "guitare", "GUITAR" }, { "studio", "STUDIO" }, { "instrument", "INSTRUMENT" } };
                        const auto lab = labels.count (p.type) ? labels.at (p.type) : String ("GUITAR");
                        const auto kf = pf (o.fo.med, 7.6f).withExtraKerningFactor (0.1f);
                        const float kw = GlyphArrangement::getStringWidth (kf, lab) + 27.0f;
                        const auto kb = Rectangle<float> (sr.getX() + 5.0f, sr.getBottom() - 21.0f, kw, 16.0f);
                        g.setColour (Colours::black.withAlpha (0.62f)); g.fillRoundedRectangle (kb, 4.0f);
                        Ico::type (g, p.type, Rectangle<float> (11.0f, 11.0f).withPosition (kb.getX() + 5.0f, kb.getCentreY() - 5.5f), Colour (0xffc9d0da));
                        g.setColour (Colour (0xffc9d0da)); g.setFont (kf);
                        g.drawText (lab, kb.withTrimmedLeft (20.0f), Justification::centredLeft);
                    }

                    // nom, versions, descriptif
                    float y = sr.getBottom() + 8.0f;
                    const float tx = c.getX() + pad + 3.0f, tw = c.getWidth() - 2.0f * pad - 6.0f;
                    g.setColour (kText); g.setFont (pf (o.fo.med, 11.4f));
                    g.drawText (p.name, Rectangle<float> (tx, y, tw, 18.0f), Justification::centredLeft, true);
                    y += 18.0f;
                    {
                        const auto vf = pf (o.fo.reg, 9.2f);
                        AttributedString as; as.setJustification (Justification::centredLeft); as.setWordWrap (AttributedString::none);
                        if (! inst) { as.append ("Not installed", vf, kFaint); as.append ("  v" + p.latestVersion(), vf, kDim); }
                        else if (upd) { as.append ("v" + p.installedVersion, vf, kDim); as.append (String::fromUTF8 ("  \xe2\x86\x92  v") + p.latestVersion(), vf, kOrange); }
                        else { as.append ("v" + (p.installedVersion.isNotEmpty() ? p.installedVersion : String ("?")), vf, kDim); as.append ("  up to date", vf, kGreen); }
                        as.draw (g, Rectangle<float> (tx, y, tw, 14.0f));
                    }
                    y += 15.0f;
                    g.setColour (kDim); g.setFont (pf (o.fo.reg, 8.6f));
                    g.drawFittedText (p.desc, Rectangle<float> (tx, y, tw, 26.0f).toNearestInt(), Justification::topLeft, 2, 1.0f);

                    // boutons
                    const auto b0 = btnCentre (it.r, 0), b1 = btnCentre (it.r, 1), b2 = btnCentre (it.r, 2);
                    const float r = 11.0f;
                    if (doing) Ico::ring (g, b0, r * 0.82f, o.progress.load(), inst ? kOrange : kAccent);
                    else if (queued) Ico::ring (g, b0, r * 0.82f, 0.02f, kFaint);
                    else if (! inst) Ico::arrow (g, b0, r, (hot && hotBtn == 0) ? kAccent.brighter (0.25f) : kAccent, true);
                    else if (upd) Ico::arrow (g, b0, r, (hot && hotBtn == 0) ? kOrange.brighter (0.2f) : kOrange, true);
                    else Ico::arrow (g, b0, r, kFaint, false);
                    Ico::check (g, b1, r, inst);
                    Ico::trash (g, b2, r * 0.95f, inst && ! doing ? ((hot && hotBtn == 2) ? Colour (0xffff7a73) : kRed) : kFaint);
                    const auto pr = pillRect (it.r);
                    if (doing || queued)
                    {
                        g.setColour (doing ? kOrange : kDim); g.setFont (pf (o.fo.med, 8.6f));
                        g.drawText (doing ? (inst ? "Updating " : "Installing ") + String (roundToInt (o.progress.load() * 100.0f)) + " %" : String ("Queued"),
                                    pr.withTrimmedLeft (-2.0f), Justification::centredLeft, false);
                    }
                    else
                    {
                        const bool on = p.notice.isNotEmpty();
                        g.setColour (on ? ((hot && hotBtn == 3) ? kAccent.brighter (0.2f) : kAccent) : kHover.brighter (0.05f));
                        g.fillRoundedRectangle (pr, 11.0f);
                        g.setColour (on ? kInk : kFaint); g.setFont (pf (o.fo.med, 8.8f));
                        g.drawText ("Notice", pr, Justification::centred);
                    }
                }
            }

            std::pair<int, int> hit (Point<int> pos) const
            {
                for (auto& it : owner->items)
                {
                    if (it.plugin < 0 || ! it.r.contains (pos)) continue;
                    const auto pf = pos.toFloat();
                    for (int k = 0; k < 3; ++k) if (btnCentre (it.r, k).getDistanceFrom (pf) <= 13.0f) return { it.plugin, k };
                    if (pillRect (it.r).contains (pf)) return { it.plugin, 3 };
                    if (starRect (it.r).expanded (2.0f).contains (pf)) return { it.plugin, 4 };
                    return { it.plugin, -1 };
                }
                return { -1, -1 };
            }
            void mouseMove (const MouseEvent& e) override
            {
                const auto h = hit (e.getPosition());
                if (h.first != hover || h.second != hotBtn) { hover = h.first; hotBtn = h.second; repaint(); }
                setMouseCursor (hotBtn >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
            }
            void mouseExit (const MouseEvent&) override { hover = hotBtn = -1; repaint(); }
            void mouseUp (const MouseEvent& e) override
            {
                const auto h = hit (e.getPosition());
                if (h.first < 0) return;
                if (e.mods.isPopupMenu()) { owner->cardMenu (h.first); return; }
                owner->cardAction (h.first, h.second);
            }
        };
        List list;

        //------------------------------------------------------------------ actions
        void cardAction (int i, int btn)
        {
            auto& p = cat.plugins[(size_t) i];
            if (btn == 0 && (! p.isInstalled() || p.hasUpdate()) && current() != p.repo && ! isQueued (p.repo)) enqueue ({ i });
            else if (btn == 2 && p.isInstalled() && current() != p.repo) askUninstall (i);
            else if (btn == 3 && p.notice.isNotEmpty()) URL (p.notice).launchInDefaultBrowser();
            else if (btn == 4)
            {
                if (favorites.contains (p.repo)) favorites.removeString (p.repo); else favorites.add (p.repo);
                dataDir().getChildFile ("Favorites.txt").replaceWithText (favorites.joinIntoString ("\n"));
                if (statusFilter == 4) layoutList();
                repaint(); list.repaint();
            }
        }

        void cardMenu (int i)
        {
            const auto& p = cat.plugins[(size_t) i];
            PopupMenu m; m.setLookAndFeel (&menuLnf);
            const auto* f = cat.family (p.family);
            addHeader (m, menuLnf.f, icoPedal, f != nullptr ? f->colour : kAccent, p.name,
                       p.isInstalled() ? "v" + p.installedVersion + " installed" : "Not installed  -  v" + p.latestVersion());
            const bool busyHere = current() == p.repo || isQueued (p.repo);
            if (! p.isInstalled()) addItem (m, 1, "Install v" + p.latestVersion(), icoLoad, ! busyHere);
            else if (p.hasUpdate()) addItem (m, 1, "Update to v" + p.latestVersion(), icoLoad, ! busyHere);
            else addItem (m, 1, "Reinstall v" + p.latestVersion(), icoLoad, ! busyHere);
            addItem (m, 2, "Uninstall", icoClear, p.isInstalled() && ! busyHere);
            m.addSeparator();
            addItem (m, 3, "Open the notice", icoInfo, p.notice.isNotEmpty());
            addItem (m, 4, "Show in folder", icoLoad, p.isInstalled());
            addItem (m, 5, "Open the GitHub page", icoInfo);
            addItem (m, 6, favorites.contains (p.repo) ? "Remove from favorites" : "Add to favorites", icoPreset);
            m.showMenuAsync (PopupMenu::Options().withMousePosition(), [sp = SafePointer<ManagerView> (this), i] (int r)
            {
                if (sp == nullptr || r <= 0) return;
                auto& q = sp->cat.plugins[(size_t) i];
                if (r == 1) sp->enqueue ({ i });
                else if (r == 2) sp->askUninstall (i);
                else if (r == 3) URL (q.notice).launchInDefaultBrowser();
                else if (r == 4) q.bundle.revealToUser();
                else if (r == 5) URL ("https://github.com/" + kOwner + "/" + q.repo).launchInDefaultBrowser();
                else if (r == 6) sp->cardAction (i, 4);
            });
        }

        void showDialog (const String& title, const String& text, const StringArray& buttons, int primary, int danger, std::function<void (int)> cb)
        {
            dialog = std::make_unique<Dialog>();
            dialog->f = fo; dialog->title = title; dialog->text = text; dialog->buttons = buttons; dialog->primary = primary; dialog->danger = danger;
            dialog->onChoice = [sp = SafePointer<ManagerView> (this), cb] (int c) { if (sp == nullptr) return; sp->dialog.reset(); if (cb) cb (c); };
            addAndMakeVisible (*dialog);
            dialog->setBounds (getLocalBounds());
        }

        // le dossier choisi doit etre accessible en ecriture
        bool checkWritable()
        {
            if (! folder.isDirectory()) { askFolder (false); return false; }
            if (canWrite (folder)) return true;
           #if JUCE_WINDOWS
            showDialog ("Administrator rights needed",
                        "Plugin manager cannot write to " + folder.getFullPathName() + ".\nRestart it as administrator to install or remove plugins in this folder.",
                        { "Cancel", "Restart as administrator" }, 1, -1, [] (int c)
            {
                if (c != 1) return;
                const auto exe = File::getSpecialLocation (File::currentExecutableFile).getFullPathName();
                if (pmplat::relaunchAsAdmin (exe.toWideCharPointer())) JUCEApplication::quit();
            });
           #else
            showDialog ("Folder is read-only",
                        "Plugin manager cannot write to " + folder.getFullPathName() + ".\nChoose a folder you can write to, for example ~/Library/Audio/Plug-Ins/VST3.",
                        { "Cancel", "Change folder..." }, 1, -1, [sp = SafePointer<ManagerView> (this)] (int c) { if (sp != nullptr && c == 1) sp->chooseFolder(); });
           #endif
            return false;
        }

        void enqueue (const std::vector<int>& idx)
        {
            if (idx.empty() || ! checkWritable()) return;
            {
                const ScopedLock sl (qLock);
                for (int i : idx)
                {
                    const auto& p = cat.plugins[(size_t) i];
                    bool dup = p.repo == currentRepo;
                    for (auto& q : queue) dup |= q.repo == p.repo;
                    if (! dup) queue.push_back (p);
                }
            }
            status = {};
            if (! installerRunning)
            {
                if (installer.joinable()) installer.join();
                installerRunning = true;
                installer = std::thread ([this] { installLoop(); });
            }
            repaint(); list.repaint();
        }

        void installLoop()
        {
            const auto dest = folder;
            int done = 0; StringArray errors;
            for (;;)
            {
                Plugin p;
                {
                    const ScopedLock sl (qLock);
                    if (queue.empty() || stopFlag) { currentRepo = {}; break; }
                    p = queue.front(); queue.pop_front();
                    currentRepo = p.repo;
                }
                progress = 0.0f;
                String err;
                const bool ok = pm::install (p, dest, [this] (float f) { progress = f; }, stopFlag, err);
                if (stopFlag) break;
                if (ok) ++done; else errors.add (err);
                MessageManager::callAsync ([sp = SafePointer<ManagerView> (this)] { if (sp != nullptr) sp->rescan(); });
            }
            installerRunning = false;
            MessageManager::callAsync ([sp = SafePointer<ManagerView> (this), done, errors]
            {
                if (sp == nullptr) return;
                sp->rescan();
                if (! errors.isEmpty()) sp->status = "!" + errors[0] + (errors.size() > 1 ? "  (+" + String (errors.size() - 1) + " more)" : String());
                else sp->status = String (done) + (done == 1 ? " plugin installed" : " plugins installed") + " in " + sp->folder.getFullPathName();
                sp->repaint();
                sp->maybeSelfUpdate();
            });
        }

        void askUninstall (int i)
        {
            const auto p = cat.plugins[(size_t) i];
            showDialog ("Uninstall " + p.name + "?", p.bundle.getFullPathName() + "\nwill be moved to the " + String (isMac ? "Trash" : "Recycle Bin") + ".",
                        { "Cancel", "Uninstall" }, -1, 1, [sp = SafePointer<ManagerView> (this), i] (int c)
            {
                if (sp == nullptr || c != 1 || ! sp->checkWritable()) return;
                String err;
                if (! uninstall (sp->cat.plugins[(size_t) i], err)) sp->status = "!" + err;
                else sp->status = sp->cat.plugins[(size_t) i].name + " uninstalled";
                sp->rescan();
            });
        }

        void askFolder (bool firstRun)
        {
            const auto def = defaultFolder();
            showDialog (firstRun ? "Choose your VST3 folder" : "Folder not found",
                        "Plugin manager scans this folder for NINE HANDS | DSP plugins, and installs new plugins and updates in it.\n\n" + def.getFullPathName(),
                        { "Choose another...", "Use this folder" }, 1, -1, [sp = SafePointer<ManagerView> (this), def] (int c)
            {
                if (sp == nullptr) return;
                if (c == 1) sp->setFolder (def);
                else if (c == 0) sp->chooseFolder();
            });
        }

        void chooseFolder()
        {
            chooser = std::make_unique<FileChooser> ("Choose the folder of your VST3 plugins", folder.isDirectory() ? folder : defaultFolder());
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                                  [sp = SafePointer<ManagerView> (this)] (const FileChooser& fc)
            {
                if (sp == nullptr) return;
                const auto f = fc.getResult();
                if (f.isDirectory()) sp->setFolder (f);
            });
        }

        void setFolder (const File& f)
        {
            folder = f;
            if (! folder.isDirectory()) folder.createDirectory();
            settings().setValue ("folder", folder.getFullPathName());
            settings().saveIfNeeded();
            status = {};
            rescan();
            if (checkedAt.toMilliseconds() == 0 && ! checking) refresh();
        }

        void rescan() { scan (cat, folder); layoutList(); resized(); repaint(); }

        // liste en ligne, visuels manquants, dernieres versions GitHub (en tache de fond)
        void refresh()
        {
            if (checking) return;
            checking = true; status = {};
            if (checker.joinable()) checker.join();
            StringArray repos; for (auto& p : cat.plugins) repos.add (p.repo);
            SafePointer<ManagerView> self (this);
            checker = std::thread ([this, self, repos]
            {
                String json; StringArray names = repos;
                {
                    const auto t = Net::getText (kRaw + "plugins.json");
                    Catalog c;
                    if (t.isNotEmpty() && c.parse (t))
                    {
                        json = t; names.clear();
                        for (auto& p : c.plugins)
                        {
                            names.add (p.repo);
                            const auto img = Catalog::imageCache().getChildFile (p.repo + ".jpg");
                            int n = 0;
                            if (! stopFlag && ! img.existsAsFile() && resource (p.repo + ".jpg", n) == nullptr)
                                Net::getFile (kRaw + "img/" + URL::addEscapeChars (p.repo, false) + ".jpg", img);
                        }
                    }
                }
                std::map<String, String> tags;
                CriticalSection tl;
                {
                    ThreadPool pool (8);
                    for (auto& r : names)
                        pool.addJob ([this, r, &tags, &tl]
                        {
                            if (stopFlag) return;
                            const auto v = Net::latestTag (r);
                            const ScopedLock sl (tl); tags[r] = v;
                        });
                    while (pool.getNumJobs() > 0) Thread::sleep (30);
                }
                const auto app = stopFlag ? String() : Net::latestTag (kAppRepo);
                MessageManager::callAsync ([sp = self, json, tags, app]
                {
                    if (sp == nullptr) return;
                    if (json.isNotEmpty() && json != Catalog::cacheFile().loadFileAsString())
                    {
                        Catalog::cacheFile().replaceWithText (json);
                        sp->cat.loadLocal();
                    }
                    int ok = 0;
                    for (auto& p : sp->cat.plugins)
                        if (auto it = tags.find (p.repo); it != tags.end() && it->second.isNotEmpty()) { p.latest = it->second; ++ok; }
                    sp->appLatest = app;
                    sp->checking = false;
                    sp->checkedAt = Time::getCurrentTime();
                    if (ok == 0 && ! tags.empty()) sp->status = "!GitHub cannot be reached: versions from the built-in list.";
                    sp->rescan();
                    sp->maybeSelfUpdate();
                });
            });
        }

        //------------------------------------------------------------------ souris (en-tete et pied)
        int headerHit (Point<float> p) const
        {
            const auto ss = statusSegs();
            for (int i = 0; i < (int) ss.size(); ++i) if (ss[(size_t) i].second.contains (p)) return i;
            const auto ts = typeSegs();
            for (int i = 0; i < (int) ts.size(); ++i) if (ts[(size_t) i].second.contains (p)) return 10 + i;
            if (folderRect().toFloat().contains (p)) return 100;
            for (auto& b : footerButtons()) if (b.r.contains (p)) return 200 + b.id;
            if (appUpdateRect().contains (p)) return 300;
            return -1;
        }
        void mouseMove (const MouseEvent& e) override
        {
            const int h = headerHit (e.position);
            if (h != hotHeader) { hotHeader = h; repaint(); }
            setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        }
        void mouseExit (const MouseEvent&) override { hotHeader = -1; repaint(); }
        void mouseUp (const MouseEvent& e) override
        {
            const int h = headerHit (e.position);
            if (h >= 0 && h < 10) { statusFilter = h; settings().setValue ("statusFilter", h); }
            else if (h >= 10 && h < 20) { typeFilter = h - 10; settings().setValue ("typeFilter", h - 10); }
            else if (h == 100) chooseFolder();
            else if (h == 201) enqueue (visibleWith ([] (const Plugin& p) { return ! p.isInstalled(); }));
            else if (h == 202) enqueue (visibleWith ([] (const Plugin& p) { return p.hasUpdate(); }));
            else if (h == 203) { if (! checking) { rescan(); refresh(); } }
            else if (h == 204) JUCEApplication::getInstance()->systemRequestedQuit();
            else if (h == 300 && ! appUpdating) { appUpdateFailed = false; maybeSelfUpdate(); }
            if (h >= 0 && h < 20) { settings().saveIfNeeded(); layoutList(); viewport.setViewPosition (0, 0); repaint(); }
        }

        void timerCallback() override
        {
            if (current().isNotEmpty() || checking || appUpdating) { repaint (0, getHeight() - footerH, getWidth(), footerH); list.repaint(); }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManagerView)
    };
}
