
#pragma once

namespace ELM {

/* CanopyHydrology::SnowInit()
DESCRIPTION:
Initialization snow layer(s) if the snow accumulation exceeds 10 mm.

INPUTS:
Land                         [LandType] struct containing information about landtype
dtime                        [double] time step length (sec)
do_capsnow                   [bool] true => do snow capping
oldfflag                     [int]  use old fsno parameterization
forc_t                       [double] atmospheric temperature (Kelvin)
t_grnd                       [double] ground temperature (Kelvin)
qflx_snow_grnd               [double]   snow on ground after interception (mm H2O/s) [+]
qflx_rain_grnd               [double]   rain on ground after interception (mm H2O/s) [+]
n_melt                       [double]   SCA shape parameter [-]

OUTPUTS:
snow_depth                   [double] snow height (m)
h2osno                       [double] snow water (mm H2O)
int_snow                     [double] integrated snowfall [mm]
h2osoi_liq[nlevgrnd+nlevsno] [double] liquid water (kg/m2)
h2osoi_ice[nlevgrnd+nlevsno] [double] ice lens (kg/m2)
t_soisno[nlevgrnd+nlevsno]   [double] soil temperature (Kelvin)
frac_iceold[nlevgrnd+nlevsno][double] fraction of ice relative to the tot water
snl                          [int] number of snow layers
dz                           [double] layer thickness (m)
z                            [double] layer cell center elevation (m)
zi                           [double] layer interface elevation (m)
snw_rds[nlevsno]             [double] snow grain radius [m^-6, microns]
qflx_snow_h2osfc             [double] snow falling on surface water (mm/s)
frac_sno_eff                 [double] fraction of ground covered by snow (0 to 1)
frac_sno                     [double] fraction of ground covered by snow (0 to 1)
*/
template <class dArray_type>
void SnowInit(const LandType &Land, const double &dtime, const bool &do_capsnow, const int &oldfflag,
              const double &forc_t, const double &t_grnd, const double &qflx_snow_grnd, const double &qflx_snow_melt,
              const double &n_melt,

              double &snow_depth, double &h2osno, double &int_snow, dArray_type swe_old, dArray_type h2osoi_liq,
              dArray_type h2osoi_ice, dArray_type t_soisno, dArray_type frac_iceold, int &snl, dArray_type dz,
              dArray_type z, dArray_type zi, dArray_type snw_rds, double &qflx_snow_h2osfc, double &frac_sno_eff,
              double &frac_sno) {

  if (!Land.lakpoi) {
    double temp_snow_depth, dz_snowf, newsnow, bifall, snowmelt, accum_factor, temp_intsnow, z_avg;
    // Determine snow height and snow water
    // Use Alta relationship, Anderson(1976); LaChapelle(1961),
    // U.S.Department of Agriculture Forest Service, Project F,
    // Progress Rep. 1, Alta Avalanche Study Center:Snow Layer Densification.
    qflx_snow_h2osfc = 0.0;
    // set temporary variables prior to updating
    temp_snow_depth = snow_depth;
    // save initial snow content
    for (int j = 0; j < nlevsno - snl; j++) {
      swe_old[j] = 0.0;
    }
    for (int j = nlevsno - snl; j < nlevsno; j++) {
      swe_old[j] = h2osoi_liq[j] + h2osoi_ice[j];
    }
    if (do_capsnow) {
      dz_snowf = 0.0;
      newsnow = qflx_snow_grnd * dtime;
      frac_sno = 1.0;
      int_snow = 5.e2;
    } else {
      if (forc_t > tfrz + 2.0) {
        bifall = 50.0 + 1.7 * pow(17.0, 1.5);
      } else if (forc_t > tfrz - 15.0) {
        bifall = 50.0 + 1.7 * pow((forc_t - tfrz + 15.0), 1.5);
      } else {
        bifall = 50.0;
      }
      // all snow falls on ground, no snow on h2osfc
      newsnow = qflx_snow_grnd * dtime;
      // update int_snow
      int_snow = std::max(int_snow, h2osno); // h2osno could be larger due to frost
      // snowmelt from previous time step * dtime
      snowmelt = qflx_snow_melt * dtime;
      // set shape factor for accumulation of snow
      accum_factor = 0.1;
      /*======================  FSCA PARAMETERIZATIONS  ======================
      fsca parameterization based on *changes* in swe
      first compute change from melt during previous time step */
      if (h2osno > 0.0) {
        if (snowmelt > 0.0) {
          double smr = std::min(1.0, (h2osno / int_snow));
          frac_sno = 1.0 - pow((acos(std::min(1.0, (2.0 * smr - 1.0))) / ELM_PI), n_melt);
        }
        // update fsca by new snow event, add to previous fsca
        if (newsnow > 0.0) {
          double fsno_new = 1.0 - (1.0 - tanh(accum_factor * newsnow)) * (1.0 - frac_sno);
          frac_sno = fsno_new;
          // reset int_snow after accumulation events
          temp_intsnow =
              (h2osno + newsnow) / (0.5 * (cos(ELM_PI * pow((1.0 - std::max(frac_sno, 1.e-6)), (1.0 / n_melt))) + 1.0));
          int_snow = std::min(1.e8, temp_intsnow);
        }
        /*====================================================================*/
        // for subgrid fluxes
        if (subgridflag == 1 && !Land.urbpoi) {
          if (frac_sno > 0.0) {
            snow_depth = snow_depth + newsnow / (bifall * frac_sno);
          } else {
            snow_depth = 0.0;
          }
        } else {
          // for uniform snow cover
          snow_depth = snow_depth + newsnow / bifall;
        }
        if (oldfflag == 1) {
          // snow cover fraction in Niu et al. 2007
          if (snow_depth > 0.0) {
            frac_sno = tanh(snow_depth / (2.5 * zlnd *
                                          pow((std::min(800.0, ((h2osno + newsnow) / snow_depth / 100.0))),
                                              1.0))); // why to the power of 1.0??
          }
          if (h2osno < 1.0) {
            frac_sno = std::min(frac_sno, h2osno);
          }
        }
      } else { // h2osno == 0
        // initialize frac_sno and snow_depth when no snow present initially
        if (newsnow > 0.0) {
          z_avg = newsnow / bifall;
          frac_sno = tanh(accum_factor * newsnow);
          // make int_snow consistent w/ new fsno, h2osno
          int_snow = 0.0; // reset prior to adding newsnow below
          temp_intsnow =
              (h2osno + newsnow) / (0.5 * (cos(ELM_PI * pow((1.0 - std::max(frac_sno, 1.e-6)), (1.0 / n_melt))) + 1.0));
          int_snow = std::min(1.e8, temp_intsnow);
          // update snow_depth and h2osno to be consistent with frac_sno, z_avg
          if (subgridflag == 1 && !Land.urbpoi) {
            snow_depth = z_avg / frac_sno;
          } else {
            snow_depth = newsnow / bifall;
          }
          // use n&y07 formulation
          if (oldfflag == 1) {
            // snow cover fraction in Niu et al. 2007
            if (snow_depth > 0.0) {
              frac_sno = tanh(snow_depth / (2.5 * zlnd *
                                            pow((std::min(800.0, ((h2osno + newsnow) / snow_depth / 100.0))),
                                                1.0))); // why to the power of 1.0??
            }
          }
        } else {
          z_avg = 0.0;
          snow_depth = 0.0;
          frac_sno = 0.0;
        }
      }

      qflx_snow_h2osfc = 0.0;    // no snow on surface water
      h2osno = h2osno + newsnow; // update h2osno for new snow
      int_snow = int_snow + newsnow;
      dz_snowf = (snow_depth - temp_snow_depth) / dtime; // update change in snow depth
    }                                                    // end else do_capsnow
    // set frac_sno_eff variable
    if (Land.ltype == istsoil || Land.ltype == istcrop) {
      if (subgridflag == 1) {
        frac_sno_eff = frac_sno;
      } else {
        frac_sno_eff = 1.0;
      }
    } else {
      frac_sno_eff = 1.0;
    }
    if (Land.ltype == istwet && t_grnd > tfrz) {
      h2osno = 0.0;
      snow_depth = 0.0;
    }
    // When the snow accumulation exceeds 10 mm, initialize snow layer
    // Currently, the water temperature for the precipitation is simply set
    // as the surface air temperature
    int newnode = 0; // flag for when snow node will be initialized
    if (snl == 0 && qflx_snow_grnd > 0.0 && (frac_sno * snow_depth) >= 0.01) {
      newnode = 1;
      snl = 1;
      dz[nlevsno - 1] = snow_depth; // meter
      z[nlevsno - 1] = -0.5 * dz[nlevsno - 1];
      zi[nlevsno - 1] = -dz[nlevsno - 1];
      t_soisno[nlevsno - 1] = std::min(tfrz, forc_t); // K
      h2osoi_ice[nlevsno - 1] = h2osno;               // kg/m2
      h2osoi_liq[nlevsno - 1] = 0.0;                  // kg/m2
      frac_iceold[nlevsno - 1] = 1.0;
      snw_rds[nlevsno - 1] = snw_rds_min;
    }
    // The change of ice partial density of surface node due to precipitation.
    // Only ice part of snowfall is added here, the liquid part will be added later.
    if (snl > 0 && newnode == 0) {
      h2osoi_ice[nlevsno - snl] = h2osoi_ice[nlevsno - snl] + newsnow;
      dz[nlevsno - snl] = dz[nlevsno - snl] + dz_snowf * dtime;
    }
  }
} // SnowInit

/* CanopyHydrology::FracH2OSfc()
DESCRIPTION:
Determine fraction of land surfaces which are submerged
based on surface microtopography and surface water storage.

INPUTS:
Land                         [LandType] struct containing information about landtype
micro_sigma                  [double] microtopography pdf sigma (m)
h2osno                       [double] snow water (mm H2O)
no_update                    [bool] flag to make calculation w/o updating variables

OUTPUTS:
h2osfc                       [double] surface water (mm)
h2osoi_liq[nlevgrnd+nlevsno] [double] ice lens (kg/m2)
frac_sno                     [double] fraction of ground covered by snow (0 to 1)
frac_sno_eff                 [double] effective fraction of ground covered by snow (0 to 1)
frac_h2osfc                  [double] fractional area with surface water greater than zero (0 to 1)
*/
template <class dArray_type>
void FracH2OSfc(const LandType &Land, const double &micro_sigma, const double &h2osno,

                double &h2osfc, dArray_type h2osoi_liq, double &frac_sno, double &frac_sno_eff, double &frac_h2osfc) {

  if (!Land.lakpoi) {
    double d, fd, dfdd, sigma;
    double min_h2osfc = 1.e-8; // arbitrary lower limit on h2osfc for safer numerics...
    // h2osfc only calculated for soil vegetated land units
    if (Land.ltype == istsoil || Land.ltype == istcrop) {
      // Use newton-raphson method to iteratively determine frac_h20sfc
      // based on amount of surface water storage (h2osfc) and
      // microtopography variability (micro_sigma)
      if (h2osfc > min_h2osfc) {
        // a cutoff is needed for numerical reasons...(nonconvergence after 5 iterations)
        d = 0.0;
        sigma = 1.0e3 * micro_sigma; // convert to mm
        for (int l = 0; l < 10; l++) {
          fd = 0.5 * d * (1.0 + erf(d / (sigma * sqrt(2.0)))) +
               sigma / sqrt(2.0 * ELM_PI) * exp(-pow(d, 2) / (2.0 * pow(sigma, 2))) - h2osfc;
          dfdd = 0.5 * (1.0 + erf(d / (sigma * sqrt(2.0))));
          d = d - fd / dfdd;
        }
        // update the submerged areal fraction using the new d value
        frac_h2osfc = 0.5 * (1.0 + erf(d / (sigma * sqrt(2.0))));
      } else {
        frac_h2osfc = 0.0;
        h2osoi_liq[nlevsno] = h2osoi_liq[nlevsno] + h2osfc;
        h2osfc = 0.0;
      }
      // adjust fh2o, fsno when sum is greater than zero
      if (frac_sno > (1.0 - frac_h2osfc) && h2osno > 0.0) {
        if (frac_h2osfc > 0.01) {
          frac_h2osfc = std::max((1.0 - frac_sno), 0.01);
          frac_sno = 1.0 - frac_h2osfc;
        } else {
          frac_sno = 1.0 - frac_h2osfc;
        }
        frac_sno_eff = frac_sno;
      }
    } else { // if landunit not istsoil/istcrop, set frac_h2osfc to zero
      frac_h2osfc = 0.0;
    }
  }
} // FracH2OSfc

} // namespace ELM