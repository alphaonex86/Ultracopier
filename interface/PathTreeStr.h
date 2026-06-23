/** \file PathTreeStr.h
\brief Memory-compact storage for a theme's display transfer-list (the GUI-side analogue of the
engine's PathTree).

A theme's TransferModel keeps one display row per file in the job. At Ultracopier's target scale
(tens of millions of files) holding two full path strings (source + destination) per row is, like
the engine's transfer list, tens of GB of RAM by itself -- a second copy of the whole path set in
the same process. PathTreeStr interns the path STRUCTURE once as a trie of single path components:
each row keeps two 4-byte node ids instead of two strings, sibling files share their ancestor
directory nodes, and a leaf carries only its own component (the file name).

It is the GUI-side twin of plugins/CopyEngine/Ultracopier-Spec/PathTree.h, deliberately kept as a
SEPARATE class with a distinct name so both can be linked into the all-in-one binary at once: the
engine tree is parameterised on INTERNALTYPEPATH (std::wstring on Windows), whereas the theme only
ever sees the contract's std::string UTF-8 full paths (ItemOfCopyList.sourceFullPath /
destinationFullPath), so this one is hard-wired to std::string.

The engine emits contract paths joined by '/' (its internal separator, on every platform), so we
split AND rejoin on '/' only: resolve() is byte-identical to the path the engine sent -- a leading
'' component reproduces a leading '/', a "C:" first component reproduces a Windows drive. Names are
length-delimited (size()/find()/substr()/push_back() only, never strlen()/.c_str()), so an embedded
NUL or any hostile byte in a component round-trips intact.

The user-visible, reorderable DISPLAY order is unchanged: it stays the order of the model's row
vector, which now holds {srcNode,dstNode} instead of strings. Reordering permutes that vector; the
trie is only storage, so a folder can appear "split" in the displayed order while its files stay
under one folder node.
\author alpha_one_x86 / Ultracopier
\licence GPL3 */

#ifndef ULTRACOPIER_PATHTREESTR_H
#define ULTRACOPIER_PATHTREESTR_H

#include <vector>
#include <map>
#include <utility>
#include <cstdint>
#include <string>

class PathTreeStr
{
public:
    static const uint32_t ROOT_SENTINEL=0xFFFFFFFF;

    PathTreeStr();
    void clear();///< drop every node (a new job / cancel / full re-sync)

    /// \brief find/create the leaf node for a full path. Directory components are interned
    /// (shared); the final file component is a fresh leaf. O(depth). Returns the leaf node id.
    uint32_t intern(const std::string &path);

    /// \brief absolute path of a node, byte-identical to what was interned. O(depth).
    std::string resolve(uint32_t node) const;

    /// \brief just the leaf component (the file name) of a node. O(1).
    const std::string &name(uint32_t node) const;

    size_t nodeCount() const;///< for tests/metrics
private:
    struct PathNode {
        std::string name;///< ONE path component, length-delimited (may be "" for a leading '/')
        uint32_t parent; ///< index into nodes[]; ROOT_SENTINEL for the first component
    };
    std::vector<PathNode> nodes;
    // intern only DIRECTORY components -> O(#dirs) entries, not O(#files). key=(parentIndex,name).
    std::map<std::pair<uint32_t,std::string>,uint32_t> dirIndex;

    uint32_t internDir(uint32_t parent,const std::string &component);//deduplicated
};

#endif // ULTRACOPIER_PATHTREESTR_H
