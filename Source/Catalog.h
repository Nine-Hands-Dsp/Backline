#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace pmplat { std::wstring fileVersion (const std::wstring&); bool relaunchAsAdmin (const std::wstring&); }

//==============================================================================
// Donnees de Backline : liste des plugins (catalog/plugins.json, copie de celle du depot Download),
// dernieres versions publiees sur GitHub, plugins trouves dans le dossier choisi, installation.
//==============================================================================
namespace pm
{
    using namespace juce;

    const String kOwner = "Nine-Hands-Dsp";
    const String kAppRepo = "Backline";
    const String kRaw = "https://raw.githubusercontent.com/Nine-Hands-Dsp/Backline/main/catalog/";

   #if JUCE_MAC
    constexpr bool isMac = true;
   #else
    constexpr bool isMac = false;
   #endif

    //------------------------------------------------------------------ dossiers et reglages
    inline File dataDir()
    {
       #if JUCE_MAC
        auto d = File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Application Support");
       #else
        auto d = File::getSpecialLocation (File::userApplicationDataDirectory);
       #endif
        d = d.getChildFile ("NINE HANDS DSP");
        const auto old = d.getChildFile ("Plugin manager");     // dossier de l'ancien nom de l'application
        d = d.getChildFile ("Backline");
        if (! d.isDirectory() && old.isDirectory()) old.copyDirectoryTo (d);
        d.createDirectory();
        return d;
    }

