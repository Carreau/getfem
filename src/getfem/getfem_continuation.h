/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================
 
 Copyright (C) 2011-2012 Tomas Ligursky, Yves Renard
 
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

/** @file getfem_continuation.h
    @author Tomas Ligursky <tomas.ligursky@gmail.com>
    @author Yves Renard <Yves.Renard@insa-lyon.fr>
    @date October 17, 2011.
    @brief (approximate) Moore-Penrose (also called Gauss-Newton) continuation method.

    NOTE: The bordered systems involved are solved by a block eliminiation
    although the bordered matrix may be ill-conditioned in some cases!
    Nevertheless, the algorithm seems to work well.
*/
#ifndef GETFEM_CONTINUATION_H__
#define GETFEM_CONTINUATION_H__

#include <getfem/getfem_model_solvers.h>

namespace getfem {


  //=========================================================================
  // Abstract Moore-Penrose continuation method
  //=========================================================================


  enum build_data { BUILD_F = 1, BUILD_F_x = 2, BUILD_ALL = 3 };

  template <typename CONT_S, typename VECT> 
  double norm_(CONT_S &S, const VECT &x) {
    double no = sqrt(S.sp(x, x));
    return no;
  }
  
  template <typename CONT_S, typename VECT> 
  double w_sp_(CONT_S &S, const VECT &x1, const VECT &x2) {
    double r = S.scfac() * S.sp(x1, x2);
    return r;
  }

  template <typename CONT_S, typename VECT> 
  double sp_(CONT_S &S, const VECT &x1, const VECT &x2,
	     double gamma1, double gamma2) {
    double r = w_sp_(S, x1, x2) + gamma1 * gamma2;
    return r;
  }

  template <typename CONT_S, typename VECT> 
  double norm_(CONT_S &S, const VECT &x, double gamma) {
    double no = sqrt(sp_(S, x, x, gamma, gamma));
    return no;
  }


  template <typename CONT_S, typename VECT>
  void compute_tangent(CONT_S &S, const VECT &x, double gamma,
		       VECT &t_x, double &t_gamma) {
    VECT g(x), y(x);
    S.F_gamma(x, gamma, g);
    S.solve_grad(x, gamma, g, y);
    t_gamma = 1. / (t_gamma - w_sp_(S, t_x, y));
    S.scale(y, -t_gamma); S.copy(y, t_x);
    
    double no = norm_(S, t_x, t_gamma);
    S.scale(t_x, 1./no); t_gamma /= no;

//     if (S.noisy() > 1) {
//       S.mult_grad(x, gamma, t_x, y); S.scaled_add(y, g, t_gamma, y);
//       cout << "new tangent computed with the residual " 
// 	   << norm_(S, y) << endl;
//     }
  }


  template <typename CONT_S, typename VECT>
  double test_function(CONT_S &S, const VECT &x, double gamma,
		       const VECT &t_x, double t_gamma) {
    double q, r, v_gamma, tau;
    VECT g(x), v_x(x), y(x), z(x);

    if (S.noisy() > 1) cout << "starting computing test function" << endl;
    S.F_gamma(x, gamma, g);
    S.solve_grad(x, gamma, g, S.b_x(), y, z);
    v_gamma = (S.b_gamma() - S.sp(t_x, z)) / (t_gamma - S.sp(t_x, y));
    S.scaled_add(z, y, -v_gamma, v_x);
    tau = 1. / (S.d() - S.sp(S.c_x(), v_x) - S.c_gamma() * v_gamma);
    S.scale(v_x, -tau); v_gamma *= -tau;

    // evaluate the norm of the residual for control
    S.mult_grad(x, gamma, v_x, y);
    S.scaled_add(y, g, v_gamma, y); S.scaled_add(y, S.b_x(), tau, y); 
    r = S.sp(y, y);
    q = S.sp(t_x, v_x) + t_gamma * v_gamma + S.b_gamma() * tau; r += q * q;
    q = S.sp(S.c_x(), v_x) + S.c_gamma() * v_gamma + S.d() * tau - 1.;
    r += q * q; r = sqrt(r);
    if (r > 1e-10)
      GMM_WARNING1("Test function evaluated with the residual " << r);

    return tau;
  }

