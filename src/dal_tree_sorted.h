/* -*- c++ -*- (enables emacs c++ mode)                                    */
/* *********************************************************************** */
/*                                                                         */
/* Library :  Dynamic Array Library (dal)                                  */
/* File    :  dal_tree_sorted.h : dynamic tas sorted with balanced trees   */
/*               AVL (Adelson-Velskii & Landis trees).                     */
/*                                                                         */
/* Date : June 01, 1995                                                    */
/* Author : Yves Renard, Yves.Renard@gmm.insa-tlse.fr                      */
/*                                                                         */
/* *********************************************************************** */
/*                                                                         */
/* Copyright (C) 1995-2002  Yves Renard.                                   */
/*                                                                         */
/* This file is a part of GETFEM++                                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU Lesser General Public License as          */
/* published by the Free Software Foundation; version 2.1 of the License.  */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU Lesser General Public License for more details.                     */
/*                                                                         */
/* You should have received a copy of the GNU Lesser General Public        */
/* License along with this program; if not, write to the Free Software     */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,  */
/* USA.                                                                    */
/*                                                                         */
/* *********************************************************************** */


#ifndef __DAL_TREE_SORTED_H
#define __DAL_TREE_SORTED_H

#include <dal_tas.h>

namespace dal
{

  /* ********************************************************************* */
  /* Definitition des iterateurs.                                          */
  /* ********************************************************************* */
  /* Attention, l'iterateur n'est plus valide apres une operation          */
  /* d'insertion ou de suppression.                                        */
  /* ********************************************************************* */

  static const size_t _DEPTHMAX_ = size_t(CHAR_BIT*sizeof(size_t)*3) / 2;
  static const size_t ST_NIL = size_t(-1);

  template<class T, class COMP = dal::less<T>, int pks = 5>
    class dynamic_tree_sorted;

  template<class T, class COMP, int pks> struct tsa_iterator
  {
    typedef T                value_type;
    typedef value_type&      reference;
    typedef value_type*      pointer;
    typedef size_t           size_type;
    typedef ptrdiff_t        difference_type;
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef int8_type short_type;

    dynamic_tree_sorted<T, COMP, pks> *p;
    size_type path[_DEPTHMAX_];
    short_type dir[_DEPTHMAX_];
    size_type depth;

    tsa_iterator(void) {}
    tsa_iterator(dynamic_tree_sorted<T, COMP, pks> &tsa)
    { p = &tsa; depth = 0; }
    void copy(const tsa_iterator<T, COMP, pks> &it);
    tsa_iterator(const tsa_iterator &it) { copy(it); }
    tsa_iterator &operator =(const tsa_iterator &it)
    { copy(it); return *this;}

    inline size_type index(void) const
    { return (depth==0) ? ST_NIL : path[depth-1];}
    inline size_type father(void) const
    { return (depth<=1) ? ST_NIL : path[depth-2];}
    inline size_type _index(void) const { return path[depth-1]; }
    inline short_type direction(void) const
    { return (depth==0) ? 0 : dir[depth-1];}
    inline void up(void) { if (depth > 0) depth--; }
    void down_left(void);
    void down_right(void);
    void down_left_all(void);
    void down_right_all(void);
    void root(void) { path[0] = p->root_elt(); dir[0] = 0; depth = 1; }
    void first(void) { root(); down_left_all(); }
    void last(void) { root(); down_right_all(); }
    void end(void) { depth = 0; }

    tsa_iterator &operator ++();
    tsa_iterator &operator --();
    tsa_iterator operator ++(int)
    { tsa_iterator tmp = *this; ++(*this); return tmp; }
    tsa_iterator operator --(int)
    { tsa_iterator tmp = *this; --(*this); return tmp; }
   
    reference operator *() const { return (*p)[index()]; }
    pointer operator->() const { return &(operator*()); }
    
    bool operator ==(const tsa_iterator &i) const
    { return ((i.depth == 0 && depth == 0) || (i._index() == _index())); }
    bool operator !=(const tsa_iterator &i) const
    { return !((i.depth == 0 && depth == 0) || (i._index() == _index())); }

  };

