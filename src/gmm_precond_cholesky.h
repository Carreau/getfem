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
/* File    :  gmm_precond_cholesky.h : form I.T.L.                         */
/*     									   */
/* Date : June 5, 2003.                                                    */
/* Author : Yves Renard, Yves.Renard@gmm.insa-tlse.fr                      */
/*                                                                         */
/* *********************************************************************** */
/*                                                                         */
/* Copyright (C) 2003  Yves Renard.                                        */
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
#ifndef GMM_PRECOND_CHOLESKY_H
#define GMM_PRECOND_CHOLESKY_H

// importation not finished ...

// Incomplete Level 0 Cholesky Preconditioner.
// For use with symmetric sparse matrices.
//
// Notes: The idea under a concrete Preconditioner such 
//        as Incomplete Cholesky is to create a Preconditioner
//        object to use in iterative methods. 
//

#include <gmm_solvers.h>

namespace gmm {

  template < class Matrix >
  class cholesky_precond {

  protected :
    typedef typename linalg_traits<Matrix>::value_type value_type;
    typedef typename principal_orientation_type<typename
      linalg_traits<Matrix>::sub_orientation>::potype sub_orientation;

    csr_matrix_ref<T *, size_type *, size_type *, 0> row_trimatrix;
    csc_matrix_ref<T *, size_type *, size_type *, 0> col_trimatrix;
    std::vector<value_type> Tri_val;
    std::vector<size_type> Tri_ind, Tri_ptr;
 
    void do_cholesky(const Matrix& A, row_major) {
      size_type Tri_loc, nnz, n = mat_nrows(A), d, g, h, i, j, k;
      value_type z;
      Tri_ptr[0] = 0;

      for (int count = 0; count < 2; ++count) {
	if (count) { Tri_val.resize(Tri_loc); Tri_ind.resize(Tri_loc); }
	Tri_loc = 0;
	for (size_type i = 0; i < n; ++i) {
	  typedef typename linalg_traits<Matrix>::const_sub_row_type row_type;
	  row_type row = mat_const_row(A, i);
	  typedef typename linalg_traits<row_type>::const_iterator
	    it = vect_const_begin(row), ite = vect_const_end(row);
	  
	  for (nnz = 0; it != ite; ++it)
	    if (it.index >= i) {
	      if (count) {
		Tri_val[Tri_loc+nnz] = *it;
		Tri_ind[Tri_loc+nnz] = it.index();
	      }
	      ++nnz;
	    }
	  Tri_loc += nnz;
	  if (count) Tri_ptr[i+1] = Tri_loc;
	}
      }
      
      for (k = 0; k < n - 1; k++) {
	d = Tri_ptr[k];
	z = Tri_val[d] = sqrt(Tri_val[d]);
	
	for (i = d + 1; i < Tri_ptr[k+1]; i++)
	  Tri_val[i] /= z;
	
	for (i = d + 1; i < Tri_ptr[k+1]; i++) {
	  z = Tri_val[i];
	  h = Tri_ind[i];
	  g = i;
	  
	  for (j = Tri_ptr[h] ; j < Tri_ptr[h+1]; j++)
	    for ( ; g < Tri_ptr[k+1] && Tri_ind[g+1] <= Tri_ind[j]; g++)
	      if (Tri_ind[g] == Tri_ind[j])
		Tri_val[j] -= z * Tri_val[g];
	}
      }
      d = Tri_ptr[n-1];
      Tri_val[d] = sqrt(Tri_val[d]);

      row_trimatrix = csr_matrix_ref<T *, size_type *, size_type *, 0>
	(&Tri_val[0], &Tri_ptr[0], &Tri_ind[0], mat_nrows(A), mat_ncols(A));
    }

    void do_cholesky(const Matrix& A, column_major) {
      size_type Tri_loc, d, g, h, i, j, k, n = A.nrows();
      value_type z;
      Tri_ptr[0] = 0;

      for (int count = 0; count < 2; ++count) {
	if (count) { Tri_val.resize(Tri_loc); Tri_ind.resize(Tri_loc); }
	Tri_loc = 0;
	for (size_type i = 0; i < n; ++i) {
	  typedef typename linalg_traits<Matrix>::const_sub_col_type col_type;
	  col_type col = mat_const_col(A, i);
	  typedef typename linalg_traits<col_type>::const_iterator
	    it = vect_const_begin(col), ite = vect_const_end(col);
	  
	  for (nnz = 0; it != ite; ++it)
	    if (it.index >= i) {
	      if (count) {
		Tri_val[Tri_loc+nnz] = *it;
		Tri_ind[Tri_loc+nnz] = it.index();
	      }
	      ++nnz;
	    }
	  Tri_loc += nnz;
	  if (count) Tri_ptr[i+1] = Tri_loc;
	}
      }
      
      for (k = 0; k < n - 1; k++) {
	d = Tri_ptr[k];
	z = Tri_val[d] = sqrt(Tri_val[d]);
	
	for (i = d + 1; i < Tri_ptr[k+1]; i++)
	  Tri_val[i] /= z;
	
	for (i = d + 1; i < Tri_ptr[k+1]; i++) {
	  z = Tri_val[i];
	  h = Tri_ind[i];
	  g = i;
	  
	  for (j = Tri_ptr[h] ; j < Tri_ptr[h+1]; j++)
	    for ( ; g < Tri_ptr[k+1] && Tri_ind[g+1] <= Tri_ind[j]; g++)
	      if (Tri_ind[g] == Tri_ind[j])
		Tri_val[j] -= z * Tri_val[g];
	}
      }
      d = Tri_ptr[n-1];
      Tri_val[d] = sqrt(Tri_val[d]);

      col_trimatrix = csc_matrix_ref<T *, size_type *, size_type *, 0>
	(&Tri_val[0], &Tri_ptr[0], &Tri_ind[0], mat_nrows(A), mat_ncols(A));
    }

  public:

    
    size_type nrows(row_major) const { return mat_nrows(row_trimatrix); }
    size_type nrows(col_major) const { return mat_nrows(col_trimatrix); }
    size_type nrows(void) const { return nrows(sub_orientation()); }
    size_type ncols(row_major) const { return mat_ncols(row_trimatrix); }
    size_type ncols(col_major) const { return mat_ncols(col_trimatrix); }
    size_type ncols(void) const { return ncols(sub_orientation()); }
    
    cholesky_precond(const Matrix& A) : Tri_ptr(A.nrows()+1)
    { do_cholesky(A, sub_orientation()); }
  };


  template <class Matrix, class V1, class V2> inline
  void mult(const cholesky_precond<Matrix>& P, const V1 &v1, V2 &v2) {
    if (P.nrows() != vect_size(v2))
      DAL_THROW(dimension_error, "dimensions mismatch");
   
    ... � continuer .. avec les tri_solve

  }


}

#endif 