  template <typename CONT_S, typename VECT>
  bool test_smooth_bifurcation(CONT_S &S, const VECT &x, double gamma,
			       const VECT &t_x, double t_gamma) {
    double tau1 = S.tau2(), tau2 = S.tau3(),
      tau3 = test_function(S, x, gamma, t_x, t_gamma);
    bool res = (tau3 * tau2 < 0) & (S.abs(tau2) < S.abs(tau1));
    S.set_tau1(tau1); S.set_tau2(tau2); S.set_tau3(tau3);
    return res;
  }
  

  template <typename CONT_S, typename VECT>
  int test_direction(CONT_S &S, const VECT &x, double gamma,
		     const VECT &t_x, double t_gamma,
		     VECT &T_x, double &T_gamma, double h) {
    int res = 1;
    double Gamma, T_Gamma = T_gamma, ang;
    VECT X(x), T_X(T_x);
    
    S.scaled_add(x, T_x, h, X); Gamma = gamma + h * T_gamma;
    S.set_build(BUILD_ALL);
    compute_tangent(S, X, Gamma, T_X, T_Gamma);
    
    ang = sp_(S, T_x, T_X, T_gamma, T_Gamma);
    if (S.noisy() > 1)
      cout << "the angle with the tested tangent " << ang << endl;
    if (ang >=  0.996) res = (h > 0) ? 3 : 4;
    else {
      ang = sp_(S, t_x, T_X, t_gamma, T_Gamma);
      if (S.noisy() > 1)
	cout << "the angle with the starting tangent " << ang << endl;
      if (ang < 0.86 && ang > -0.86) {
	res = 2;
	S.copy(T_X, T_x); T_gamma = T_Gamma; // try the new tangent next(?)
      }
    }
    return res;
  }