  template<class T, class COMP, int pks> 
    void tsa_iterator<T, COMP, pks>::copy(const tsa_iterator<T, COMP, pks> &it)
  {
    p = it.p; depth = it.depth;
    size_type *p1it=&(path[0]), *pend=&path[depth];
    const size_type *p2it=&(it.path[0]);
    short_type *d1it=&(dir[0]);
    const short_type *d2it=&(it.dir[0]);
    while (p1it != pend) { *p1it++ = *p2it++; *d1it++ = *d2it++; }
  }
  
  template<class T, class COMP, int pks> 
    void tsa_iterator<T, COMP, pks>::down_left(void)
  {
    #ifdef __GETFEM_VERIFY
      if (depth <= 0 || depth >= _DEPTHMAX_ || index() == ST_NIL)
	DAL_THROW(internal_error, "internal error");
    #endif
    path[depth] = p->left_elt(_index()); dir[depth++] = -1;
  }

  template<class T, class COMP, int pks> 
    void tsa_iterator<T, COMP, pks>::down_right(void)
  { 
    #ifdef __GETFEM_VERIFY
      if (depth <= 0 || depth >= _DEPTHMAX_ || index() == ST_NIL)
	DAL_THROW(internal_error, "internal error");
    #endif
    path[depth] = p->right_elt(_index()); dir[depth++] = 1;
  }

  template<class T, class COMP, int pks> 
    void tsa_iterator<T, COMP, pks>::down_left_all(void)
  { while (_index() != ST_NIL) down_left(); up(); }

  template<class T, class COMP, int pks> 
    void tsa_iterator<T, COMP, pks>::down_right_all(void)
  { while (_index() != ST_NIL) down_right(); up();}
   
  template<class T, class COMP, int pks>
    tsa_iterator<T, COMP, pks> &tsa_iterator<T, COMP, pks>::operator ++()
  { 
    if (depth == 0) first();
    if (p->right_elt(_index()) != ST_NIL) { down_right(); down_left_all(); }
    else                { up(); while (dir[depth] == 1) up(); }
    return *this;
  }
  
  template<class T, class COMP, int pks>
    tsa_iterator<T, COMP, pks> &tsa_iterator<T, COMP, pks>::operator --()
  { 
    if (depth == 0) last();
    if (p->left_elt(_index()) != ST_NIL) { down_left(); down_right_all(); }
    else               { up(); while (dir[depth] == -1) up(); }
    return *this;
  }


  template<class T, class COMP, int pks> struct const_tsa_iterator
  {
    typedef T                  value_type;
    typedef const value_type&  reference;
    typedef const value_type*  pointer;
    typedef size_t             size_type;
    typedef ptrdiff_t          difference_type;
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef int8_type short_type;

    const dynamic_tree_sorted<T, COMP, pks> *p;
    size_type path[_DEPTHMAX_];
    short_type dir[_DEPTHMAX_];
    size_type depth;

    const_tsa_iterator(void) {}
    const_tsa_iterator(const dynamic_tree_sorted<T, COMP, pks> &tsa)
    { p = &tsa; depth = 0; }
    void copy(const const_tsa_iterator<T, COMP, pks> &it);
    const_tsa_iterator(const const_tsa_iterator &it) { copy(it); }
    const_tsa_iterator(const tsa_iterator<T, COMP, pks> &it);
    const_tsa_iterator &operator =(const const_tsa_iterator &it)
    { copy(it); return *this; }
    
    inline size_type index(void) const
    { return (depth==0) ? ST_NIL : path[depth-1];}
    inline size_type father(void) const
    { return (depth<=1) ? ST_NIL : path[depth-2];}
    inline size_type _index(void) const { return path[depth-1]; }
    inline short_type direction(void) const
    { return (depth==0) ? 0 : dir[depth-1];}
    inline void up(void) { if (depth > 0) depth--; }
    void down_left(void);
    void down_right(void);
    void down_left_all(void);
    void down_right_all(void);
    void root(void) { path[0] = p->root_elt(); dir[0] = 0; depth = 1; }
    void first(void) { root(); down_left_all(); }
    void last(void) { root(); down_right_all(); }
    void end(void) { depth = 0; }

    const_tsa_iterator &operator ++();
    const_tsa_iterator &operator --();
    const_tsa_iterator operator ++(int)
    { const_tsa_iterator tmp = *this; ++(*this); return tmp; }
    const_tsa_iterator operator --(int)
    { const_tsa_iterator tmp = *this; --(*this); return tmp; }
   
    reference operator *() const { return (*p)[index()]; }
    pointer operator->() const { return &(operator*()); }
    
    bool operator ==(const const_tsa_iterator &i) const
    { return ((i.depth == 0 && depth == 0) || (i._index() == _index())); }
    bool operator !=(const const_tsa_iterator &i) const
    { return !((i.depth == 0 && depth == 0) || (i._index() == _index())); }

  };