    inline File defaultFolder()
    {
       #if JUCE_MAC
        return File::getSpecialLocation (File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/VST3");
       #elif JUCE_WINDOWS
        return File (SystemStats::getEnvironmentVariable ("CommonProgramFiles", "C:\\Program Files\\Common Files")).getChildFile ("VST3");
       #else
        return File::getSpecialLocation (File::userHomeDirectory).getChildFile (".vst3");
       #endif
    }

    inline PropertiesFile& settings()
    {
        static PropertiesFile p (dataDir().getChildFile ("Settings.xml"), PropertiesFile::Options());
        return p;
    }

    // ressource integree d'apres son nom de fichier d'origine
    inline const char* resource (const String& fileName, int& size)
    {
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            if (fileName == BinaryData::originalFilenames[i])
                return BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        size = 0;
        return nullptr;
    }

    //------------------------------------------------------------------ versions
    inline String cleanVersion (String v)
    {
        v = v.trim().trimCharactersAtStart ("vV");
        String out;
        for (auto c : v) { if (CharacterFunctions::isDigit (c) || c == '.') out << String::charToString (c); else break; }
        return out.trimCharactersAtEnd (".");
    }
    inline int compareVersions (const String& a, const String& b)
    {
        auto A = StringArray::fromTokens (cleanVersion (a), ".", ""), B = StringArray::fromTokens (cleanVersion (b), ".", "");
        for (int i = 0; i < jmax (A.size(), B.size()); ++i)
        {
            const int x = i < A.size() ? A[i].getIntValue() : 0, y = i < B.size() ? B[i].getIntValue() : 0;
            if (x != y) return x < y ? -1 : 1;
        }
        return 0;
    }

    //------------------------------------------------------------------ catalogue
    struct Family { String id, title; Colour colour; };

    struct Plugin
    {
        String name, catalogVersion, family, desc, repo, urlWin, urlMac, notice, type, released, imgHash;
        String latest;              // version de la derniere Release GitHub (vide tant qu'elle n'est pas connue)
        String installedVersion;    // vide : pas installe
        File bundle;                // .vst3 trouve dans le dossier
        Image image;

        bool isInstalled() const { return bundle.exists(); }
        String latestVersion() const
        {
            if (latest.isEmpty()) return catalogVersion;
            if (catalogVersion.isEmpty()) return latest;
            return compareVersions (latest, catalogVersion) >= 0 ? latest : catalogVersion; // jamais en dessous du catalogue
        }
        bool hasUpdate() const
        {
            return isInstalled() && installedVersion.isNotEmpty() && compareVersions (installedVersion, latestVersion()) < 0;
        }
        String downloadUrl() const { return isMac ? urlMac : urlWin; }
        bool isNew() const
        {
            if (released.isEmpty()) return false;
            const auto t = Time::fromISO8601 (released);
            return t.toMilliseconds() > 0 && (Time::getCurrentTime() - t).inHours() < 48.0;
        }
    };

    struct Catalog
    {
        String date;
        std::vector<Family> families;
        std::vector<Plugin> plugins;

        static File cacheFile() { return dataDir().getChildFile ("plugins.json"); }
        static File imageCache() { auto d = dataDir().getChildFile ("img"); d.createDirectory(); return d; }

        bool parse (const String& json)
        {
            auto v = JSON::parse (json);
            auto* ps = v.getProperty ("plugins", var()).getArray();
            if (ps == nullptr || ps->isEmpty()) return false;
            date = v.getProperty ("date", "").toString();
            families.clear(); plugins.clear();
            if (auto* fs = v.getProperty ("familles", var()).getArray())
                for (auto& f : *fs)
                    families.push_back ({ f["id"].toString(), f["titre"].toString(), Colour::fromString ("ff" + f["couleur"].toString().trimCharactersAtStart ("#")) });
            for (auto& p : *ps)
            {
                Plugin e;
                e.name = p["nom"].toString(); e.catalogVersion = p["version"].toString(); e.family = p["famille"].toString();
                e.desc = p["description"].toString(); e.repo = p["depot"].toString(); e.urlWin = p["windows"].toString();
                e.urlMac = p["macos"].toString(); e.notice = p["notice"].toString(); e.type = p.getProperty ("type", "guitare").toString();
                e.released = p.getProperty ("sortie", "").toString();
                e.imgHash = p.getProperty ("imgHash", "").toString();     // empreinte de la vignette (tools/sync_catalog.py)
                if (e.name.isNotEmpty() && e.repo.isNotEmpty()) plugins.push_back (e);
            }
            return true;
        }

        // liste integree a l'application, remplacee par la derniere liste telechargee si elle existe
        void loadLocal()
        {
            const auto cached = cacheFile().loadFileAsString();
            if (cached.isEmpty() || ! parse (cached))
            {
                int n = 0;
                if (auto* d = resource ("plugins.json", n)) parse (String::fromUTF8 (d, n));
            }
            for (auto& p : plugins) p.image = loadImage (p.repo);
        }

        static Image loadImage (const String& repo)
        {
            const auto f = imageCache().getChildFile (repo + ".jpg");
            if (f.existsAsFile()) if (auto im = ImageFileFormat::loadFrom (f); im.isValid()) return im;
            int n = 0;
            if (auto* d = resource (repo + ".jpg", n)) return ImageFileFormat::loadFrom (d, (size_t) n);
            return {};
        }

        const Family* family (const String& id) const
        {
            for (auto& f : families) if (f.id == id) return &f;
            return nullptr;
        }
    };

    //------------------------------------------------------------------ reseau
    namespace Net
    {
        inline URL::InputStreamOptions opts (int redirects, int timeoutMs, StringPairArray* headers = nullptr, int* status = nullptr)
        {
            return URL::InputStreamOptions (URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs (timeoutMs).withNumRedirectsToFollow (redirects)
                       .withExtraHeaders ("User-Agent: NineHandsBackline/" APP_VERSION "\r\nCache-Control: no-cache\r\nPragma: no-cache\r\n")
                       .withResponseHeaders (headers).withStatusCode (status);
        }

        inline String noCache (const String& url) { return url + (url.contains ("?") ? "&" : "?") + "nocache=" + String (Time::currentTimeMillis()); }

        inline String getText (const String& url, int timeoutMs = 12000)
        {
            int status = 0;
            auto s = URL (url).createInputStream (opts (5, timeoutMs, nullptr, &status));
            if (s == nullptr || status >= 400) return {};
            return s->readEntireStreamAsString();
        }

        inline bool getFile (const String& url, const File& dest, int timeoutMs = 15000)
        {
            int status = 0;
            auto s = URL (url).createInputStream (opts (5, timeoutMs, nullptr, &status));
            if (s == nullptr || status >= 400) return false;
            MemoryBlock mb;
            s->readIntoMemoryBlock (mb);
            return mb.getSize() > 0 && dest.replaceWithData (mb.getData(), mb.getSize());
        }

        // derniere version publiee : la page releases/latest redirige vers .../releases/tag/v1.2.3 (pas de limite d'API)
        inline String latestTag (const String& repo)
        {
            StringPairArray headers; int status = 0;
            auto s = URL (noCache ("https://github.com/" + kOwner + "/" + repo + "/releases/latest")).createInputStream (opts (0, 10000, &headers, &status));
            String loc;
            for (auto& k : headers.getAllKeys()) if (k.equalsIgnoreCase ("location")) loc = headers[k];
            if (loc.contains ("/tag/")) return cleanVersion (URL::removeEscapeChars (loc.fromLastOccurrenceOf ("/tag/", false, false)));
            // sinon : flux Atom des Releases (pas de limite d'API), premiere entree = plus recente
            {
                const auto atom = getText (noCache ("https://github.com/" + kOwner + "/" + repo + "/releases.atom"), 10000);
                if (atom.contains ("/releases/tag/"))
                {
                    auto t = atom.fromFirstOccurrenceOf ("/releases/tag/", false, false).upToFirstOccurrenceOf ("\"", false, false);
                    t = cleanVersion (URL::removeEscapeChars (t));
                    if (t.isNotEmpty()) return t;
                }
            }
            // dernier recours : API GitHub (limitee a 60 requetes par heure)
            const auto j = JSON::parse (getText (noCache ("https://api.github.com/repos/" + kOwner + "/" + repo + "/releases/latest"), 10000));
            return cleanVersion (j.getProperty ("tag_name", "").toString());
        }
    }

    //------------------------------------------------------------------ plugins trouves dans le dossier
    struct Found { File bundle; String name, vendor, version; };

    inline String jsonValue (const String& text, const String& key)
    {
        const int k = text.indexOf ("\"" + key + "\"");
        if (k < 0) return {};
        const int c = text.indexOfChar (k + key.length() + 2, ':');
        const int q = text.indexOfChar (c, '"');
        if (c < 0 || q < 0) return {};
        return text.substring (q + 1, text.indexOfChar (q + 1, '"'));
    }
    inline String plistValue (const String& text, const String& key)
    {
        const int k = text.indexOf ("<key>" + key + "</key>");
        if (k < 0) return {};
        return text.substring (k).fromFirstOccurrenceOf ("<string>", false, false).upToFirstOccurrenceOf ("</string>", false, false).trim();
    }

    inline Found readBundle (const File& b)
    {
        Found f; f.bundle = b; f.name = b.getFileNameWithoutExtension();
        const auto mi = b.getChildFile ("Contents/Resources/moduleinfo.json");
        if (mi.existsAsFile())
        {
            const auto t = mi.loadFileAsString();
            f.version = cleanVersion (jsonValue (t, "Version"));
            if (auto n = jsonValue (t, "Name"); n.isNotEmpty()) f.name = n;
            f.vendor = jsonValue (t, "Vendor");
        }
        if (f.version.isEmpty())
        {
            const auto pl = b.getChildFile ("Contents/Info.plist");
            if (pl.existsAsFile()) f.version = cleanVersion (plistValue (pl.loadFileAsString(), "CFBundleShortVersionString"));
        }
       #if JUCE_WINDOWS
        if (f.version.isEmpty())
        {
            File bin = b.existsAsFile() ? b : File();
            if (! bin.exists())
                for (auto& x : b.getChildFile ("Contents/x86_64-win").findChildFiles (File::findFiles, false, "*.vst3")) { bin = x; break; }
            if (bin.existsAsFile()) f.version = cleanVersion (String (pmplat::fileVersion (bin.getFullPathName().toWideCharPointer()).c_str()));
        }
       #endif
        return f;
    }

    inline void findBundles (const File& dir, Array<File>& out, int depth)
    {
        for (auto& c : dir.findChildFiles (File::findFilesAndDirectories | File::ignoreHiddenFiles, false))
        {
            if (c.hasFileExtension ("vst3")) out.add (c);
            else if (c.isDirectory() && depth > 0) findBundles (c, out, depth - 1);
        }
    }

    inline String normalise (const String& s)
    {
        String o;
        for (auto c : s.toLowerCase()) if (CharacterFunctions::isLetterOrDigit (c)) o << String::charToString (c);
        return o;
    }

    // plugins installes par l'application : depot -> chemin du .vst3 et version
    struct Manifest
    {
        static File file() { return dataDir().getChildFile ("Installed.json"); }
        static var load() { auto v = JSON::parse (file()); return v.isObject() ? v : var (new DynamicObject()); }
        // version : celle de la Release installee ; reported : celle que le plugin annonce lui-meme (parfois en retard d'un numero)
        static void set (const String& repo, const File& bundle, const String& version, const String& reported)
        {
            auto v = load();
            auto* o = new DynamicObject(); o->setProperty ("bundle", bundle.getFullPathName()); o->setProperty ("version", version);
            o->setProperty ("reported", reported);
            v.getDynamicObject()->setProperty (repo, var (o));
            file().replaceWithText (JSON::toString (v));
        }
        static void remove (const String& repo)
        {
            auto v = load(); v.getDynamicObject()->removeProperty (repo);
            file().replaceWithText (JSON::toString (v));
        }
    };

    // associe les .vst3 du dossier aux plugins du catalogue
    inline void scan (Catalog& cat, const File& folder)
    {
        Array<File> bundles;
        if (folder.isDirectory()) findBundles (folder, bundles, 3);
        std::vector<Found> found;
        for (auto& b : bundles)
        {
            auto f = readBundle (b);
            const auto v = f.vendor.toLowerCase();
            if (v.isNotEmpty() && ! v.contains ("nine hands") && ! v.contains ("wiga")) continue;   // plugin d'un autre fabricant
            found.push_back (f);
        }
        const auto man = Manifest::load();
        for (auto& p : cat.plugins)
        {
            p.bundle = File(); p.installedVersion = {};
            const auto keys = StringArray { normalise (p.name), normalise (p.repo) };
            for (auto& f : found)
            {
                if (! keys.contains (normalise (f.name)) && ! keys.contains (normalise (f.bundle.getFileNameWithoutExtension()))) continue;
                if (! p.bundle.exists() || compareVersions (f.version, p.installedVersion) > 0) { p.bundle = f.bundle; p.installedVersion = f.version; }
            }
            // installe par l'application : la version de la Release installee fait foi tant que le plugin n'a pas change
            // (certains plugins annoncent un numero de moins que leur Release : sans cela, la mise a jour serait proposee sans fin)
            const auto m = man.getProperty (p.repo, var());
            if (m.isObject())
            {
                const File b (m["bundle"].toString());
                if (! p.bundle.exists() && b.exists() && b.isAChildOf (folder)) p.bundle = b;
                if (p.bundle == b && (p.installedVersion.isEmpty() || p.installedVersion == m["reported"].toString()))
                    p.installedVersion = m["version"].toString();
            }
        }
    }

    //------------------------------------------------------------------ installation
    inline bool canWrite (const File& folder)
    {
        if (! folder.isDirectory() && ! folder.createDirectory()) return false;
        const auto t = folder.getNonexistentChildFile (".nh-write-test", ".tmp", false);
        if (! t.create()) return false;
        t.deleteFile();
        return true;
    }

    inline File firstBundle (const File& dir, int depth = 3)
    {
        for (auto& c : dir.findChildFiles (File::findFilesAndDirectories, false))
        {
            if (c.getFileName().startsWith ("__MACOSX")) continue;
            if (c.hasFileExtension ("vst3")) return c;
            if (c.isDirectory() && depth > 0) if (auto r = firstBundle (c, depth - 1); r.exists()) return r;
        }
        return {};
    }

    // telecharge la derniere version et la copie dans le dossier (remplace l'ancienne) ; progress de 0 a 1
    inline bool install (const Plugin& p, const File& folder, std::function<void (float)> progress, const std::atomic<bool>& stop, String& err)
    {
        const auto url = p.downloadUrl();
        if (url.isEmpty()) { err = "No " + String (isMac ? "macOS" : "Windows") + " download for " + p.name + "."; return false; }
        auto tmp = dataDir().getChildFile ("Temp");
        tmp.deleteRecursively(); tmp.createDirectory();
        const auto zip = tmp.getChildFile (p.repo + ".zip");
        {
            int status = 0;
            auto s = URL (url).createInputStream (Net::opts (10, 20000, nullptr, &status));
            if (s == nullptr || status >= 400) { err = "Download failed (" + p.name + (status > 0 ? ", HTTP " + String (status) : String()) + ")."; return false; }
            FileOutputStream out (zip);
            if (! out.openedOk()) { err = "Cannot write the temporary file."; return false; }
            const auto total = s->getTotalLength();
            HeapBlock<char> buf (65536);
            int64 got = 0;
            while (! s->isExhausted())
            {
                if (stop) return false;
                const int n = s->read (buf, 65536);
                if (n <= 0) break;
                out.write (buf, (size_t) n); got += n;
                if (total > 0) progress (0.85f * (float) got / (float) total);
            }
            out.flush();
            if (got == 0) { err = "Empty download (" + p.name + ")."; return false; }
        }
        progress (0.88f);
        const auto stage = tmp.getChildFile ("unzip");
        stage.createDirectory();
       #if JUCE_MAC
        {
            ChildProcess cp;
            if (! cp.start (StringArray { "/usr/bin/ditto", "-x", "-k", zip.getFullPathName(), stage.getFullPathName() }) || ! cp.waitForProcessToFinish (180000) || cp.getExitCode() != 0)
            { err = "Cannot unzip " + p.name + "."; return false; }
        }
       #else
        {
            ZipFile z (zip);
            if (z.getNumEntries() == 0 || z.uncompressTo (stage, true).failed()) { err = "Cannot unzip " + p.name + "."; return false; }
        }
       #endif
        const auto fresh = firstBundle (stage);
        if (! fresh.exists()) { err = "No .vst3 found in the " + p.name + " download."; return false; }
        progress (0.93f);
        // ancienne version retiree (dossier choisi), puis nouvelle copiee a la place
        const auto dest = (p.bundle.exists() && p.bundle.getFileName() == fresh.getFileName()) ? p.bundle : folder.getChildFile (fresh.getFileName());
        for (auto old : { p.bundle, dest })
            if (old.exists() && ! old.deleteRecursively())
            { err = p.name + " is in use: close your DAW, then try again."; return false; }
        bool ok = fresh.moveFileTo (dest);
        if (! ok) ok = fresh.isDirectory() ? fresh.copyDirectoryTo (dest) : fresh.copyFileTo (dest);
        if (! ok) { err = "Cannot copy " + p.name + " to the folder."; return false; }
       #if JUCE_MAC
        { ChildProcess cp; if (cp.start (StringArray { "/usr/bin/xattr", "-cr", dest.getFullPathName() })) cp.waitForProcessToFinish (30000); }
       #endif
        Manifest::set (p.repo, dest, p.latestVersion(), readBundle (dest).version);
        tmp.deleteRecursively();
        progress (1.0f);
        return true;
    }

    //------------------------------------------------------------------ mise a jour de l'application elle-meme
    // telecharge la derniere version de Backline, remplace l'application en place et renvoie ce qu'il faut relancer
    inline bool selfUpdate (std::function<void (float)> progress, const std::atomic<bool>& stop, String& err, File& relaunch)
    {
        const String url = "https://github.com/" + kOwner + "/" + kAppRepo + "/releases/latest/download/Backline-"
                         + String (isMac ? "macOS" : "Windows") + ".zip";
        auto tmp = dataDir().getChildFile ("AppUpdate");
        tmp.deleteRecursively(); tmp.createDirectory();
        const auto zip = tmp.getChildFile ("update.zip");
        {
            int status = 0;
            auto s = URL (url).createInputStream (Net::opts (10, 20000, nullptr, &status));
            if (s == nullptr || status >= 400) { err = "Backline update: download failed" + (status > 0 ? " (HTTP " + String (status) + ")" : String()) + "."; return false; }
            FileOutputStream out (zip);
            if (! out.openedOk()) { err = "Backline update: cannot write the temporary file."; return false; }
            const auto total = s->getTotalLength();
            HeapBlock<char> buf (65536);
            int64 got = 0;
            while (! s->isExhausted())
            {
                if (stop) return false;
                const int n = s->read (buf, 65536);
                if (n <= 0) break;
                out.write (buf, (size_t) n); got += n;
                if (total > 0) progress (0.9f * (float) got / (float) total);
            }
            out.flush();
            if (got == 0) { err = "Backline update: empty download."; return false; }
        }
        const auto stage = tmp.getChildFile ("unzip");
        stage.createDirectory();
       #if JUCE_MAC
        auto run = [] (const StringArray& args) { ChildProcess cp; return cp.start (args) && cp.waitForProcessToFinish (180000) && cp.getExitCode() == 0; };
        if (! run ({ "/usr/bin/ditto", "-x", "-k", zip.getFullPathName(), stage.getFullPathName() })) { err = "Backline update: cannot unzip."; return false; }
        File fresh;
        for (auto& c : stage.findChildFiles (File::findDirectories, false, "*.app")) { fresh = c; break; }
        const auto app = File::getSpecialLocation (File::currentApplicationFile);
        if (! fresh.isDirectory() || ! app.hasFileExtension ("app")) { err = "Backline update: no application in the download."; return false; }
        const auto backup = tmp.getChildFile ("previous.app");
        // l'application en cours est mise de cote, la nouvelle prend sa place
        if (! run ({ "/bin/mv", app.getFullPathName(), backup.getFullPathName() })) { err = "Backline update: cannot replace " + app.getFullPathName() + "."; return false; }
        if (! run ({ "/bin/mv", fresh.getFullPathName(), app.getFullPathName() }))
        {
            run ({ "/bin/mv", backup.getFullPathName(), app.getFullPathName() });
            err = "Backline update: cannot install the new version."; return false;
        }
        run ({ "/usr/bin/xattr", "-cr", app.getFullPathName() });
        relaunch = app;
       #elif JUCE_WINDOWS
        {
            ZipFile z (zip);
            if (z.getNumEntries() == 0 || z.uncompressTo (stage, true).failed()) { err = "Backline update: cannot unzip."; return false; }
        }
        File fresh;
        for (auto& c : stage.findChildFiles (File::findFiles, true, "*.exe")) { fresh = c; break; }
        const auto exe = File::getSpecialLocation (File::currentExecutableFile);
        if (! fresh.existsAsFile()) { err = "Backline update: no application in the download."; return false; }
        // un .exe en cours d'execution ne peut pas etre ecrase, mais il peut etre renomme
        const auto old = exe.getSiblingFile (exe.getFileName() + ".old");
        old.deleteFile();
        if (! exe.moveFileTo (old)) { err = "Backline update: cannot replace " + exe.getFullPathName() + "."; return false; }
        if (! fresh.copyFileTo (exe)) { old.moveFileTo (exe); err = "Backline update: cannot install the new version."; return false; }
        relaunch = exe;
       #else
        ignoreUnused (relaunch, stage);
        err = "Self-update is not available on this system."; return false;
       #endif
        progress (1.0f);
        return true;
    }

    // relance la nouvelle version (l'appelant quitte ensuite l'application)
    inline bool relaunchApp (const File& target)
    {
       #if JUCE_MAC
        ChildProcess cp;
        return cp.start (StringArray { "/usr/bin/open", "-n", target.getFullPathName(), "--args", "--after-update" }) && cp.waitForProcessToFinish (20000);
       #else
        return target.startAsProcess ("--after-update");
       #endif
    }

    // au demarrage : restes de la mise a jour precedente
    inline void cleanAfterUpdate()
    {
       #if JUCE_WINDOWS
        const auto exe = File::getSpecialLocation (File::currentExecutableFile);
        exe.getSiblingFile (exe.getFileName() + ".old").deleteFile();
       #endif
        dataDir().getChildFile ("AppUpdate").deleteRecursively();
    }

    inline bool uninstall (const Plugin& p, String& err)
    {
        if (! p.bundle.exists()) return true;
        if (! p.bundle.moveToTrash() && ! p.bundle.deleteRecursively()) { err = p.name + " is in use: close your DAW, then try again."; return false; }
        Manifest::remove (p.repo);
        return true;
    }
}