  template <typename CONT_S, typename VECT>
  void init_Moore_Penrose_continuation(CONT_S &S, const VECT &x,
				       double gamma, VECT &t_x,
				       double &t_gamma, double &h) {
    S.set_build(BUILD_ALL);
    S.clear(t_x); t_gamma = (t_gamma >= 0) ? 1. : -1.;
    if (S.noisy() > 0) cout << "computing initial tangent" << endl;
    compute_tangent(S, x, gamma, t_x, t_gamma);
    h = S.h_init();
    S.init_border(x);
    double tau = test_function(S, x, gamma, t_x, t_gamma); S.set_tau3(tau);
  }

  
  /* Perform one step of the Moore-Penrose continuation. If a new point 
     (x, gamma) is found, it has to be saved in the model in the end! */
  template <typename CONT_S, typename VECT>
    void Moore_Penrose_continuation(CONT_S &S, VECT &x, double &gamma,
				    VECT &t_x, double &t_gamma, double &h) {
    bool bifurcation = false, converged, finished = false;
    int tangent_status = 0;
      /* 0: no manipulation with tangent direction so far;
	 1: current direction neither admitted nor rejected;
	 2: direction rejected;
	 3: direction admitted with plus sign;
	 4: direction admitted with minus sign; */
    unsigned long it, step_dec = 0;
    double Delta_Gamma, Gamma, T_gamma, r, no, res, diff, ang;
    VECT F(x), g(x), Delta_X(x), X(x), T_x(x), y(x);

    do { // step control

      // prediction
      if (S.noisy() > 0) cout << "prediction with h = " << h << endl;
      S.scaled_add(x, t_x, h, X); Gamma = gamma + h * t_gamma;
      S.set_build(BUILD_ALL);
      S.copy(t_x, T_x); T_gamma = t_gamma;
      
      // correction
      if (S.noisy() > 0) cout << "starting correction " << endl;
      it = 0;
      S.F(X, Gamma, F);
      
      do { // Newton iterations
	S.F_gamma(X, Gamma, g);
	S.solve_grad(X, Gamma, F, g, Delta_X, y);
	r = w_sp_(S, T_x, y);

	Delta_Gamma = w_sp_(S, T_x, Delta_X) / (r - T_gamma);
	S.scaled_add(Delta_X, y, -Delta_Gamma, Delta_X);
	S.scaled_add(X, Delta_X, -1., X); Gamma -= Delta_Gamma;
	S.set_build(BUILD_ALL);
	
	T_gamma = 1. / (T_gamma - r);
	S.scale(y, -T_gamma); S.copy(y, T_x);
	no = norm_(S, T_x, T_gamma);
	S.scale(T_x, 1./no); T_gamma /= no;

	S.F(X, Gamma, F); res = norm_(S, F); 
	diff = norm_(S, Delta_X, Delta_Gamma);
	converged = (res <= S.maxres() && diff <= S.maxdiff());
	it++;

	if (S.noisy() > 1)
	  cout << "iter " << it << " residual " << res
	       << " difference " << diff
	       << " cos " << sp_(S, t_x, T_x, t_gamma, T_gamma) << endl;

      } while (!converged && it < S.maxit() && res < 1.e8);

      if (converged) {
	ang = sp_(S, t_x, T_x, t_gamma, T_gamma);
	if (S.noisy() > 0) cout << "cos " << ang << endl;
// 	if (S.noisy() > 1) {
// 	  S.F_gamma(X, Gamma, g);
// 	  S.update_matrix(X, Gamma); S.mult_grad(X, Gamma, T_x, y);
// 	  S.scaled_add(y, g, T_gamma, y);
// 	  cout << "final tangent computed with the residual "
// 	       << norm_(S, y) << endl;
// 	}
	if (ang >= S.minang()) { // accept the new couple
// 	  if (tangent_status == 0)
	    bifurcation = test_smooth_bifurcation(S, X, Gamma, T_x, T_gamma);
	  if (bifurcation) cout << "Bifurcation detected!" << endl;
	  if (step_dec == 0 && it < S.thrit()) // elongate the step size
	    h = (S.h_inc() * h < S.h_max()) ? S.h_inc() * h : S.h_max();
	  finished = true;
	}
      }
      
      if (!finished) {
	if (h > S.h_min()) { // diminish the step size
	  h = (S.h_dec() * h > S.h_min()) ? S.h_dec() * h : S.h_min();
	  step_dec++;
	}
	else if (tangent_status == 0) {
	  if (S.noisy() > 1)
	    cout << "Seeking a new tangent direction" << endl;
	  unsigned long tan = 0;
	  S.copy(t_x, T_x); T_gamma = t_gamma;
	  S.scaled_add(x, T_x, h, X); Gamma = gamma + h * T_gamma;
	  S.set_build(BUILD_ALL);
	  compute_tangent(S, X, Gamma, T_x, T_gamma);

	  do { // seek a new tangent
	    if (S.noisy() > 1)
	      cout << "Trying direction " << tan + 1 << endl;
	    h = S.h_min();

	    do { // test (T_x, T_gamma)
	      tangent_status =
		test_direction(S, x, gamma, t_x, t_gamma, T_x, T_gamma, h);
	      if (tangent_status == 1) {
		h *= -1.;
		tangent_status =
		  test_direction(S, x, gamma, t_x, t_gamma, T_x, T_gamma, h);
	        h *= -2.;
	      }
	    } while (tangent_status == 1 && h <= 1e5);

	    tan++;
	  } while (tangent_status <= 2 && tan < 1); // tan >= 1?
	  
	  if (tangent_status >= 3) {
	    if (S.noisy() > 1)
	      cout << "Direction " << tan << " accepted" << endl;
	    S.copy(T_x, t_x); t_gamma = T_gamma;
	    if (tangent_status == 4) { 
	      S.scale(t_x, -1.); t_gamma *= -1.; h /= 2.;
	    }
	    h = (h < S.h_init()) ? S.h_init() : h; step_dec = 0;
	  } else break;
	} else break;
      }
    } while (!finished);

    if (finished) {
      S.copy(X, x); gamma = Gamma;
      S.copy(T_x, t_x); t_gamma = T_gamma;
    } else h = 0;
  }



  //=========================================================================
  // Moore-Penrose continuation method for Getfem models
  //=========================================================================


#ifdef GETFEM_MODELS_H__
 
  struct cont_struct_getfem_model {

    typedef base_vector VECT;

