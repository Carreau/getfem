/*===========================================================================

 Copyright (C) 2000-2017 Yves Renard

 This file is a part of GetFEM++

 GetFEM++  is  free software;  you  can  redistribute  it  and/or modify it
 under  the  terms  of the  GNU  Lesser General Public License as published
 by  the  Free Software Foundation;  either version 3 of the License,  or
 (at your option) any later version along with the GCC Runtime Library
 Exception either version 3.1 or (at your option) any later version.
 This program  is  distributed  in  the  hope  that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License and GCC Runtime Library Exception for more details.
 You  should  have received a copy of the GNU Lesser General Public License
 along  with  this program;  if not, write to the Free Software Foundation,
 Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

===========================================================================*/

#include "getfem/bgeot_geotrans_inv.h"
#include "gmm/gmm_solver_bfgs.h"
namespace bgeot
{ 
  bool geotrans_inv_convex::invert(const base_node& n, base_node& n_ref,
				   scalar_type IN_EPS) {
    assert(pgt);
    n_ref.resize(pgt->structure()->dim());
    bool converged = true;
    if (pgt->is_linear()) {
      return invert_lin(n, n_ref,IN_EPS);
    } else {
      return invert_nonlin(n, n_ref,IN_EPS,converged,true);
    }
  }

  bool geotrans_inv_convex::invert(const base_node& n, base_node& n_ref, 
				   bool &converged, 
				   scalar_type IN_EPS) {
    assert(pgt);
    n_ref.resize(pgt->structure()->dim());
    converged = true;
    if (pgt->is_linear()) {
      return invert_lin(n, n_ref,IN_EPS);
    } else return invert_nonlin(n, n_ref,IN_EPS,converged, false);
  }


  /* inversion for linear geometric transformations */
  bool geotrans_inv_convex::invert_lin(const base_node& n, base_node& n_ref, scalar_type IN_EPS) {
    base_node y(n); for (size_type i=0; i < N; ++i) y[i] -= G(i,0);
    gmm::mult(gmm::transposed(B), y, n_ref);
    if (pgt->convex_ref()->is_in(n_ref) < IN_EPS) {
      if (P == N) return true;
      else {
	gmm::mult(K,gmm::scaled(n_ref,-1.0),y,y);
	//        y -= K * n_ref;
        if (gmm::vect_norm2(y) < IN_EPS) return true;
      }
    }
    return false;
  }
  
  void geotrans_inv_convex::update_B() {
    if (P != N) {
      pgt->compute_K_matrix(G, pc, K);
      gmm::mult(gmm::transposed(K), K, CS);
      bgeot::lu_inverse(&(*(CS.begin())), P);
      gmm::mult(K, CS, B);
    }
    else {
      // L'inversion peut �tre optimis�e par le non calcul global de B
      // et la resolution d'un syst�me lin�aire.
      base_matrix KT(K.nrows(), K.ncols());
      pgt->compute_K_matrix(G, pc, KT);
      gmm::copy(gmm::transposed(KT), K);
      gmm::copy(K, B);
      bgeot::lu_inverse(&(*(K.begin())), P); B.swap(K); 
    }
  }

  class geotrans_inv_convex_bfgs {
    geotrans_inv_convex &gic;
    base_node xreal;
  public:
    geotrans_inv_convex_bfgs(geotrans_inv_convex &gic_, 
			     const base_node &xr) : gic(gic_), xreal(xr) {}
    scalar_type operator()(const base_node& x) const {
      base_node r = gic.pgt->transform(x, gic.G) - xreal;
      return gmm::vect_norm2_sqr(r)/2.;
    }
    void operator()(const base_node& x, base_small_vector& gr) const {
      gic.pgt->poly_vector_grad(x, gic.pc);
      gic.update_B();
      base_node r = gic.pgt->transform(x, gic.G) - xreal;
      gr.resize(x.size());
      gmm::mult(gmm::transposed(gic.K), r, gr); 
    }
  };