  template<class T, class COMP, int pks> 
    void const_tsa_iterator<T, COMP, pks>::copy(
				   const const_tsa_iterator<T, COMP, pks> &it)
  {
    p = it.p; depth = it.depth;
    size_type *p1it=&(path[0]), *pend=&path[depth];
    const size_type *p2it=&(it.path[0]);
    short_type *d1it=&(dir[0]);
    const short_type *d2it=&(it.dir[0]);
    while (p1it != pend) { *p1it++ = *p2it++; *d1it++ = *d2it++; }
  }

  template<class T, class COMP, int pks>
    const_tsa_iterator<T, COMP, pks>::const_tsa_iterator(
					const tsa_iterator<T, COMP, pks> &it)
  {
    p = it.p; depth = it.depth;
    size_type *p1it=&(path[0]), *pend=&path[depth];
    const size_type *p2it=&(it.path[0]);
    short_type *d1it=&(dir[0]);
    const short_type *d2it=&(it.dir[0]);
    while (p1it != pend) { *p1it++ = *p2it++; *d1it++ = *d2it++; }
  }

  template<class T, class COMP, int pks>
    void const_tsa_iterator<T, COMP, pks>::down_left(void)
  {
    #ifdef __GETFEM_VERIFY
      if (depth <= 0 || depth >= _DEPTHMAX_ || index() == ST_NIL)
	DAL_THROW(internal_error, "internal error");
    #endif
    path[depth] = p->left_elt(_index()); dir[depth++] = -1;
  }

  template<class T, class COMP, int pks>
    void const_tsa_iterator<T, COMP, pks>::down_right(void)
  { 
    #ifdef __GETFEM_VERIFY
      if (depth <= 0 || depth >= _DEPTHMAX_ || index() == ST_NIL)
	DAL_THROW(internal_error, "internal error");
    #endif
    path[depth] = p->right_elt(_index()); dir[depth++] = 1;
  }

  template<class T, class COMP, int pks>
    void const_tsa_iterator<T, COMP, pks>::down_left_all(void)
  { while (_index() != ST_NIL) down_left(); up(); }

  template<class T, class COMP, int pks> 
    void const_tsa_iterator<T, COMP, pks>::down_right_all(void) 
  { while (_index() != ST_NIL) down_right(); up();}
  
  template<class T, class COMP, int pks>
    const_tsa_iterator<T, COMP, pks> &
      const_tsa_iterator<T, COMP, pks>::operator ++()
  {  
    if (depth == 0) last();
    if (p->right_elt(_index()) != ST_NIL) { down_right(); down_left_all(); }
    else                { up(); while (dir[depth] == 1) up(); }
    return *this;
  }

  template<class T, class COMP, int pks>
    const_tsa_iterator<T, COMP, pks> &
      const_tsa_iterator<T, COMP, pks>::operator --()
  {  
    if (depth == 0) last();
    if (p->left_elt(_index()) != ST_NIL) { down_left(); down_right_all(); }
    else               { up(); while (dir[depth] == -1) up(); }
    return *this;
  }
  
