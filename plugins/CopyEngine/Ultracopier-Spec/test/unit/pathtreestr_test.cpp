/** \file pathtreestr_test.cpp
\brief Standalone unit test for the THEME-side tree storage (interface/PathTreeStr).

PathTreeStr is the GUI-side twin of the engine's PathTree: a theme's TransferModel now keeps two
4-byte node ids per display row instead of two full path strings, interning the contract's
std::string UTF-8 full paths into one trie. This test exercises that class directly (no Qt, no
theme) and asserts the same guarantees the refactor relies on:

  1. split-folder interning shares the directory node once yet resolve() is byte-exact;
  2. reordering the flat display order (move up/down/top/bottom + delete-on-finish) is a pure
     permutation that never changes resolve();
  3. appending a row under an existing directory reuses that directory node (+1 leaf only);
  4. 255-byte / >255-byte / >4096-byte length extremes round-trip without truncation;
  5. an embedded NUL in a component round-trips (length-delimited, never strlen());
  6. a Windows drive-letter first component resolves verbatim.

It mirrors pathtree_test.cpp but is std::string-only (the theme never sees wstring). Keeping it as
a separate, dependency-light test guards against the two near-identical trees silently diverging.
Exit 0 == all pass. */

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include "PathTreeStr.h"

static int g_failures=0;
static int g_checks=0;

static void check(bool cond,const char *what)
{
    ++g_checks;
    if(!cond){ ++g_failures; std::printf("    [pathtreestr] FAIL: %s\n",what); }
}
static void check_eq(const std::string &got,const std::string &want,const char *what)
{
    ++g_checks;
    if(got!=want){ ++g_failures; std::printf("    [pathtreestr] FAIL: %s\n         got  \"%s\" (len %zu)\n         want \"%s\" (len %zu)\n",
                                              what,got.c_str(),got.size(),want.c_str(),want.size()); }
}

static void test_split_folder()
{
    PathTreeStr t;
    const uint32_t n1=t.intern("/folderA/file1.dat");
    const uint32_t n2=t.intern("/folderB/file2.dat");
    const uint32_t n3=t.intern("/fileC.dat");
    const uint32_t n4=t.intern("/folderA/file3.dat");   // folderA again -> split in flat order
    check_eq(t.resolve(n1),"/folderA/file1.dat","resolve folderA part1");
    check_eq(t.resolve(n2),"/folderB/file2.dat","resolve folderB");
    check_eq(t.resolve(n3),"/fileC.dat","resolve root fileC");
    check_eq(t.resolve(n4),"/folderA/file3.dat","resolve folderA part2");
    check_eq(t.name(n4),"file3.dat","name leaf");
    check(t.nodeCount()==7,"folderA shared -> exactly 7 nodes");
}

static void moveUp(std::vector<uint32_t>&v,size_t i){ if(i>0) std::swap(v[i],v[i-1]); }
static void moveDown(std::vector<uint32_t>&v,size_t i){ if(i+1<v.size()) std::swap(v[i],v[i+1]); }
static void moveToTop(std::vector<uint32_t>&v,size_t i){ uint32_t x=v[i]; v.erase(v.begin()+i); v.insert(v.begin(),x); }
static void moveToBottom(std::vector<uint32_t>&v,size_t i){ uint32_t x=v[i]; v.erase(v.begin()+i); v.push_back(x); }

static void test_reorder_invariance()
{
    PathTreeStr t;
    std::vector<std::string> paths={
        "/data/folderA/a1.bin","/data/folderB/b1.bin","/data/folderA/a2.bin",
        "/data/c.bin","/data/folderB/b2.bin"};
    std::vector<uint32_t> flat; std::vector<std::string> expectById;
    for(const std::string &p:paths){ uint32_t id=t.intern(p); flat.push_back(id); if(expectById.size()<=id) expectById.resize(id+1); expectById[id]=p; }
    auto all=[&](const char*ctx){ for(uint32_t id:flat) check_eq(t.resolve(id),expectById[id],ctx); };
    all("before reorder");
    moveUp(flat,2);        all("after moveUp");
    moveDown(flat,0);      all("after moveDown");
    moveToTop(flat,4);     all("after moveToTop");
    moveToBottom(flat,1);  all("after moveToBottom");
    std::reverse(flat.begin(),flat.end()); all("after reverse");
    flat.erase(flat.begin()); all("after delete-on-finish");   // head completes
    const size_t nodesAfter=t.nodeCount();
    const uint32_t appended=t.intern("/data/folderA/a3.bin");  // append under existing dir
    check(t.nodeCount()==nodesAfter+1,"append reuses dir node (+1 leaf only)");
    check_eq(t.resolve(appended),"/data/folderA/a3.bin","resolve appended");
}

static void test_length_extremes()
{
    PathTreeStr t;
    const std::string n255(255,'n');
    const uint32_t a=t.intern("/dir/"+n255);
    check_eq(t.resolve(a),"/dir/"+n255,"resolve 255-byte component");
    check(t.name(a).size()==255,"name() 255-byte leaf length");
    const std::string big(300,'B');
    const uint32_t b=t.intern("/x/"+big+"/leaf");
    check_eq(t.resolve(b),"/x/"+big+"/leaf","resolve >255-byte component");
    std::string deep;
    for(int i=0;i<200;++i){ deep.push_back('/'); deep+=std::string(30,(char)('a'+(i%26))); }
    check(deep.size()>4096,"deep path >4096 bytes");
    const uint32_t c=t.intern(deep);
    check_eq(t.resolve(c),deep,"resolve >4096-byte total path");
}

static void test_embedded_nul()
{
    PathTreeStr t;
    std::string dir="a"; dir.push_back('\0'); dir+="b";          // "a\0b"
    std::string file="c"; file.push_back('\0'); file+=".d";      // "c\0.d"
    const std::string p="/"+dir+"/"+file;
    const uint32_t id=t.intern(p);
    check_eq(t.resolve(id),p,"resolve preserves embedded NUL");
    check(t.name(id).size()==4,"name() length includes NUL (length-delimited)");
}

static void test_drive_and_forest()
{
    PathTreeStr t;
    const uint32_t w1=t.intern("C:/dir/sub/file.txt");
    const uint32_t u1=t.intern("/posix/abs/path");
    check_eq(t.resolve(w1),"C:/dir/sub/file.txt","resolve Windows drive path");
    check_eq(t.resolve(u1),"/posix/abs/path","resolve POSIX absolute path");
}

int main()
{
    std::printf("== PathTreeStr unit test (theme) ==\n");
    test_split_folder();
    test_reorder_invariance();
    test_length_extremes();
    test_embedded_nul();
    test_drive_and_forest();
    std::printf("== pathtreestr: %d checks, %d failure(s) ==\n",g_checks,g_failures);
    return g_failures==0?0:1;
}
