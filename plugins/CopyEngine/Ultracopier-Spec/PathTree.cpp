/** \file PathTree.cpp \brief see PathTree.h \licence GPL3 */

#include "PathTree.h"

// Internal path separator. The scanner uses '/' internally on BOTH platforms (Windows paths are
// normalised to '/'); '/' is ASCII so the literal is valid for char and wchar_t alike.
static const INTERNALTYPECHAR PATHTREE_SEP=(INTERNALTYPECHAR)'/';

PathTree::PathTree()
{
    // reserve nothing: a tray-resident idle engine should hold ~0; grows with the job.
}

void PathTree::clear()
{
    nodes.clear();
    dirIndex.clear();
}

size_t PathTree::nodeCount() const
{
    return nodes.size();
}

uint32_t PathTree::internDir(uint32_t parent,const INTERNALTYPEPATH &component)
{
    const std::pair<uint32_t,INTERNALTYPEPATH> key(parent,component);
    const std::map<std::pair<uint32_t,INTERNALTYPEPATH>,uint32_t>::const_iterator it=dirIndex.find(key);
    if(it!=dirIndex.cend())
        return it->second;
    PathNode n;
    n.name=component;
    n.parent=parent;
    nodes.push_back(n);
    const uint32_t idx=(uint32_t)(nodes.size()-1);
    dirIndex[key]=idx;
    return idx;
}

uint32_t PathTree::intern(const INTERNALTYPEPATH &path)
{
    // Split on '/', interning every directory component (shared) and creating the final file
    // component as a fresh leaf. The leading '/' yields an empty first component which is interned
    // like any other (so all absolute paths share that single root node) and reproduced verbatim
    // by resolve().
    uint32_t parent=ROOT_SENTINEL;
    size_t start=0;
    while(true)
    {
        const size_t slash=path.find(PATHTREE_SEP,start);
        if(slash==INTERNALTYPEPATH::npos)
        {
            // final component -> the file leaf (unique, not indexed)
            PathNode leaf;
            leaf.name=path.substr(start);
            leaf.parent=parent;
            nodes.push_back(leaf);
            return (uint32_t)(nodes.size()-1);
        }
        else
        {
            // a directory component -> interned / shared
            parent=internDir(parent,path.substr(start,slash-start));
            start=slash+1;
        }
    }
}

const INTERNALTYPEPATH &PathTree::name(uint32_t node) const
{
    return nodes[node].name;
}

INTERNALTYPEPATH PathTree::resolve(uint32_t node) const
{
    // collect components leaf->root, then emit root->leaf joined by '/'
    std::vector<uint32_t> chain;
    for(uint32_t i=node;i!=ROOT_SENTINEL;i=nodes[i].parent)
        chain.push_back(i);
    INTERNALTYPEPATH p;
    bool first=true;
    for(std::vector<uint32_t>::const_reverse_iterator it=chain.rbegin();it!=chain.rend();++it)
    {
        if(first)
        {
            p=nodes[*it].name;//first component (often "" for a leading '/', or "C:" on Windows)
            first=false;
        }
        else
        {
            p.push_back(PATHTREE_SEP);
            p+=nodes[*it].name;
        }
    }
    return p;
}