  /* ********************************************************************* */
  /* Definitition of dynamic_tree_sorted.                                  */
  /* ********************************************************************* */

  template<class T, class COMP, int pks>
    class dynamic_tree_sorted : public dynamic_tas<T, pks>
  {
    public :
     

      typedef typename dynamic_tas<T, pks>::tas_iterator tas_iterator;
      typedef typename dynamic_tas<T, pks>::const_tas_iterator const_tas_iterator;
      typedef typename dynamic_tas<T, pks>::iterator iterator;
      typedef typename dynamic_tas<T, pks>::const_iterator const_iterator;
      typedef typename dynamic_tas<T, pks>::size_type size_type;
                                                    
      typedef int8_type short_type;
      typedef tsa_iterator<T, COMP, pks> sorted_iterator;
      typedef const_tsa_iterator<T, COMP, pks> const_sorted_iterator;
      typedef dal::reverse_iter<const_iterator> const_reverse_sorted_iterator;
      typedef dal::reverse_iter<iterator> reverse_sorted_iterator;
      
    protected :

      COMP compar;
      struct tree_elt
      { 
	size_type r, l;
	short_type eq;
	inline void init(void) { eq = 0; r = l = ST_NIL; }
	tree_elt(void) { init(); }
      };
      
      dynamic_array<tree_elt, pks> nodes;
      size_type first_node;

      size_type rotate_right(size_type i);
      size_type rotate_left(size_type i);
      size_type rotate_left_right(size_type i);
      size_type rotate_right_left(size_type i);
      size_type balance_again(size_type i);

      void add_index(size_type i, const_sorted_iterator &it);
      void sup_index(size_type i, const_sorted_iterator &it);

    public :

      #ifdef __GETFEM_VERIFY

        int verify_balance(size_type i = size_type(-2)) const
        {
	  if (i == size_type(-2)) i = first_node;
	  if (i == ST_NIL) return 0;
	  int l = verify_balance(nodes[i].l);
	  int r = verify_balance(nodes[i].r);
	  if (short_type(r - l) !=  nodes[i].eq || 
	      !(nodes[i].eq <= 1 && nodes[i].eq>=-1))
	    DAL_THROW(internal_error, "internal error");
	  return std::max(l,r) + 1;
	}

      #endif

      void insert_path(const T &elt, const_sorted_iterator &it) const;
      void search_sorted_iterator(const T &elt, 
				  const_sorted_iterator &it) const;
      void find_sorted_iterator(size_type i, const_sorted_iterator &it) const;
      COMP &comparator(void) { return compar; }
      const COMP &comparator(void) const { return compar; }
    

      dynamic_tree_sorted(COMP cp = COMP()) 
      { first_node = ST_NIL; compar = cp; }

      size_type root_elt(void) const { return first_node; }
      size_type right_elt(size_type n) const { return nodes[n].r; }
      size_type left_elt(size_type n)  const { return nodes[n].l; }
      short_type balance(size_type n) const
      { return (n == ST_NIL) ? 0 : nodes[n].eq; }
      size_type search(const T &elt) const
      { const_sorted_iterator it(*this); search_sorted_iterator(elt,it); return it.index(); }
      size_type search_ge(const T &) const;
      size_type memsize(void) const;
      void clear(void)
      { first_node = ST_NIL; nodes.clear(); dynamic_tas<T,pks>::clear(); }
      size_type add(const T &);
      void add_to_index(size_type, const T &);
      size_type add_norepeat(const T &, bool replace = false,
			                bool *present = NULL);
      void resort(void);
      void sup(size_type);
      void swap(size_type, size_type);
      void compact(void);

