/** \file PathTreeStr.cpp \brief see PathTreeStr.h \licence GPL3 */

#include "PathTreeStr.h"

// The engine emits contract full paths joined by '/' (its internal separator on every platform).
static const char PATHTREESTR_SEP='/';

PathTreeStr::PathTreeStr()
{
    // reserve nothing: an idle theme should hold ~0; grows with the job.
}

void PathTreeStr::clear()
{
    nodes.clear();
    dirIndex.clear();
}

size_t PathTreeStr::nodeCount() const
{
    return nodes.size();
}

uint32_t PathTreeStr::internDir(uint32_t parent,const std::string &component)
{
    const std::pair<uint32_t,std::string> key(parent,component);
    const std::map<std::pair<uint32_t,std::string>,uint32_t>::const_iterator it=dirIndex.find(key);
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

uint32_t PathTreeStr::intern(const std::string &path)
{
    // Split on '/', interning every directory component (shared) and creating the final file
    // component as a fresh leaf. The leading '/' yields an empty first component, interned like
    // any other (so all absolute paths share that single root node) and reproduced verbatim.
    uint32_t parent=ROOT_SENTINEL;
    size_t start=0;
    while(true)
    {
        const size_t slash=path.find(PATHTREESTR_SEP,start);
        if(slash==std::string::npos)
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

const std::string &PathTreeStr::name(uint32_t node) const
{
    return nodes[node].name;
}

std::string PathTreeStr::resolve(uint32_t node) const
{
    // collect components leaf->root, then emit root->leaf joined by '/'
    std::vector<uint32_t> chain;
    for(uint32_t i=node;i!=ROOT_SENTINEL;i=nodes[i].parent)
        chain.push_back(i);
    std::string p;
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
            p.push_back(PATHTREESTR_SEP);
            p+=nodes[*it].name;
        }
    }
    return p;
}