  /* inversion for non-linear geometric transformations 
     (Newton on Grad(pgt)(y - pgt(x)) = 0 )
  */
  bool geotrans_inv_convex::invert_nonlin(const base_node& xreal,
	       			  base_node& x, scalar_type IN_EPS,
				  bool &converged, bool throw_except) {
    using namespace gmm;

    converged = true;
    base_node xn(P), y, z,x0;
    /* find an initial guess */
    x0 = (pgt->geometric_nodes())[0]; copy(mat_col(G, 0), y);  
    scalar_type d = gmm::vect_dist2_sqr(y, xreal);
    for (size_type j = 1; j < pgt->nb_points(); ++j) { 
      scalar_type d2 = gmm::vect_dist2_sqr(mat_col(G, j), xreal);
      if (d2 < d)
        { d = d2; x0 = pgt->geometric_nodes()[j]; copy(mat_col(G, j), y); }
    }
    x = x0;

    base_node vres(P);
    base_node rn(xreal); rn -= y; 
    pgt->poly_vector_grad(x, pc);
    update_B();
    
    gmm::mult(gmm::transposed(K), rn, vres);
    scalar_type res = gmm::vect_norm2(vres);

    //cerr << "DEBUT: res0=" << res << ", X=" << xreal << "\nB=" << B << ", K=" << K << "\n" << ", pc=" << pc << "\n";
    unsigned cnt = 50;
    while (res > EPS/10 && --cnt) {
      gmm::mult(gmm::transposed(B), rn, xn);
      scalar_type newres;
      for (unsigned i=1; i<=256; i*=2) {
	z = x + xn / scalar_type(i);
	y = pgt->transform(z, G);
	
	rn = xreal - y; 
	
	pgt->poly_vector_grad(z, pc);
	update_B();

	// cout << "K =  " << K << endl;
	
	if (P != N) {
	  gmm::mult(gmm::transposed(K), rn, vres);
	  newres = gmm::vect_norm2(vres); 
	} else {
	  newres = gmm::vect_norm2(rn); // "better" residu
	}
	if (newres < 1.5*res) break;
      }
      x = z; res = newres;
      // cerr << "cnt=" << cnt << ", x=" << x << ", res=" << res << "\n";
    }
    //cerr << " invert_nonlin done\n";
    //cerr << "cnt=" << cnt << ", P=" << P << ", N=" << N << ", G=" << G << "\nX=" << xreal << " Xref=" << x << "\nresidu=" << res << "\nB=" << B << ", K=" << K << "\n" << ", pc=" << pc << "\n-------------------^^^^^^^^\n";
    if (cnt == 0) {
      //cout << "BFGS in geotrans_inv_convex!\n";
      geotrans_inv_convex_bfgs b(*this, xreal);
      gmm::iteration iter(EPS,0);
      x = x0;
      gmm::bfgs(b,b,x,10,iter);
      rn = pgt->transform(x, G) - xreal; 
      
      if (pgt->convex_ref()->is_in(x) < IN_EPS &&
	  (N==P && gmm::vect_norm2(rn) > IN_EPS)) {
	GMM_ASSERT1(!throw_except,
		    "inversion of non-linear geometric transformation "
		    "failed ! (too much iterations -- xreal=" << xreal
		    << ", rn=" << rn 
		    << ", xref=" << x 
		    << ", is_in(x)=" << pgt->convex_ref()->is_in(x) 
		    << ", eps=" << IN_EPS << ")");
	converged = false;
	return false;
      }
    }
    // Test un peu sev�re peut-�tre en ce qui concerne rn.
    if (pgt->convex_ref()->is_in(x) < IN_EPS
        && (P == N || gmm::vect_norm2(rn) < IN_EPS)) {
      // cout << "point " << x << "is IN (" << pgt->convex_ref()->is_in(x)
      //      << ")\n";
      return true;
    } // else cout << "point IS OUT\n";
    return false;
  }

}  /* end of namespace bgeot.                                             */
