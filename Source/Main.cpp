#include "ManagerView.h"

// Backline (NINE HANDS | DSP) : application autonome Windows / macOS
class PluginManagerApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Backline"; }
    const juce::String getApplicationVersion() override { return APP_VERSION; }
    // une seule fenetre, sauf a la reouverture qui suit une mise a jour (l'ancienne version se ferme a ce moment-la)
    bool moreThanOneInstanceAllowed() override { return getCommandLineParameters().contains ("--after-update"); }

    void initialise (const juce::String& cmd) override
    {
       #if JUCE_LINUX
        // tests sous Linux : --snapshot image.png [--folder dossier] (capture de la fenetre sans l'afficher)
        if (cmd.contains ("--snapshot"))
        {
            auto args = juce::StringArray::fromTokens (cmd, true);
            const juce::File out (args[args.indexOf ("--snapshot") + 1].unquoted());
            auto* v = new pm::ManagerView();
            if (args.contains ("--folder")) v->useFolder (juce::File (args[args.indexOf ("--folder") + 1].unquoted()));
            v->setSize (1440, 900);
            if (args.contains ("--install")) v->testInstall (args[args.indexOf ("--install") + 1].unquoted());
            snapshot.reset (v);
            juce::Timer::callAfterDelay (2500, [this, out]
            {
                juce::PNGImageFormat png; juce::FileOutputStream os (out); os.setPosition (0); os.truncate();
                png.writeImageToStream (snapshot->createComponentSnapshot (snapshot->getLocalBounds()), os);
                os.flush(); quit();
            });
            return;
        }
       #endif
        juce::ignoreUnused (cmd);
        window = std::make_unique<MainWindow>();
    }
    void shutdown() override { window = nullptr; snapshot = nullptr; }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted (const juce::String&) override { if (window != nullptr) window->toFront (true); }

    struct MainWindow : public juce::DocumentWindow
    {
        MainWindow() : DocumentWindow ("Backline", juce::Colour (0xff13171d), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new pm::ManagerView(), true);
            setResizable (true, true);
            setResizeLimits (1040, 680, 10000, 10000);
            const auto area = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay() != nullptr
                                ? juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds.toNearestInt() : juce::Rectangle<int> (0, 0, 1440, 900);
            centreWithSize (juce::jmin (1440, area.getWidth() - 40), juce::jmin (900, area.getHeight() - 40));
            setVisible (true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };

private:
    std::unique_ptr<MainWindow> window;
    std::unique_ptr<juce::Component> snapshot;
};

START_JUCE_APPLICATION (PluginManagerApp)
