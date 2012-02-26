/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================
 
 Copyright (C) 2006-2012 Yves Renard
 
 This file is a part of GETFEM++
 
 Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
 under  the  terms  of the  GNU  Lesser General Public License as published
 by  the  Free Software Foundation;  either version 2.1 of the License,  or
 (at your option) any later version.
 This program  is  distributed  in  the  hope  that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License for more details.
 You  should  have received a copy of the GNU Lesser General Public License
 along  with  this program;  if not, write to the Free Software Foundation,
 Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 
 As a special exception, you  may use  this file  as it is a part of a free
 software  library  without  restriction.  Specifically,  if   other  files
 instantiate  templates  or  use macros or inline functions from this file,
 or  you compile this  file  and  link  it  with other files  to produce an
 executable, this file  does  not  by itself cause the resulting executable
 to be covered  by the GNU Lesser General Public License.  This   exception
 does not  however  invalidate  any  other  reasons why the executable file
 might be covered by the GNU Lesser General Public License.
 
===========================================================================*/

/**@file getfem_fourth_order.h
   @author  Yves Renard <Yves.Renard@insa-lyon.fr>,
            Julien Pommier <Julien.Pommier@insa-toulouse.fr>
   @date January 6, 2006.
   @brief assembly procedures and bricks for fourth order pdes.
*/
#ifndef GETFEM_FOURTH_ORDER_H_
#define GETFEM_FOURTH_ORDER_H__

#include "getfem_modeling.h"
#include "getfem_models.h"
#include "getfem_assembling_tensors.h"

namespace getfem {
  
  /* ******************************************************************** */
  /*		Bilaplacian assembly routines.                            */
  /* ******************************************************************** */

