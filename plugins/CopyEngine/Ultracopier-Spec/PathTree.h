/** \file PathTree.h
\brief Memory-compact storage for the transfer file-list (the #1 scalability blocker).

The transfer list used to hold TWO full path strings per entry (source + destination). At
Ultracopier's target scale (tens of millions of files / 200+ TB) that is tens of GB of RAM by
itself. PathTree stores the path STRUCTURE once as a trie of single path components: every entry
keeps two 4-byte node ids instead of two strings, sibling files share all their ancestor
directory nodes, and a leaf carries only its own component (the file name), not the whole path.
On a deep tree this shrinks the list ~5-10x.

It is a forest of absolute paths interned into one node array:
 - DIRECTORY components are interned (deduplicated) via a (parent,name) index, so /a/b/c is one
   chain of nodes shared by every file under it -- on BOTH the source side and the destination
   side (each side has its own root, naturally disjoint).
 - The final FILE component is a fresh leaf (files are unique; not indexed -> the index stays
   O(#dirs), not O(#files)).
 - resolve() walks leaf->root, O(depth), and is byte-identical to the path the scanner produced
   (the leading "" component reproduces a leading '/', a "C:" first component reproduces a Windows
   drive). So renames, multiple source bases, and Windows drives all "just work" -- we intern
   exactly what the scanner computed, never re-derive a destination.

The flat, user-reorderable DISPLAY order is unchanged: it is still the order of the
actionToDoListTransfer vector, which now holds {srcNode,dstNode} pairs instead of strings.
Reordering permutes that vector; the trie is only storage, so a folder can appear "split" in the
displayed order while its files stay under one folder node.

Paths are length-delimited (INTERNALTYPEPATH = std::string or std::wstring): join uses
size()/operator+/push_back only, never strlen()/.c_str(), preserving the embedded-NUL /
filename-robustness invariant up to the existing syscall boundary.
\author alpha_one_x86 / Ultracopier
\licence GPL3 */

#ifndef ULTRACOPIER_PATHTREE_H
#define ULTRACOPIER_PATHTREE_H

#include <vector>
#include <map>
#include <utility>
#include <cstdint>
#include "TransferThread.h" // INTERNALTYPEPATH / INTERNALTYPECHAR

class PathTree
{
public:
    static const uint32_t ROOT_SENTINEL=0xFFFFFFFF;

    PathTree();
    void clear();///< drop every node (a new job / cancel)

    /// \brief find/create the leaf node for a full path. Directory components are interned
    /// (shared); the final file component is a fresh leaf. O(depth). Returns the leaf node id.
    uint32_t intern(const INTERNALTYPEPATH &path);

    /// \brief absolute path of a node, byte-identical to what was interned. O(depth).
    INTERNALTYPEPATH resolve(uint32_t node) const;

    /// \brief just the leaf component (the file name) of a node. O(1).
    const INTERNALTYPEPATH &name(uint32_t node) const;

    /// \brief the parent node id of `node`, or ROOT_SENTINEL for a first (root) component. O(1).
    /// Read-only; used by the io_uring/IOCP op-decomposition shadow scheduler to derive an entry's
    /// destination-parent directory. Behaviour-neutral (the async backend compiles but never calls it).
    uint32_t parentOf(uint32_t node) const { return nodes[node].parent; }

    size_t nodeCount() const;///< for tests/metrics
private:
    struct PathNode {
        INTERNALTYPEPATH name;///< ONE path component, length-delimited (may be "" for a leading '/')
        uint32_t parent;      ///< index into nodes[]; ROOT_SENTINEL for the first component
    };
    std::vector<PathNode> nodes;
    // intern only DIRECTORY components -> O(#dirs) entries, not O(#files). key=(parentIndex,name).
    std::map<std::pair<uint32_t,INTERNALTYPEPATH>,uint32_t> dirIndex;

    uint32_t internDir(uint32_t parent,const INTERNALTYPEPATH &component);//deduplicated
};

#endif // ULTRACOPIER_PATHTREE_H
