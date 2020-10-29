#include "gdfreeorion.h"
#include <chrono>
#include <atomic>
#include <memory>

#include "../util/Directories.h"
#include "../util/OptionsDB.h"
#include "../util/Version.h"
#include "../client/ClientNetworking.h"

#include "GodotClientApp.h"
#include "OptionsDB.h"

using namespace godot;

std::atomic_bool quit(false);

void GDFreeOrion::do_the_ping(GDFreeOrion* n) {
    while(!quit) {
        if (auto msg = n->app->Networking().GetMessage())
            n->emit_signal("ping");
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void GDFreeOrion::_register_methods() {
    // register_method("_process", &GDCppTest::_process);
    register_method("_exit_tree", &GDFreeOrion::_exit_tree);
    register_signal<GDFreeOrion>((char *)"ping");
    register_property<GDFreeOrion, godot::OptionsDB*>("optionsDB",
        &GDFreeOrion::set_options,
        &GDFreeOrion::get_options,
        nullptr);
    register_property<GDFreeOrion, GodotNetworking*>("networking",
        &GDFreeOrion::set_networking,
        &GDFreeOrion::get_networking,
        nullptr);
}

GDFreeOrion::GDFreeOrion() {
}

GDFreeOrion::~GDFreeOrion() {
    // add your cleanup here
}

void GDFreeOrion::_init() {
    InitDirs("");

    GetOptionsDB().SetFromFile(GetConfigPath(), FreeOrionVersionString());
    GetOptionsDB().SetFromFile(GetPersistentConfigPath());

    CompleteXDGMigration();

    // Handle the case where the resource.path does not exist anymore
    // gracefully by resetting it to the standard path into the
    // application bundle.  This may happen if a previous installed
    // version of FreeOrion was residing in a different directory.
    if (!boost::filesystem::exists(GetResourceDir()) ||
        !boost::filesystem::exists(GetResourceDir() / "credits.xml") ||
        !boost::filesystem::exists(GetResourceDir() / "data" / "art" / "misc" / "missing.png"))
    {
        DebugLogger() << "Resources directory from config.xml missing or does not contain expected files. Resetting to default.";

        GetOptionsDB().Set<std::string>("resource.path", "");

        // double-check that resetting actually fixed things...
        if (!boost::filesystem::exists(GetResourceDir()) ||
            !boost::filesystem::exists(GetResourceDir() / "credits.xml") ||
            !boost::filesystem::exists(GetResourceDir() / "data" / "art" / "misc" / "missing.png"))
        {
            DebugLogger() << "Default Resources directory missing or does not contain expected files. Cannot start game.";
            throw std::runtime_error("Unable to load game resources at default location: " +
                                     PathToString(GetResourceDir()) + " : Install may be broken.");
        }
    }

    // initialize any variables here
    app = std::make_unique<GodotClientApp>();
    optionsDB = godot::OptionsDB::_new();
    networking = GodotNetworking::_new();

    t = std::thread(do_the_ping, this);
}

void GDFreeOrion::_process(float delta) {
}

void GDFreeOrion::_exit_tree() {
    quit = true;
    t.join();
    app.reset();

    networking->free();
    optionsDB->free();

    ShutdownLoggingSystemFileSink();
}

godot::OptionsDB* GDFreeOrion::get_options() const
{ return optionsDB; }

void GDFreeOrion::set_options(godot::OptionsDB* ptr) {
    // ignore it
    Godot::print(String("Ignore setting options"));
}

GodotNetworking* GDFreeOrion::get_networking() const
{ return networking; }

void GDFreeOrion::set_networking(GodotNetworking* ptr) {
    // ignore it
    Godot::print(String("Ignore setting networking"));
}

