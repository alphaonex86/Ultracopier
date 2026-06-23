/** \file pathtree_test.cpp
\brief Standalone unit test for the engine's PathTree (the memory-compact transfer-list storage).

This is the C++ half of the user request: the transfer list is kept as a TREE internally but
shown as a flat, user-reorderable list of paths. It directly exercises the NEW PathTree.cpp
(no engine/Qt/threading involved beyond the INTERNALTYPEPATH typedef) and asserts the properties
the refactor must guarantee:

  1. SPLIT-FOLDER interning + byte-exact resolve. Interning files in the interleaved flat order
     folderA(part1), folderB, fileC, folderA(part2) shares the folderA directory node once (the
     memory optimisation) yet resolve() reproduces every file's full path byte-for-byte. This is
     the literal "folderA(part1), FolderB, FileC, folderA(part2)" scenario from the request.
  2. REORDER permutation-invariance. The flat display order is just a vector of node ids; moving a
     selected id up / down / to-top / to-bottom (and deleting one on "copy finish") permutes that
     vector and NEVER touches the tree, so resolve() of every surviving id is unchanged. This is
     the exact invariant the {srcNode,dstNode} refactor introduced.
  3. APPEND reuses the shared directory node (node count grows by exactly one leaf), proving the
     dir index stays O(#dirs) as new files are appended -- the bounded-memory goal.
  4. LENGTH extremes: a 255-byte component, a >255-byte single component, and a path whose total
     length exceeds 4096 -- all interned and resolved without truncation (PathTree imposes no
     PATH_MAX/NAME_MAX of its own; those limits live only at the syscall boundary).
  5. EMBEDDED-NUL / length-delimited fidelity: a component containing a 0x00 byte round-trips
     intact because the split/join uses size()/find()/substr()/push_back() only -- never strlen()
     or c_str(). (POSIX can't store a NUL in a real name, so this is an internal data invariant,
     same split as weird_names' embedded-NUL check.)
  6. WINDOWS DRIVE first component ("C:") and a multi-root forest resolve verbatim.

It is compiled TWICE by the harness -- once as std::string (INTERNALTYPEPATH default) and once
with DEFINES+=WIDESTRING as std::wstring -- so both string types the request names are covered on
Linux; the wstring path is additionally compile-verified by the Windows/MXE build.

No QtTest: a tiny check()/CASE() harness keeps it dependency-light and the failure output legible.
Exit code 0 == all pass; nonzero == at least one failure (so the suite gates on it). */

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include "PathTree.h"   // pulls INTERNALTYPEPATH / INTERNALTYPECHAR via TransferThread.h

// ---- char-generic literal: L"..." under WIDESTRING, "..." otherwise --------------------
#ifdef WIDESTRING
#  define TXT(s) L##s
static const char *MODE_NAME="wstring";
#else
#  define TXT(s) s
static const char *MODE_NAME="string";
#endif

static int g_failures=0;
static int g_checks=0;

// Render an INTERNALTYPEPATH for diagnostics. For wstring we only print the byte length and the
// ASCII-range chars (enough to make a length/truncation failure legible without iconv).
static std::string showLen(const INTERNALTYPEPATH &p)
{
    std::string out="len="+std::to_string((unsigned long)p.size())+" \"";
    for(size_t i=0;i<p.size() && i<80;++i)
    {
        const unsigned long c=(unsigned long)(typename std::make_unsigned<INTERNALTYPECHAR>::type)p[i];
        if(c==0) out+="\\0";
        else if(c>=0x20 && c<0x7f) out+=(char)c;
        else out+='.';
    }
    out+="\"";
    return out;
}

static void check(bool cond,const char *what)
{
    ++g_checks;
    if(!cond)
    {
        ++g_failures;
        std::printf("    [%s] FAIL: %s\n",MODE_NAME,what);
    }
}

static void check_eq(const INTERNALTYPEPATH &got,const INTERNALTYPEPATH &want,const char *what)
{
    ++g_checks;
    if(!(got==want))
    {
        ++g_failures;
        std::printf("    [%s] FAIL: %s\n         got  %s\n         want %s\n",
                    MODE_NAME,what,showLen(got).c_str(),showLen(want).c_str());
    }
}

// build INTERNALTYPEPATH("a" + NUL + "b") etc. -- a length-delimited join helper for the test
static INTERNALTYPEPATH cat(std::initializer_list<INTERNALTYPEPATH> parts)
{
    INTERNALTYPEPATH r;
    for(const INTERNALTYPEPATH &p:parts) r+=p;
    return r;
}

