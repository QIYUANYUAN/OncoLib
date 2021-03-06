/*
 *  rootedcladisticancestrygraph.cpp
 *
 *   Created on: 29-sep-2015
 *       Author: M. El-Kebir
 */

#include "rootedcladisticancestrygraph.h"

#include <lemon/connectivity.h>

namespace gm {
  
RootedCladisticAncestryGraph::RootedCladisticAncestryGraph(const RealTensor& F,
                                                           const StateTreeVector& S)
  : _F(F)
  , _S(S)
  , _G()
  , _root(lemon::INVALID)
  , _charStateToNode(_F.n(), NodeVector(_F.k(), lemon::INVALID))
  , _nodeToCharState(_G)
  , _nodeToCharStateList(_G)
  , _label(_G)
{
}
  
bool RootedCladisticAncestryGraph::sample(StringPairSet& sampledTree) const
{
  sampledTree.clear();
  
  BoolNodeMap nodeMap(_G, false);
  BoolArcMap arcMap(_G, false);
  SubDigraph T(_G, nodeMap, arcMap);
  
  for (NodeIt v(_G); v != lemon::INVALID; ++v)
  {
    if (v == _root) continue;
    
    nodeMap[v] = true;

    ArcVector options;
    for (InArcIt a(_G, v); a != lemon::INVALID; ++a)
    {
      if (_G.source(a) != _root)
      {
        options.push_back(a);
      }
    }
    if (!options.empty())
    {
      std::shuffle(options.begin(), options.end(), g_rng);
      
      arcMap[options.front()] = true;
      nodeMap[T.source(options.front())] = true;
    }
  }
  
  for (SubArcIt a(T); a != lemon::INVALID; ++a)
  {
    Node u = T.source(a);
    Node v = T.target(a);
    
    if (u == _root) continue;
    
    int c = _nodeToCharState[u].begin()->first;
    int d = _nodeToCharState[v].begin()->first;
    
    sampledTree.insert(StringPair(_F.getColLabel(c), _F.getColLabel(d)));
  }
  
  bool ok = true;
  for (SubNodeIt v(T); v != lemon::INVALID; ++v)
  {
    const IntPair& ci = *_nodeToCharState[v].begin();
    for (int p = 0; p < _F.m(); ++p)
    {
      double f_pci = _F.getCumFreq(p, ci.first, _S[ci.first].D(ci.second));
      double sum = 0.;
      for (SubOutArcIt a(T, v); a != lemon::INVALID; ++a)
      {
        Node w = T.target(a);
        const IntPair& dj = *_nodeToCharState[w].begin();
        double f_pdj = _F.getCumFreq(p, dj.first, _S[dj.first].D(dj.second));
        sum += f_pdj;
      }
      if (g_tol.less(f_pci, sum))
      {
        ok = false;
      }
    }
  }
  
  return ok;
}

double RootedCladisticAncestryGraph::fracOfIncomparablePairs() const
{
  lemon::DynArcLookUp<Digraph> arcLookUp(_G);
  
  double frac = 0.;
  int total = 0;
  for (int c = 0; c < _F.n(); ++c)
  {
    Node v_c1 =  _charStateToNode[c][1];
    for (int d = c + 1; d < _F.n(); ++d)
    {
      Node v_d1 =  _charStateToNode[d][1];
      
      ++total;
      if ((arcLookUp(v_c1, v_d1) != lemon::INVALID) ||
          (arcLookUp(v_d1, v_c1) != lemon::INVALID))
      {
        frac += 1.;
      }
    }
  }
  
  frac /= total;
  return frac;
}
  
void RootedCladisticAncestryGraph::writeDOT(std::ostream& out) const
{
  const int m = _F.m();
  out << "digraph G {" << std::endl;
  out.precision(3);

  for (NodeIt v_ci(_G); v_ci != lemon::INVALID; ++v_ci)
  {
    const IntPairSet& X_ci = _nodeToCharState[v_ci];
    out << "\t" << _G.id(v_ci) << " [label=\"" << _label[v_ci] << "\\n";

    for (const IntPair& ci : X_ci)
    {
      const IntSet& D_ci = _S[ci.first].D(ci.second);
      out << "{";
      for (IntSetIt it = D_ci.begin(); it != D_ci.end(); ++it)
      {
        out << " " << *it;
      }
      out << " } ";
    }
    out << "\\n";

    for (int p = 0; p < m; ++p)
    {
      bool first = true;
      for (const IntPair& ci : X_ci)
      {
        if (first)
          first = false;
        else
          out << " | ";
        out << _F.getCumFreq(p, ci.first, _S[ci.first].D(ci.second));
      }
      out << "\\n";
    }
    out << "\"]" << std::endl;
  }
  
  for (ArcIt a(_G); a != lemon::INVALID; ++a)
  {
    out << "\t" << _G.id(_G.source(a)) << " -> " << _G.id(_G.target(a)) << std::endl;
  }
  
  out << "}" << std::endl;
}
  
void RootedCladisticAncestryGraph::init()
{
  const int k = _F.k();
  const int m = _F.m();
  const int n = _F.n();
  
  // add root node
  _root = _G.addNode();
  for (int c = 0; c < n; ++c)
  {
    _nodeToCharState[_root].insert(IntPair(c, 0));
    _charStateToNode[c][0] = _root;
  }
  
  // add the other n(k-1) nodes
  for (int c = 0; c < n; ++c)
  {
    for (int i = 1; i < k; ++i)
    {
      // we should not be removing states present in the state tree!
      if (!_S[c].isPresent(i))
        continue;

      Node v_ci = _G.addNode();
      _charStateToNode[c][i] = v_ci;
      _nodeToCharState[v_ci].insert(std::make_pair(c, i));
      _nodeToCharStateList[v_ci].push_back(std::make_pair(c, i));
    }
  }

  // let's add the edges according to the state trees
  for (int c = 0; c < n; ++c)
  {
    for (int i = 0; i < k; ++i)
    {
      Node v_ci = _charStateToNode[c][i];
      if (v_ci == lemon::INVALID) continue;
      for (int j = 1; j < k; ++j)
      {
        if (i == j) continue;
        
        Node v_cj = _charStateToNode[c][j];
        if (v_cj == lemon::INVALID) continue;
        
        if (_S[c].isParent(i, j))
        {
          _G.addArc(v_ci, v_cj);
        }
      }
    }
  }
  
  // now let's add edges according to frequency tensor (for distinct characters)
  for (int c = 0; c < n; ++c)
  {
    for (int d = 0; d < n; ++d)
    {
      if (c == d) continue;
      
      for (int i = 1; i < k; ++i)
      {
        // there's only one root vertex
        if (i == 0 && c != 0) continue;
        if (_charStateToNode[c][i] == lemon::INVALID) continue;
        
        for (int j = 1; j < k; ++j)
        {
          if (j == 0 && d != 0) continue;
          if (_charStateToNode[d][j] == lemon::INVALID) continue;
          
          // respect the state tree, also for the root vertex
          if (c == 0 && i == 0 && _S[d].parent(j) != 0)
            continue;
          
          bool ok = true;
          for (int p = 0; p < m; ++p)
          {
            double F_p_ci = _F.getCumFreq(p, c, _S[c].D(i));
            double F_p_dj = _F.getCumFreq(p, d, _S[d].D(j));

            ok &= !g_tol.less(F_p_ci, F_p_dj);
            if (!ok) break;
          }
          
          if (ok)
          {
            _G.addArc(_charStateToNode[c][i], _charStateToNode[d][j]);
          }
        }
      }
    }
  }
}
  
} // namespace gm
