// -*- c++ -*- (enables emacs c++ mode)
/* *********************************************************************** */
/*                                                                         */
/* Copyright (C) 2002-2005 Yves Renard, Julien Pommier.                    */
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

/**
 * Goal : scalar Signorini problem with Xfem.
 *
 * Research program.
 */

#include <getfem_assembling.h> /* import assembly methods (and norms comp.) */
#include <getfem_export.h>   /* export functions (save solution in a file)  */
#include <getfem_derivatives.h>
#include <getfem_regular_meshes.h>
#include <getfem_model_solvers.h>
#include <getfem_mesh_im_level_set.h>
#include <getfem_import.h>
#include <gmm.h>

/* some Getfem++ types that we will be using */
using bgeot::base_small_vector; /* special class for small (dim<16) vectors */
using bgeot::base_vector;
using bgeot::base_node;  /* geometrical nodes(derived from base_small_vector)*/
using bgeot::scalar_type; /* = double */
using bgeot::short_type;  /* = short */
using bgeot::size_type;   /* = unsigned long */
using bgeot::base_matrix; /* small dense matrix. */

/* definition of some matrix/vector types. These ones are built
 * using the predefined types in Gmm++
 */
typedef getfem::modeling_standard_sparse_vector sparse_vector;
typedef getfem::modeling_standard_sparse_matrix sparse_matrix;
typedef getfem::modeling_standard_plain_vector  plain_vector;


/* 
 * Exact solution 
 */
double Radius;

double u_exact(const base_node &p) {
  double sum = std::accumulate(p.begin(), p.end(), double(0));
  double norm_sqr = gmm::vect_norm2_sqr(p);
  return sin(sum) * (norm_sqr - Radius*Radius);
}

double rhs(const base_node &p) {
  double sum = std::accumulate(p.begin(), p.end(), double(0));
  double norm_sqr = gmm::vect_norm2_sqr(p);
  double N = double(gmm::vect_size(p));
  return N * sin(sum) * (norm_sqr - Radius*Radius-2.0) - 4.0 * sum * cos(sum);
}

/*
 * Test procedures
 */

void test_mim(getfem::mesh_im_level_set &mim, getfem::mesh_fem &mf_rhs) {
  unsigned N =  mim.linked_mesh().dim();
  size_type nbdof = mf_rhs.nb_dof();
  plain_vector V(nbdof), W(1);
  std::fill(V.begin(), V.end(), 1.0);

  getfem::generic_assembly assem("u = data(#1); V()+=comp(Base(#1))(i).u(i)");
  assem.push_mi(mim); assem.push_mf(mf_rhs); assem.push_data(V);
  assem.push_vec(W);
  assem.assembly(getfem::mesh_region::all_convexes());
  double exact(0);
  switch (N) {
    case 1: exact = 2*Radius; break;
    case 2: exact = Radius*Radius*M_PI; break;
    case 3: exact = 4.0*M_PI*Radius*Radius*Radius/3.0; break;
    default: assert(N <= 3);
  }
  cout << "Area: " << W[0] << " should be " << exact << endl;


//   base_matrix G;
//   for (dal::bv_visitor i(mim.linked_mesh().convex_index());
//        !i.finished(); ++i) {
//     double area = 0;
//     getfem::papprox_integration pai
//       = mim.int_method_of_element(i)->approx_method();
//     if (!pai) continue;
//     bgeot::vectors_to_base_matrix(G, mim.linked_mesh().points_of_convex(i));
//     bgeot::geotrans_interpolation_context 
//       c(mim.linked_mesh().trans_of_convex(i), pai->point(0), G);
//     for (size_type j = 0; j < pai->nb_points_on_convex(); ++j) {
//       c.set_xref(pai->point(j));
//       area += pai->coeff(j) * c.J(); 
//     }
//     cout << "area of elt " << i << ": " << area << endl;
//   }


}

void test_mimbound(getfem::mesh_im_level_set &mim, getfem::mesh_fem &mf_rhs) {
  unsigned N =  mim.linked_mesh().dim();
  size_type nbdof = mf_rhs.nb_dof();
  plain_vector V(nbdof), W(1);
  std::fill(V.begin(), V.end(), 1.0);

  getfem::generic_assembly assem("u = data(#1); V()+=comp(Base(#1))(i).u(i)");
  assem.push_mi(mim); assem.push_mf(mf_rhs); assem.push_data(V);
  assem.push_vec(W);
  assem.assembly(getfem::mesh_region::all_convexes());
  double exact(0);
  switch (N) {
    case 1: exact = 2.0; break;
    case 2: exact = 2.0*Radius*M_PI; break;
    case 3: exact = 4.0*M_PI*Radius*Radius; break;
    default: assert(N <= 3);
  }
  cout << "Boundary length: " << W[0] << " should be " << exact << endl;
  assert(gmm::abs(exact-W[0])/exact < 0.1); 
}





/* 
 * Main program 
 */

