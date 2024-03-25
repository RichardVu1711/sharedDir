//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: mvnpdf_code.cpp
//
// MATLAB Coder version            : 5.2
// C/C++ source code generated on  : 02-Feb-2022 00:16:46
//

// Include Files
#include "mvnpdf_code.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Function Declarations
namespace coder {
namespace internal {
namespace blas {
static double xnrm2(int n, const double x_data[], int ix0);

}
namespace lapack {
static void xgeqp3(double A_data[], const int A_size[2], double tau_data[],
                   int *tau_size, int jpvt_data[], int jpvt_size[2]);

}
static void mrdiv(double A_data[], int A_size[2], const double B_data[],
                  const int B_size[2]);

static int rankFromQR(const double A_data[], const int A_size[2]);

} // namespace internal
} // namespace coder
static double rt_hypotd_snf(double u0, double u1);

static double rt_powd_snf(double u0, double u1);

// Function Definitions
//
// Arguments    : int n
//                const double x_data[]
//                int ix0
// Return Type  : double
//
namespace coder {
namespace internal {
namespace blas {
static double xnrm2(int n, const double x_data[], int ix0)
{
  double y;
  y = 0.0;
  if (n >= 1) {
    if (n == 1) {
      y = std::abs(x_data[ix0 - 1]);
    } else {
      double scale;
      int kend;
      scale = 3.3121686421112381E-170;
      kend = (ix0 + n) - 1;
      for (int k{ix0}; k <= kend; k++) {
        double absxk;
        absxk = std::abs(x_data[k - 1]);
        if (absxk > scale) {
          double t;
          t = scale / absxk;
          y = y * t * t + 1.0;
          scale = absxk;
        } else {
          double t;
          t = absxk / scale;
          y += t * t;
        }
      }
      y = scale * std::sqrt(y);
    }
  }
  return y;
}

//
// Arguments    : double A_data[]
//                const int A_size[2]
//                double tau_data[]
//                int *tau_size
//                int jpvt_data[]
//                int jpvt_size[2]
// Return Type  : void
//
} // namespace blas
namespace lapack {
static void xgeqp3(double A_data[], const int A_size[2], double tau_data[],
                   int *tau_size, int jpvt_data[], int jpvt_size[2])
{
  double vn1_data[6];
  double vn2_data[6];
  double work_data[6];
  int ix;
  int m;
  int n;
  m = A_size[0];
  n = A_size[1];
  ix = A_size[0];
  *tau_size = A_size[1];
  if (ix < *tau_size) {
    *tau_size = ix;
  }
  if (0 <= *tau_size - 1) {
    std::memset(&tau_data[0], 0, *tau_size * sizeof(double));
  }
  if ((A_size[0] == 0) || (A_size[1] == 0) || (*tau_size < 1)) {
    ix = A_size[1];
    jpvt_size[0] = 1;
    jpvt_size[1] = ix;
    if (0 <= ix - 1) {
      std::memset(&jpvt_data[0], 0, ix * sizeof(int));
    }
    for (int j{0}; j < n; j++) {
      jpvt_data[j] = j + 1;
    }
  } else {
    double d;
    int j;
    int lastc;
    int ma;
    ix = A_size[1];
    jpvt_size[0] = 1;
    jpvt_size[1] = ix;
    if (0 <= ix - 1) {
      std::memset(&jpvt_data[0], 0, ix * sizeof(int));
    }
    for (lastc = 0; lastc < n; lastc++) {
      jpvt_data[lastc] = lastc + 1;
    }
    ma = A_size[0];
    ix = A_size[1];
    if (0 <= ix - 1) {
      std::memset(&work_data[0], 0, ix * sizeof(double));
    }
    ix = A_size[1];
    if (0 <= ix - 1) {
      std::memset(&vn1_data[0], 0, ix * sizeof(double));
    }
    ix = A_size[1];
    if (0 <= ix - 1) {
      std::memset(&vn2_data[0], 0, ix * sizeof(double));
    }
    for (j = 0; j < n; j++) {
      d = blas::xnrm2(m, A_data, j * ma + 1);
      vn1_data[j] = d;
      vn2_data[j] = d;
    }
    for (int i{0}; i < *tau_size; i++) {
      double atmp;
      double s;
      double smax;
      int b_i;
      int ii;
      int ip1;
      int knt;
      int mmi;
      int nmi;
      int pvt;
      ip1 = i + 2;
      j = i * ma;
      ii = j + i;
      nmi = n - i;
      mmi = m - i;
      if (nmi < 1) {
        ix = -1;
      } else {
        ix = 0;
        if (nmi > 1) {
          smax = std::abs(vn1_data[i]);
          for (lastc = 2; lastc <= nmi; lastc++) {
            s = std::abs(vn1_data[(i + lastc) - 1]);
            if (s > smax) {
              ix = lastc - 1;
              smax = s;
            }
          }
        }
      }
      pvt = i + ix;
      if (pvt != i) {
        ix = pvt * ma;
        for (lastc = 0; lastc < m; lastc++) {
          knt = ix + lastc;
          smax = A_data[knt];
          b_i = j + lastc;
          A_data[knt] = A_data[b_i];
          A_data[b_i] = smax;
        }
        ix = static_cast<signed char>(jpvt_data[pvt]);
        jpvt_data[pvt] = static_cast<signed char>(jpvt_data[i]);
        jpvt_data[i] = ix;
        vn1_data[pvt] = vn1_data[i];
        vn2_data[pvt] = vn2_data[i];
      }
      if (i + 1 < m) {
        atmp = A_data[ii];
        ix = ii + 2;
        tau_data[i] = 0.0;
        if (mmi > 0) {
          smax = blas::xnrm2(mmi - 1, A_data, ii + 2);
          if (smax != 0.0) {
            s = rt_hypotd_snf(A_data[ii], smax);
            if (A_data[ii] >= 0.0) {
              s = -s;
            }
            if (std::abs(s) < 1.0020841800044864E-292) {
              knt = -1;
              b_i = ii + mmi;
              do {
                knt++;
                for (lastc = ix; lastc <= b_i; lastc++) {
                  A_data[lastc - 1] *= 9.9792015476736E+291;
                }
                s *= 9.9792015476736E+291;
                atmp *= 9.9792015476736E+291;
              } while (!(std::abs(s) >= 1.0020841800044864E-292));
              s = rt_hypotd_snf(atmp, blas::xnrm2(mmi - 1, A_data, ii + 2));
              if (atmp >= 0.0) {
                s = -s;
              }
              tau_data[i] = (s - atmp) / s;
              smax = 1.0 / (atmp - s);
              for (lastc = ix; lastc <= b_i; lastc++) {
                A_data[lastc - 1] *= smax;
              }
              for (lastc = 0; lastc <= knt; lastc++) {
                s *= 1.0020841800044864E-292;
              }
              atmp = s;
            } else {
              tau_data[i] = (s - A_data[ii]) / s;
              smax = 1.0 / (A_data[ii] - s);
              b_i = ii + mmi;
              for (lastc = ix; lastc <= b_i; lastc++) {
                A_data[lastc - 1] *= smax;
              }
              atmp = s;
            }
          }
        }
        A_data[ii] = atmp;
      } else {
        tau_data[i] = 0.0;
      }
      if (i + 1 < n) {
        int jA;
        int lastv;
        atmp = A_data[ii];
        A_data[ii] = 1.0;
        jA = (ii + ma) + 1;
        if (tau_data[i] != 0.0) {
          boolean_T exitg2;
          lastv = mmi - 1;
          ix = (ii + mmi) - 1;
          while ((lastv + 1 > 0) && (A_data[ix] == 0.0)) {
            lastv--;
            ix--;
          }
          lastc = nmi - 2;
          exitg2 = false;
          while ((!exitg2) && (lastc + 1 > 0)) {
            int exitg1;
            ix = jA + lastc * ma;
            j = ix;
            do {
              exitg1 = 0;
              if (j <= ix + lastv) {
                if (A_data[j - 1] != 0.0) {
                  exitg1 = 1;
                } else {
                  j++;
                }
              } else {
                lastc--;
                exitg1 = 2;
              }
            } while (exitg1 == 0);
            if (exitg1 == 1) {
              exitg2 = true;
            }
          }
        } else {
          lastv = -1;
          lastc = -1;
        }
        if (lastv + 1 > 0) {
          if (lastc + 1 != 0) {
            if (0 <= lastc) {
              std::memset(&work_data[0], 0, (lastc + 1) * sizeof(double));
            }
            knt = 0;
            b_i = jA + ma * lastc;
            for (pvt = jA; ma < 0 ? pvt >= b_i : pvt <= b_i; pvt += ma) {
              smax = 0.0;
              ix = pvt + lastv;
              for (j = pvt; j <= ix; j++) {
                smax += A_data[j - 1] * A_data[(ii + j) - pvt];
              }
              work_data[knt] += smax;
              knt++;
            }
          }
          if (!(-tau_data[i] == 0.0)) {
            for (j = 0; j <= lastc; j++) {
              d = work_data[j];
              if (d != 0.0) {
                smax = d * -tau_data[i];
                b_i = lastv + jA;
                for (knt = jA; knt <= b_i; knt++) {
                  A_data[knt - 1] += A_data[(ii + knt) - jA] * smax;
                }
              }
              jA += ma;
            }
          }
        }
        A_data[ii] = atmp;
      }
      for (j = ip1; j <= n; j++) {
        ix = i + (j - 1) * ma;
        d = vn1_data[j - 1];
        if (d != 0.0) {
          smax = std::abs(A_data[ix]) / d;
          smax = 1.0 - smax * smax;
          if (smax < 0.0) {
            smax = 0.0;
          }
          s = d / vn2_data[j - 1];
          s = smax * (s * s);
          if (s <= 1.4901161193847656E-8) {
            if (i + 1 < m) {
              d = blas::xnrm2(mmi - 1, A_data, ix + 2);
              vn1_data[j - 1] = d;
              vn2_data[j - 1] = d;
            } else {
              vn1_data[j - 1] = 0.0;
              vn2_data[j - 1] = 0.0;
            }
          } else {
            vn1_data[j - 1] = d * std::sqrt(smax);
          }
        }
      }
    }
  }
}

//
// Arguments    : double A_data[]
//                int A_size[2]
//                const double B_data[]
//                const int B_size[2]
// Return Type  : void
//
} // namespace lapack
static void mrdiv(double A_data[], int A_size[2], const double B_data[],
                  const int B_size[2])
{
  double b_A_data[36];
  double Y_data[6];
  double b_B_data[6];
  double tau_data[6];
  int jpvt_data[6];
  int b_A_size[2];
  int jpvt_size[2];
  int m;
  if ((A_size[1] == 0) || ((B_size[0] == 0) || (B_size[1] == 0))) {
    A_size[0] = 1;
    A_size[1] = static_cast<signed char>(B_size[0]);
    m = static_cast<signed char>(B_size[0]);
    if (0 <= m - 1) {
      std::memset(&A_data[0], 0, m * sizeof(double));
    }
  } else if (B_size[0] == B_size[1]) {
    double wj;
    int i;
    int j;
    int jp1j;
    int k;
    int ldap1;
    int n;
    int u1;
    int yk;
    n = B_size[1];
    b_A_size[0] = B_size[0];
    m = B_size[1];
    for (i = 0; i < m; i++) {
      yk = B_size[0];
      for (jp1j = 0; jp1j < yk; jp1j++) {
        b_A_data[jp1j + b_A_size[0] * i] = B_data[jp1j + B_size[0] * i];
      }
    }
    m = B_size[1];
    jpvt_data[0] = 1;
    yk = 1;
    for (k = 2; k <= m; k++) {
      yk++;
      jpvt_data[k - 1] = yk;
    }
    ldap1 = B_size[1];
    yk = B_size[1] - 1;
    u1 = B_size[1];
    if (yk < u1) {
      u1 = yk;
    }
    for (j = 0; j < u1; j++) {
      int jj;
      int mmj_tmp;
      int mn;
      int rankA;
      mmj_tmp = n - j;
      rankA = j * (n + 1);
      jj = j * (ldap1 + 1);
      jp1j = rankA + 2;
      if (mmj_tmp < 1) {
        yk = -1;
      } else {
        yk = 0;
        if (mmj_tmp > 1) {
          wj = std::abs(b_A_data[jj]);
          for (k = 2; k <= mmj_tmp; k++) {
            double s;
            s = std::abs(b_A_data[(rankA + k) - 1]);
            if (s > wj) {
              yk = k - 1;
              wj = s;
            }
          }
        }
      }
      if (b_A_data[jj + yk] != 0.0) {
        if (yk != 0) {
          m = j + yk;
          jpvt_data[j] = m + 1;
          for (k = 0; k < n; k++) {
            yk = k * n;
            mn = j + yk;
            wj = b_A_data[mn];
            yk += m;
            b_A_data[mn] = b_A_data[yk];
            b_A_data[yk] = wj;
          }
        }
        i = jj + mmj_tmp;
        for (yk = jp1j; yk <= i; yk++) {
          b_A_data[yk - 1] /= b_A_data[jj];
        }
      }
      yk = rankA + n;
      m = jj + ldap1;
      for (mn = 0; mn <= mmj_tmp - 2; mn++) {
        wj = b_A_data[yk + mn * n];
        if (wj != 0.0) {
          i = m + 2;
          jp1j = mmj_tmp + m;
          for (rankA = i; rankA <= jp1j; rankA++) {
            b_A_data[rankA - 1] += b_A_data[((jj + rankA) - m) - 1] * -wj;
          }
        }
        m += n;
      }
    }
    if (A_size[1] != 0) {
      for (j = 0; j < n; j++) {
        m = n * j;
        for (k = 0; k < j; k++) {
          i = k + m;
          if (b_A_data[i] != 0.0) {
            A_data[j] -= b_A_data[i] * A_data[k];
          }
        }
        A_data[j] *= 1.0 / b_A_data[j + m];
      }
    }
    if (A_size[1] != 0) {
      for (j = n; j >= 1; j--) {
        m = n * (j - 1) - 1;
        i = j + 1;
        for (k = i; k <= n; k++) {
          jp1j = k + m;
          if (b_A_data[jp1j] != 0.0) {
            A_data[j - 1] -= b_A_data[jp1j] * A_data[k - 1];
          }
        }
      }
    }
    i = B_size[1] - 1;
    for (j = i; j >= 1; j--) {
      jp1j = jpvt_data[j - 1];
      if (jp1j != j) {
        wj = A_data[j - 1];
        A_data[j - 1] = A_data[jp1j - 1];
        A_data[jp1j - 1] = wj;
      }
    }
  } else {
    int i;
    int j;
    int jp1j;
    int mn;
    int rankA;
    int yk;
    m = A_size[1];
    if (0 <= m - 1) {
//      std::copy(&A_data[0], &A_data[m], &b_B_data[0]);
      for(int ii=0; ii <m;ii++) b_B_data[ii] = A_data[ii];
    }
    b_A_size[0] = B_size[1];
    b_A_size[1] = B_size[0];
    m = B_size[0];
    for (i = 0; i < m; i++) {
      yk = B_size[1];
      for (jp1j = 0; jp1j < yk; jp1j++) {
        b_A_data[jp1j + b_A_size[0] * i] = B_data[i + B_size[0] * jp1j];
      }
    }
    lapack::xgeqp3(b_A_data, b_A_size, tau_data, &m, jpvt_data, jpvt_size);
    rankA = rankFromQR(b_A_data, b_A_size);
    jp1j = static_cast<signed char>(b_A_size[1]);
    if (0 <= jp1j - 1) {
      std::memset(&Y_data[0], 0, jp1j * sizeof(double));
    }
    m = b_A_size[0];
    yk = b_A_size[0];
    mn = b_A_size[1];
    if (yk < mn) {
      mn = yk;
    }
    for (j = 0; j < mn; j++) {
      if (tau_data[j] != 0.0) {
        double wj;
        wj = b_B_data[j];
        i = j + 2;
        for (yk = i; yk <= m; yk++) {
          wj += b_A_data[(yk + b_A_size[0] * j) - 1] * b_B_data[yk - 1];
        }
        wj *= tau_data[j];
        if (wj != 0.0) {
          b_B_data[j] -= wj;
          for (yk = i; yk <= m; yk++) {
            b_B_data[yk - 1] -= b_A_data[(yk + b_A_size[0] * j) - 1] * wj;
          }
        }
      }
    }
    for (yk = 0; yk < rankA; yk++) {
      Y_data[jpvt_data[yk] - 1] = b_B_data[yk];
    }
    for (j = rankA; j >= 1; j--) {
      i = jpvt_data[j - 1];
      m = b_A_size[0] * (j - 1);
      Y_data[i - 1] /= b_A_data[(j + m) - 1];
      for (yk = 0; yk <= j - 2; yk++) {
        Y_data[jpvt_data[yk] - 1] -=
            Y_data[jpvt_data[j - 1] - 1] * b_A_data[yk + m];
      }
    }
    A_size[0] = 1;
    A_size[1] = static_cast<signed char>(b_A_size[1]);
    if (0 <= jp1j - 1) {
//      std::copy(&Y_data[0], &Y_data[jp1j], &A_data[0]);
     for(int ii=0; ii<jp1j;ii++)
    	 A_data[ii]= Y_data[ii];
    }
  }
}

//
// Arguments    : const double A_data[]
//                const int A_size[2]
// Return Type  : int
//
static int rankFromQR(const double A_data[], const int A_size[2])
{
  int maxmn;
  int minmn;
  int r;
  r = 0;
  if (A_size[0] < A_size[1]) {
    minmn = A_size[0];
    maxmn = A_size[1];
  } else {
    minmn = A_size[1];
    maxmn = A_size[0];
  }
  if (minmn > 0) {
    double tol;
    tol = 2.2204460492503131E-15 * static_cast<double>(maxmn) *
          std::abs(A_data[0]);
    while ((r < minmn) && (!(std::abs(A_data[r + A_size[0] * r]) <= tol))) {
      r++;
    }
  }
  return r;
}

//
// Arguments    : double u0
//                double u1
// Return Type  : double
//
} // namespace internal
} // namespace coder
static double rt_hypotd_snf(double u0, double u1)
{
  double a;
  double y;
  a = std::abs(u0);
  y = std::abs(u1);
  if (a < y) {
    a /= y;
    y *= std::sqrt(a * a + 1.0);
  } else if (a > y) {
    y /= a;
    y = a * std::sqrt(y * y + 1.0);
  } else if (!std::isnan(y)) {
    y = a * 1.4142135623730951;
  }
  return y;
}