      sorted_iterator sorted_begin(void)
      { sorted_iterator it(*this); it.first(); return it; }
      const_sorted_iterator sorted_begin(void) const
      { const_sorted_iterator it(*this); it.first(); return it; }
      sorted_iterator sorted_end(void)
      { sorted_iterator it(*this); it.end(); return it; }
      const_sorted_iterator sorted_end(void) const
      { const_sorted_iterator it(*this); it.end(); return it; }
      reverse_sorted_iterator rbegin(void)
	{ return reverse_sorted_iterator(end()); }
      const_reverse_sorted_iterator rbegin(void) const
      { return const_reverse_sorted_iterator(end()); }
      reverse_sorted_iterator rend(void)
	{ return reverse_sorted_iterator(begin()); }
      const_reverse_sorted_iterator rend(void) const
      { return const_reverse_sorted_iterator(begin()); }
      sorted_iterator sorted_first(void)
      { sorted_iterator it(*this); it.first(); return it; }
      const_sorted_iterator sorted_first(void) const
      { const_sorted_iterator it(*this); it.first(); return it; }
      sorted_iterator sorted_last(void)
      { sorted_iterator it(*this); it.last(); return it; }
      const_sorted_iterator sorted_last(void) const
      { const_sorted_iterator it(*this); it.last(); return it; }
      sorted_iterator sorted_ge(const T &elt);
      const_sorted_iterator sorted_ge(const T &elt) const;
  }; 