int main(int argc, char *argv[]) {

  DAL_SET_EXCEPTION_DEBUG; // Exceptions make a memory fault, to debug.
  FE_ENABLE_EXCEPT;        // Enable floating point exception for Nan.

  // getfem::getfem_mesh_level_set_noisy();

  try {
    
    // Read parameters.
    ftool::md_param PARAM;
    PARAM.read_command_line(argc, argv);

    // Load the mesh
    getfem::mesh mesh;
    std::string MESH_FILE = PARAM.string_value("MESH_FILE", "Mesh file");
    getfem::import_mesh(MESH_FILE, mesh);
    unsigned N = mesh.dim();

    // center the mesh in (0, 0).
    base_node Pmin(N), Pmax(N);
    mesh.bounding_box(Pmin, Pmax);
    Pmin += Pmax; Pmin /= -2.0;
    mesh.translation(Pmin);

    // Level set definition
    unsigned lsdeg = PARAM.int_value("LEVEL_SET_DEGREE", "level set degree");
    Radius = PARAM.real_value("RADIUS", "Domain radius");
    getfem::level_set ls(mesh, lsdeg);
    const getfem::mesh_fem &lsmf = ls.get_mesh_fem();
    for (unsigned i = 0; i < lsmf.nb_dof(); ++i)
      ls.values()[i] = gmm::vect_norm2_sqr(lsmf.point_of_dof(i))-Radius*Radius;
    getfem::mesh_level_set mls(mesh);
    mls.add_level_set(ls);
    mls.adapt();
    
    getfem::mesh mcut;
    mls.global_cut_mesh(mcut);
    mcut.write_to_file("cut.mesh");


    // Integration method on the domain
    std::string IM = PARAM.string_value("IM", "Mesh file");
    std::string IMS = PARAM.string_value("IM_SIMPLEX", "Mesh file");
    int intins = getfem::mesh_im_level_set::INTEGRATE_INSIDE;
    getfem::mesh_im_level_set mim(mls, intins,
				  getfem::int_method_descriptor(IMS));
    mim.set_integration_method(mesh.convex_index(),
			       getfem::int_method_descriptor(IM));
    mim.adapt();


    // Integration method on the boudary
    int intbound = getfem::mesh_im_level_set::INTEGRATE_BOUNDARY;
    getfem::mesh_im_level_set mimbound(mls, intbound,
				       getfem::int_method_descriptor(IMS));
    mimbound.set_integration_method(mesh.convex_index(),
				    getfem::int_method_descriptor(IM));
    mimbound.adapt();
    

    // Finite element method for the unknown
    getfem::mesh_fem mf(mesh);
    std::string FEM = PARAM.string_value("FEM", "finite element method");
    mf.set_finite_element(mesh.convex_index(), getfem::fem_descriptor(FEM));
    size_type nb_dof = mf.nb_dof();
    
    // Finite element method for the rhs
    getfem::mesh_fem mf_rhs(mesh);
    std::string FEMR = PARAM.string_value("FEM_RHS", "finite element method");
    mf_rhs.set_finite_element(mesh.convex_index(),
			      getfem::fem_descriptor(FEMR));
    size_type nb_dof_rhs = mf_rhs.nb_dof();
    cout << "nb_dof_rhs = " << nb_dof_rhs << endl;
    
    // Finite element method for the multipliers
    getfem::mesh_fem mf_mult(mesh);
    std::string FEMM = PARAM.string_value("FEM_MULT", "fem for multipliers");
    mf_mult.set_finite_element(mesh.convex_index(),
			       getfem::fem_descriptor(FEMM));
    size_type nb_dof_mult = mf_mult.nb_dof();
    cout << "nb_dof_mult = " << nb_dof_mult << endl;
    

    // Stiffness matrix for the Poisson problem
    sparse_matrix K(nb_dof, nb_dof);
    getfem::asm_stiffness_matrix_for_homogeneous_laplacian(K, mim, mf);


    // Mass matrix on the boundary
    sparse_matrix B(nb_dof, nb_dof_mult);
    getfem::asm_mass_matrix(B, mimbound, mf, mf_mult);

    // rhs
    plain_vector F(nb_dof), FD(nb_dof_rhs);
    for (size_type i = 0; i < nb_dof_rhs; ++i)
      FD[i] = rhs(mf_rhs.point_of_dof(i));
    getfem::asm_source_term(F, mim, mf, mf_rhs, FD);


    // Tests
    test_mim(mim, mf_rhs);
    test_mimbound(mimbound, mf_rhs);
    


    gmm::clean(K, 1E-9);
    // cout << "K = " << K << endl;


    gmm::clean(B, 1E-9);
    // cout << "B = " << B << endl;

    // TODO:
    // - s�lectionner les ddls utiles pour la solution.
    // - retenir les ddls utiles pour les multiplicateurs.
    // - r�soudre le syst�me.
    // - calculer l'erreur.
   
  }
  DAL_STANDARD_CATCH_ERROR;

  return 0; 
}
