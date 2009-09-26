/*! \file
    \ingroup CCSORT
    \brief Enter brief description of file here 
*/

/*! \defgroup CCSORT ccsort: Sort integrals for Coupled-Cluster Modules */

#include <string>

namespace psi { namespace ccsort {

struct Local {
  int natom;
  int nso;
  int nocc;
  int nvir;
  int domain_polar;
  int domain_mag;
  int domain_sep;
  int *aostart;
  int *aostop;
  int **domain;
  int **pairdomain;
  int *domain_len;
  int *pairdom_len;
  int *pairdom_nrlen;
  int *weak_pairs;
  double ***V;
  double ***W;
  double *eps_occ;
  double **eps_vir;
  double cutoff;
  std::string method;
  std::string weakp;
  int filter_singles;
  double weak_pair_energy;
  double cphf_cutoff;
  double core_cutoff;
  std::string freeze_core;
  std::string pairdef;
};

}} // namespace psi::ccsort
