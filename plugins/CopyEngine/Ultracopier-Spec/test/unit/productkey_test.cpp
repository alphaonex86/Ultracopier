// Unit test for ProductKey's online ban-list check (ProductKey::httpFinished).
// Regression: with NO valid key, lastKey is empty and QString::contains("") is true, so every
// downloaded ban list "banned" the user and emitted changeToUltimate() -> SystrayIcon removed
// "Register product key" from the tray menu ~15s after start (seen under wine).
// The REAL ProductKey.cpp is compiled here with only the ban URL redirected to an embedded qrc:
// file, so no internet is used. Built + run by cases/productkey_ban_check.py.
#include "Variable.h"
#undef ULTRACOPIER_BAN_URL
#define ULTRACOPIER_BAN_URL "qrc:/bannedkeys.txt"
#include "ProductKey.cpp"

#include "OptionEngine.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <cstdio>
#include <vector>

OptionEngine *OptionEngine::optionEngine=NULL;
ProductKey *ProductKey::productKey=NULL;

static std::string g_storedKey;
static std::vector<std::string> g_log;

OptionEngine::OptionEngine() : currentBackend(Memory), settings(NULL) {}
OptionEngine::~OptionEngine() {}
std::string OptionEngine::getOptionValue(const std::string &,const std::string &variableName) const
{
    return variableName=="key" ? g_storedKey : std::string();
}
void OptionEngine::setOptionValue(const std::string &,const std::string &,const std::string &) {}
std::string FacilityEngine::version() { return "test"; }
void DebugEngine::addDebugInformationStatic(const Ultracopier::DebugLevel &,const std::string &function,const std::string &text,
                                            const std::string &,const int &,const std::string &)
{
    g_log.push_back(function+": "+text);
}

static int g_fail = 0;
#define CHECK(cond, msg) do{ if(!(cond)){ std::printf("FAIL [%s]: %s\n", name, msg); g_fail++; } }while(0)

static bool logged(const std::string &prefix)
{
    for(const std::string &line : g_log)
        if(line.compare(0,prefix.size(),prefix)==0)
            return true;
    return false;
}

static void runScenario(const char *name,const std::string &storedKey)
{
    g_storedKey=storedKey;
    g_log.clear();
    ProductKey productKey;
    ProductKey::productKey=&productKey;
    QSignalSpy toUltimate(&productKey,&ProductKey::changeToUltimate);
    QSignalSpy banned(&productKey,&ProductKey::bannedKey);
    CHECK(QMetaObject::invokeMethod(&productKey,"downloadFile"),"invoke downloadFile");
    QElapsedTimer timer;
    timer.start();
    while(!logged("httpFinished: start") && timer.elapsed()<10000)
        QCoreApplication::processEvents(QEventLoop::AllEvents,50);
    CHECK(logged("httpFinished: start"),"ban list request never finished");
    CHECK(!logged("httpFinished: get the new update failed") && !logged("httpFinished: redirection denied"),
          "ban list fetch failed -> the check was not exercised");
    CHECK(toUltimate.count()==0,"changeToUltimate emitted -> tray 'Register product key' removed");
    CHECK(banned.count()==0,"bannedKey emitted without any key");
    CHECK(!productKey.isUltimate(),"became ultimate without a valid key");
    ProductKey::productKey=NULL;
}

int main(int argc,char *argv[])
{
    QApplication app(argc,argv);
    runScenario("no key",std::string());
    runScenario("invalid key",std::string("UC3-NOT-A-VALID-KEY"));
    std::printf("%s (%d failure(s))\n",g_fail==0 ? "ALL PASS" : "FAILED",g_fail);
    return g_fail==0 ? 0 : 1;
}
