/* functions derived from SurfaceRadiationMod.F90

Call sequence:
CanopySunShadeFrac()
SurfRadZeroFluxes()
SurfRadAbsorbed()
SurfRadLayers()
SurfRadReflected
*/
#pragma once

#include "clm_constants.h"
#include "landtype.h"
#include <assert.h>
#include <cmath>

namespace ELM {

/* SurfaceRadiation::SurfRadZeroFluxes()
DESCRIPTION: zero out fluxes before surface radiation calculations

INPUTS:
Land                [LandType] struct containing information about landtype

OUTPUTS:
sabg_soil           [double] solar radiation absorbed by soil (W/m**2)
sabg_snow           [double] solar radiation absorbed by snow (W/m**2)
sabg                [double] solar radiation absorbed by ground (W/m**2)
sabv                [double] solar radiation absorbed by vegetation (W/m**2)
fsa                 [double] solar radiation absorbed (total) (W/m**2)
sabg_lyr[nlevsno+1] [double] absorbed radiative flux (pft,lyr) [W/m2]
*/
template <class dArray_type>
void SurfRadZeroFluxes(const LandType &Land, double &sabg_soil, double &sabg_snow, double &sabg, double &sabv,
                       double &fsa, dArray_type sabg_lyr) {

  // Initialize fluxes
  if (!Land.urbpoi) {
    sabg_soil = 0.0;
    sabg_snow = 0.0;
    sabg = 0.0;
    sabv = 0.0;
    fsa = 0.0;

    for (int j = 0; j < nlevsno + 1; j++) {
      sabg_lyr[j] = 0.0;
    }
  }
}

/* SurfaceRadiation::SurfRadAbsorbed()
DESCRIPTION: calculate solar flux absorbed by canopy, soil, snow, and ground

INPUTS:
Land               [LandType] struct containing information about landtype
snl                [int] number of snow layers
ftdd[numrad]       [double] down direct flux below canopy per unit direct flux
ftid[numrad]       [double] down diffuse flux below canopy per unit direct flux
ftii[numrad]       [double] down diffuse flux below canopy per unit diffuse flux
forc_solad[numrad] [double] direct beam radiation (W/m**2)
forc_solai[numrad] [double] diffuse radiation (W/m**2)
fabd[numrad]       [double] flux absorbed by canopy per unit direct flux
fabi[numrad]       [double] flux absorbed by canopy per unit diffuse flux
albsod[numrad]     [double] soil albedo: direct
albsoi[numrad]     [double] soil albedo: diffuse
albsnd_hst[numrad] [double] snow albedo, direct , for history files
albsni_hst[numrad] [double] snow albedo, diffuse, for history files
albgrd[numrad]     [double] ground albedo (direct)
albgri[numrad]     [double] ground albedo (diffuse)

OUTPUTS:
sabv               [double] solar radiation absorbed by vegetation (W/m**2)
fsa                [double] solar radiation absorbed (total) (W/m**2)
sabg               [double] solar radiation absorbed by ground (W/m**2)
sabg_soil          [double] solar radiation absorbed by soil (W/m**2)
sabg_snow          [double] solar radiation absorbed by snow (W/m**2))
trd[numrad]        [double] transmitted solar radiation: direct (W/m**2)
tri[numrad]        [double] transmitted solar radiation: diffuse (W/m**2)

*/
template <class dArray_type>
void SurfRadAbsorbed(const LandType &Land, const int &snl, const dArray_type ftdd, const dArray_type ftid,
                     const dArray_type ftii, const dArray_type forc_solad, const dArray_type forc_solai,
                     const dArray_type fabd, const dArray_type fabi, const dArray_type albsod, const dArray_type albsoi,
                     const dArray_type albsnd_hst, const dArray_type albsni_hst, const dArray_type albgrd,
                     const dArray_type albgri, double &sabv, double &fsa, double &sabg, double &sabg_soil,
                     double &sabg_snow, double *trd, double *tri) {

  double absrad, cad[numrad], cai[numrad];
  if (!Land.urbpoi) {
    for (int ib = 0; ib < numrad; ib++) {

      cad[ib] = forc_solad[ib] * fabd[ib];
      cai[ib] = forc_solai[ib] * fabi[ib];
      sabv += cad[ib] + cai[ib];
      fsa += cad[ib] + cai[ib];

      // Transmitted = solar fluxes incident on ground
      trd[ib] = forc_solad[ib] * ftdd[ib];
      tri[ib] = forc_solad[ib] * ftid[ib] + forc_solai[ib] * ftii[ib];
      // Solar radiation absorbed by ground surface
      // calculate absorbed solar by soil/snow separately
      absrad = trd[ib] * (1.0 - albsod[ib]) + tri[ib] * (1.0 - albsoi[ib]);
      sabg_soil += absrad;
      absrad = trd[ib] * (1.0 - albsnd_hst[ib]) + tri[ib] * (1.0 - albsni_hst[ib]);
      sabg_snow += absrad;
      absrad = trd[ib] * (1.0 - albgrd[ib]) + tri[ib] * (1.0 - albgri[ib]);
      sabg += absrad;
      fsa += absrad;

      if (snl == 0) {
        sabg_snow = sabg;
        sabg_soil = sabg;
      }

      if (subgridflag == 0) {
        sabg_snow = sabg;
        sabg_soil = sabg;
      }
    } // end of numrad
  }   // end if not urban
}

/* SurfaceRadiation::SurfRadLayers()
DESCRIPTION: compute absorbed flux in each snow layer and top soil layer

INPUTS:
Land                 [LandType] struct containing information about landtype
snl                  [int] number of snow layers
sabg                 [double] solar radiation absorbed by ground (W/m**2)
sabg_snow            [double] solar radiation absorbed by snow (W/m**2)
snow_depth           [double] snow height (m)
flx_absdv[nlevsno+1] [double] direct flux absorption factor: VIS
flx_absdn[nlevsno+1] [double] direct flux absorption factor: NIR
flx_absiv[nlevsno+1] [double] diffuse flux absorption factor: VIS
flx_absin[nlevsno+1] [double] diffuse flux absorption factor: NIR
trd[numrad]          [double] transmitted solar radiation: direct (W/m**2)
tri[numrad]          [double] transmitted solar radiation: diffuse (W/m**2)

OUTPUTS:
sabg_lyr[nlevsno+1]  [double] absorbed radiative flux (pft,lyr) [W/m2]
*/
template <class dArray_type>
void SurfRadLayers(const LandType &Land, const int &snl, const double &sabg, const double &sabg_snow,
                   const double &snow_depth, const dArray_type flx_absdv, const dArray_type flx_absdn,
                   const dArray_type flx_absiv, const dArray_type flx_absin, const double *trd, const double *tri,
                   dArray_type sabg_lyr) {

  double err_sum = 0.0;
  double sabg_snl_sum;
  // compute absorbed flux in each snow layer and top soil layer,
  // based on flux factors computed in the radiative transfer portion of SNICAR.
  if (!Land.urbpoi) {

    sabg_snl_sum = 0.0;

    // CASE1: No snow layers: all energy is absorbed in top soil layer
    if (snl == 0) {
      for (int i = 0; i <= nlevsno; i++) {
        sabg_lyr[i] = 0.0;
      }
      sabg_lyr[nlevsno] = sabg;
      sabg_snl_sum = sabg_lyr[nlevsno];
    } else { // CASE 2: Snow layers present: absorbed radiation is scaled according to flux factors computed by SNICAR

      for (int i = 0; i < nlevsno + 1; i++) {
        sabg_lyr[i] = flx_absdv[i] * trd[0] + flx_absdn[i] * trd[1] + flx_absiv[i] * tri[0] + flx_absin[i] * tri[1];
        // summed radiation in active snow layers:
        // if snow layer is at or below snow surface
        if (i >= nlevsno - snl) {
          sabg_snl_sum += sabg_lyr[i];
        }
      }

      // Error handling: The situation below can occur when solar radiation is
      // NOT computed every timestep.
      // When the number of snow layers has changed in between computations of the
      // absorbed solar energy in each layer, we must redistribute the absorbed energy
      // to avoid physically unrealistic conditions. The assumptions made below are
      // somewhat arbitrary, but this situation does not arise very frequently.
      // This error handling is implemented to accomodate any value of the
      // radiation frequency.
      // change condition to match sabg_snow isntead of sabg

      if (std::abs(sabg_snl_sum - sabg_snow) > 0.00001) {
        if (snl == 0) {
          for (int j = 0; j < nlevsno; j++) {
            sabg_lyr[j] = 0.0;
          }
          sabg_lyr[nlevsno] = sabg;
        } else if (snl == 1) {
          for (int j = 0; j < nlevsno - 1; j++) {
            sabg_lyr[j] = 0.0;
          }
          sabg_lyr[nlevsno - 1] = sabg_snow * 0.6;
          sabg_lyr[nlevsno] = sabg_snow * 0.4;
        } else {
          for (int j = 0; j <= nlevsno; j++) {
            sabg_lyr[j] = 0.0;
          }
          sabg_lyr[nlevsno - snl] = sabg_snow * 0.75;
          sabg_lyr[nlevsno - snl + 1] = sabg_snow * 0.25;
        }
      }

      // If shallow snow depth, all solar radiation absorbed in top or top two snow layers
      // to prevent unrealistic timestep soil warming

      if (subgridflag == 0) {
        if (snow_depth < 0.1) {
          if (snl == 0) {
            for (int j = 0; j < nlevsno; j++) {
              sabg_lyr[j] = 0.0;
            }
            sabg_lyr[nlevsno] = sabg;
          } else if (snl == 1) {
            for (int j = 0; j < nlevsno - 1; j++) {
              sabg_lyr[j] = 0.0;
            }
            sabg_lyr[nlevsno - 1] = sabg;
            sabg_lyr[nlevsno] = 0.0;
          } else {
            for (int j = 0; j <= nlevsno; j++) {
              sabg_lyr[j] = 0.0;
            }
            sabg_lyr[nlevsno - snl] = sabg_snow * 0.75;
            sabg_lyr[nlevsno - snl + 1] = sabg_snow * 0.25;
          }
        }
      }
    }

    // Error check - This situation should not happen:
    for (int j = 0; j <= nlevsno; j++) {
      err_sum += sabg_lyr[j];
    }
    assert(!(std::abs(err_sum - sabg_snow) > 0.00001));
  }
}

/* SurfaceRadiation::SurfRadReflected()
DESCRIPTION: calculate reflected solar radiation

INPUTS:
Land               [LandType] struct containing information about landtype
albd[numrad]       [double] surface albedo (direct)
albi[numrad]       [double] surface albedo (diffuse)
forc_solad[numrad] [double] direct beam radiation (W/m**2)
forc_solai[numrad] [double] diffuse radiation (W/m**2)

OUTPUTS:
fsr                [double] solar radiation reflected (W/m**2)
*/
template <class dArray_type>
void SurfRadReflected(const LandType &Land, const dArray_type albd, const dArray_type albi,
                      const dArray_type forc_solad, const dArray_type forc_solai, double &fsr) {

  double fsr_vis_d, fsr_nir_d, fsr_vis_i, fsr_nir_i, rvis, rnir;
  // Radiation diagnostics
  if (!Land.urbpoi) {
    // NDVI and reflected solar radiation
    rvis = albd[0] * forc_solad[0] + albi[0] * forc_solai[0];
    rnir = albd[1] * forc_solad[1] + albi[1] * forc_solai[1];
    fsr = rvis + rnir;
  } else {
    // Solar reflected per unit ground area (roof, road) and per unit wall area (sunwall, shadewall)
    fsr_vis_d = albd[0] * forc_solad[0];
    fsr_nir_d = albd[1] * forc_solad[1];
    fsr_vis_i = albi[0] * forc_solai[0];
    fsr_nir_i = albi[1] * forc_solai[1];

    fsr = fsr_vis_d + fsr_nir_d + fsr_vis_i + fsr_nir_i;
  }
}

/* SurfaceRadiation::CanopySunShadeFractions()
DESCRIPTION:
This subroutine calculates and returns:
1) absorbed PAR for sunlit leaves in canopy layer
2) absorbed PAR for shaded leaves in canopy layer
3) sunlit leaf area
4) shaded  leaf area
5) sunlit leaf area for canopy layer
6) shaded leaf area for canopy layer
7) sunlit fraction of canopy

INPUTS:
Land                [LandType] struct containing information about landtype
nrad                [int] number of canopy layers above snow for radiative transfer
elai                [double] one-sided leaf area index
tlai_z[nlevcan]     [double] tlai increment for canopy layer
fsun_z[nlevcan]     [double] sunlit fraction of canopy layer
forc_solad[numrad]  [double] direct beam radiation (W/m**2)
forc_solai[numrad]  [double] diffuse radiation (W/m**2))
fabd_sun_z[nlevcan] [double] absorbed sunlit leaf direct PAR
fabd_sha_z[nlevcan] [double] absorbed shaded leaf direct PAR
fabi_sun_z[nlevcan] [double] absorbed sunlit leaf diffuse PAR
fabi_sha_z[nlevcan] [double] absorbed shaded leaf diffuse PAR

OUTPUTS:
parsun_z[nlevcan]   [double] absorbed PAR for sunlit leaves
parsha_z[nlevcan]   [double] absorbed PAR for shaded leaves
laisun_z[nlevcan]   [double] sunlit leaf area for canopy layer
laisha_z[nlevcan]   [double] shaded leaf area for canopy layer
laisun              [double] sunlit leaf area
laisha              [double] shaded  leaf area
*/
template <class dArray_type>
void CanopySunShadeFractions(const LandType &Land, const int &nrad, const double &elai, const dArray_type tlai_z,
                             const dArray_type fsun_z, const dArray_type forc_solad, const dArray_type forc_solai,
                             const dArray_type fabd_sun_z, const dArray_type fabd_sha_z, const dArray_type fabi_sun_z,
                             const dArray_type fabi_sha_z, dArray_type parsun_z, dArray_type parsha_z,
                             dArray_type laisun_z, dArray_type laisha_z, double &laisun, double &laisha) {

  if (!Land.urbpoi) {
    int ipar = 0; // The band index for PAR
    for (int iv = 0; iv < nrad; iv++) {
      parsun_z[iv] = 0.0;
      parsha_z[iv] = 0.0;
      laisun_z[iv] = 0.0;
      laisha_z[iv] = 0.0;
    }
    // Loop over patches to calculate laisun_z and laisha_z for each layer.
    // Derive canopy laisun, laisha, from layer sums.
    // If sun/shade big leaf code, nrad=1 and fsun_z[0] and tlai_z[0] from
    // SurfaceAlbedo is canopy integrated so that layer value equals canopy value.
    laisun = 0.0;
    laisha = 0.0;

    for (int iv = 0; iv < nrad; iv++) {
      laisun_z[iv] = tlai_z[iv] * fsun_z[iv];
      laisha_z[iv] = tlai_z[iv] * (1.0 - fsun_z[iv]);
      laisun += laisun_z[iv];
      laisha += laisha_z[iv];
    }

    // Absorbed PAR profile through canopy
    // If sun/shade big leaf code, nrad=1 and fluxes from SurfaceAlbedo
    // are canopy integrated so that layer values equal big leaf values.
    for (int iv = 0; iv < nrad; iv++) {
      parsun_z[iv] = forc_solad[ipar] * fabd_sun_z[iv] + forc_solai[ipar] * fabi_sun_z[iv];
      parsha_z[iv] = forc_solad[ipar] * fabd_sha_z[iv] + forc_solai[ipar] * fabi_sha_z[iv];
    }
  }
}

} // namespace ELM