//
// Arguments    : double u0
//                double u1
// Return Type  : double
//
static double rt_powd_snf(double u0, double u1)
{
	double y;
	double d;
	double d1;
	d = std::abs(u0);
	d1 = std::abs(u1);
	if (std::isinf(u1)) {
	  if (d == 1.0) {
		y = 1.0;
	  } else if (d > 1.0) {
		  y = 0.0;
	  } else if (u1 > 0.0) {
		y = 0.0;
	  }
	} else if (d1 == 0.0) {
	  y = 1.0;
	} else if (d1 == 1.0) {
	  if (u1 > 0.0) {
		y = u0;
	  } else {
		y = 1.0 / u0;
	  }
	} else if (u1 == 2.0) {
	  y = u0 * u0;
	} else if ((u1 == 0.5) && (u0 >= 0.0)) {
	  y = std::sqrt(u0);
	} else {
	  y = std::pow(u0, u1);
	}

  return y;
}

//
// Arguments    : const double zCap[6]
//                const double Mu[6]
//                const double Pzx[6][6]
//                double n_obs
// Return Type  : double
//
fixed_type mvnpdf_code(fixed_type zCap_in[6], fixed_type [6],
		fixed_type Pzx[6*6], int n_obs)
{
  double A_data[36];
  double Pzx_fi_data[36];
  double zCap[6];
  double Y_data[36];
  double xRinv_data[36];
  double logSqrtDetSigma_data[6];
  double tau_data[6];
  int jpvt_data[6];
  int A_size[2];
  int Pzx_fi_size[2];
  int logSqrtDetSigma_size[2];
  int b_loop_ub;
  int c_loop_ub;
  int d;
  int i;
  int i1;
  int idxA1j;
  int idxAjj;
  int info;
  int isReverse;
  int iy;
  int loop_ub;
  int nb;
  int nmj;
  signed char sz_idx_0;
  boolean_T sigmaIsDiag;
  for(int i=0; i < 6;i++){
	  zCap[i] = zCap_in[i];
  }
  if (1.0 > n_obs) {
    loop_ub = 0;
    b_loop_ub = 0;
    c_loop_ub = 0;
  } else {
    loop_ub = n_obs;
    b_loop_ub = n_obs;
    c_loop_ub = n_obs;
  }
  Pzx_fi_size[0] = b_loop_ub;
  Pzx_fi_size[1] = c_loop_ub;
  for (i = 0; i < c_loop_ub; i++) {
    for (i1 = 0; i1 < b_loop_ub; i1++) {
      Pzx_fi_data[i1 + b_loop_ub * i] = Pzx[i*6+i1];
    }
  }
  // MVNPDF Multivariate normal probability density function (pdf).
  //    Y = MVNPDF(X) returns the probability density of the multivariate normal
  //    distribution with zero mean and identity covariance matrix, evaluated at
  //    each row of X.  Rows of the N-by-D matrix X correspond to observations
  //    or points, and columns correspond to variables or coordinates.  Y is an
  //    N-by-1 vector.
  //
  //    Y = MVNPDF(X,MU) returns the density of the multivariate normal
  //    distribution with mean MU and identity covariance matrix, evaluated
  //    at each row of X.  MU is a 1-by-D vector, or an N-by-D matrix, in which
  //    case the density is evaluated for each row of X with the corresponding
  //    row of MU.  MU can also be a scalar value, which MVNPDF replicates to
  //    match the size of X.
  //
  //    Y = MVNPDF(X,MU,SIGMA) returns the density of the multivariate normal
  //    distribution with mean MU and covariance SIGMA, evaluated at each row
  //    of X.  SIGMA is a D-by-D matrix, or an D-by-D-by-N array, in which case
  //    the density is evaluated for each row of X with the corresponding page
  //    of SIGMA, i.e., MVNPDF computes Y(I) using X(I,:) and SIGMA(:,:,I).
  //    If the covariance matrix is diagonal, containing variances along the
  //    diagonal and zero covariances off the diagonal, SIGMA may also be
  //    specified as a 1-by-D matrix or a 1-by-D-by-N array, containing
  //    just the diagonal. Pass in the empty matrix for MU to use its default
  //    value when you want to only specify SIGMA.
  //
  //    If X is a 1-by-D vector, MVNPDF replicates it to match the leading
  //    dimension of MU or the trailing dimension of SIGMA.
  //
  //    Example:
  //
  //       mu = [1 -1]; Sigma = [.9 .4; .4 .3];
  //       [X1,X2] = meshgrid(linspace(-1,3,25)', linspace(-3,1,25)');
  //       X = [X1(:) X2(:)];
  //       p = mvnpdf(X, mu, Sigma);
  //       surf(X1,X2,reshape(p,25,25));
  //
  //    See also MVTPDF, MVNCDF, MVNRND, NORMPDF.
  //    Copyright 1993-2020 The MathWorks, Inc.
  //  Get size of data.  Column vectors provisionally interpreted as multiple
  //  scalar data.
  d = 1;
  //  Assume zero mean, data are already centered
  sz_idx_0 = static_cast<signed char>(b_loop_ub);
  if ((static_cast<signed char>(b_loop_ub) == 1) &&
      (static_cast<signed char>(c_loop_ub) > 1)) {
    //  Just the diagonal of Sigma has been passed in.
    sz_idx_0 = static_cast<signed char>(c_loop_ub);
    sigmaIsDiag = true;
  } else {
    sigmaIsDiag = false;
  }
  isReverse = 1;
  //  Special case: if Sigma is supplied, then use it to try to interpret
  //  X and Mu as row vectors if they were both column vectors.
  if ((loop_ub > 1) && (sz_idx_0 == loop_ub)) {
    isReverse = 0;
    d = loop_ub;
  }
  // Check that sigma is the right size
  if (sigmaIsDiag) {
    i = c_loop_ub - 1;
    for (nmj = 0; nmj <= i; nmj++) {
      for (nb = 0; nb < b_loop_ub; nb++) {
        iy = nb + b_loop_ub * nmj;
        Pzx_fi_data[iy] = std::sqrt(Pzx_fi_data[iy]);
      }
    }
    if (isReverse == 1) {
      for (i = 0; i < loop_ub; i++) {
        tau_data[i] = zCap[i] / Pzx_fi_data[i];
      }
      info = loop_ub;
      idxA1j = 1;
      if (0 <= loop_ub - 1) {
//        std::copy(&tau_data[0], &tau_data[loop_ub], &xRinv_data[0]);
          for(int i =0; i <loop_ub;i++)
          	xRinv_data[i] = tau_data[i];
      }
    } else {
      info = 1;
      idxA1j = loop_ub;
      for (i = 0; i < loop_ub; i++) {
        xRinv_data[i] = zCap[i] / Pzx_fi_data[b_loop_ub * i];
      }
    }
    for (nmj = 0; nmj < c_loop_ub; nmj++) {
      for (nb = 0; nb < b_loop_ub; nb++) {
        iy = nb + b_loop_ub * nmj;
        Pzx_fi_data[iy] = std::log(Pzx_fi_data[iy]);
      }
    }
    if ((b_loop_ub == 0) || (c_loop_ub == 0)) {
      loop_ub = static_cast<signed char>(c_loop_ub);
      if (0 <= loop_ub - 1) {
        std::memset(&logSqrtDetSigma_data[0], 0, loop_ub * sizeof(double));
      }
    } else {
      for (nmj = 0; nmj < c_loop_ub; nmj++) {
        idxAjj = b_loop_ub * nmj;
        logSqrtDetSigma_data[nmj] = Pzx_fi_data[idxAjj];
        for (nb = 2; nb <= b_loop_ub; nb++) {
          if (b_loop_ub >= 2) {
            logSqrtDetSigma_data[static_cast<signed char>(nmj + 1) - 1] +=
                Pzx_fi_data[(nb + idxAjj) - 1];
          }
        }
      }
    }
  } else {
    double ssq;
    int ia0;
    int idxAjjp1;
    int j;
    //  Make sure Sigma is a valid covariance matrix
    if (c_loop_ub != 0) {
      boolean_T exitg1;
      info = -1;
      j = 0;
      exitg1 = false;
      while ((!exitg1) && (j <= c_loop_ub - 1)) {
        idxA1j = j * c_loop_ub;
        idxAjj = idxA1j + j;
        ssq = 0.0;
        if (j >= 1) {
          for (nmj = 0; nmj < j; nmj++) {
            iy = idxA1j + nmj;
            ssq += Pzx_fi_data[iy] * Pzx_fi_data[iy];
          }
        }
        ssq = Pzx_fi_data[idxAjj] - ssq;
        if (ssq > 0.0) {
          ssq = std::sqrt(ssq);
          Pzx_fi_data[idxAjj] = ssq;
          if (j + 1 < c_loop_ub) {
            nmj = (c_loop_ub - j) - 2;
            ia0 = (idxA1j + c_loop_ub) + 1;
            idxAjjp1 = idxAjj + c_loop_ub;
            if ((j != 0) && (nmj + 1 != 0)) {
              iy = idxAjjp1;
              i = ia0 + c_loop_ub * nmj;
              for (idxAjj = ia0; c_loop_ub < 0 ? idxAjj >= i : idxAjj <= i;
                   idxAjj += c_loop_ub) {
                double c;
                c = 0.0;
                i1 = (idxAjj + j) - 1;
                for (nb = idxAjj; nb <= i1; nb++) {
                  c +=
                      Pzx_fi_data[nb - 1] * Pzx_fi_data[(idxA1j + nb) - idxAjj];
                }
                Pzx_fi_data[iy] += -c;
                iy += c_loop_ub;
              }
            }
            ssq = 1.0 / ssq;
            i = (idxAjjp1 + c_loop_ub * nmj) + 1;
            for (nmj = idxAjjp1 + 1; c_loop_ub < 0 ? nmj >= i : nmj <= i;
                 nmj += c_loop_ub) {
              Pzx_fi_data[nmj - 1] *= ssq;
            }
          }
          j++;
        } else {
          Pzx_fi_data[idxAjj] = ssq;
          info = j;
          exitg1 = true;
        }
      }
      if (info + 1 == 0) {
        idxAjj = c_loop_ub;
      } else {
        idxAjj = info;
      }
      for (j = 0; j < idxAjj; j++) {
        i = j + 2;
        for (iy = i; iy <= idxAjj; iy++) {
          Pzx_fi_data[(iy + b_loop_ub * j) - 1] = 0.0;
        }
      }
    }
    //  Create array of standardized data, and compute log(sqrt(det(Sigma)))
    if (isReverse == 1) {
      if ((loop_ub == 0) || (b_loop_ub == 0)) {
        info = static_cast<signed char>(loop_ub);
        iy = static_cast<signed char>(b_loop_ub);
        idxA1j = static_cast<signed char>(b_loop_ub);
        for (i = 0; i < iy; i++) {
          nmj = static_cast<signed char>(loop_ub);
          for (i1 = 0; i1 < nmj; i1++) {
            xRinv_data[i1 + static_cast<signed char>(loop_ub) * i] = 0.0;
          }
        }
      } else if (b_loop_ub == 1) {
        for (i = 0; i < loop_ub; i++) {
          tau_data[i] = zCap[i] / Pzx_fi_data[0];
        }
        info = loop_ub;
        idxA1j = 1;
        if (0 <= loop_ub - 1) {
//        std::copy(&tau_data[0], &tau_data[loop_ub], &xRinv_data[0]);
        for(int i =0; i <loop_ub;i++)
        	xRinv_data[i] = tau_data[i];
        }
      } else {
        A_size[0] = c_loop_ub;
        A_size[1] = b_loop_ub;
        for (i = 0; i < b_loop_ub; i++) {
          for (i1 = 0; i1 < c_loop_ub; i1++) {
            A_data[i1 + c_loop_ub * i] = Pzx_fi_data[i + b_loop_ub * i1];
          }
        }
        coder::internal::lapack::xgeqp3(A_data, A_size, tau_data, &iy,
                                        jpvt_data, logSqrtDetSigma_size);
        idxAjj = coder::internal::rankFromQR(A_data, A_size);
        nb = static_cast<signed char>(loop_ub);
        ia0 = static_cast<signed char>(A_size[1]);
        idxAjjp1 = static_cast<signed char>(loop_ub);
        nmj = static_cast<signed char>(loop_ub);
        for (i = 0; i < nmj; i++) {
          for (i1 = 0; i1 < ia0; i1++) {
            Y_data[i1 + ia0 * i] = 0.0;
          }
        }
        for (nmj = 0; nmj < nb; nmj++) {
          if (0 <= idxAjj - 1) {
            Y_data[(jpvt_data[0] + ia0 * nmj) - 1] = zCap[nmj];
          }
          for (j = idxAjj; j >= 1; j--) {
            iy = (jpvt_data[0] + ia0 * nmj) - 1;
            Y_data[iy] /= A_data[0];
          }
        }
        info = static_cast<signed char>(loop_ub);
        idxA1j = static_cast<signed char>(A_size[1]);
        for (i = 0; i < ia0; i++) {
          for (i1 = 0; i1 < idxAjjp1; i1++) {
            xRinv_data[i1 + static_cast<signed char>(loop_ub) * i] =
                Y_data[i + ia0 * i1];
          }
        }
      }
    } else {
      logSqrtDetSigma_size[0] = 1;
      logSqrtDetSigma_size[1] = loop_ub;
      if (0 <= loop_ub - 1) {
//        std::copy(&zCap[0], &zCap[loop_ub], &logSqrtDetSigma_data[0]);
        for(int i =0; i <loop_ub;i++)
        	logSqrtDetSigma_data[i] = zCap[i];
      }
      coder::internal::mrdiv(logSqrtDetSigma_data, logSqrtDetSigma_size,
                             Pzx_fi_data, Pzx_fi_size);
      info = 1;
      idxA1j = logSqrtDetSigma_size[1];
      loop_ub = logSqrtDetSigma_size[1];
      if (0 <= loop_ub - 1) {
//        std::copy(&logSqrtDetSigma_data[0], &logSqrtDetSigma_data[loop_ub],
//                  &xRinv_data[0]);
        for(int i =0; i <loop_ub;i++)
        	xRinv_data[i] = logSqrtDetSigma_data[i];
      }
    }
    if ((b_loop_ub == 1) && (c_loop_ub == 1)) {
      iy = 1;
      tau_data[0] = Pzx_fi_data[0];
    } else {
      if (b_loop_ub < c_loop_ub) {
        idxAjj = b_loop_ub;
      } else {
        idxAjj = c_loop_ub;
      }
      if (0 < c_loop_ub) {
        iy = idxAjj;
      } else {
        iy = 0;
      }
      i = iy - 1;
      for (nmj = 0; nmj <= i; nmj++) {
        tau_data[nmj] = Pzx_fi_data[nmj + b_loop_ub * nmj];
      }
    }
    for (nmj = 0; nmj < iy; nmj++) {
      tau_data[nmj] = std::log(tau_data[nmj]);
    }
    if (iy == 0) {
      ssq = 0.0;
    } else {
      ssq = tau_data[0];
      for (nmj = 2; nmj <= iy; nmj++) {
        if (iy >= 2) {
          ssq += tau_data[nmj - 1];
        }
      }
    }
    logSqrtDetSigma_data[0] = ssq;
  }
  //  The quadratic form is the inner products of the standardized data
  idxAjj = static_cast<signed char>(idxA1j);
  for (nmj = 0; nmj < idxAjj; nmj++) {
    iy = static_cast<signed char>(info);
    for (nb = 0; nb < iy; nb++) {
      A_data[nb + static_cast<signed char>(info) * nmj] =
          rt_powd_snf(xRinv_data[nb + info * nmj], 2.0);
    }
  }
  idxAjj = static_cast<signed char>(idxA1j);
  if ((static_cast<signed char>(info) == 0) ||
      (static_cast<signed char>(idxA1j) == 0)) {
    loop_ub = static_cast<signed char>(info);
    if (0 <= loop_ub - 1) {
      std::memset(&tau_data[0], 0, loop_ub * sizeof(double));
    }
  } else {
    i = static_cast<signed char>(info);
    if (0 <= i - 1) {
//      std::copy(&A_data[0], &A_data[i], &tau_data[0]);
      for(int j =0; j <i;j++)
    	  tau_data[j] = A_data[j];
    }
    for (nmj = 2; nmj <= idxAjj; nmj++) {
      i = static_cast<signed char>(info);
      for (nb = 0; nb < i; nb++) {
        if (static_cast<signed char>(idxA1j) >= 2) {
          tau_data[nb] +=
              A_data[nb + static_cast<signed char>(info) * (nmj - 1)];
        }
      }
    }
  }
  double y = std::exp((-0.5 * tau_data[0] - logSqrtDetSigma_data[0]) -
                  static_cast<double>(d) * 1.8378770664093453 / 2.0);
  return (fixed_type)y;
}

extern "C"{
void mvnpdf_fpCall(fixed_type zCap_in[N_MEAS],
					fixed_type Pzx[N_MEAS*N_MEAS],
					int n_obs, fixed_type p_cal[1]){
	fixed_type Mu[6] = {0,0,0,0,0,0};
	p_cal[0] =mvnpdf_code(zCap_in,Mu,Pzx,n_obs);

}
}
