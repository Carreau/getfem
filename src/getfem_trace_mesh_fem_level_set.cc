// -*- c++ -*- (enables emacs c++ mode)
//========================================================================
//
// Copyright (C) 2007-2007 Yves Renard
//
// This file is a part of GETFEM++
//
// Getfem++ is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 2.1 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,
// USA.
//
//========================================================================

#include <getfem_trace_mesh_fem_level_set.h>

namespace getfem {

  //======================================================================
  // Fem part (sub space fem)
  //======================================================================

  void sub_space_fem::init() {
    cvr = org_fem->ref_convex(cv);
    dim_ = cvr->structure()->dim();
    is_equiv = real_element_defined = true;
    is_polycomp = is_pol = is_lag = false;
    es_degree = 5; /* humm ... */
    ntarget_dim = 1;

    std::stringstream nm;
    nm << "FEM_SUB_SPACE(" << org_fem->debug_name() << ", " << B << ")";
    debug_name_ = nm.str();
    
    init_cvs_node();
    if (org_fem->target_dim() != 1)
      DAL_THROW(to_be_done_error, "Vectorial fems not (yet) supported");

    base_node P(dim()); gmm::fill(P, 1./4.);
    for (size_type k = 0; k < ind.size(); ++k)
	add_node(global_dof(dim()), P);
  }

  size_type sub_space_fem::index_of_global_dof(size_type, size_type j) const
  { return ind[j]; }

  void sub_space_fem::base_value(const base_node &, 
				 base_tensor &) const
  { DAL_THROW(internal_error, "No base values, real only element."); }
  void sub_space_fem::grad_base_value(const base_node &, 
				      base_tensor &) const
  { DAL_THROW(internal_error, "No base values, real only element."); }
  void sub_space_fem::hess_base_value(const base_node &, 
			     base_tensor &) const
  { DAL_THROW(internal_error, "No base values, real only element."); }

  void sub_space_fem::real_base_value(const fem_interpolation_context &c,
				    base_tensor &t, bool) const {
    fem_interpolation_context c0 = c;
    base_tensor val_e;
    
    if (c0.have_pfp()) {
      c0.set_pfp(fem_precomp(org_fem, &c0.pfp()->get_point_tab()));
    } else { c0.set_pf(org_fem); }
    c0.base_value(val_e);
    
    t.mat_transp_reduction(val_e, B, 0);
    // cout << "elt " << c.convex_num() << " t = " << t << endl;
  }

  void sub_space_fem::real_grad_base_value(const fem_interpolation_context &c,
					   base_tensor &t, bool) const {
    fem_interpolation_context c0 = c;
    base_tensor grad_e;
    
    if (c0.have_pfp()) {
      c0.set_pfp(fem_precomp(org_fem, &c0.pfp()->get_point_tab()));
    } else { c0.set_pf(org_fem); }
    c0.grad_base_value(grad_e);

    t.mat_transp_reduction(grad_e, B, 0);
  }
  
  void sub_space_fem::real_hess_base_value(const fem_interpolation_context &c,
				  base_tensor &t, bool) const {
    fem_interpolation_context c0 = c;
    base_tensor hess_e;
   
    if (c0.have_pfp()) {
      c0.set_pfp(fem_precomp(org_fem, &c0.pfp()->get_point_tab()));
    } else { c0.set_pf(org_fem); }
    c0.hess_base_value(hess_e);
    
    t.mat_transp_reduction(hess_e, B, 0);
  }

  //======================================================================
  // Mesh fem part
  //======================================================================

  
  void trace_mesh_fem_level_set::receipt(const MESH_CLEAR &)
  { clear(); is_adapted = false; }
  void trace_mesh_fem_level_set::receipt(const MESH_DELETE &)
  { clear(); is_adapted = false; }
  void trace_mesh_fem_level_set::clear_build_methods() {
    for (size_type i = 0; i < build_methods.size(); ++i)
      del_stored_object(build_methods[i]);
    build_methods.clear();
  }
  void trace_mesh_fem_level_set::clear(void) {
    mesh_fem::clear();
    clear_build_methods();
    is_adapted = false;
  }

  static void clear_pairs(gmm::row_matrix<gmm::rsvector<bool> > &pairs,
			  size_type i) {
    gmm::linalg_traits<gmm::rsvector<bool> >::const_iterator
      it = gmm::vect_const_begin(pairs[i]),
      ite = gmm::vect_const_end(pairs[i]);
    for (; it != ite; ++it) if (it.index() != i) pairs[it.index()].sup(i);
    gmm::clear(pairs[i]);
  }
  