    model *md;  // for real models only
    std::string parameter_name_;
    rmodel_plsolver_type lsolver;
    double scfac_;
    unsigned long maxit_, thrit_;
    double maxres_, maxdiff_, minang_, h_init_, h_max_, h_min_, h_inc_,
      h_dec_, epsilon_, maxres_solve_;
    int noisy_;
    bool with_parametrized_data;
    std::string initdata_name_, finaldata_name_, currentdata_name_;
    build_data build;
    double tau1_, tau2_, tau3_;
    VECT b_x_, c_x_;
    double b_gamma_, c_gamma_, d_;

    cont_struct_getfem_model
    (model &m, const std::string &pn, rmodel_plsolver_type ls, double sfac,
     unsigned long mit = 10, unsigned long tit = 8, double mres = 1.e-6,
     double mdiff = 1.e-9, double mang = 0.9, double hin = 1.e-2,
     double hmax = 1.e-1, double hmin = 1.e-5, double hinc = 1.3,
     double hdec = 0.5, double eps = 1.e-8, double mress = 1.e-7,
     int noi = 0, double t1 = 1.e4, double t2 = 1.e4, double t3 = 1.e4)
      : md(&m), parameter_name_(pn), lsolver(ls), scfac_(sfac), maxit_(mit),
	thrit_(tit), maxres_(mres), maxdiff_(mdiff), minang_(mang),
	h_init_(hin), h_max_(hmax), h_min_(hmin), h_inc_(hinc), h_dec_(hdec),
	epsilon_(eps), maxres_solve_(mress), noisy_(noi),
	with_parametrized_data(false), build(BUILD_ALL), tau1_(t1),
	tau2_(t2), tau3_(t3)
    {}
    
    cont_struct_getfem_model
    (model &m, const std::string &pn, const std::string &in,
     const std::string &fn, const std::string &cn, rmodel_plsolver_type ls,
     double sfac, unsigned long mit = 10, unsigned long tit = 8,
     double mres = 1.e-6, double mdiff = 1.e-9, double mang = 0.9,
     double hin = 1.e-2, double hmax = 1.e-1, double hmin = 1.e-5,
     double hinc = 1.3, double hdec = 0.5, double eps = 1.e-8,
     double mress = 1.e-7, int noi = 0, double t1 = 1.e4,
     double t2 = 1.e4, double t3 = 1.e4)
      : md(&m), parameter_name_(pn), lsolver(ls), scfac_(sfac), maxit_(mit),
	thrit_(tit), maxres_(mres), maxdiff_(mdiff), minang_(mang),
	h_init_(hin), h_max_(hmax), h_min_(hmin), h_inc_(hinc), h_dec_(hdec),
	epsilon_(eps), maxres_solve_(mress), noisy_(noi),
	with_parametrized_data(true), initdata_name_(in),
	finaldata_name_(fn), currentdata_name_(cn), build(BUILD_ALL),
	tau1_(t1), tau2_(t2), tau3_(t3)
    {}

    cont_struct_getfem_model(void) {}
    

    // Linear algebra functions
    double abs(double a)
    { return gmm::abs(a); }
    void clear(VECT &v)
    { gmm::clear(v); }
    void copy(const VECT &v1, VECT &v)
    { gmm::copy(v1, v); }
    void scale(VECT &v, double a)
    { gmm::scale(v, a); }
    void scaled_add(const VECT &v1, const VECT &v2, double a, VECT &v)
    { gmm::add(v1, gmm::scaled(v2, a), v); }
    double sp(const VECT &v1, const VECT &v2)
    { return gmm::vect_sp(v1, v2); }


    // Evaluation of  ...
    void set_variables(const VECT &x, double gamma) {
      md->set_real_variable(parameter_name_)[0] = gamma;
      if (with_parametrized_data) {
	gmm::add(gmm::scaled(md->real_variable(initdata_name_), 1. - gamma),
		 gmm::scaled(md->real_variable(finaldata_name_), gamma),
		 md->set_real_variable(currentdata_name_));
      }
      md->to_variables(x);
    }

    // F(x, gamma) --> f
    void F(const VECT &x, double gamma, VECT &f) {
      if (build == BUILD_ALL) set_variables(x, gamma);
      if (build & BUILD_F) {
	md->assembly(model::BUILD_RHS);
	build = build_data(build ^ BUILD_F);
      }
      gmm::copy(gmm::scaled(md->real_rhs(), -1.), f);
    }
    
