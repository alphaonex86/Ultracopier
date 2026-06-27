// In-process, deterministic unit test of the Oxygen theme's end-of-copy WINDOW decision
// (interface.cpp Themes::actionInProgess(Idle) + the comboBox_copyEnd option). It instantiates
// the REAL Themes widget, drives it to "copy started -> (maybe error) -> idle/ended", and
// asserts whether the theme emits cancel() (which closes the copy window) exactly per the
// DOCUMENTED comboBox_copyEnd index meanings (interface.cpp setItemText 0/1/2):
//
//   index 0  "Don't close if errors are found":  success -> close,  error -> stay open
//   index 1  "Never close":                      success -> stay,   error -> stay open
//   index 2  "Always close":                     success -> close,  error -> close
//
// No real copy, no disk, no GUI mapping -> runs in milliseconds.
//
// Why a unit test and not black-box window observation: under the headless test mode (offscreen
// QPA, CLI `cp`) the copy window is never shown as a visible top-level widget (verified -- only
// the engine's option sub-dialogs appear), so "did the window open/close" cannot be observed
// from outside the process. The decision that comboBox_copyEnd governs CAN be exercised directly
// here, and end-to-end copy correctness is already covered by the black-box copy/faulty cases.
#include "interface.h"
#include "../../../../../interface/PluginInterface_Themes.h"
#include "../../../../../interface/FacilityInterface.h"
#include <QApplication>
#include <QColor>
#include <QSignalSpy>
#include <cstdio>
#include <string>
#include <vector>

// Minimal FacilityInterface the Themes ctor + Idle handler need (sizeToString / translateText /
// secondsToTimeDecomposition are touched while building the "Completed in ..." label).
class StubFacility : public FacilityInterface {
public:
    void retranslate() override {}
    std::string sizeToString(const double &) const override { return std::string(); }
    std::string sizeUnitToString(const Ultracopier::SizeUnit &) const override { return std::string(); }
    std::string translateText(const std::string &t) const override { return t; }
    std::string speedToString(const double &) const override { return std::string(); }
    Ultracopier::TimeDecomposition secondsToTimeDecomposition(const uint32_t &) const override { return Ultracopier::TimeDecomposition(); }
    bool haveFunctionality(const std::string &) const override { return false; }
    std::string callFunctionality(const std::string &, const std::vector<std::string> &) override { return std::string(); }
    std::string simplifiedRemainingTime(const uint32_t &) const override { return std::string(); }
    std::string ultimateUrl() const override { return std::string(); }
    std::string softwareName() const override { return std::string("test"); }
    bool isUltimate() const override { return false; }
    bool isIllegal() const override { return false; }
    void *prepareOpusAudio(const std::string &, QBuffer &) const override { return nullptr; }
};

static int g_checks = 0, g_fail = 0;
static void check(bool cond, const std::string &msg) {
    ++g_checks;
    if (!cond) { ++g_fail; std::fprintf(stderr, "  FAIL: %s\n", msg.c_str()); }
}

static Themes *makeTheme(StubFacility *f, quint8 copyEndIdx) {
    // All non-copyEnd args are inert defaults; minimizeToSystray/startMinimized false keep the
    // widget plainly constructed under the offscreen QPA.
    return new Themes(
        /*alwaysOnTop*/false, /*showProgressionInTheTitle*/false,
        QColor(Qt::blue), QColor(Qt::green), QColor(Qt::gray),
        /*showDualProgression*/false, /*comboBox_copyEnd*/copyEndIdx,
        /*speedWithProgressBar*/false, /*currentSpeed*/0, /*checkBoxShowSpeed*/false,
        f, /*moreButtonPushed*/false, /*minimizeToSystray*/false, /*startMinimized*/false,
        /*savePosition*/false, /*generalMargin*/0, /*generalSpacing*/0, /*fileProgression*/0);
}

// Drive the theme: copy started (sets haveStarted + durationStarted), optional error, then idle
// (the end-of-copy decision). Return how many times cancel() (== close the window) was emitted.
static int decisionCancels(StubFacility *f, quint8 idx, bool withError) {
    Themes *t = makeTheme(f, idx);
    QSignalSpy spy(t, &PluginInterface_Themes::cancel);
    t->actionInProgess(Ultracopier::Copying);
    if (withError)
        t->errorDetected();
    t->actionInProgess(Ultracopier::Idle);
    const int n = spy.count();
    delete t;
    return n;
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);   // QApplication (widgets) under offscreen QPA
    StubFacility f;

    // index 0 -- "Don't close if errors are found"
    check(decisionCancels(&f, 0, /*withError*/false) == 1, "idx0 success -> cancel (window closes)");
    check(decisionCancels(&f, 0, /*withError*/true)  == 0, "idx0 error -> no cancel (window stays open)");
    // index 1 -- "Never close"
    check(decisionCancels(&f, 1, /*withError*/false) == 0, "idx1 success -> no cancel (never close)");
    check(decisionCancels(&f, 1, /*withError*/true)  == 0, "idx1 error -> no cancel (never close)");
    // index 2 -- "Always close"
    check(decisionCancels(&f, 2, /*withError*/false) == 1, "idx2 success -> cancel (always close)");
    check(decisionCancels(&f, 2, /*withError*/true)  == 1, "idx2 error -> cancel (always close)");

    std::fprintf(stdout, "== window_close (comboBox_copyEnd decision): %d checks, %d failure(s) ==\n",
                 g_checks, g_fail);
    return g_fail ? 1 : 0;
}