  trace_mesh_fem_level_set::trace_mesh_fem_level_set(const mesh_level_set &me,
						     const mesh_fem &mef,
						     unsigned degree_,
						     unsigned strategy_)
    : mesh_fem(mef.linked_mesh()), mls(me), mf(mef), degree(degree_),
      strategy(strategy_) {
    if (mf.get_qdim() != 1)
      DAL_THROW(to_be_done_error, "base mesh_fem for mesh_fem_level_set has "
		"to be of qdim one for the moment ...");
    this->add_dependency(mls);
    is_adapted = false;
  }

  DAL_SIMPLE_KEY(special_tracemf_key, pfem);

  void trace_mesh_fem_level_set::adapt(void) {
    context_check();
    clear();
    
    pfem pf
      = classical_fem(bgeot::simplex_geotrans(linked_mesh().dim(), 1), degree);
    base_matrix G;
    std::vector<base_node> pts;
    size_type mfndof = mf.nb_dof();
    gmm::row_matrix<gmm::rsvector<bool> > pairs(mfndof, mfndof);
    std::vector<std::vector<size_type> > selected_comb(mfndof);
    size_type nbdof(0);
    for (dal::bv_visitor cv(linked_mesh().convex_index());
	 !cv.finished(); ++cv) {
      
      if (mls.is_convex_cut(cv)) {


	pts.resize(0);
	  
	// Step 1 - Building a set of points of the intersection of the
	// element with the level-set.
	const mesh &msh(mls.mesh_of_convex(cv));
	
	bgeot::pgeometric_trans pgt = linked_mesh().trans_of_convex(cv);
	bgeot::pgeometric_trans pgt2
	  = msh.trans_of_convex(msh.convex_index().first_true());
	std::vector<mesher_level_set> mesherls0(mls.nb_level_sets());
	std::vector<mesher_level_set> mesherls1(mls.nb_level_sets());
	
	for (unsigned i = 0; i < mls.nb_level_sets(); ++i) {
	  mesherls0[i] =  mls.get_level_set(i)->mls_of_convex(cv, 0, false);
	  if (mls.get_level_set(i)->has_secondary())
	    mesherls1[i] = mls.get_level_set(i)->mls_of_convex(cv, 1, false);
	}
	
	scalar_type lenght_estimate(0);
	base_small_vector up(msh.dim());
	for (dal::bv_visitor i(msh.convex_index()); !i.finished(); ++i) {
	  unsigned f; bool lisin = false;
	  for (f = 0; f < pgt2->structure()->nb_faces(); ++f) {
	    
	    // for each face of the sub-elements, testing if this face is on
	    // the level-set.
	    for (unsigned ils = 0; ils < mls.nb_level_sets(); ++ils) {
	      lisin = true;
	      for (unsigned ipt = 0;
		   ipt < pgt2->structure()->nb_points_of_face(f); ++ipt) {
		lisin = lisin &&
		  (gmm::abs((mesherls0[ils])
			    (msh.points_of_face_of_convex(i, f)[ipt])) < 1E-7);
		if (mls.get_level_set(ils)->has_secondary())
		  lisin = lisin &&
		    ((mesherls1[ils])
		     (msh.points_of_face_of_convex(i, f)[ipt]) < -1E-7);
	      }
	      if (lisin) break;
	    }
	    if (!lisin) continue;
	    // A face is detected to be on the level-set,
	    // building a set of points on this face.
	    
	    vectors_to_base_matrix(G, msh.points_of_convex(i));
	    bgeot::geotrans_interpolation_context
	      c(msh.trans_of_convex(i),
		pf->node_convex(0).points_of_face(f)[0], G);
	    gmm::mult(c.B(), msh.trans_of_convex(i)->normals()[f], up);
	    lenght_estimate += c.J() * gmm::vect_norm2(up);
	    for (size_type j=0; j<pf->structure(0)->nb_points_of_face(f);
		 ++j) {
	      c.set_xref(pf->node_convex(0).points_of_face(f)[j]);
	      pts.push_back(c.xreal());
	    }
	  }
	}
	
	if (pts.size() == 0) continue;
	
	if (strategy == 3) {
	  cout << "cv " << cv << " l = " << lenght_estimate << endl;
	  if (lenght_estimate > 0.3) {
	    ++nbdof;
	    for (size_type j = 0; j < mf.nb_dof_of_element(cv); ++j)
	      selected_comb[mf.ind_dof_of_element(cv)[j]].push_back(nbdof);
	  }
	}	

	if (strategy == 1 || strategy == 2) {
	
	  // Step 2 - Selecting the right number of points
	  std::vector<size_type> selection;
	  for (size_type k=0; k < pf->structure(0)->nb_points_of_face(0);++k) {
	    size_type ipt = size_type(-1);
	    scalar_type maxdmin = scalar_type(0);
	    
	    for (size_type i = 0; i < pts.size(); ++i) {
	      scalar_type dmin = (selection.size() == 0)
		? gmm::vect_norm2(pts[i])
		: gmm::vect_dist2(pts[i], pts[selection[0]]);
	      for (size_type j = 1; j < selection.size(); ++j) {
		dmin=std::min(dmin,gmm::vect_dist2(pts[i], pts[selection[j]]));
		if (dmin <= maxdmin) break;
	      }
	      if (dmin > maxdmin) { maxdmin = dmin; ipt = i; }
	    }
	    
	    if (ipt ==  size_type(-1))
	      DAL_THROW(failure_error, "Trace fem: not enought points");
	    selection.push_back(ipt);
	  }
	  
	  
	  // Step 3 - Now that a set of points is selected, evaluating the base
	  // function of the fem on it and retaining the two base functions
	  // having the largest value on each point. Adding them to the list
	  // of candidate pairs of dof.
	  
	  pfem pfcv = mf.fem_of_element(cv);
	  vectors_to_base_matrix(G, linked_mesh().points_of_convex(cv));
	  
	  fem_interpolation_context c(pgt, pfcv, pts[selection[0]], G, cv);
	  base_tensor t;
	  
	  for (size_type i = 0; i < selection.size(); ++i) { 
	    c.set_xref(pts[selection[i]]);
	    c.base_value(t);
	    
	    size_type n1 = size_type(-1), n2(0);
	    scalar_type y1(0), y2(0);
	    
	    for (size_type j = 0; j < pfcv->nb_dof(cv); ++j) {
	      if (t[j] > y1) { n2 = n1; y2 = y1; n1 = j; y1 = t[j]; }
	      else if (t[j] > y2) { n2 = j; y2 = t[j]; }
	    }
	    if (n1 == size_type(-1)) DAL_INTERNAL_ERROR("");
	    size_type nd1 = mf.ind_dof_of_element(cv)[n1];
	    size_type nd2 = mf.ind_dof_of_element(cv)[n2];
	    
	    if (y2 < scalar_type(1e-2)) pairs(nd1, nd1) = true;
	    else pairs(nd1, nd2) = pairs(nd2, nd1) = true;
	  }
	  
	  
	}
	
      }
    }

    // Step 4 - At this stage, a certain number of pairs of dof have been
    // produced.
    // Now, a selection is made following the chosen strategy.

    bool ttouched;
    
    switch(strategy) {
   
    case 1:
      // Strategy 1:
      //   a - Singletons are selected, other pairs containing
      //       this dof are eliminated.
      //   b - Between pairs having a dof exclusively for their own and
      //       attached to the same other dof, one pair is arbitrary
      //       selected and the others are eliminated.
      //   c - If all the pairs are linked with both the two dofs, one pair
      //       is arbitrary selected, the pairs having a common dof with this
      //       pair are eliminated. loop to b.

      for (size_type i = 0; i < mf.nb_dof(); ++i)
	if (pairs(i,i))
	  { selected_comb[i].push_back(++nbdof); clear_pairs(pairs, i); }
      
      do {
	int nb_nnz(0), lasti(0), lastj(0);
	ttouched = false;
	
	for (size_type i = 0; i < mf.nb_dof(); ++i) { // to be optimized ... 
	  if (gmm::nnz(pairs[i]) > 0) {
	    lasti = i;
	    lastj = gmm::vect_const_begin(pairs[i]).index();
	    ++nb_nnz;
	    if (gmm::nnz(pairs[i]) == 1) {
	      selected_comb[lasti].push_back(++nbdof);
	      selected_comb[lastj].push_back(nbdof);
	      clear_pairs(pairs, lasti); clear_pairs(pairs, lastj);
	      ttouched = true;
	    }
	  }
	}
	if (nb_nnz > 0 && !ttouched) {
	  selected_comb[lasti].push_back(++nbdof);
	  selected_comb[lastj].push_back(nbdof);
	  clear_pairs(pairs, lasti); clear_pairs(pairs, lastj);
	  ttouched = true;
	}
      } while (ttouched);
      break;

    case 2:
      // Strategy 2:
      //   a - Singletons not having their dof in common with any pair are 
      //       selected, others are eliminated.
      //   b - Pairs having exclusivity of one dof are selected
      //   c - Pairs having there two dofs in common and at least one with
      //       a retained pair are eliminated.
      //   d - if b and c gives no result, a pair is selected, preferabily
      //       having no common dof with selected pairs. loop to a.

      for (size_type i = 0; i < mf.nb_dof(); ++i)
	if (pairs(i,i) == true) {
	  if (gmm::nnz(pairs[i]) == 1) selected_comb[i].push_back(++nbdof);
	  pairs[i].sup(i);
	  cout << "adding sing " << i << endl;
	}
      
      do {
	int nb_nnz(0), lasti(0), lastj(0);
	ttouched = false;

	for (size_type i = 0; i < mf.nb_dof(); ++i) { // to be optimized ... 
	  if (gmm::nnz(pairs[i]) == 1 && selected_comb[i].size() == 0) {
	    lasti = i;
	    lastj = gmm::vect_const_begin(pairs[i]).index();
	    selected_comb[lasti].push_back(++nbdof);
	    selected_comb[lastj].push_back(nbdof);
	    pairs[lasti].sup(lastj); pairs[lastj].sup(lasti);
	    ttouched = true;
	  }
	}

	for (size_type i = 0; i < mf.nb_dof(); ++i) { // to be optimized ...
	  size_type a1 = selected_comb[i].size(), b1 = gmm::nnz(pairs[i]);
	  gmm::linalg_traits<gmm::rsvector<bool> >::const_iterator
	    it = gmm::vect_const_begin(pairs[i]),
	    ite = gmm::vect_const_end(pairs[i]);
	  for (; it != ite; ++it) {
	    lasti = i; ++nb_nnz;
	    lastj = it.index();
	    // cout << "lastj = " << lastj << endl;
	    size_type a2 = selected_comb[lastj].size();
	    size_type b2 = gmm::nnz(pairs[lastj]);
	    if ((a1 > 0 || a2 > 0) && (a1+b1 > 1) && (a2+b2 > 1)) {
	      pairs[lasti].sup(lastj); pairs[lastj].sup(lasti);
	      it = gmm::vect_const_begin(pairs[i]);
	      ite = gmm::vect_const_end(pairs[i]);
	      if (it == ite) break;
	      ttouched = true;
	    }
	  }
	}

	if (nb_nnz > 0 && !ttouched) {	  
	  selected_comb[lasti].push_back(++nbdof);
	  selected_comb[lastj].push_back(nbdof);
	  pairs[lasti].sup(lastj); pairs[lastj].sup(lasti);
	  ttouched = true;
	}
      } while (ttouched);
      break;

    case 3:
      // Strategy 3:
      //  Only adapted to P1. One new base function per element, which is
      //  the sum of the base function on the element.
      //  nothing to do here.

      break;

    }
    // Step 5 - Now that the convenient pairs of dofs are selected, the
    // special fem are built.

    std::vector<size_type> glob_dof;
    for (dal::bv_visitor cv(linked_mesh().convex_index());
	 !cv.finished(); ++cv) {
      base_matrix B;
      if (mls.is_convex_cut(cv)) {
	glob_dof.resize(0);
	size_type nr = 0;
	gmm::resize(B, nr, mf.nb_dof_of_element(cv));

	for (size_type i = 0; i < mf.nb_dof_of_element(cv); ++i) {
	  size_type id = mf.ind_dof_of_element(cv)[i];
	  if (selected_comb[id].size() > 0) {
	    std::vector<size_type>::const_iterator
	      it = selected_comb[id].begin(),
	      ite = selected_comb[id].end();
	    for (; it != ite; ++it) {
	      std::vector<size_type>::iterator itf =
		std::find(glob_dof.begin(), glob_dof.end(), (*it - 1));
	      if (itf == glob_dof.end()) {
		glob_dof.push_back(*it - 1);
		++nr;
		gmm::resize(B, nr, mf.nb_dof_of_element(cv));
		B(nr-1, i) = scalar_type(1);
	      }
	      else
		B(itf - glob_dof.begin(), i) = scalar_type(1);
	    }
	  }
	}
	
	if (glob_dof.size()) {
	  cout << "elt " << cv << "glob_dof = " << glob_dof 
	       << " B = " << B << endl;
	  
      	  pfem pfnew = new sub_space_fem(mf.fem_of_element(cv), glob_dof,
					 B, cv);
	  dal::add_stored_object(new special_tracemf_key(pfnew), pfnew,
				 pfnew->ref_convex(0),
				 pfnew->node_tab(0));
	  build_methods.push_back(pfnew);
	  set_finite_element(cv, pfnew);
	}
      }
    }
    is_adapted = true; touch();
  }


}  /* end of namespace getfem.                                            */

