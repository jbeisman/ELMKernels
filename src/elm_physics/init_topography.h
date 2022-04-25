/*! \file init_topography.h
\brief Functions derived from from initVerticalMod.F90
*/
#pragma once

#include "init_topography.h"
#include "elm_constants.h"
#include <algorithm>
#include <cmath>

#include "kokkos_includes.hh"

namespace ELM::init_topo {

/*! Set minimum slope value
/param topo_slope [double] slope (-)
*/
ACCELERATED
void init_topo_slope(double& topo_slope);

/*! Initialize n_melt and micro_sigma for SCA calculations.
\param[in]  ltype       [integer] landunit type
\param[in]  topo_slope  [double] gridcell topographic slope -- this must be % slope
\param[in]  topo_std    [double] gridcell elevation standard deviation
\param[out] n_melt      [double] SCA shape parameter
\param[out] micro_sigma [double] microtopography pdf sigma (m)
*/
ACCELERATED
void init_micro_topo(const int& ltype, const double& topo_slope, const double& topo_std, double& n_melt,
                     double& micro_sigma);

} // namespace ELM::init_topo

#include "init_topography_impl.hh"