    // (F(x, gamma + epsilon_) - F(x, gamma)) / epsilon_ --> g
    void F_gamma(const VECT &x, double gamma, VECT &g) {
      VECT F0(x), F1(x);
      F(x, gamma, F0);
      build = BUILD_ALL; F(x, gamma + epsilon_, F1); build = BUILD_ALL;
      gmm::add(F1, gmm::scaled(F0, -1.), g);
      gmm::scale(g, 1./epsilon_);
    }

    void update_matrix(const VECT &x, double gamma) {
      if (build == BUILD_ALL) set_variables(x, gamma);
      if (build & BUILD_F_x) {
	if (noisy_ > 1) cout << "starting computing tangent matrix" << endl;
	md->assembly(model::BUILD_MATRIX);
	build = build_data(build ^ BUILD_F_x);
      }
    }

    // solve F_x(x, gamma) * g = L
    void solve_grad(const VECT &x, double gamma,
		    const VECT &L, VECT &g) {
      update_matrix(x, gamma);
      if (noisy_ > 1) cout << "starting linear solver" << endl;
      gmm::iteration iter(maxres_solve_, noisy_, 40000);
      (*lsolver)(md->real_tangent_matrix(), g, L, iter);
      if (noisy_ > 1) cout << "linear solver done" << endl;
    }

    // solve F_x(x, gamma) * (g1|g2) = (L1|L2)
    // (can be optimised)
    void solve_grad(const VECT &x, double gamma, const VECT &L1,
		    const VECT &L2, VECT &g1, VECT &g2) {
      update_matrix(x, gamma);
      if (noisy_ > 1) cout << "starting linear solver" << endl;
      gmm::iteration iter(maxres_solve_, noisy_, 40000);
      (*lsolver)(md->real_tangent_matrix(), g1, L1, iter);
      iter.init();
      (*lsolver)(md->real_tangent_matrix(), g2, L2, iter);
      if (noisy_ > 1) cout << "linear solver done" << endl;
    }

    // F_x(x, gamma) * w --> y
    void mult_grad(const VECT &x, double gamma,
			const VECT &w, VECT &y) {
      update_matrix(x, gamma);
      gmm::mult(md->real_tangent_matrix(), w, y);
    }

    
    // Misc.
    void init_border(const VECT &v) {
      srand(unsigned(time(NULL)));
      gmm::resize(b_x_, gmm::vect_size(v)); gmm::fill_random(b_x_);
      gmm::resize(c_x_, gmm::vect_size(v)); gmm::fill_random(c_x_);
      b_gamma_ = gmm::random(1.); c_gamma_ = gmm::random(1.);
      d_ = gmm::random(1.);
    }

    model &linked_model(void) { return *md; }
    std::string parameter_name(void) { return parameter_name_; }
    double scfac(void) { return scfac_; }
    unsigned long thrit(void) { return thrit_; }
    unsigned long maxit(void) { return maxit_; }
    double epsilon(void) { return epsilon_; }
    double minang(void) { return minang_; }
    double maxres(void) { return maxres_; }
    double maxdiff(void) { return maxdiff_; }
    double h_init(void) { return h_init_; }
    double h_min(void) { return h_min_; }
    double h_max(void) { return h_max_; }
    double h_dec(void) { return h_dec_; }
    double h_inc(void) { return h_inc_; }
    int noisy(void) { return noisy_; }
    void set_build(build_data build_) { build = build_; }
    void set_tau1(double tau) { tau1_ = tau; }
    double tau1(void) { return tau1_; }
    void set_tau2(double tau) { tau2_ = tau; }
    double tau2(void) { return tau2_; }
    void set_tau3(double tau) { tau3_ = tau; }
    double tau3(void) { return tau3_; }
    VECT &b_x(void) { return b_x_; }
    VECT &c_x(void) { return c_x_; }
    double b_gamma(void) { return b_gamma_; }
    double c_gamma(void) { return c_gamma_; }
    double d(void) { return d_; }
  };

#endif


}  /* end of namespace getfem.                                             */


#endif /* GETFEM_CONTINUATION_H__ */