  /**
     assembly of @f$\int_\Omega \Delta u \Delta v@f$.
     @ingroup asm
  */
  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_bilaplacian
  (const MAT &M, const mesh_im &mim, const mesh_fem &mf,
   const mesh_fem &mf_data, const VECT &A,
   const mesh_region &rg = mesh_region::all_convexes()) {
    generic_assembly assem
      ("a=data$1(#2);"
       "M(#1,#1)+=sym(comp(Hess(#1).Hess(#1).Base(#2))(:,i,i,:,j,j,k).a(k))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_mf(mf_data);
    assem.push_data(A);
    assem.push_mat(const_cast<MAT &>(M));
    assem.assembly(rg);
  }

  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_homogeneous_bilaplacian
  (const MAT &M, const mesh_im &mim, const mesh_fem &mf,
   const VECT &A, const mesh_region &rg = mesh_region::all_convexes()) {
    generic_assembly assem
      ("a=data$1(1);"
       "M(#1,#1)+=sym(comp(Hess(#1).Hess(#1))(:,i,i,:,j,j).a(1))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_data(A);
    assem.push_mat(const_cast<MAT &>(M));
    assem.assembly(rg);
  }


  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_bilaplacian_KL
  (const MAT &M, const mesh_im &mim, const mesh_fem &mf,
   const mesh_fem &mf_data, const VECT &D_, const VECT &nu_,
   const mesh_region &rg = mesh_region::all_convexes()) {
    generic_assembly assem
      ("d=data$1(#2); n=data$2(#2);"
       "t=comp(Hess(#1).Hess(#1).Base(#2).Base(#2));"
       "M(#1,#1)+=sym(t(:,i,j,:,i,j,k,l).d(k)-t(:,i,j,:,i,j,k,l).d(k).n(l)"
       "+t(:,i,i,:,j,j,k,l).d(k).n(l))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_mf(mf_data);
    assem.push_data(D_);
    assem.push_data(nu_);
    assem.push_mat(const_cast<MAT &>(M));
    assem.assembly(rg);
  }

  template<typename MAT, typename VECT>
  void asm_stiffness_matrix_for_homogeneous_bilaplacian_KL
  (const MAT &M, const mesh_im &mim, const mesh_fem &mf,
   const VECT &D_, const VECT &nu_,
   const mesh_region &rg = mesh_region::all_convexes()) {
    generic_assembly assem
      ("d=data$1(1); n=data$2(1);"
       "t=comp(Hess(#1).Hess(#1));"
       "M(#1,#1)+=sym(t(:,i,j,:,i,j).d(1)-t(:,i,j,:,i,j).d(1).n(1)"
       "+t(:,i,i,:,j,j).d(1).n(1))");
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_data(D_);
    assem.push_data(nu_);
    assem.push_mat(const_cast<MAT &>(M));
    assem.assembly(rg);
  }

  /* ******************************************************************** */
  /*		Bilaplacian new bricks.                                    */
  /* ******************************************************************** */

  
  /** Adds a bilaplacian brick on the variable
      `varname` and on the mesh region `region`. 
      This represent a term :math:`\Delta(D \Delta u)`. 
      where :math:`D(x)` is a coefficient determined by `dataname` which
      could be constant or described on a f.e.m. The corresponding weak form
      is :math:`\int D(x)\Delta u(x) \Delta v(x) dx`.
  */
  size_type add_bilaplacian_brick
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &dataname, size_type region = size_type(-1));

  /** Adds a bilaplacian brick on the variable
      `varname` and on the mesh region `region`.
      This represent a term :math:`\Delta(D \Delta u)` where :math:`D(x)`
      is a the flexion modulus determined by `dataname1`. The term is
      integrated by part following a Kirchhoff-Love plate model
      with `dataname2` the poisson ratio.
  */
  size_type add_bilaplacian_brick_KL
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &dataname1, const std::string &dataname2,
   size_type region = size_type(-1));

  
  /* ******************************************************************** */
  /*		Bilaplacian old brick.                                    */
  /* ******************************************************************** */

# define MDBRICK_BILAPLACIAN 783465
  
  /** Bilaplacian brick @f$ D \Delta \Delta u @f$.

  @see asm_stiffness_matrix_for_bilaplacian
  @see mdbrick_mixed_isotropic_linearized_plate
  @ingroup bricks 
  */
  template<typename MODEL_STATE = standard_model_state>
  class mdbrick_bilaplacian
    : public mdbrick_abstract_linear_pde<MODEL_STATE> {
    
    TYPEDEF_MODEL_STATE_TYPES;

    bool KL;    /* Pure bilaplacian or Kirchhoff-Love plate model.        */
    mdbrick_parameter<VECTOR> D_;  /* D_ a scalar field (flexion modulus). */
    mdbrick_parameter<VECTOR> nu_;  /* nu_ a scalar field (Poisson ratio). */

    void proper_update_K(void) {
      if (!KL) {
	GMM_TRACE2("Assembling bilaplacian operator");
	asm_stiffness_matrix_for_bilaplacian
	  (this->K, this->mim, this->mf_u, D().mf(),  D().get(),
	   this->mf_u.linked_mesh().get_mpi_region());
      }
      else {
	GMM_ASSERT1(&(D().mf()) == &(nu().mf()), "mesh fems for the two "
		    "coefficients must be the same");
	GMM_TRACE2("Assembling bilaplacian for a Kirchhoff-Love plate");
	asm_stiffness_matrix_for_bilaplacian_KL
	  (this->K, this->mim, this->mf_u, D().mf(),  D().get(), nu().get(),
	   this->mf_u.linked_mesh().get_mpi_region());
      }
    }
  public :

    /** accessor to the coefficient D */
    mdbrick_parameter<VECTOR> &D() { return D_; }
    const mdbrick_parameter<VECTOR> &D() const { return D_; }
    /** accessor to the coefficient nu */
    mdbrick_parameter<VECTOR> &nu() { return nu_; }
    const mdbrick_parameter<VECTOR> &nu() const { return nu_; }

    void set_to_KL(void) { KL = true; }

    /** Constructor, the default coeff is a scalar equal to one
	(i.e. it gives the Laplace operator).

        The coeff can be later changed.

	@param mim the integration method that is used. 
	@param mf_u the mesh_fem for the unknown u.
	@param KL_ true for the Kirchhoff-Love plate model.
    */
    mdbrick_bilaplacian(const mesh_im &mim_, const mesh_fem &mf_u_, 
			bool KL_ = false)
      : mdbrick_abstract_linear_pde<MODEL_STATE>(mim_, mf_u_,
						 MDBRICK_BILAPLACIAN),
	KL(KL_), D_("D", mf_u_.linked_mesh(), this),
	nu_("nu", mf_u_.linked_mesh(), this) { D().set(1.); nu().set(0.3); }
  };


  /* ******************************************************************** */
  /*		Normale derivative source term assembly routines.         */
  /* ******************************************************************** */

  /**
     assembly of @f$\int_\Gamma{\partial_n u f}@f$.
     @ingroup asm
  */
  template<typename VECT1, typename VECT2>
  void asm_normal_derivative_source_term
  (VECT1 &B, const mesh_im &mim, const mesh_fem &mf, const mesh_fem &mf_data,
   const VECT2 &F, const mesh_region &rg) {
    GMM_ASSERT1(mf_data.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");

    size_type Q = gmm::vect_size(F) / mf_data.nb_dof();

    const char *s;
    if (mf.get_qdim() == 1 && Q == 1)
      s = "F=data(#2);"
	"V(#1)+=comp(Grad(#1).Normal().Base(#2))(:,i,i,j).F(j);";
    else if (mf.get_qdim() == 1 && Q == gmm::sqr(mf.linked_mesh().dim()))
      s = "F=data(mdim(#1),mdim(#1),#2);"
	"V(#1)+=comp(Grad(#1).Normal().Normal().Normal().Base(#2))"
	"(:,i,i,k,l,j).F(k,l,j);";
    else if (mf.get_qdim() > size_type(1) && Q == mf.get_qdim())
      s = "F=data(qdim(#1),#2);"
	"V(#1)+=comp(vGrad(#1).Normal().Base(#2))(:,i,k,k,j).F(i,j);";
    else if (mf.get_qdim() > size_type(1) &&
	     Q == size_type(mf.get_qdim()*gmm::sqr(mf.linked_mesh().dim())))
      s = "F=data(qdim(#1),mdim(#1),mdim(#1),#2);"
	"V(#1)+=comp(vGrad(#1).Normal().Normal().Normal().Base(#2))"
	"(:,i,k,k,l,m,j).F(i,l,m,j);";
    else
      GMM_ASSERT1(false, "invalid rhs vector");
    asm_real_or_complex_1_param(B, mim, mf, mf_data, F, rg, s);
  }

  template<typename VECT1, typename VECT2>
  void asm_homogeneous_normal_derivative_source_term
  (VECT1 &B, const mesh_im &mim, const mesh_fem &mf,
   const VECT2 &F, const mesh_region &rg) {
    
    size_type Q = gmm::vect_size(F);

    const char *s;
    if (mf.get_qdim() == 1 && Q == 1)
      s = "F=data(1);"
	"V(#1)+=comp(Grad(#1).Normal())(:,i,i).F(1);";
    else if (mf.get_qdim() == 1 && Q == gmm::sqr(mf.linked_mesh().dim()))
      s = "F=data(mdim(#1),mdim(#1));"
	"V(#1)+=comp(Grad(#1).Normal().Normal().Normal())"
	"(:,i,i,l,j).F(l,j);";
    else if (mf.get_qdim() > size_type(1) && Q == mf.get_qdim())
      s = "F=data(qdim(#1));"
	"V(#1)+=comp(vGrad(#1).Normal())(:,i,k,k).F(i);";
    else if (mf.get_qdim() > size_type(1) &&
	     Q == size_type(mf.get_qdim()*gmm::sqr(mf.linked_mesh().dim())))
      s = "F=data(qdim(#1),mdim(#1),mdim(#1));"
	"V(#1)+=comp(vGrad(#1).Normal().Normal().Normal())"
	"(:,i,k,k,l,m).F(i,l,m);";
    else
      GMM_ASSERT1(false, "invalid rhs vector");
    asm_real_or_complex_1_param(B, mim, mf, mf, F, rg, s);
  }


  /* ******************************************************************** */
  /*		Normale derivative source term new brick.                 */
  /* ******************************************************************** */


  /** Adds a normal derivative source term brick
      :math:`F = \int b.\partial_n v` on the variable `varname` and the
      mesh region `region`.
     
      Update the right hand side of the linear system.
      `dataname` represents `b` and `varname` represents `v`.
  */
  size_type add_normal_derivative_source_term_brick
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &dataname, size_type region);



  /* ******************************************************************** */
  /*		Normale derivative source term old brick.                 */
  /* ******************************************************************** */


  /**
     Normal derivative source term brick ( @f$ F = \int b.\partial_n v @f$ ).
     
     Update the right hand side of the linear system.

     @see asm_source_term
     @ingroup bricks
  */
  template<typename MODEL_STATE = standard_model_state>
  class mdbrick_normal_derivative_source_term
    : public mdbrick_abstract<MODEL_STATE>  {

    TYPEDEF_MODEL_STATE_TYPES;

    mdbrick_parameter<VECTOR> B_;
    VECTOR F_;
    bool F_uptodate;
    size_type boundary, num_fem, i1, nbd;

    const mesh_fem &mf_u(void) const { return this->get_mesh_fem(num_fem); }

    void proper_update(void) {
      i1 = this->mesh_fem_positions[num_fem];
      nbd = mf_u().nb_dof();
      gmm::resize(F_, nbd);
      gmm::clear(F_);
      F_uptodate = false;
    }

  public :

    mdbrick_parameter<VECTOR> &scalar_source_term(void)
    { B_.reshape(mf_u().get_qdim()); return B_;  }

    mdbrick_parameter<VECTOR> &tensorial_source_term(void) {
      B_.reshape(mf_u().get_qdim()*gmm::sqr(mf_u().linked_mesh().dim()));
      return B_;
    }

    const mdbrick_parameter<VECTOR> &source_term(void) const { return B_; }

    /// gives the right hand side of the linear system.
    const VECTOR &get_F(void) { 
      this->context_check();
      if (!F_uptodate || this->parameters_is_any_modified()) {
	F_uptodate = true;
	GMM_TRACE2("Assembling a source term");
	asm_normal_derivative_source_term
	  (F_, *(this->mesh_ims[0]), mf_u(), B_.mf(), B_.get(),
	   mf_u().linked_mesh().get_mpi_sub_region(boundary));
	this->parameters_set_uptodate();
      }
      return F_;
    }

    virtual void do_compute_tangent_matrix(MODEL_STATE &, size_type,
					   size_type) { }
    virtual void do_compute_residual(MODEL_STATE &MS, size_type i0,
				     size_type) {
      gmm::add(gmm::scaled(get_F(), value_type(-1)),
	       gmm::sub_vector(MS.residual(), gmm::sub_interval(i0+i1, nbd)));
    }

    /** Constructor defining the rhs
	@param problem the sub-problem to which this brick applies.
	@param mf_data_ the mesh_fem on which B_ is defined.
	@param B_ the value of the source term.
	@param bound the mesh boundary number on which the source term
	is applied.
	@param num_fem_ the mesh_fem number on which this brick is is applied.
    */
    mdbrick_normal_derivative_source_term
    (mdbrick_abstract<MODEL_STATE> &problem, const mesh_fem &mf_data_,
     const VECTOR &B__, size_type bound,
     size_type num_fem_=0) : B_("source_term",mf_data_, this), boundary(bound),
			     num_fem(num_fem_) {
      this->add_sub_brick(problem);
      if (bound != size_type(-1))
	this->add_proper_boundary_info(num_fem, bound,
				       MDBRICK_NORMAL_DERIVATIVE_NEUMANN);
      this->force_update();
      size_type Nb = gmm::vect_size(B__);
      if (Nb) {
	if (Nb == mf_data_.nb_dof() * mf_u().get_qdim()) {
	  B_.reshape(mf_u().get_qdim());
	   
	}
	else if (Nb == mf_data_.nb_dof() * mf_u().get_qdim()
		 * gmm::sqr(mf_u().linked_mesh().dim())) {
	  B_.reshape(mf_u().get_qdim()*gmm::sqr(mf_u().linked_mesh().dim()));
	}
	else 
	  GMM_ASSERT1(false, "Rhs vector has a wrong size");
	B_.set(B__);
      }
      else {
	B_.reshape(this->get_mesh_fem(num_fem).get_qdim());
      }
    }
  };

  /* ******************************************************************** */
  /*   	Special boundary condition for Kirchhoff-Love model.              */
  /* ******************************************************************** */

  /*
    assembly of the special boundary condition for Kirchhoff-Love model.
    @ingroup asm
  */
  template<typename VECT1, typename VECT2>
  void asm_neumann_KL_term
  (VECT1 &B, const mesh_im &mim, const mesh_fem &mf, const mesh_fem &mf_data,
   const VECT2 &M, const VECT2 &divM, const mesh_region &rg) {
    GMM_ASSERT1(mf_data.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");

    generic_assembly assem
      ("MM=data$1(mdim(#1),mdim(#1),#2);"
       "divM=data$2(mdim(#1),#2);"
       "V(#1)+=comp(Base(#1).Normal().Base(#2))(:,i,j).divM(i,j);"
       "V(#1)+=comp(Grad(#1).Normal().Base(#2))(:,i,j,k).MM(i,j,k)*(-1);"
       "V(#1)+=comp(Grad(#1).Normal().Normal().Normal().Base(#2))(:,i,i,j,k,l).MM(j,k,l);");
    
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_mf(mf_data);
    assem.push_data(M);
    assem.push_data(divM);
    assem.push_vec(B);
    assem.assembly(rg);
  }

  template<typename VECT1, typename VECT2>
  void asm_neumann_KL_homogeneous_term
  (VECT1 &B, const mesh_im &mim, const mesh_fem &mf,
   const VECT2 &M, const VECT2 &divM, const mesh_region &rg) {

    generic_assembly assem
      ("MM=data$1(mdim(#1),mdim(#1));"
       "divM=data$2(mdim(#1));"
       "V(#1)+=comp(Base(#1).Normal())(:,i).divM(i);"
       "V(#1)+=comp(Grad(#1).Normal())(:,i,j).MM(i,j)*(-1);"
       "V(#1)+=comp(Grad(#1).Normal().Normal().Normal())(:,i,i,j,k).MM(j,k);");
    
    assem.push_mi(mim);
    assem.push_mf(mf);
    assem.push_data(M);
    assem.push_data(divM);
    assem.push_vec(B);
    assem.assembly(rg);
  }

  /* ******************************************************************** */
  /*		Kirchoff Love Neumann term new brick.                     */
  /* ******************************************************************** */


  /** Adds a Neumann term brick for Kirchhoff-Love model
      on the variable `varname` and the mesh region `region`.
      `dataname1` represents the bending moment tensor and  `dataname2`
      its divergence.
  */
  size_type add_Kirchoff_Love_Neumann_term_brick
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &dataname1, const std::string &dataname2,
   size_type region);



  /**
     Old Brick for Special boundary condition for Kirchhoff-Love model

     @see asm_source_term
     @ingroup bricks
  */
  template<typename MODEL_STATE = standard_model_state>
  class mdbrick_neumann_KL_term : public mdbrick_abstract<MODEL_STATE>  {

    TYPEDEF_MODEL_STATE_TYPES;

    mdbrick_parameter<VECTOR> M_, divM_;
    VECTOR F_;
    bool F_uptodate;
    size_type boundary, num_fem, i1, nbd;

    const mesh_fem &mf_u(void) const { return this->get_mesh_fem(num_fem); }

    void proper_update(void) {
      i1 = this->mesh_fem_positions[num_fem];
      nbd = mf_u().nb_dof();
      gmm::resize(F_, nbd);
      gmm::clear(F_);
      F_uptodate = false;
    }

  public :

    mdbrick_parameter<VECTOR> &M(void) {
      M_.reshape(gmm::sqr(mf_u().linked_mesh().dim()));
      return M_;
    }

    const mdbrick_parameter<VECTOR> &M(void) const { return M_; }

    mdbrick_parameter<VECTOR> &divM(void) {
      divM_.reshape(mf_u().linked_mesh().dim());
      return divM_;
    }

    const mdbrick_parameter<VECTOR> &divM(void) const { return divM_; }

    /// gives the right hand side of the linear system.
    const VECTOR &get_F(void) { 
      this->context_check();
      if (!F_uptodate || this->parameters_is_any_modified()) {
	F_uptodate = true;
	GMM_TRACE2("Assembling a source term");
	asm_neumann_KL_term
	  (F_, *(this->mesh_ims[0]), mf_u(), M_.mf(), M_.get(), divM_.get(),
	   mf_u().linked_mesh().get_mpi_sub_region(boundary));
	this->parameters_set_uptodate();
      }
      return F_;
    }

    virtual void do_compute_tangent_matrix(MODEL_STATE &, size_type,
					   size_type) { }
    virtual void do_compute_residual(MODEL_STATE &MS, size_type i0,
				     size_type) {
      gmm::add(gmm::scaled(get_F(), value_type(-1)),
	       gmm::sub_vector(MS.residual(), gmm::sub_interval(i0+i1, nbd)));
    }

    mdbrick_neumann_KL_term
    (mdbrick_abstract<MODEL_STATE> &problem, const mesh_fem &mf_data_,
     const VECTOR &M__, const VECTOR &divM__, size_type bound,
     size_type num_fem_=0)
      : M_("M",mf_data_, this),
	divM_("divM",mf_data_, this),
	boundary(bound), num_fem(num_fem_) {
      this->add_sub_brick(problem);
      if (bound != size_type(-1))
	this->add_proper_boundary_info(num_fem, bound,
				       MDBRICK_NORMAL_DERIVATIVE_NEUMANN);
      this->force_update();
      size_type Nb = gmm::vect_size(M__);
      if (Nb) {
	M().set(mf_data_, M__);
	divM().set(mf_data_, divM__);
      }
      else {
	M_.reshape(gmm::sqr(mf_u().linked_mesh().dim()));
	divM_.reshape(mf_u().linked_mesh().dim());
      }
    }
  };


  /* ******************************************************************** */
  /*		Normal derivative Dirichlet assembly routines.            */
  /* ******************************************************************** */

  /**
     Assembly of normal derivative Dirichlet constraints
     @f$ \partial_n u(x) = r(x) @f$ in a weak form
     @f[ \int_{\Gamma} \partial_n u(x)v(x)=\int_{\Gamma} r(x)v(x) \forall v@f],
     where @f$ v @f$ is in
     the space of multipliers corresponding to mf_mult.

     size(r_data) = Q   * nb_dof(mf_rh);

     version = |ASMDIR_BUILDH : build H
     |ASMDIR_BUILDR : build R
     |ASMDIR_BUILDALL : do everything.

     @ingroup asm
  */

  template<typename MAT, typename VECT1, typename VECT2>
  void asm_normal_derivative_dirichlet_constraints
  (MAT &H, VECT1 &R, const mesh_im &mim, const mesh_fem &mf_u,
   const mesh_fem &mf_mult, const mesh_fem &mf_r,
   const VECT2 &r_data, const mesh_region &rg, bool R_must_be_derivated, 
   int version) {
    typedef typename gmm::linalg_traits<VECT1>::value_type value_type;
    typedef typename gmm::number_traits<value_type>::magnitude_type magn_type;
    
    rg.from_mesh(mim.linked_mesh()).error_if_not_faces();
    
    if (version & ASMDIR_BUILDH) {
      const char *s;
      if (mf_u.get_qdim() == 1 && mf_mult.get_qdim() == 1)
	s = "M(#1,#2)+=comp(Base(#1).Grad(#2).Normal())(:,:,i,i)";
      else
	s = "M(#1,#2)+=comp(vBase(#1).vGrad(#2).Normal())(:,i,:,i,j,j);";
      
      generic_assembly assem(s);
      assem.push_mi(mim);
      assem.push_mf(mf_mult);
      assem.push_mf(mf_u);
      assem.push_mat(H);
      assem.assembly(rg);
      gmm::clean(H, gmm::default_tol(magn_type())
		 * gmm::mat_maxnorm(H) * magn_type(1000));
    }
    if (version & ASMDIR_BUILDR) {
      GMM_ASSERT1(mf_r.get_qdim() == 1,
		"invalid data mesh fem (Qdim=1 required)");
      if (!R_must_be_derivated) {
	asm_normal_source_term(R, mim, mf_mult, mf_r, r_data, rg);
      } else {
	asm_real_or_complex_1_param
	  (R, mim, mf_mult, mf_r, r_data, rg,
	   "R=data(#2); V(#1)+=comp(Base(#1).Grad(#2).Normal())(:,i,j,j).R(i)");
      }
    }
  }

  /* ******************************************************************** */
  /*		Normal derivative Dirichlet condition new bricks.         */
  /* ******************************************************************** */

  /** Adds a Dirichlet condition on the normal derivative of the variable
      `varname` and on the mesh region `region` (which should be a boundary. 
      The general form is
      :math:`\int \partial_n u(x)v(x) = \int r(x)v(x) \forall v`
      where :math:`r(x)` is
      the right hand side for the Dirichlet condition (0 for
      homogeneous conditions) and :math:`v` is in a space of multipliers
      defined by the variable `multname` on the part of boundary determined
      by `region`. `dataname` is an optional parameter which represents
      the right hand side of the Dirichlet condition.
      If `R_must_be_derivated` is set to `true` then the normal
      derivative of `dataname` is considered.
  */
  size_type add_normal_derivative_Dirichlet_condition_with_multipliers
  (model &md, const mesh_im &mim, const std::string &varname,
   const std::string &multname, size_type region,
   const std::string &dataname = std::string(),
   bool R_must_be_derivated = false);
  

  /** Adds a Dirichlet condition on the normal derivative of the variable
      `varname` and on the mesh region `region` (which should be a boundary. 
      The general form is
      :math:`\int \partial_n u(x)v(x) = \int r(x)v(x) \forall v`
      where :math:`r(x)` is
      the right hand side for the Dirichlet condition (0 for
      homogeneous conditions) and :math:`v` is in a space of multipliers
      defined by the trace of mf_mult on the part of boundary determined
      by `region`. `dataname` is an optional parameter which represents
      the right hand side of the Dirichlet condition.
      If `R_must_be_derivated` is set to `true` then the normal
      derivative of `dataname` is considered.
  */
  size_type add_normal_derivative_Dirichlet_condition_with_multipliers
  (model &md, const mesh_im &mim, const std::string &varname,
   const mesh_fem &mf_mult, size_type region,
   const std::string &dataname = std::string(),
   bool R_must_be_derivated = false);

  /** Adds a Dirichlet condition on the normal derivative of the variable
      `varname` and on the mesh region `region` (which should be a boundary. 
      The general form is
      :math:`\int \partial_n u(x)v(x) = \int r(x)v(x) \forall v`
      where :math:`r(x)` is
      the right hand side for the Dirichlet condition (0 for
      homogeneous conditions) and :math:`v` is in a space of multipliers
      defined by the trace of a Lagranfe finite element method of degree
      `degree` and on the boundary determined
      by `region`. `dataname` is an optional parameter which represents
      the right hand side of the Dirichlet condition.
      If `R_must_be_derivated` is set to `true` then the normal
      derivative of `dataname` is considered.
  */
  size_type add_normal_derivative_Dirichlet_condition_with_multipliers
  (model &md, const mesh_im &mim, const std::string &varname,
   dim_type degree, size_type region,
   const std::string &dataname = std::string(),
   bool R_must_be_derivated = false);

    /** Adds a Dirichlet condition on the normal derivative of the variable
      `varname` and on the mesh region `region` (which should be a boundary. 
      The general form is
      :math:`\int \partial_n u(x)v(x) = \int r(x)v(x) \forall v`
      where :math:`r(x)` is
      the right hand side for the Dirichlet condition (0 for
      homogeneous conditions). For this brick the condition is enforced with
      a penalisation with a penanalization parameter `penalization_coeff` on
      the boundary determined by `region`.
      `dataname` is an optional parameter which represents
      the right hand side of the Dirichlet condition.
      If `R_must_be_derivated` is set to `true` then the normal
      derivative of `dataname` is considered.
      Note that is is possible to change the penalization coefficient
      using the function `getfem::change_penalization_coeff` of the standard
      Dirichlet condition.
  */
  size_type add_normal_derivative_Dirichlet_condition_with_penalization
  (model &md, const mesh_im &mim, const std::string &varname,
   scalar_type penalisation_coeff, size_type region, 
   const std::string &dataname = std::string(),
   bool R_must_be_derivated = false);
  

  /* ******************************************************************** */
  /*		Normal derivative Dirichlet condition old brick.          */
  /* ******************************************************************** */



  /** Normal derivative Dirichlet condition old brick.
   *
   *  This brick represent a Dirichlet condition on the normal derivative
   *  of the unknow for fourth order pdes.
   *  The general form is
   *  :math:`\int \partial_n u(x)v(x) = \int r(x)v(x) \forall v`
   *  where :math:`r(x)` is
   *  the right hand side for the Dirichlet condition (0 for
   *  homogeneous conditions) and :math:`v` is in a space of multipliers
   *  defined by the trace of mf_mult on the considered part of boundary.
   *
   *  @see asm_normal_derivative_dirichlet_constraints
   *  @see mdbrick_constraint
   *  @ingroup bricks
   */
  template<typename MODEL_STATE = standard_model_state>
  class mdbrick_normal_derivative_Dirichlet
    : public mdbrick_constraint<MODEL_STATE>  {
    
    TYPEDEF_MODEL_STATE_TYPES;

    mdbrick_parameter<VECTOR> R_;
    
    size_type boundary;
    bool mfdata_set, B_to_be_computed;
    bool R_must_be_derivated_; /* if true, then R(x) is a scalar field, and we will impose 
				  grad(u).n = grad(R).n on the boundary */
    gmm::sub_index SUB_CT;
    const mesh_fem *mf_mult;
    
    const mesh_fem &mf_u() { return *(this->mesh_fems[this->num_fem]); }
    const mesh_im  &mim() { return *(this->mesh_ims[0]); }

    void compute_constraints(unsigned version) {
      size_type ndu = mf_u().nb_dof(), ndm = mf_mult->nb_dof();
      gmm::row_matrix<gmm::rsvector<value_type> > M(ndm, ndu);
      VECTOR V(ndm);
      GMM_TRACE2("Assembling normal derivative Dirichlet constraints, version "
		 << version);
      asm_normal_derivative_dirichlet_constraints
	(M, V, mim(), mf_u(), *mf_mult, rhs().mf(), R_.get(),
	 mf_u().linked_mesh().get_mpi_sub_region(boundary), 
	 R_must_be_derivated_, version);
      if (version & ASMDIR_BUILDH)
	gmm::copy(gmm::sub_matrix(M, SUB_CT, gmm::sub_interval(0, ndu)), 
		  this->B);
      gmm::copy(gmm::sub_vector(V, SUB_CT), this->CRHS);
    }

    virtual void recompute_B_sizes(void) {
      if (!mfdata_set) {
	rhs().set(classical_mesh_fem(mf_u().linked_mesh(), 0), 0);
 	mfdata_set = true;
      }
      size_type nd = mf_u().nb_dof();
      dal::bit_vector dof_on_bound;
      if (mf_mult->is_reduced())
	dof_on_bound.add(0, mf_mult->nb_dof());
      else
	dof_on_bound = mf_mult->basic_dof_on_region(boundary);
      size_type nb_const = dof_on_bound.card();
      std::vector<size_type> ind_ct;
      for (dal::bv_visitor i(dof_on_bound); !i.finished(); ++i)
	ind_ct.push_back(i);
      SUB_CT = gmm::sub_index(ind_ct);
      gmm::resize(this->B, nb_const, nd);
      gmm::resize(this->CRHS, nb_const);
      B_to_be_computed = true;
    }

    virtual void recompute_B(void) {
      unsigned version = 0;
      if (R_.is_modified()) { version = ASMDIR_BUILDR; }
      if (B_to_be_computed) { version = ASMDIR_BUILDR | ASMDIR_BUILDH; }
      if (version) { 
	compute_constraints(version);
	this->parameters_set_uptodate();
	B_to_be_computed = false;
      }
    }

  public :

    /** Change the @f$ r(x) @f$ right hand side.
     *	@param R a vector of size @c Q*mf_data.nb_dof() .
     */
    mdbrick_parameter<VECTOR> &rhs() { 
      unsigned n = (R_must_be_derivated_ == false ? mf_u().linked_mesh().dim() : 1);
      R_.reshape(n*mf_u().get_qdim());
      return R_; 
    }

    void R_must_be_derivated() {
      R_must_be_derivated_ = true;
    }
    
    /** Constructor which does not define the rhs (i.e. which sets an
     *	homogeneous Dirichlet condition)
     *	@param problem the sub problem to which this brick is applied.
     *	@param bound the boundary number for the dirichlet condition.
     *  @param mf_mult_ the mesh_fem for the multipliers.
     *	@param num_fem_ the mesh_fem number on which this brick is is applied.
     */
    mdbrick_normal_derivative_Dirichlet
    (mdbrick_abstract<MODEL_STATE> &problem, size_type bound,
     const mesh_fem &mf_mult_ = dummy_mesh_fem(), size_type num_fem_=0)
      : mdbrick_constraint<MODEL_STATE>(problem, num_fem_), R_("R", this),
	boundary(bound) {
      mf_mult = (&mf_mult_ == &dummy_mesh_fem()) ? &(mf_u()) : &mf_mult_;
      this->add_proper_boundary_info(this->num_fem, boundary, 
				     MDBRICK_NORMAL_DERIVATIVE_DIRICHLET);
      this->add_dependency(*mf_mult);
      mfdata_set = false; B_to_be_computed = true;
      R_must_be_derivated_ = false;
      this->force_update();
    }
  };




}  /* end of namespace getfem.                                             */


#endif /* GETFEM_FOURTH_ORDER_H__ */