  template<class T, class COMP, int pks>
    std::ostream& operator <<(std::ostream& o,
			      dynamic_tree_sorted<T, COMP, pks> &m)
  {
    o << "Nomber of elt :" << m.card() << '\n';
    o << "Index du noeud racine :" << m.root_elt() << '\n';
    for (size_t i = 0; i < m.size(); ++i)
      o << "elt " << i << "  left :" << int(m.left_elt(i)) << " right : "
	<< int(m.right_elt(i)) << " balance :" << int(m.balance(i)) << '\n';
    return o;
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::rotate_right(size_type i)
  {
    tree_elt *pni = &(nodes[i]);
    size_type f = pni->l;
    tree_elt *pnf = &(nodes[f]);
    pni->l = pnf->r; pnf->r = i; pnf->eq = pni->eq = 0;
    return f;
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::rotate_left(size_type i)
  {
    tree_elt *pni = &(nodes[i]);
    size_type f = pni->r;
    tree_elt *pnf = &(nodes[f]);
    pni->r = pnf->l; pnf->l = i; pnf->eq = pni->eq = 0;
    return f;
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::rotate_left_right(size_type i)
  {
    tree_elt *pni = &(nodes[i]);
    size_type f = pni->l;
    tree_elt *pnf = &(nodes[f]);
    short_type uba = pnf->eq, ubb = nodes[pnf->r].eq;
    pni->l = rotate_left(f); f = rotate_right(i);
    pnf = &(nodes[f]);
    pnf->eq = uba - 1;
    nodes[pnf->l].eq = uba - 1 - ((ubb == 1) ? 1 : 0);
    nodes[pnf->r].eq = ((ubb == -1) ? 1 : 0);

    if (uba == 0 && ubb == 1)
    { 
      pnf->l = balance_again(pnf->l);
      if (nodes[pnf->l].eq == 0) pnf->eq = 0;
    }
    return f;
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::rotate_right_left(size_type i)
  { 
    size_type f = nodes[i].r;
    short_type uba = nodes[f].eq, ubb = nodes[nodes[f].l].eq;
    nodes[i].r = rotate_right(f); f = rotate_left(i);
    nodes[f].eq = uba + 1;
    nodes[nodes[f].r].eq = uba + 1 + ((ubb == -1) ? 1 : 0);
    nodes[nodes[f].l].eq = ((ubb == +1) ? -1 : 0);

    if (uba == 0 && ubb == -1)
    { 
      nodes[f].r = balance_again(nodes[f].r);
      if (nodes[nodes[f].r].eq == 0) nodes[f].eq = 0;
    }
    return f;
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::balance_again(size_type i)
  {
    tree_elt *pn = &(nodes[i]);
    switch (pn->eq)
    {
      case -2 : if (nodes[pn->l].eq == -1) return rotate_right(i);
                                      else return rotate_left_right(i);
      case +2 : if (nodes[pn->r].eq == 1) return rotate_left(i);
                                      else return rotate_right_left(i);
      case  0 : case -1 : case 1 : return i;
    #ifdef __GETFEM_VERIFY
      default : DAL_THROW(internal_error, "internal error");
    #endif
    }
    #ifndef __GETFEM_VERIFY
      return ST_NIL;
    #endif
  }

  template<class T, class COMP, int pks>
   void dynamic_tree_sorted<T, COMP, pks>::search_sorted_iterator(const T &elt,
					     const_sorted_iterator &it) const
  {
    it.root();
    while (it.index() != ST_NIL)
    {
      int cp = compar(elt, (*this)[it.index()]);
      if (cp < 0) it.down_left();
      else if (cp > 0) it.down_right(); else break;
    }
  }

  template<class T, class COMP, int pks>
    void dynamic_tree_sorted<T, COMP, pks>::find_sorted_iterator(size_type i,
						    const_sorted_iterator &it) const 
  {
    const T *pelt = &((*this)[i]);
    it.root();
    while (it.index() != ST_NIL)
    {
      int cp = compar(*pelt, (*this)[it.index()]);
      if (cp == 0) { if (it.index() == i) break; else it.down_left(); }
      else if (cp < 0) it.down_left(); else it.down_right();
    }
    if (it.index() == ST_NIL) it.up();
    while (it.index() != i && it.index() != ST_NIL) ++it;
    /* peut etre il faudrait controler dans la boucle le depacement        */
    /* pour eviter de faire tout le tableau en cas de faux indice.         */
  }

  template<class T, class COMP, int pks>
    void dynamic_tree_sorted<T, COMP, pks>::insert_path(const T &elt,
					 const_sorted_iterator &it) const
  {
    it.root();
    while (it.index() != ST_NIL)
    {
      int cp = compar(elt, (*this)[it.index()]);
      if (cp <= 0) it.down_left(); else it.down_right();
    }
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::search_ge(const T &elt) const 
  {
    const_sorted_iterator it(*this); insert_path(elt, it);
    short_type dir = it.direction();
    if (it.index() == ST_NIL)
    { it.up(); if (it.index() != ST_NIL && dir == +1) ++it; }
    return it.index();
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::sorted_iterator
      dynamic_tree_sorted<T, COMP, pks>::sorted_ge(const T &elt)
  {
    const_sorted_iterator it(*this); insert_path(elt, it);
    short_type dir = it.direction();
    if (it.index() == ST_NIL)
    { it.up(); if (it.index() != ST_NIL && dir == +1) ++it; }
    return it;
  } 

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::const_sorted_iterator
      dynamic_tree_sorted<T, COMP, pks>::sorted_ge(const T &elt) const
  {
    const_sorted_iterator it(*this); insert_path(elt, it);
    short_type dir = it.direction();
    if (it.index() == ST_NIL)
    { it.up(); if (it.index() != ST_NIL && dir == +1) ++it; }
    return it;
  }
  

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::memsize(void) const
  {
    return dynamic_tas<T, pks>::memsize() + nodes.memsize()
      + sizeof(dynamic_tree_sorted<T, COMP, pks>);
  }

  template<class T, class COMP, int pks>
    void dynamic_tree_sorted<T, COMP, pks>::compact(void)
  { 
    if (!empty())
      while (ind.last_true() >= ind.card())
	swap(ind.first_false(), ind.last_true());
  }

  template<class T, class COMP, int pks>
    void dynamic_tree_sorted<T, COMP, pks>::add_index(size_type i,
						      const_sorted_iterator &it)
  {
    nodes[i].init();
    if (first_node == ST_NIL)
      first_node = i;
    else
    { 
      short_type dir = it.direction();
      it.up();
      if (dir == -1) nodes[it.index()].l = i; else nodes[it.index()].r = i;

      while(it.index() != ST_NIL)
      {
	short_type *peq = &(nodes[it.index()].eq);
	if (*peq == 0) *peq += dir;
	else
	{ 
	  *peq += dir; 
	  size_type f = balance_again(it.index());
	  dir = it.direction();
	  it.up();
	  switch (dir)
	  {
	    case 0 : first_node = f; break;
	    case -1 : nodes[it.index()].l = f; break;
	    case +1 : nodes[it.index()].r = f; break;
	  }
	  break;
	}
	dir = it.direction();
	it.up();
      }
    }
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::add(const T &f)
  {
    const_sorted_iterator it(*this); insert_path(f, it);
    size_type num = dynamic_tas<T,pks>::add(f);
    add_index(num, it);
    return num;
  }

  template<class T, class COMP, int pks> void
      dynamic_tree_sorted<T, COMP, pks>::add_to_index(size_type i,const T &f) {
    if (!(index_valid(i) && compar(f, (*this)[i]) == 0)) {
      if (index_valid(i)) sup(i);
      dynamic_tas<T,pks>::add_to_index(i, f);
      const_sorted_iterator it(*this); insert_path(f, it);
      add_index(i, it);
    }
  }

  template<class T, class COMP, int pks>
    typename dynamic_tree_sorted<T, COMP, pks>::size_type
      dynamic_tree_sorted<T, COMP, pks>::add_norepeat(const T &f,
						  bool replace, bool *present)
  {
    const_sorted_iterator it(*this); search_sorted_iterator(f, it);
    size_type num = it.index();
    if (num == ST_NIL)
    { 
      if (present != NULL) *present = false;
      num = dynamic_tas<T,pks>::add(f); add_index(num, it);
    }
    else
    {
      if (present != NULL) *present = true;
      if (replace) (*this)[num] = f;
    }
    return num;
  }  

  template<class T, class COMP, int pks> 
    void dynamic_tree_sorted<T, COMP, pks>::resort(void) 
  {
    const_tas_iterator itb
      = ((const dynamic_tree_sorted<T, COMP, pks> *)(this))->tas_begin();
    const_tas_iterator ite
      = ((const dynamic_tree_sorted<T, COMP, pks> *)(this))->tas_end();
    const_sorted_iterator it(*this);
    first_node = ST_NIL;
    while (itb != ite)
    { insert_path(*itb, it); add_index(itb.index(), it); ++itb; }
  }     

  template<class T, class COMP, int pks>
    void dynamic_tree_sorted<T, COMP, pks>::sup_index(size_type i,
						      const_sorted_iterator &it)
  {
    size_type f, ni = i, ic;
    short_type dir;
    tree_elt *pni = &(nodes[i]), *pnc = 0;

    if (pni->l == ST_NIL || pni->r == ST_NIL)
    {
      f = (pni->l != ST_NIL) ? pni->l : pni->r;
      dir = it.direction(); it.up(); ic = it.index();
      if (ic != ST_NIL) pnc = &(nodes[ic]);
      switch (dir)
      {
        case 0 : first_node = f; break;
        case -1 : pnc->l = f; break;
        case +1 : pnc->r = f; break;
      }
    }
    else
    {
      f = it.father(); 
      dir = it.direction();
      it--; ni = it.index();
      switch (dir)
      {
        case 0 : first_node = ni; break;
        case -1 : nodes[f].l = ni; break;
        case +1 : nodes[f].r = ni; break;
      }
      pnc = &(nodes[ni]); f = pnc->l; *pnc = *pni;
      dir = it.direction();
      it.up(); ic = it.index(); if (ic == i) ic = ni; pnc = &(nodes[ic]);
      if (dir == -1) pnc->l = f; else pnc->r = f;
    }

    while (it.index() != ST_NIL)
    {
      short_type ub = pnc->eq;
      pnc->eq -= dir;
      if (ub == 0) break;
      f = balance_again(ic);
      ub = nodes[f].eq;
      dir = it.direction();
      it.up(); ic = it.index(); if (ic == i) ic = ni;
      if (ic != ST_NIL) pnc = &(nodes[ic]);
      switch (dir)
      {
        case 0 : first_node = f; break;
        case -1 : pnc->l = f; break;
        case +1 : pnc->r = f; break;
      }
      if (ub != 0) break;
    }
  }
  
  template<class T, class COMP, int pks>
    void dynamic_tree_sorted<T, COMP, pks>::sup(size_type i)
  {
    if (i >= INT_MAX)
      { DAL_THROW(std::out_of_range, "index" << long(i) << " out of range"); }

    const_sorted_iterator it(*this); find_sorted_iterator(i, it);
    if (it.index() != ST_NIL)
    { sup_index(i, it); dynamic_tas<T, pks>::sup(i); }
  }

  template<class T, class COMP, int pks>
    void dynamic_tree_sorted<T, COMP, pks>::swap(size_type i, size_type j)
  {
    if (i >= INT_MAX || j >= INT_MAX) {
      DAL_THROW(std::out_of_range, "index" << long(std::max(i, j))
		<< " out of range");
    }

    if (i != j)
    {
      const_sorted_iterator it1(*this), it2(*this); it1.end(); it2.end();

      if (index_valid(i)) find_sorted_iterator(i, it1);
      if (index_valid(j)) find_sorted_iterator(j, it2);

      short_type dir1 = it1.direction(), dir2 = it2.direction();
      it1.up(); it2.up(); 
      size_type f1 = it1.index(), f2 = it2.index();

      if (f1!=ST_NIL) { if (dir1==-1) nodes[f1].l = j; else nodes[f1].r = j; }
      if (f2!=ST_NIL) { if (dir2==-1) nodes[f2].l = i; else nodes[f2].r = i; }
      if (first_node==i) first_node=j; else if (first_node==j) first_node=i;
      
      std::swap(nodes[i], nodes[j]);
      ((dynamic_tas<T> *)(this))->swap(i,j);
    }
  }

  /* ********************************************************************* */
  /* Definitition d'un dynamic_tree_sorted utilise comme index sur tableau.*/
  /* ********************************************************************* */
  /* pas completement satisfaisant. A utiliser avec precautions.           */

  template<class T, class TAB, class COMP> struct less_index
       : public std::binary_function<size_t, size_t, int>
  {
    const TAB *tab;
    COMP compare;
    mutable const T *search_elt;
    
    int operator()(size_t i, size_t j) const
    { 
      return compare( (i == ST_NIL) ? *search_elt : (*tab)[i],
		      (j == ST_NIL) ? *search_elt : (*tab)[j] );
    }

    less_index(const TAB &t, const COMP &c)
    { compare = c; tab = &t; }
    less_index(void) {}

  };


  template<class T, class TAB, class COMP = dal::less<T>, int pks = 5>
    class dynamic_tree_sorted_index : public
         dynamic_tree_sorted<size_t, dal::less_index<T,TAB,COMP>, pks>
  {
    public :
     
      typedef typename dynamic_tree_sorted<size_t,
                      dal::less_index<T,TAB,COMP>, pks>::size_type size_type;

    protected :
      
      typedef dynamic_tree_sorted<size_t, dal::less_index<T,TAB,COMP>, pks>
        dts_type;

    public :

      dynamic_tree_sorted_index(TAB &t, COMP cp = COMP())
             : dts_type(less_index<T,TAB,COMP>(t, cp)) { }

      void change_tab(TAB &t, COMP cp = COMP())
      {  comparator() = less_index<T,TAB,COMP>(t, cp); }

      size_type search(const T &elt) const
      { 
	compar.search_elt = &elt;
	return dts_type::search(ST_NIL);
      }
      size_type search_ge(const T &elt) const
      {
	compar.search_elt = &elt;
	return dts_type::search_ge(ST_NIL);
      }
      size_type add(size_type i)
      {
	typename dts_type::const_sorted_iterator it(*this); (*this)[i] = i;
	ind[i] = true; insert_path(i, it); add_index(i, it); return i;
      }

  };

}

#endif /* __DAL_TREE_SORTED_H */
