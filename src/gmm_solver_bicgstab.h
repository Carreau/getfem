/* -*- c++ -*- (enables emacs c++ mode)                                    */
//=======================================================================
// Copyright (C) 1997-2001
// Authors: Andrew Lumsdaine <lums@osl.iu.edu> 
//          Lie-Quan Lee     <llee@osl.iu.edu>
//
// This file is part of the Iterative Template Library
//
// You should have received a copy of the License Agreement for the
// Iterative Template Library along with the software;  see the
// file LICENSE.  
//
// Permission to modify the code and to distribute modified code is
// granted, provided the text of this NOTICE is retained, a notice that
// the code was modified is included with the above COPYRIGHT NOTICE and
// with the COPYRIGHT NOTICE in the LICENSE file, and that the LICENSE
// file is distributed with the modified code.
//
// LICENSOR MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.
// By way of example, but not limitation, Licensor MAKES NO
// REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY
// PARTICULAR PURPOSE OR THAT THE USE OF THE LICENSED SOFTWARE COMPONENTS
// OR DOCUMENTATION WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS, TRADEMARKS
// OR OTHER RIGHTS.
//=======================================================================
/* *********************************************************************** */
/*                                                                         */
/* Library :  Generic Matrix Methods  (gmm)                                */
/* File    :  gmm_solver_bicgstab.h                                        */
/*            modified version from I.T.L.                                 */
/*            (http://www.osl.iu.edu/research/itl)                         */
/*     									   */
/* Date : October 13, 2002.                                                */
/* Author : Yves Renard, Yves.Renard@gmm.insa-tlse.fr                      */
/*                                                                         */
/* *********************************************************************** */
/*                                                                         */
/* Copyright (C) 2002  Yves Renard.                                        */
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


#ifndef __GMM_SOLVER_BICGSTAB_H
#define __GMM_SOLVER_BICGSTAB_H

namespace gmm {

  /* ******************************************************************** */
  /*		BiConjugate Gradient Stabilized               		  */
  /* (preconditionned, with parametrable scalar product)        	  */
  /* ******************************************************************** */



  template <class Matrix, class Vector, class VectorB, class Preconditioner>
  int bicgstab(const Matrix& A, Vector& x, const VectorB& b,
	   const Preconditioner& M, int itemax, double residu, int noisy = 1) {

    typedef typename linalg_traits<Vector>::value_type T;
    typedef typename temporary_plain_vector<Vector>::vector_type temp_vector;
    
    T rho_1, rho_2, alpha, beta, omega;
    temp_vector p(vect_size(x)), phat(vect_size(x)), s(vect_size(x)),
      shat(vect_size(x)), 
      t(vect_size(x)), v(vect_size(x)), r(vect_size(x)), rtilde(vect_size(x));
    
    gmm::mult(A, gmm::scaled(x, -1.0), b, r);	  
    gmm::copy(r, rtilde);
    int iter = 0;
    T norm_r = gmm::vect_norm2(r);
    T norm_b = gmm::vect_norm2(b);

    if (norm_b == T(0))
      clear(x);
    else {
      while (norm_r > residu * norm_b) {
	
	rho_1 = gmm::vect_sp(rtilde, r);
	if (rho_1 == T(0) || iter > itemax) return 1;
	
	if (iter == 0)
	  gmm::copy(r, p);
	else {
	  if (omega == T(0.)) return 1;
	  
	  beta = (rho_1 / rho_2) * (alpha / omega);
	  
	  gmm::add(gmm::scaled(v, -omega), p); // c'est bon �a ? 
	  gmm::add(r, gmm::scaled(p, beta), p);      
	}
	gmm::mult(M, p, phat);
	gmm::mult(A, phat, v);	
	alpha = rho_1 / gmm::vect_sp(v, rtilde);
	gmm::add(r, gmm::scaled(v, -alpha), s);
	
	T norm_s = gmm::vect_norm2(s);
	if (norm_s <= residu * norm_b) {
	  gmm::add(gmm::scaled(phat, alpha), x); 
	  break;
	}
	
	gmm::mult(M, s, shat);	
	gmm::mult(A, shat, t);	
	omega = gmm::vect_sp(t, s) / gmm::vect_sp(t, t);
	
	gmm::add(gmm::scaled(phat, alpha), x); 
	gmm::add(gmm::scaled(shat, omega), x);
	gmm::add(s, gmm::scaled(t, -omega), r); 
	norm_r = gmm::vect_norm2(r);
	rho_2 = rho_1;
	
	++iter;
	if (noisy > 0)  cout << "iter " << iter << " residu "
			     << norm_r / norm_b << endl;
      }
    }
    return 0;
  }

  template <class Matrix, class Vector, class VectorB, class Preconditioner>
  int bicgstab(const Matrix& A, const Vector& x, const VectorB& b,
	   const Preconditioner& M, int itemax, double residu, int noisy = 1) {
    return bicgstab(A, linalg_const_cast(x), b, M, itemax, residu, noisy);
  }
  
}


#endif //  __GMM_SOLVER_BICGSTAB_H