// ----------------------------------------------------------------------------------------
// 1) split-folder interning + byte-exact resolve, with directory-node sharing
// ----------------------------------------------------------------------------------------
static void test_split_folder()
{
    PathTree t;
    // interleaved flat order: folderA(part1), folderB, fileC, folderA(part2)
    const uint32_t n1=t.intern(TXT("/folderA/file1.dat"));   // folderA part 1
    const uint32_t n2=t.intern(TXT("/folderB/file2.dat"));   // folderB
    const uint32_t n3=t.intern(TXT("/fileC.dat"));           // root file C
    const uint32_t n4=t.intern(TXT("/folderA/file3.dat"));   // folderA part 2 (interleaved)

    check_eq(t.resolve(n1),TXT("/folderA/file1.dat"),"resolve folderA part1");
    check_eq(t.resolve(n2),TXT("/folderB/file2.dat"),"resolve folderB");
    check_eq(t.resolve(n3),TXT("/fileC.dat"),"resolve root fileC");
    check_eq(t.resolve(n4),TXT("/folderA/file3.dat"),"resolve folderA part2");
    check_eq(t.name(n1),TXT("file1.dat"),"name leaf part1");
    check_eq(t.name(n4),TXT("file3.dat"),"name leaf part2");

    // folderA interned ONCE: nodes = root("") + folderA + file1 + folderB + file2 + fileC + file3
    check(t.nodeCount()==7,"folderA shared -> exactly 7 nodes (dir dedup)");
}

// ----------------------------------------------------------------------------------------
// 2) reorder permutation-invariance: any flat-list permutation leaves resolve() unchanged
// ----------------------------------------------------------------------------------------
static void assert_all_resolve(PathTree &t,const std::vector<uint32_t> &order,
                               const std::vector<INTERNALTYPEPATH> &expectById,const char *ctx)
{
    for(size_t i=0;i<order.size();++i)
        check_eq(t.resolve(order[i]),expectById[order[i]],ctx);
}

static void moveUp(std::vector<uint32_t> &v,size_t i){ if(i>0) std::swap(v[i],v[i-1]); }
static void moveDown(std::vector<uint32_t> &v,size_t i){ if(i+1<v.size()) std::swap(v[i],v[i+1]); }
static void moveToTop(std::vector<uint32_t> &v,size_t i){ uint32_t x=v[i]; v.erase(v.begin()+i); v.insert(v.begin(),x); }
static void moveToBottom(std::vector<uint32_t> &v,size_t i){ uint32_t x=v[i]; v.erase(v.begin()+i); v.push_back(x); }

static void test_reorder_invariance()
{
    PathTree t;
    // expectById[node] = its canonical full path; index by node id (ids are dense from 0)
    std::vector<INTERNALTYPEPATH> srcPaths={
        TXT("/data/folderA/a1.bin"),
        TXT("/data/folderB/b1.bin"),
        TXT("/data/folderA/a2.bin"),   // folderA again -> split in flat order
        TXT("/data/c.bin"),
        TXT("/data/folderB/b2.bin"),
    };
    std::vector<uint32_t> flat;                 // the user-visible reorderable order (node ids)
    std::vector<INTERNALTYPEPATH> expectById;   // resolve() target per node id
    for(const INTERNALTYPEPATH &p:srcPaths)
    {
        const uint32_t id=t.intern(p);
        flat.push_back(id);
        if(expectById.size()<=id) expectById.resize(id+1);
        expectById[id]=p;
    }

    assert_all_resolve(t,flat,expectById,"resolve before reorder");

    // a sequence of user reorder operations on the FLAT list (the tree must never change)
    moveUp(flat,2);        // pull folderA/a2 up next to folderA/a1 (un-split the folder visually)
    assert_all_resolve(t,flat,expectById,"resolve after moveUp");
    moveDown(flat,0);      // push first item down
    assert_all_resolve(t,flat,expectById,"resolve after moveDown");
    moveToTop(flat,4);     // last item to top
    assert_all_resolve(t,flat,expectById,"resolve after moveToTop");
    moveToBottom(flat,1);  // some item to bottom
    assert_all_resolve(t,flat,expectById,"resolve after moveToBottom");
    std::reverse(flat.begin(),flat.end());   // extreme permutation
    assert_all_resolve(t,flat,expectById,"resolve after full reverse");

    // 3) DELETE on copy-finish: drop the head (the running item completes), rest still resolve
    const size_t before=flat.size();
    flat.erase(flat.begin());
    check(flat.size()==before-1,"delete-on-finish shrinks flat list by one");
    assert_all_resolve(t,flat,expectById,"resolve after delete-on-finish");

    // node count is unchanged by reorder/delete (the tree is storage, not the display order)
    const size_t nodesAfter=t.nodeCount();

    // 3b) APPEND a new file under the EXISTING folderA -> reuses folderA's node (only +1 leaf)
    const uint32_t appended=t.intern(TXT("/data/folderA/a3.bin"));
    check(t.nodeCount()==nodesAfter+1,"append under existing dir adds exactly one (leaf) node");
    check_eq(t.resolve(appended),TXT("/data/folderA/a3.bin"),"resolve appended file");
}

