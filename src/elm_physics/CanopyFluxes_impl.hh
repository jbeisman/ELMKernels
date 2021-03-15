/* functions derived from CanopyFluxesMod.F90
DESCRIPTION:
Calculate leaf temperature, leaf fluxes, transpiration, photosynthesis and updates the dew accumulation due to
evaporation. Also calculates the irrigation rate.

The functions should be called in the following order:
InitializeFlux_Can()
StabilityIteration_Can()
ComputeFlux_Can()

Irrigation() can be called anytime after InitializeFlux()
*/
#include "FrictionVelocity.h"
#include "Photosynthesis.hh"
#include "QSat.h"
#include "SoilMoistStress.hh"
#include "clm_constants.h"
#include "landtype.h"
#include "vegproperties.h"
#include <algorithm>
#include <assert.h>
#include <cmath>

namespace ELM {

// */
// void Irrigation(
//  const LandType& Land,
//  const double elai,
//  const double *irrigated, // doesn't need to be in array form if 1 pft/cell -- fix
//
//  int& n_irrig_steps_left,
//  double& irrig_rate)
//{
//  if (!Land.lakpoi && !Land.urbpoi && frac_veg_nosno != 0) {
//    // Determines target soil moisture level for irrigation. If h2osoi_liq_so is the soil moisture level at
//    // which stomata are fully open and h2osoi_liq_sat is the soil moisture level at saturation (eff_porosity),
//    // then the target soil moisture level is (h2osoi_liq_so + irrig_factor*(h2osoi_liq_sat - h2osoi_liq_so)).
//    // A value of 0 means that the target soil moisture level is h2osoi_liq_so.
//    // A value of 1 means that the target soil moisture level is h2osoi_liq_sat
//    double irrig_factor = 0.7;
//
//    double irrig_min_lai = 0.0;  // Minimum LAI for irrigation
//    double irrig_btran_thresh = 0.999999; // Irrigate when btran falls below 0.999999 rather than 1 to allow for
//    round-off error int irrig_start_time = isecspday/4   // Time of day to check whether we need irrigation, seconds
//    (0 = midnight). int irrig_length = isecspday/6; // Desired amount of time to irrigate per day (sec). Actual time
//    may differ if this is not a multiple of dtime. Irrigation won't work properly if dtime > secsperday int
//    irrig_nsteps_per_day = ((irrig_length + (dtime - 1))/dtime);  // number of time steps per day in which we
//    irrigate
//
//    // Determine if irrigation is needed (over irrigated soil columns)
//    // First, determine in what grid cells we need to bother 'measuring' soil water, to see if we need irrigation
//    // Also set n_irrig_steps_left for these grid cells
//    // n_irrig_steps_left(p) > 0 is ok even if irrig_rate(p) ends up = 0
//    // in this case, we'll irrigate by 0 for the given number of time steps
//
//    // get_prev_date(yr, mon, day, time)  ! get time as of beginning of time step --- figure out!! -- need variable
//    'time' if (irrigated[Land.vtype] == 1.0 && elai > irrig_min_lai && btran < irrig_btran_thresh) {
//      //see if it's the right time of day to start irrigating:
//      int local_time = (((time + round(londeg/degpsec)) % isecspday) + isecspday) % isecspday;
//      int seconds_since_irrig_start_time = (((local_time - irrig_start_time) % isecspday) + isecspday) % isecspday;
//      if (seconds_since_irrig_start_time < dtime) { // it's time to start irrigating
//        bool check_for_irrig = true;
//        n_irrig_steps_left = irrig_nsteps_per_day;
//        irrig_rate = 0.0  // reset; we'll add to this later
//      } else {
//        bool check_for_irrig = false;
//      }
//    } else { // non-irrig pft or elai<=irrig_min_lai or btran>irrig_btran_thresh
//      bool check_for_irrig = false;
//    }
//
//    // Now 'measure' soil water for the grid cells identified above and see if the
//    // soil is dry enough to warrant irrigation
//    // (Note: frozen_soil could probably be a column-level variable, but that would be
//    // slightly less robust to potential future modifications)
//    // This should not be operating on FATES patches (see is_fates filter above, pushes
//    // check_for_irrig = false
//    bool frozen_soil = false;
//    if (check_for_irrig && !frozen_soil) {
//      for (int i = 0; i < nlevgrnd; i++) {
//        // if level i was frozen, then we don't look at any levels below L
//        if (t_soisno[nlevsno+i] <= tfrz) {
//          frozen_soil = true;
//        } else if (rootfr > 0.0) {
//          // determine soil water deficit in this layer:
//          // Calculate vol_liq_so - i.e., vol_liq at which smp_node = smpso - by inverting the above equations
//          // for the root resistance factors
//          vol_liq_so = eff_porosity[i] * pow((-smpso[Land.vtype]/sucsat[i]), (-1.0/bsw[i]));
//          // Translate vol_liq_so and eff_porosity into h2osoi_liq_so and h2osoi_liq_sat and calculate deficit
//          h2osoi_liq_so = vol_liq_so * denh2o * dz[i];
//          h2osoi_liq_sat = eff_porosity[i] * denh2o * dz[i];
//          deficit = std::max((h2osoi_liq_so + irrig_factor * (h2osoi_liq_sat - h2osoi_liq_so)) -
//          h2osoi_liq[nlevsno+i], 0.0);
//          // Add deficit to irrig_rate, converting units from mm to mm/sec
//          irrig_rate += deficit / (dtime*irrig_nsteps_per_day);
//        }
//      }
//    }
//  } // if (!Land.lakpoi && !Land.urbpoi && frac_veg_nosno != 0)
//} // Irrigation

/* InitializeFlux_Can()
INPUTS:
Land                            [LandType] struct containing information about landtype
snl                             [int] number of snow layers
frac_veg_nosno                  [int]  fraction of vegetation not covered by snow (0 OR 1) [-]
frac_sno                        [double] fraction of ground covered by snow (0 to 1)
forc_hgt_u_patch                [double] observational height of wind at pft level [m]
thm                             [double] intermediate variable (forc_t+0.0098*forc_hgt_t_patch)
thv                             [double] virtual potential temperature (kelvin)
max_dayl                        [double] maximum daylength for this grid cell (s)
dayl                            [double] daylength (seconds)
altmax_indx                     [int] index corresponding to maximum active layer depth from current year
altmax_lastyear_indx            [int] index corresponding to maximum active layer depth from prior year
t_soisno[nlevgrnd+nlevsno]      [double] col soil temperature (Kelvin)
h2osoi_ice[nlevgrnd+nlevsno]    [double] ice lens (kg/m2)
h2osoi_liq[nlevgrnd+nlevsno]    [double] liquid water (kg/m2)
dz[nlevgrnd+nlevsno]            [double] layer thickness (m)
rootfr[nlevgrnd]                [double] fraction of roots in each soil layer
tc_stress                       [double] critical soil temperature for soil water stress (C)
sucsat[nlevgrnd]                [double] minimum soil suction (mm)
watsat[nlevgrnd]                [double] volumetric soil water at saturation (porosity)
bsw[nlevgrnd]                   [double] Clapp and Hornberger "b
smpso[numpft]                   [double] soil water potential at full stomatal opening (mm)
smpsc[numpft]                   [double] soil water potential at full stomatal closure (mm)
elai                            [double] one-sided leaf area index with burying by snow
esai                            [double] one-sided stem area index with burying by snow
emv                             [double] vegetation emissivity
emg                             [double] ground emissivity
qg                              [double] ground specific humidity [kg/kg]
t_grnd                          [double] ground temperature (Kelvin)
forc_t                          [double] atmospheric temperature (Kelvin)
forc_pbot                       [double] atmospheric pressure (Pa)
forc_lwrad                      [double] downward infrared (longwave) radiation (W/m**2)
forc_u                          [double] atmospheric wind speed in east direction (m/s)
forc_v                          [double] atmospheric wind speed in north direction (m/s)
forc_q                          [double] atmospheric specific humidity (kg/kg)
forc_th                         [double] atmospheric potential temperature (Kelvin)
z0mg                            [double] roughness length over ground, momentum [m]

OUTPUTS:
btran                           [double]  transpiration wetness factor (0 to 1)
z0mv                            [double] roughness length over vegetation, momentum [m]
z0hv                            [double] roughness length over vegetation, sensible heat [m]
z0qv                            [double] roughness length over vegetation, latent heat [m]
displa                          [double]  displacement height (m)
rootr[nlevgrnd]                 [double] effective fraction of roots in each soil layer
eff_porosity[nlevgrnd]          [double] effective soil porosity
dayl_factor                     [double] scalar (0-1) for daylength effect on Vcmax
air                             [double] atmos. radiation temporay set
bir                             [double] atmos. radiation temporay set
cir                             [double] atmos. radiation temporay set
el                              [double] vapor pressure on leaf surface [pa]
qsatl                           [double] leaf specific humidity [kg/kg]
qsatldT                         [double] derivative of "qsatl" on "t_veg"
taf                             [double] air temperature within canopy space [K]
qaf                             [double] humidity of canopy air [kg/kg]
um                              [double] wind speed including the stablity effect [m/s]
ur                              [double] wind speed at reference height [m/s]
obu                             [double] Monin-Obukhov length (m)
zldis                           [double] reference height "minus" zero displacement height [m]
delq                            [double] temporary
t_veg                           [double]  vegetation temperature (Kelvin)
*/
template <class dArray_type>
void InitializeFlux_Can(const LandType &Land, const int &snl, const int &frac_veg_nosno, const double &frac_sno,
                        const double &forc_hgt_u_patch, const double &thm, const double &thv, const double &max_dayl,
                        const double &dayl, const int &altmax_indx, const int &altmax_lastyear_indx,
                        const dArray_type t_soisno, const dArray_type h2osoi_ice, const dArray_type h2osoi_liq,
                        const dArray_type dz, const dArray_type rootfr, const double &tc_stress,
                        const dArray_type sucsat, const dArray_type watsat, const dArray_type bsw,
                        const dArray_type smpso, const dArray_type smpsc, const double &elai, const double &esai,
                        const double &emv, const double &emg, const double &qg, const double &t_grnd,
                        const double &forc_t, const double &forc_pbot, const double &forc_lwrad, const double &forc_u,
                        const double &forc_v, const double &forc_q, const double &forc_th, const double &z0mg,
                        double &btran, double &displa, double &z0mv, double &z0hv, double &z0qv, dArray_type rootr,
                        dArray_type eff_porosity, double &dayl_factor, double &air, double &bir, double &cir,
                        double &el, double &qsatl, double &qsatldT, double &taf, double &qaf, double &um, double &ur,
                        double &obu, double &zldis, double &delq, double &t_veg) {
  // -----------------------------------------------------------------
  // Time step initialization of photosynthesis variables
  // -----------------------------------------------------------------
  // call photosyns_vars%TimeStepInit(bounds)
  // need to decide where to place photosyns_vars%TimeStepInit()

  const double btran0 = 0.0;

  if (!Land.lakpoi && !Land.urbpoi) {
    double lt; // elai+esai
    if (frac_veg_nosno == 0) {
      btran = 0.0;
      t_veg = forc_t;
      double cf_bare = forc_pbot / (ELM_RGAS * 0.001 * thm) * 1.e06; // heat transfer coefficient from bare ground [-]
      double rssun = 1.0 / 1.e15 * cf_bare;
      double rssha = 1.0 / 1.e15 * cf_bare;
      // lbl_rsc_h2o = 0.0;
      for (int i = 0; i < nlevgrnd; i++) {
        rootr[i] = 0.0;
        // rresis[i] = 0.0;
      }
    } else {

      btran = btran0;

      // calculate dayl_factor as the ratio of (current:max dayl)^2
      // set a minimum of 0.01 (1%) for the dayl_factor
      dayl_factor = std::min(1.0, std::max(0.01, (dayl * dayl) / (max_dayl * max_dayl)));

      // compute effective soil porosity
      calc_effective_soilporosity(watsat, h2osoi_ice, dz, eff_porosity);
      // compute volumetric liquid water content
      double h2osoi_liqvol[nlevgrnd + nlevsno];
      calc_volumetric_h2oliq(eff_porosity, h2osoi_liq, dz, h2osoi_liqvol);
      // calculate root moisture stress
      calc_root_moist_stress(Land.vtype, h2osoi_liqvol, rootfr, t_soisno, tc_stress, sucsat, watsat, bsw, smpso, smpsc,
                             eff_porosity, altmax_indx, altmax_lastyear_indx, rootr, btran);

      // Modify aerodynamic parameters for sparse/dense canopy (X. Zeng)
      lt = std::min(elai + esai, tlsai_crit);
      double egvf = (1.0 - exp(-lt)) / (1.0 - exp(-tlsai_crit));
      displa *= egvf;
      z0mv = exp(egvf * std::log(z0mv) + (1.0 - egvf) * std::log(z0mg));
      z0hv = z0mv;
      z0qv = z0mv;

      // Net absorbed longwave radiation by canopy and ground
      air = emv * (1.0 + (1.0 - emv) * (1.0 - emg)) * forc_lwrad;
      bir = -(2.0 - emv * (1.0 - emg)) * emv * sb;
      cir = emv * emg * sb;

      // Saturated vapor pressure, specific humidity, and their derivatives at the leaf surface
      double deldT; // derivative of "el" on "t_veg" [pa/K]
      QSat(t_veg, forc_pbot, el, deldT, qsatl, qsatldT);

      // Initialize flux profile
      taf = (t_grnd + thm) / 2.0;
      qaf = (forc_q + qg) / 2.0;
      ur = std::max(1.0, std::sqrt(forc_u * forc_u + forc_v * forc_v));
      double dth = thm - taf;
      double dqh = forc_q - qaf;
      delq = qg - qaf;
      double dthv = dth * (1.0 + 0.61 * forc_q) + 0.61 * forc_th * dqh;
      zldis = forc_hgt_u_patch - displa;

      // Check to see if the forcing height is below the canopy height
      assert(zldis >= 0.0);
      // Initialize Monin-Obukhov length and wind speed
      MoninObukIni(ur, thv, dthv, zldis, z0mv, um, obu);
    }
  }
} // InitializeFlux_Can()

/* StabilityIteration_Can()
DESCRIPTION:
calculate Monin-Obukhov length and wind speed, call photosynthesis, calculate ET & SH flux
Iterates until convergence, up to 40 iterations, calling friction velocity functions, then
photosynthesis for sun & shade.

INPUTS:
Land                       [LandType] struct containing information about landtype
dtime                      [double] timestep size (sec)
snl                        [int] number of snow layers
frac_veg_nosno             [int]  fraction of vegetation not covered by snow (0 OR 1) [-]
frac_sno                   [double] fraction of ground covered by snow (0 to 1)
forc_hgt_u_patch           [double] observational height of wind at pft level [m]
forc_hgt_t_patch           [double] observational height of temperature at pft level [m]
forc_hgt_q_patch           [double] observational height of specific humidity at pft level [m]
dleaf[numpft]              [double] characteristic leaf dimension (m)
fwet                       [double] fraction of canopy that is wet (0 to 1)
fdry                       [double] fraction of foliage that is green and dry [-]
laisun                     [double] sunlit leaf area
laisha                     [double] shaded leaf area
forc_rho                   [double] air density (kg/m**3)
snow_depth                 [double] snow height (m)
soilbeta                   [double] soil wetness relative to field capacity
frac_h2osfc                [double] fraction of ground covered by surface water (0 to 1)
t_h2osfc                   [double] surface water temperature
sabv                       [double] solar radiation absorbed by vegetation (W/m**2)
h2ocan                     [double] canopy water (mm H2O)
htop                       [double] canopy top(m)
t_soisno[nlevgrnd+nlevsno] [double] col soil temperature (Kelvin)
air                        [double] atmos. radiation temporay set
bir                        [double] atmos. radiation temporay set
cir                        [double] atmos. radiation temporay set
ur                         [double] wind speed at reference height [m/s]
zldis                      [double] reference height "minus" zero displacement height [m]
displa                     [double]  displacement height (m)
elai                       [double] one-sided leaf area index with burying by snow
esai                       [double] one-sided stem area index with burying by snow
t_grnd                     [double] ground temperature (Kelvin)
forc_pbot                  [double]  atmospheric pressure (Pa)
forc_q                     [double] atmospheric specific humidity (kg/kg)
forc_th                    [double] atmospheric potential temperature (Kelvin)
z0mg                       [double] roughness length over ground, momentum [m]
z0mv                       [double] roughness length over vegetation, momentum [m]
z0hv                       [double] roughness length over vegetation, sensible heat [m]
z0qv                       [double] roughness length over vegetation, latent heat [m]
thm                        [double]  intermediate variable (forc_t+0.0098*forc_hgt_t_patch)
thv                        [double] virtual potential temperature (kelvin)
qg                         [double] ground specific humidity [kg/kg]
veg                        [VegProperties] struct containing vegetation constant parameters
nrad                       [int]  number of canopy layers above snow for radiative transfer
t10                        [double] 10-day running mean of the 2 m temperature (K)
tlai_z[nlevcan]            [double] pft total leaf area index for canopy layer
vcmaxcintsha               [double] leaf to canopy scaling coefficient - shade
vcmaxcintsun               [double] leaf to canopy scaling coefficient - sun
parsha_z[nlevcan]          [double] par absorbed per unit lai for canopy layer (w/m**2) - shade
parsun_z[nlevcan]          [double] par absorbed per unit lai for canopy layer (w/m**2) - sun
laisha_z[nlevcan]          [double] leaf area index for canopy layer, sunlit or shaded - shade
laisun_z[nlevcan]          [double] leaf area index for canopy layer, sunlit or shaded - sun
forc_pco2                  [double]  partial pressure co2 (Pa)
forc_po2                   [double]  partial pressure o2 (Pa))
dayl_factor                [double] scalar (0-1) for daylength effect on Vcmax

OUTPUTS:
btran                      [double]  transpiration wetness factor (0 to 1)
qflx_tran_veg              [double] vegetation transpiration (mm H2O/s) (+ = to atm)
qflx_evap_veg              [double] vegetation evaporation (mm H2O/s) (+ = to atm)
eflx_sh_veg                [double] sensible heat flux from leaves (W/m**2) [+ to atm]
wtg                        [double] heat conductance for ground [m/s]
wtl0                       [double] normalized heat conductance for leaf [-]
wta0                       [double] normalized heat conductance for air [-]
wtal                       [double] normalized heat conductance for air and leaf [-]
el                         [double] vapor pressure on leaf surface [pa]
qsatl                      [double] leaf specific humidity [kg/kg]
qsatldT                    [double] derivative of "qsatl" on "t_veg"
taf                        [double] air temperature within canopy space [K]
qaf                        [double] humidity of canopy air [kg/kg]
um                         [double] wind speed including the stablity effect [m/s]
dth                        [double] diff of virtual temp. between ref. height and surface
dqh                        [double] diff of humidity between ref. height and surface
obu                        [double] Monin-Obukhov length (m)
temp1                      [double] relation for potential temperature profile
temp2                      [double] relation for specific humidity profile
temp12m                    [double] relation for potential temperature profile applied at 2-m
temp22m                    [double] relation for specific humidity profile applied at 2-m
tlbef                      [double] leaf temperature from previous iteration [K]
delq                       [double] temporary
dt_veg                     [double] change in t_veg, last iteration (Kelvin)
t_veg                      [double]  vegetation temperature (Kelvin)
wtgq                       [double] latent heat conductance for ground [m/s]
wtalq                      [double] normalized latent heat cond. for air and leaf [-]
wtlq0                      [double] normalized latent heat conductance for leaf [-]
wtaq0                      [double] normalized latent heat conductance for air [-]
*/
template <class dArray_type>
void StabilityIteration_Can(const LandType &Land, const double &dtime, const int &snl, const int &frac_veg_nosno,
                            const double &frac_sno, const double &forc_hgt_u_patch, const double &forc_hgt_t_patch,
                            const double &forc_hgt_q_patch, const dArray_type dleaf, const double &fwet,
                            const double &fdry, const double &laisun, const double &laisha, const double &forc_rho,
                            const double &snow_depth, const double &soilbeta, const double &frac_h2osfc,
                            const double &t_h2osfc, const double &sabv, const double &h2ocan, const double &htop,
                            const dArray_type t_soisno, const double &air, const double &bir, const double &cir,
                            const double &ur, const double &zldis, const double &displa, const double &elai,
                            const double &esai, const double &t_grnd, const double &forc_pbot, const double &forc_q,
                            const double &forc_th, const double &z0mg, const double &z0mv, const double &z0hv,
                            const double &z0qv, const double &thm, const double &thv, const double &qg,
                            const VegProperties &veg, const int &nrad, const double &t10, const dArray_type tlai_z,
                            const double &vcmaxcintsha, const double &vcmaxcintsun, const dArray_type parsha_z,
                            const dArray_type parsun_z, const dArray_type laisha_z, const dArray_type laisun_z,
                            const double &forc_pco2, const double &forc_po2, const double &dayl_factor, double &btran,
                            double &qflx_tran_veg, double &qflx_evap_veg, double &eflx_sh_veg, double &wtg,
                            double &wtl0, double &wta0, double &wtal, double &el, double &qsatl, double &qsatldT,
                            double &taf, double &qaf, double &um, double &dth, double &dqh, double &obu, double &temp1,
                            double &temp2, double &temp12m, double &temp22m, double &tlbef, double &delq,
                            double &dt_veg, double &t_veg, double &wtgq, double &wtalq, double &wtlq0, double &wtaq0) {

  if (!Land.lakpoi && !Land.urbpoi && frac_veg_nosno != 0) {
    bool stop = false;
    int itmax = 40; // maximum number of iteration [-]
    int itmin = 2;  // minimum number of iteration [-]
    int itlef = 0;
    int nmozsgn = 0;       // number of times stability changes sign
    double del = 0.0;      // absolute change in leaf temp in current iteration [K]
    double csoilc = 0.004; // Drag coefficient for soil under canopy [-]
    double efeb = 0.0;     // latent heat flux from leaf (previous iter) [mm/s]
    double obuold = 0.0;   // monin-obukhov length from previous iteration
    double ustar, del2, uaf, cf, rb, ram, rah[2], raw[2];
    double csoilcn, csoilb, ri, ricsoilc, w, svpts, eah;
    double wta, wtl, wtshi, wtg0, wtga, wtsqi, wtgq0, wtgaq;
    double snow_depth_c, fsno_dl, elai_dl, rdl, rppdry, efpot, rpp;
    double dc1, dc2, efsh, erre, err, efe, efeold, lw_grnd, dels, ecidif;
    double tstar, qstar, thvstar, wc, zeta, wtaq, wtlq, dele, det, deldT;
    double rssun, rssha;
    const double btran0 = 0.0;

    while (itlef <= itmax && !stop) {
      // Determine friction velocity, and potential temperature and humidity profiles of the surface boundary layer
      FrictionVelocityWind(forc_hgt_u_patch, displa, um, obu, z0mv, ustar);
      FrictionVelocityTemperature(forc_hgt_t_patch, displa, obu, z0hv, temp1);
      FrictionVelocityHumidity(forc_hgt_q_patch, forc_hgt_t_patch, displa, obu, z0hv, z0qv, temp1, temp2);
      FrictionVelocityTemperature2m(obu, z0hv, temp12m);
      FrictionVelocityHumidity2m(obu, z0hv, z0qv, temp12m, temp22m);

      // save leaf temp and leaf temp delta from previous iteration
      tlbef = t_veg;
      del2 = del;
      // Determine aerodynamic resistances
      ram = 1.0 / (ustar * ustar / um);
      rah[0] = 1.0 / (temp1 * ustar);
      raw[0] = 1.0 / (temp2 * ustar);
      // Bulk boundary layer resistance of leaves
      uaf = um * std::sqrt(1.0 / (ram * um));
      // Use pft parameter for leaf characteristic width dleaf
      cf = 0.01 / (std::sqrt(uaf) * std::sqrt(dleaf[Land.vtype]));
      rb = 1.0 / (cf * uaf);

      // Parameterization for variation of csoilc with canopy density from X. Zeng, University of Arizona
      w = exp(-(elai + esai));
      // changed by K.Sakaguchi from here
      // transfer coefficient over bare soil is changed to a local variable
      // just for readability of the code (from line 680)
      csoilb = (vkc / (0.13 * pow((z0mg * uaf / 1.5e-5), 0.45)));
      // compute the stability parameter for ricsoilc  ("S" in Sakaguchi&Zeng,2008)
      ri = (grav * htop * (taf - t_grnd)) / pow((taf * uaf), 2.0);

      // modify csoilc value (0.004) if the under-canopy is in stable condition
      if ((taf - t_grnd) > 0.0) {
        // decrease the value of csoilc by dividing it with (1+gamma*min(S, 10.0))
        // ria ("gmanna" in Sakaguchi&Zeng, 2008) is a constant (=0.5)
        ricsoilc = csoilc / (1.0 + 0.5 * std::min(ri, 10.0));
        csoilcn = csoilb * w + ricsoilc * (1.0 - w);
      } else {
        csoilcn = csoilb * w + csoilc * (1.0 - w);
      }

      // Sakaguchi changes for stability formulation ends here
      rah[1] = 1.0 / (csoilcn * uaf);
      raw[1] = rah[1];
      // Stomatal resistances for sunlit and shaded fractions of canopy.
      // Done each iteration to account for differences in eah, tv.
      svpts = el;                    // pa
      eah = forc_pbot * qaf / 0.622; // pa

      if (Land.vtype == nsoybean || Land.vtype == nsoybeanirrig) {
        btran = std::min(1.0, btran * 1.25);
      }

      // call photosynthesis (phase=sun)
      Photosynthesis(veg, Land.vtype, nrad, forc_pbot, t_veg, t10, svpts, eah, forc_po2, forc_pco2, rb, btran,
                     dayl_factor, thm, tlai_z, vcmaxcintsun, parsun_z, laisun_z, rssun);

      if (Land.vtype == nsoybean || Land.vtype == nsoybeanirrig) {
        btran = std::min(1.0, btran * 1.25);
      }

      // call photosynthesis (phase=shade)
      Photosynthesis(veg, Land.vtype, nrad, forc_pbot, t_veg, t10, svpts, eah, forc_po2, forc_pco2, rb, btran,
                     dayl_factor, thm, tlai_z, vcmaxcintsha, parsha_z, laisha_z, rssha);

      // Sensible heat conductance for air, leaf and ground
      wta = 1.0 / rah[0];       // air
      wtl = (elai + esai) / rb; // leaf
      wtg = 1.0 / rah[1];       // ground
      wtshi = 1.0 / (wta + wtl + wtg);
      wtl0 = wtl * wtshi; // leaf
      wtg0 = wtg * wtshi; // ground
      wta0 = wta * wtshi; // air
      wtga = wta0 + wtg0; // ground + air
      wtal = wta0 + wtl0; // air + leaf

      // Fraction of potential evaporation from leaf
      if (fdry > 0.0) {
        rppdry = fdry * rb * (laisun / (rb + rssun) + laisha / (rb + rssha)) / elai;
      } else {
        rppdry = 0.0;
      }

      efpot = forc_rho * wtl * (qsatl - qaf);
      if (efpot > 0.0) {
        if (btran > btran0) {
          qflx_tran_veg = efpot * rppdry;
          rpp = rppdry + fwet;
        } else {
          // No transpiration if btran below 1.e-10
          rpp = fwet;
          qflx_tran_veg = 0.0;
        }
        // Check total evapotranspiration from leaves
        rpp = std::min(rpp, (qflx_tran_veg + h2ocan / dtime) / efpot);
      } else {
        // No transpiration if potential evaporation less than zero
        rpp = 1.0;
        qflx_tran_veg = 0.0;
      }

      // Update conductances for changes in rpp
      // Latent heat conductances for ground and leaf.
      // Air has same conductance for both sensible and latent heat.
      wtaq = frac_veg_nosno / raw[0];                   // air
      wtlq = frac_veg_nosno * (elai + esai) / rb * rpp; // leaf
      // Litter layer resistance. Added by K.Sakaguchi
      snow_depth_c = 0.05; // critical depth for 100% litter burial by snow (=litter thickness) -- litter thickness
                           // hardwired as 0.05 m
      fsno_dl = snow_depth / snow_depth_c; // effective snow cover for (dry)plant litter
      elai_dl = 0.5 * (1.0 - std::min(fsno_dl,
                                      1.0)); // exposed (dry)litter area index -- liter area hardwired as 0.5 m^2 m^-2
      rdl = (1.0 - exp(-elai_dl)) / (0.004 * uaf); // dry litter layer resistance
      // add litter resistance and Lee and Pielke 1992 beta
      if (delq < 0.0) { // dew. Do not apply beta for negative flux (follow old rsoil)
        wtgq = frac_veg_nosno / (raw[1] + rdl);
      } else {
        wtgq = soilbeta * frac_veg_nosno / (raw[1] + rdl);
      }

      wtsqi = 1.0 / (wtaq + wtlq + wtgq);
      wtgq0 = wtgq * wtsqi;  // ground
      wtlq0 = wtlq * wtsqi;  // leaf
      wtaq0 = wtaq * wtsqi;  // air
      wtgaq = wtaq0 + wtgq0; // air + ground
      wtalq = wtaq0 + wtlq0; // air + leaf
      dc1 = forc_rho * cpair * wtl;
      dc2 = hvap * forc_rho * wtlq;
      efsh = dc1 * (wtga * t_veg - wtg0 * t_grnd - wta0 * thm);
      efe = dc2 * (wtgaq * qsatl - wtgq0 * qg - wtaq0 * forc_q);

      // Evaporation flux from foliage
      erre = 0.0;
      if (efe * efeb < 0.0) {
        efeold = efe;
        efe = 0.1 * efeold;
        erre = efe - efeold;
      }

      // fractionate ground emitted longwave
      lw_grnd = (frac_sno * pow(t_soisno[nlevsno - snl], 4.0) +
                 (1.0 - frac_sno - frac_h2osfc) * pow(t_soisno[nlevsno], 4.0) + frac_h2osfc * pow(t_h2osfc, 4.0));
      dt_veg = (sabv + air + bir * pow(t_veg, 4.0) + cir * lw_grnd - efsh - efe) /
               (-4.0 * bir * pow(t_veg, 3.0) + dc1 * wtga + dc2 * wtgaq * qsatldT);
      t_veg = tlbef + dt_veg;
      dels = dt_veg;
      del = std::abs(dels);
      err = 0.0;
      if (del > 1.0) { // maxchange in  leaf temperature [K] --  hardwired as 1.0
        dt_veg = dels / del;
        t_veg = tlbef + dt_veg;
        err = sabv + air + bir * pow(tlbef, 3.0) * (tlbef + 4.0 * dt_veg) + cir * lw_grnd -
              (efsh + dc1 * wtga * dt_veg) - (efe + dc2 * wtgaq * qsatldT * dt_veg);
      }

      // Fluxes from leaves to canopy space
      // "efe" was limited as its sign changes frequently.  This limit may
      // result in an imbalance in "hvap*qflx_evap_veg" and
      // "efe + dc2*wtgaq*qsatdt_veg"
      efpot = forc_rho * wtl * (wtgaq * (qsatl + qsatldT * dt_veg) - wtgq0 * qg - wtaq0 * forc_q);
      qflx_evap_veg = rpp * efpot;
      // Calculation of evaporative potentials (efpot) and interception losses; flux in kg m**-2 s-1.
      // ecidif holds the excess energy if all intercepted water is evaporated during the timestep.
      // This energy is later added to the sensible heat flux.
      ecidif = 0.0;
      if (efpot > 0.0 && btran > btran0) {
        qflx_tran_veg = efpot * rppdry;
      } else {
        qflx_tran_veg = 0.0;
      }
      ecidif = std::max(0.0, qflx_evap_veg - qflx_tran_veg - h2ocan / dtime);
      qflx_evap_veg = std::min(qflx_evap_veg, qflx_tran_veg + h2ocan / dtime);
      qflx_evap_veg = qflx_evap_veg;
      qflx_tran_veg = qflx_tran_veg;
      // The energy loss due to above two limits is added to the sensible heat flux.
      eflx_sh_veg = efsh + dc1 * wtga * dt_veg + err + erre + hvap * ecidif;
      eflx_sh_veg = eflx_sh_veg;
      // Re-calculate saturated vapor pressure, specific humidity, and their derivatives at the leaf surface
      QSat(t_veg, forc_pbot, el, deldT, qsatl, qsatldT);

      // Update vegetation/ground surface temperature, canopy air
      // temperature, canopy vapor pressure, aerodynamic temperature, and
      // Monin-Obukhov stability parameter for next iteration.
      taf = wtg0 * t_grnd + wta0 * thm + wtl0 * t_veg;
      qaf = wtlq0 * qsatl + wtgq0 * qg + forc_q * wtaq0;
      // Update Monin-Obukhov length and wind speed including the stability effect
      dth = thm - taf;
      dqh = forc_q - qaf;
      delq = wtalq * qg - wtlq0 * qsatl - wtaq0 * forc_q;
      tstar = temp1 * dth;
      qstar = temp2 * dqh;
      thvstar = tstar * (1.0 + 0.61 * forc_q) + 0.61 * forc_th * qstar;
      zeta = zldis * vkc * grav * thvstar / (pow(ustar, 2.0) * thv);
      if (zeta >= 0.0) { // stable
        zeta = std::min(2.0, std::max(zeta, 0.01));
        um = std::max(ur, 0.1);
      } else {
        zeta = std::max(-100.0, std::min(zeta, -0.01));
        wc = pow((-grav * ustar * thvstar * 1000.0 / thv),
                 0.333); // convective boundary layer height hardwired as 1000.0
        um = std::sqrt(ur * ur + wc * wc);
      }
      obu = zldis / zeta;
      if (obuold * obu < 0.0) {
        nmozsgn += 1;
      }
      if (nmozsgn >= 4) {
        obu = zldis / (-0.01);
      }
      obuold = obu;

      // Test for convergence
      itlef += 1;
      if (itlef > itmin) {
        dele = std::abs(efe - efeb);
        efeb = efe;
        det = std::max(del, del2);
        if ((det < 0.01) && (dele < 0.1)) {
          stop = true;
        }
      }
    } // stability iteration
  }   // land type
} // StabilityIteration_Can()

/* ComputeFlux_Can()
INPUTS:
Land                       [LandType] struct containing information about landtype
dtime                      [double] timestep size (sec)
snl                        [int] number of snow layers
frac_veg_nosno             [int]  fraction of vegetation not covered by snow (0 OR 1) [-]
frac_sno                   [double] fraction of ground covered by snow (0 to 1)
t_soisno[nlevgrnd+nlevsno] [double] col soil temperature (Kelvin)
frac_h2osfc                [double] fraction of ground covered by surface water (0 to 1)
t_h2osfc                   [double] surface water temperature
sabv                       [double] solar radiation absorbed by vegetation (W/m**2)
qg_snow                    [double] specific humidity at snow surface [kg/kg]
qg_soil                    [double] specific humidity at soil surface [kg/kg]
qg_h2osfc                  [double] specific humidity at h2osfc [kg/kg]
dqgdT                      [double] d(qg)/dT
htvp                       [double] latent heat of vapor of water (or sublimation) [j/kg]
wtg                        [double] heat conductance for ground [m/s]
wtl0                       [double]  normalized heat conductance for leaf [-]
wta0                       [double] normalized heat conductance for air [-]
wtal                       [double] normalized heat conductance for air and leaf [-]
air                        [double] atmos. radiation temporay set
bir                        [double] atmos. radiation temporay set
cir                        [double] atmos. radiation temporay set
qsatl                      [double] leaf specific humidity [kg/kg]
qsatldT                    [double] derivative of "qsatl" on "t_ve
dth                        [double] diff of virtual temp. between ref. height and surface
dqh                        [double] diff of humidity between ref. height and surface
temp1                      [double] relation for potential temperature profile
temp2                      [double] relation for specific humidity profile
temp12m                    [double] relation for potential temperature profile applied at 2-m
temp22m                    [double] relation for specific humidity profile applied at 2-m
tlbef                      [double] leaf temperature from previous iteration [K]
delq                       [double] temporary
dt_veg                     [double] change in t_veg, last iteration (Kelvin)
t_veg                      [double]  vegetation temperature (Kelvin)
t_grnd                     [double] ground temperature (Kelvin)
forc_pbot                  [double]  atmospheric pressure (Pa)
qflx_tran_veg              [double] vegetation transpiration (mm H2O/s) (+ = to atm)
qflx_evap_veg              [double] vegetation evaporation (mm H2O/s) (+ = to atm)
eflx_sh_veg                [double] sensible heat flux from leaves (W/m**2) [+ to atm]
forc_q                     [double] atmospheric specific humidity (kg/kg)
forc_rho                   [double] air density (kg/m**3)
thm                        [double]  intermediate variable (forc_t+0.0098*forc_hgt_t_patch)
emv                        [double] vegetation emissivity
emg                        [double] ground emissivity
forc_lwrad                 [double]  downward infrared (longwave) radiation (W/m**2)
wtgq                       [double] latent heat conductance for ground [m/s]
wtalq                      [double] normalized latent heat cond. for air and leaf [-]
wtlq0                      [double] normalized latent heat conductance for leaf [-]
wtaq0                      [double] normalized latent heat conductance for air [-]

OUTPUTS:
h2ocan                     [double] canopy water (mm H2O)
eflx_sh_grnd               [double] sensible heat flux from ground (W/m**2) [+ to atm]
eflx_sh_snow               [double] sensible heat flux from snow (W/m**2) [+ to atm]
eflx_sh_soil               [double] sensible heat flux from soil (W/m**2) [+ to atm]
eflx_sh_h2osfc             [double] sensible heat flux from h2osfc (W/m**2) [+ to atm]
qflx_evap_soi              [double] soil evaporation (mm H2O/s) (+ = to atm)
qflx_ev_snow               [double] evaporation flux from snow (W/m**2) [+ to atm]
qflx_ev_soil               [double] evaporation flux from soil (W/m**2) [+ to atm]
qflx_ev_h2osfc             [double] evaporation flux from h2osfc (W/m**2) [+ to atm]
dlrad                      [double] downward longwave radiation below the canopy [W/m2]
ulrad                      [double] upward longwave radiation above the canopy [W/m2]
cgrnds                     [double] deriv, of soil sensible heat flux wrt soil temp [w/m2/k]
cgrndl                     [double] deriv of soil latent heat flux wrt soil temp [w/m**2/k]
cgrnd                      [double] deriv. of soil energy flux wrt to soil temp [w/m2/k]
t_ref2m                    [double]  2 m height surface air temperature (Kelvin)
t_ref2m_r                  [double]  Rural 2 m height surface air temperature (Kelvin)
q_ref2m                    [double]  2 m height surface specific humidity (kg/kg)
rh_ref2m_r                 [double]  Rural 2 m height surface relative humidity (%)
rh_ref2m                   [double]  2 m height surface relative humidity (%)
*/
template <class dArray_type>
void ComputeFlux_Can(const LandType &Land, const double &dtime, const int &snl, const int &frac_veg_nosno,
                     const double &frac_sno, const dArray_type t_soisno, const double &frac_h2osfc,
                     const double &t_h2osfc, const double &sabv, const double &qg_snow, const double &qg_soil,
                     const double &qg_h2osfc, const double &dqgdT, const double &htvp, const double &wtg,
                     const double &wtl0, const double &wta0, const double &wtal, const double &air, const double &bir,
                     const double &cir, const double &qsatl, const double &qsatldT, const double &dth,
                     const double &dqh, const double &temp1, const double &temp2, const double &temp12m,
                     const double &temp22m, const double &tlbef, const double &delq, const double &dt_veg,
                     const double &t_veg, const double &t_grnd, const double &forc_pbot, const double &qflx_tran_veg,
                     const double &qflx_evap_veg, const double &eflx_sh_veg, const double &forc_q,
                     const double &forc_rho, const double &thm, const double &emv, const double &emg,
                     const double &forc_lwrad, const double &wtgq, const double &wtalq, const double &wtlq0,
                     const double &wtaq0, double &h2ocan, double &eflx_sh_grnd, double &eflx_sh_snow,
                     double &eflx_sh_soil, double &eflx_sh_h2osfc, double &qflx_evap_soi, double &qflx_ev_snow,
                     double &qflx_ev_soil, double &qflx_ev_h2osfc, double &dlrad, double &ulrad, double &cgrnds,
                     double &cgrndl, double &cgrnd, double &t_ref2m, double &t_ref2m_r, double &q_ref2m,
                     double &rh_ref2m, double &rh_ref2m_r) {

  if (!Land.lakpoi && !Land.urbpoi && frac_veg_nosno != 0) {

    double e_ref2m, de2mdT, qsat_ref2m, dqsat2mdT;

    // Energy balance check in canopy
    double lw_grnd = (frac_sno * pow(t_soisno[nlevsno - snl], 4.0) +
                      (1.0 - frac_sno - frac_h2osfc) * pow(t_soisno[nlevsno], 4.0) + frac_h2osfc * pow(t_h2osfc, 4.0));
    double err = sabv + air + bir * pow(tlbef, 3.0) * (tlbef + 4.0 * dt_veg) + cir * lw_grnd - eflx_sh_veg -
                 hvap * qflx_evap_veg;

    // Fluxes from ground to canopy space
    double delt = wtal * t_grnd - wtl0 * t_veg - wta0 * thm;
    eflx_sh_grnd = cpair * forc_rho * wtg * delt;

    // compute individual sensible heat fluxes
    double delt_snow = wtal * t_soisno[nlevsno - snl] - wtl0 * t_veg - wta0 * thm;
    eflx_sh_snow = cpair * forc_rho * wtg * delt_snow;
    double delt_soil = wtal * t_soisno[nlevsno] - wtl0 * t_veg - wta0 * thm;
    eflx_sh_soil = cpair * forc_rho * wtg * delt_soil;
    double delt_h2osfc = wtal * t_h2osfc - wtl0 * t_veg - wta0 * thm;
    eflx_sh_h2osfc = cpair * forc_rho * wtg * delt_h2osfc;
    qflx_evap_soi = forc_rho * wtgq * delq;

    // compute individual latent heat fluxes
    double delq_snow = wtalq * qg_snow - wtlq0 * qsatl - wtaq0 * forc_q;
    qflx_ev_snow = forc_rho * wtgq * delq_snow;
    double delq_soil = wtalq * qg_soil - wtlq0 * qsatl - wtaq0 * forc_q;
    qflx_ev_soil = forc_rho * wtgq * delq_soil;
    double delq_h2osfc = wtalq * qg_h2osfc - wtlq0 * qsatl - wtaq0 * forc_q;
    qflx_ev_h2osfc = forc_rho * wtgq * delq_h2osfc;

    // 2 m height air temperature
    t_ref2m = thm + temp1 * dth * (1.0 / temp12m - 1.0 / temp1);
    t_ref2m_r = t_ref2m;
    // 2 m height specific humidity
    q_ref2m = forc_q + temp2 * dqh * (1.0 / temp22m - 1.0 / temp2);
    // 2 m height relative humidity
    QSat(t_ref2m, forc_pbot, e_ref2m, de2mdT, qsat_ref2m, dqsat2mdT);
    rh_ref2m = std::min(100.0, (q_ref2m / qsat_ref2m) * 100.0);
    rh_ref2m_r = rh_ref2m;

    // Downward longwave radiation below the canopy
    dlrad = (1.0 - emv) * emg * forc_lwrad + emv * emg * sb * pow(tlbef, 3.0) * (tlbef + 4.0 * dt_veg);
    // Upward longwave radiation above the canopy
    ulrad = ((1.0 - emg) * (1.0 - emv) * (1.0 - emv) * forc_lwrad +
             emv * (1.0 + (1.0 - emg) * (1.0 - emv)) * sb * pow(tlbef, 3.0) * (tlbef + 4.0 * dt_veg) +
             emg * (1.0 - emv) * sb * lw_grnd);
    // Derivative of soil energy flux with respect to soil temperature
    cgrnds = cgrnds + cpair * forc_rho * wtg * wtal;
    cgrndl = cgrndl + forc_rho * wtgq * wtalq * dqgdT;
    cgrnd = cgrnds + cgrndl * htvp;
    // Update dew accumulation (kg/m2)
    h2ocan = std::max(0.0, h2ocan + (qflx_tran_veg - qflx_evap_veg) * dtime);

    // Determine total photosynthesis -- need to implement -- vars don't get used - diagnostics, maybe??
    // PhotosynthesisTotal(fn, filterp, atm2lnd_vars, cnstate_vars, canopystate_vars, photosyns_vars)

    // evaluate error and write?? -- best way to write?
    //  if (abs(err(p)) > 0.1_r8)
  }
} // ComputeFlux_Can()

} // namespace ELM