// ----------------------------------------------------------------------------------------
// 4) length extremes: 255-byte, >255-byte single component, and a >4096 total path
// ----------------------------------------------------------------------------------------
static void test_length_extremes()
{
    PathTree t;

    // a 255-byte (ext4 NAME_MAX) component
    const INTERNALTYPEPATH n255(255,(INTERNALTYPECHAR)'n');
    const INTERNALTYPEPATH p255=cat({TXT("/dir/"),n255});
    const uint32_t a=t.intern(p255);
    check_eq(t.resolve(a),p255,"resolve 255-byte component");
    check(t.name(a).size()==255,"name() of 255-byte leaf has length 255");

    // a >255-byte single component (NOT filesystem-storable -> internal-only robustness)
    const INTERNALTYPEPATH big(300,(INTERNALTYPECHAR)'B');
    const INTERNALTYPEPATH pbig=cat({TXT("/x/"),big,TXT("/leaf")});
    const uint32_t b=t.intern(pbig);
    check_eq(t.resolve(b),pbig,"resolve >255-byte component");

    // a path whose TOTAL length exceeds 4096 (PATH_MAX): 200 components x ~30 chars
    INTERNALTYPEPATH deep;
    for(int i=0;i<200;++i)
    {
        deep.push_back((INTERNALTYPECHAR)'/');
        deep+=INTERNALTYPEPATH(30,(INTERNALTYPECHAR)('a'+(i%26)));
    }
    check(deep.size()>4096,"constructed deep path exceeds 4096 bytes");
    const uint32_t c=t.intern(deep);
    check_eq(t.resolve(c),deep,"resolve >4096-byte total path");
}

// ----------------------------------------------------------------------------------------
// 5) embedded-NUL / length-delimited fidelity
// ----------------------------------------------------------------------------------------
static void test_embedded_nul()
{
    PathTree t;
    const INTERNALTYPECHAR NUL=(INTERNALTYPECHAR)0;
    // component "ab\0cd" (5 chars incl the NUL) as a directory, and another in the file leaf
    INTERNALTYPEPATH dir; dir+=TXT("a"); dir.push_back(NUL); dir+=TXT("b");      // "a\0b"
    INTERNALTYPEPATH file; file+=TXT("c"); file.push_back(NUL); file+=TXT(".d"); // "c\0.d"
    const INTERNALTYPEPATH p=cat({TXT("/"),dir,TXT("/"),file});
    check(p.size()==1+3+1+4,"constructed embedded-NUL path has the exact length (no strlen cut)");

    const uint32_t id=t.intern(p);
    check_eq(t.resolve(id),p,"resolve preserves embedded NUL byte-for-byte");
    check(t.name(id).size()==4,"name() length includes the NUL (length-delimited, not strlen)");
    check_eq(t.name(id),file,"name() equals the NUL-bearing leaf component");
}

// ----------------------------------------------------------------------------------------
// 6) Windows drive first-component + multi-root forest
// ----------------------------------------------------------------------------------------
static void test_drive_and_forest()
{
    PathTree t;
    const uint32_t w1=t.intern(TXT("C:/dir/sub/file.txt"));
    const uint32_t w2=t.intern(TXT("D:/other.txt"));
    const uint32_t u1=t.intern(TXT("/posix/abs/path"));
    check_eq(t.resolve(w1),TXT("C:/dir/sub/file.txt"),"resolve Windows C: drive path");
    check_eq(t.resolve(w2),TXT("D:/other.txt"),"resolve Windows D: drive path");
    check_eq(t.resolve(u1),TXT("/posix/abs/path"),"resolve POSIX absolute path (forest root)");
    // distinct roots are disjoint: C: and D: and "" do not collide
    check(t.resolve(w1)!=t.resolve(w2),"distinct drive roots stay disjoint");
}

int main()
{
    std::printf("== PathTree unit test (%s) ==\n",MODE_NAME);
    test_split_folder();
    test_reorder_invariance();
    test_length_extremes();
    test_embedded_nul();
    test_drive_and_forest();
    std::printf("== %s: %d checks, %d failure(s) ==\n",MODE_NAME,g_checks,g_failures);
    return g_failures==0?0:1;
}
