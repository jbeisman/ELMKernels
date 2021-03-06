
#include "CallBareGroundFluxes.hh"
#include "CallCanopyFluxes.hh"
#include "CallCanopyHydrology.hh"
#include "CallCanopyTemperature.hh"
#include "CallSurfaceRadiation.hh"

#include "elm_constants.h"
#include "Kokkos_Core.hpp"
#include "land_data.h"
#include "pft_data.h"

using ArrayB1 = Kokkos::View<bool *>;
using ArrayI1 = Kokkos::View<int *>;
using ArrayD1 = Kokkos::View<double *>;
using ArrayD2 = Kokkos::View<double **>;
using ArrayP1 = Kokkos::View<ELM::PSNVegData *>;

template <typename Array_t> Array_t create(const std::string &name, int D0) { return Array_t(name, D0); }

template <typename Array_t> Array_t create(const std::string &name, int D0, int D1) { return Array_t(name, D0, D1); }

template <typename Array_t> Array_t create(const std::string &name, int D0, int D1, int D2) {
  return Array_t(name, D0, D1, D2);
}

template <class Array_t, typename Scalar_t> void assign(Array_t &arr, Scalar_t val) { Kokkos::deep_copy(arr, val); }

int main(int argc, char **argv) {
  Kokkos::initialize(argc, argv);

  const int ncells = 30000;
  const int ntimes = 200000;

  { // scope to make Kokkos happy
    // instantiate data
    // for CanopyHydrology
    ELM::LandType Land;

    const std::string data_dir("/Users/80x/Software/elm_kernels/test/data/");
    const std::string pft_file = "clm_params_c180524.nc";
    // read veg constants
    ELM::VegData<ArrayD1, ArrayD2> vegdata;
    vegdata.read_veg_data(data_dir, pft_file);
    // get veg constants for a single pft

    // explicitly define Kokkos view of struct PSNVegData
    auto psnveg = create<ArrayP1>("psnveg", ncells);

    // init PSNVegData
    //auto Land = create <Kokkos::View<ELM::LandType *> >("Land", ncells); 
    //Kokkos::parallel_for ("psnveg_init", ncells, KOKKOS_LAMBDA (const int i) {
    //  psnveg[i] = vegdata.get_pft_psnveg(Land[i].vtype);
    //});

    Kokkos::parallel_for ("psnveg_init", ncells, KOKKOS_LAMBDA (const int i) {
      psnveg[i] = vegdata.get_pft_psnveg(Land.vtype);
    });
  




    double dtime = 0.5;
    auto frac_veg_nosno = create<ArrayI1>("frac_veg_nosno", ncells);
    auto snl = create<ArrayI1>("snl", ncells);
    assign(snl, 5);
    auto n_irrig_steps_left = create<ArrayI1>("n_irrig_steps_left", ncells);
    auto forc_rain = create<ArrayD1>("forc_rain", ncells);
    auto forc_snow = create<ArrayD1>("forc_snow", ncells);
    auto elai = create<ArrayD1>("elai", ncells);
    auto esai = create<ArrayD1>("esai", ncells);
    auto h2ocan = create<ArrayD1>("h2ocan", ncells);
    auto irrig_rate = create<ArrayD1>("irrig_rate", ncells);
    auto qflx_irrig = create<ArrayD1>("qflx_irrig", ncells);
    auto qflx_prec_grnd = create<ArrayD1>("qflx_prec_grnd", ncells);
    auto qflx_snwcp_liq = create<ArrayD1>("qflx_snwcp_liq", ncells);
    auto qflx_snwcp_ice = create<ArrayD1>("qflx_snwcp_ice", ncells);
    auto qflx_snow_grnd = create<ArrayD1>("qflx_snow_grnd", ncells);
    auto qflx_rain_grnd = create<ArrayD1>("qflx_rain_grnd", ncells);
    auto fwet = create<ArrayD1>("fwet", ncells);
    auto fdry = create<ArrayD1>("fdry", ncells);
    auto forc_t = create<ArrayD1>("forc_t", ncells);
    auto t_grnd = create<ArrayD1>("t_grnd", ncells);
    auto qflx_snow_melt = create<ArrayD1>("qflx_snow_melt", ncells);
    auto n_melt = create<ArrayD1>("n_melt", ncells);
    auto micro_sigma = create<ArrayD1>("micro_sigma", ncells);
    auto snow_depth = create<ArrayD1>("snow_depth", ncells);
    auto h2osno = create<ArrayD1>("h2osno", ncells);
    auto int_snow = create<ArrayD1>("int_snow", ncells);
    //auto qflx_snow_h2osfc = create<ArrayD1>("qflx_snow_h2osfc", ncells); - always 0.0
    auto h2osfc = create<ArrayD1>("h2osfc", ncells);
    auto frac_h2osfc = create<ArrayD1>("frac_h2osfc", ncells);
    auto frac_sno_eff = create<ArrayD1>("frac_sno_eff", ncells);
    auto frac_sno = create<ArrayD1>("frac_sno", ncells);
    auto swe_old = create<ArrayD2>("swe_old", ncells, ELM::nlevsno);
    auto h2osoi_liq = create<ArrayD2>("h2osoi_liq", ncells, ELM::nlevsno + ELM::nlevgrnd);
    auto h2osoi_ice = create<ArrayD2>("h2osoi_ice", ncells, ELM::nlevsno + ELM::nlevgrnd);
    auto t_soisno = create<ArrayD2>("t_soisno", ncells, ELM::nlevsno + ELM::nlevgrnd);
    auto frac_iceold = create<ArrayD2>("frac_iceold", ncells, ELM::nlevsno + ELM::nlevgrnd);
    auto dz = create<ArrayD2>("dz", ncells, ELM::nlevsno + ELM::nlevgrnd);
    auto z = create<ArrayD2>("z", ncells, ELM::nlevsno + ELM::nlevgrnd);
    auto zi = create<ArrayD2>("zi", ncells, ELM::nlevsno + ELM::nlevgrnd + 1);
    auto snw_rds = create<ArrayD2>("snw_rds", ncells, ELM::nlevsno);

    // run CanopyHydrology functions
    int i = 0;
    while (i != ntimes) {
      canopyHydrologyInvoke(ncells, Land, frac_veg_nosno, n_irrig_steps_left, snl, dtime, forc_rain, forc_snow, elai,
                            esai, h2ocan, irrig_rate, qflx_irrig, qflx_prec_grnd, qflx_snwcp_liq, qflx_snwcp_ice,
                            qflx_snow_grnd, qflx_rain_grnd, fwet, fdry, forc_t, t_grnd, qflx_snow_melt, n_melt,
                            micro_sigma, snow_depth, h2osno, int_snow, h2osfc, frac_h2osfc,
                            frac_sno_eff, frac_sno, swe_old, h2osoi_liq, h2osoi_ice, t_soisno, frac_iceold, dz, z, zi,
                            snw_rds);
      i++;
    }

    auto nrad = create<ArrayI1>("nrad", ncells);
    auto fsr = create<ArrayD1>("fsr", ncells);
    auto laisun = create<ArrayD1>("laisun", ncells);
    auto laisha = create<ArrayD1>("laisha", ncells);
    auto sabg_soil = create<ArrayD1>("sabg_soil", ncells);
    auto sabg_snow = create<ArrayD1>("sabg_snow", ncells);
    auto sabg = create<ArrayD1>("sabg", ncells);
    auto sabv = create<ArrayD1>("sabv", ncells);
    auto fsa = create<ArrayD1>("fsa", ncells);
    auto tlai_z = create<ArrayD2>("tlai_z", ncells, ELM::nlevcan);
    auto fsun_z = create<ArrayD2>("fsun_z", ncells, ELM::nlevcan);
    auto forc_solad = create<ArrayD2>("forc_solad", ncells, ELM::numrad);
    auto forc_solai = create<ArrayD2>("forc_solai", ncells, ELM::numrad);
    auto fabd_sun_z = create<ArrayD2>("fabd_sun_z", ncells, ELM::nlevcan);
    auto fabd_sha_z = create<ArrayD2>("fabd_sha_z", ncells, ELM::nlevcan);
    auto fabi_sun_z = create<ArrayD2>("fabi_sun_z", ncells, ELM::nlevcan);
    auto fabi_sha_z = create<ArrayD2>("fabi_sha_z", ncells, ELM::nlevcan);
    auto parsun_z = create<ArrayD2>("parsun_z", ncells, ELM::nlevcan);
    auto parsha_z = create<ArrayD2>("parsha_z", ncells, ELM::nlevcan);
    auto laisun_z = create<ArrayD2>("laisun_z", ncells, ELM::nlevcan);
    auto laisha_z = create<ArrayD2>("laisha_z", ncells, ELM::nlevcan);
    auto sabg_lyr = create<ArrayD2>("sabg_lyr", ncells, ELM::nlevsno + 1);
    auto ftdd = create<ArrayD2>("ftdd", ncells, ELM::numrad);
    auto ftid = create<ArrayD2>("ftid", ncells, ELM::numrad);
    auto ftii = create<ArrayD2>("ftii", ncells, ELM::numrad);
    auto fabd = create<ArrayD2>("fabd", ncells, ELM::numrad);
    auto fabi = create<ArrayD2>("fabi", ncells, ELM::numrad);
    auto albsod = create<ArrayD2>("albsod", ncells, ELM::numrad);
    auto albsoi = create<ArrayD2>("albsoi", ncells, ELM::numrad);
    auto albsnd_hst = create<ArrayD2>("albsnd_hst", ncells, ELM::numrad);
    auto albsni_hst = create<ArrayD2>("albsni_hst", ncells, ELM::numrad);
    auto albgrd = create<ArrayD2>("albgrd", ncells, ELM::numrad);
    auto albgri = create<ArrayD2>("albgri", ncells, ELM::numrad);
    auto flx_absdv = create<ArrayD2>("flx_absdv", ncells, ELM::nlevsno + 1);
    auto flx_absdn = create<ArrayD2>("flx_absdn", ncells, ELM::nlevsno + 1);
    auto flx_absiv = create<ArrayD2>("flx_absiv", ncells, ELM::nlevsno + 1);
    auto flx_absin = create<ArrayD2>("flx_absin", ncells, ELM::nlevsno + 1);
    auto albd = create<ArrayD2>("albd", ncells, ELM::numrad);
    auto albi = create<ArrayD2>("albi", ncells, ELM::numrad);

    i = 0;
    while (i != ntimes) {
      surfaceRadiationInvoke(ncells, Land, nrad, snl, elai, snow_depth, fsr, laisun, laisha, sabg_soil, sabg_snow, sabg,
                             sabv, fsa, tlai_z, fsun_z, forc_solad, forc_solai, fabd_sun_z, fabd_sha_z, fabi_sun_z,
                             fabi_sha_z, parsun_z, parsha_z, laisun_z, laisha_z, sabg_lyr, ftdd, ftid, ftii, fabd, fabi,
                             albsod, albsoi, albsnd_hst, albsni_hst, albgrd, albgri, flx_absdv, flx_absdn, flx_absiv,
                             flx_absin, albd, albi);
      i++;
    }

    auto veg_active = create<ArrayB1>("veg_active", ncells);
    auto t_h2osfc = create<ArrayD1>("t_h2osfc", ncells);
    auto t_h2osfc_bef = create<ArrayD1>("t_h2osfc_bef", ncells);
    auto smpmin = create<ArrayD1>("smpmin", ncells);
    auto forc_q = create<ArrayD1>("forc_q", ncells);
    auto forc_pbot = create<ArrayD1>("forc_pbot", ncells);
    auto forc_th = create<ArrayD1>("forc_th", ncells);
    auto htop = create<ArrayD1>("htop", ncells);
    auto forc_hgt_u = create<ArrayD1>("forc_hgt_u", ncells);
    auto forc_hgt_t = create<ArrayD1>("forc_hgt_t", ncells);
    auto forc_hgt_q = create<ArrayD1>("forc_hgt_q", ncells);
    auto z_0_town = create<ArrayD1>("z_0_town", ncells);
    auto z_d_town = create<ArrayD1>("z_d_town", ncells);
    auto soilalpha = create<ArrayD1>("soilalpha", ncells);
    auto soilalpha_u = create<ArrayD1>("soilalpha_u", ncells);
    auto soilbeta = create<ArrayD1>("soilbeta", ncells);
    auto qg_snow = create<ArrayD1>("qg_snow", ncells);
    auto qg_soil = create<ArrayD1>("qg_soil", ncells);
    auto qg = create<ArrayD1>("qg", ncells);
    auto qg_h2osfc = create<ArrayD1>("qg_h2osfc", ncells);
    auto dqgdT = create<ArrayD1>("dqgdT", ncells);
    auto htvp = create<ArrayD1>("htvp", ncells);
    auto emg = create<ArrayD1>("emg", ncells);
    auto emv = create<ArrayD1>("emv", ncells);
    auto z0mg = create<ArrayD1>("z0mg", ncells);
    auto z0hg = create<ArrayD1>("z0hg", ncells);
    auto z0qg = create<ArrayD1>("z0qg", ncells);
    auto z0mv = create<ArrayD1>("z0mv", ncells);
    auto z0hv = create<ArrayD1>("z0hv", ncells);
    auto z0qv = create<ArrayD1>("z0qv", ncells);
    auto beta = create<ArrayD1>("beta", ncells);
    auto zii = create<ArrayD1>("zii", ncells);
    auto thv = create<ArrayD1>("thv", ncells);
    auto z0m = create<ArrayD1>("z0m", ncells);
    auto displa = create<ArrayD1>("displa", ncells);
    auto cgrnds = create<ArrayD1>("cgrnds", ncells);
    auto cgrndl = create<ArrayD1>("cgrndl", ncells);
    auto cgrnd = create<ArrayD1>("cgrnd", ncells);
    auto forc_hgt_u_patch = create<ArrayD1>("forc_hgt_u_patch", ncells);
    auto forc_hgt_t_patch = create<ArrayD1>("forc_hgt_t_patch", ncells);
    auto forc_hgt_q_patch = create<ArrayD1>("forc_hgt_q_patch", ncells);
    auto thm = create<ArrayD1>("thm", ncells);
    auto eflx_sh_tot = create<ArrayD1>("eflx_sh_tot", ncells);
    auto eflx_sh_tot_u = create<ArrayD1>("eflx_sh_tot_u", ncells);
    auto eflx_sh_tot_r = create<ArrayD1>("eflx_sh_tot_r", ncells);
    auto eflx_lh_tot = create<ArrayD1>("eflx_lh_tot", ncells);
    auto eflx_lh_tot_u = create<ArrayD1>("eflx_lh_tot_u", ncells);
    auto eflx_lh_tot_r = create<ArrayD1>("eflx_lh_tot_r", ncells);
    auto eflx_sh_veg = create<ArrayD1>("eflx_sh_veg", ncells);
    auto qflx_evap_tot = create<ArrayD1>("qflx_evap_tot", ncells);
    auto qflx_evap_veg = create<ArrayD1>("qflx_evap_veg", ncells);
    auto qflx_tran_veg = create<ArrayD1>("qflx_tran_veg", ncells);
    auto tssbef = create<ArrayD2>("tssbef", ncells, ELM::nlevgrnd + ELM::nlevsno);
    auto watsat = create<ArrayD2>("watsat", ncells, ELM::nlevgrnd);
    auto sucsat = create<ArrayD2>("sucsat", ncells, ELM::nlevgrnd);
    auto bsw = create<ArrayD2>("bsw", ncells, ELM::nlevgrnd);
    auto watdry = create<ArrayD2>("watdry", ncells, ELM::nlevgrnd);
    auto watopt = create<ArrayD2>("watopt", ncells, ELM::nlevgrnd);
    auto rootfr_road_perv = create<ArrayD2>("rootfr_road_perv", ncells, ELM::nlevgrnd);
    auto rootr_road_perv = create<ArrayD2>("rootr_road_perv", ncells, ELM::nlevgrnd);
    auto watfc = create<ArrayD2>("watfc", ncells, ELM::nlevgrnd);
    auto displar = create<ArrayD2>("displar", ncells, ELM::numpft);
    auto z0mr = create<ArrayD2>("z0mr", ncells, ELM::numpft);

    i = 0;
    while (i != ntimes) {
      canopyTemperatureInvoke(ncells, Land, veg_active, snl, frac_veg_nosno, t_h2osfc, t_h2osfc_bef, frac_sno_eff,
                              frac_h2osfc, frac_sno, smpmin, forc_q, forc_pbot, forc_th, elai, esai, htop, forc_hgt_u,
                              forc_hgt_t, forc_hgt_q, z_0_town, z_d_town, forc_t, t_grnd, soilalpha, soilalpha_u,
                              soilbeta, qg_snow, qg_soil, qg, qg_h2osfc, dqgdT, emg, emv, htvp, z0mg, z0hg, z0qg, z0mv,
                              z0hv, z0qv, thv, z0m, displa, forc_hgt_u_patch,
                              forc_hgt_t_patch, forc_hgt_q_patch, thm, eflx_sh_tot, eflx_sh_tot_u, eflx_sh_tot_r,
                              eflx_lh_tot, eflx_lh_tot_u, eflx_lh_tot_r, eflx_sh_veg, qflx_evap_tot, qflx_evap_veg,
                              qflx_tran_veg, tssbef, t_soisno, h2osoi_liq, h2osoi_ice, dz, watsat, sucsat, bsw, watdry,
                              watopt, rootfr_road_perv, rootr_road_perv, watfc, displar, z0mr);
      i++;
    }

    auto forc_u = create<ArrayD1>("forc_u", ncells);
    auto forc_v = create<ArrayD1>("forc_v", ncells);
    auto dlrad = create<ArrayD1>("dlrad", ncells);
    auto ulrad = create<ArrayD1>("ulrad", ncells);
    auto forc_rho = create<ArrayD1>("forc_rho", ncells);
    auto eflx_sh_grnd = create<ArrayD1>("eflx_sh_grnd", ncells);
    auto eflx_sh_snow = create<ArrayD1>("eflx_sh_snow", ncells);
    auto eflx_sh_soil = create<ArrayD1>("eflx_sh_soil", ncells);
    auto eflx_sh_h2osfc = create<ArrayD1>("eflx_sh_h2osfc", ncells);
    auto qflx_evap_soi = create<ArrayD1>("qflx_evap_soi", ncells);
    auto qflx_ev_snow = create<ArrayD1>("qflx_ev_snow", ncells);
    auto qflx_ev_soil = create<ArrayD1>("qflx_ev_soil", ncells);
    auto qflx_ev_h2osfc = create<ArrayD1>("qflx_ev_h2osfc", ncells);
    auto t_ref2m = create<ArrayD1>("t_ref2m", ncells);
    auto t_ref2m_r = create<ArrayD1>("t_ref2m_r", ncells);
    auto q_ref2m = create<ArrayD1>("q_ref2m", ncells);
    auto rh_ref2m = create<ArrayD1>("rh_ref2m", ncells);
    auto rh_ref2m_r = create<ArrayD1>("rh_ref2m_r", ncells);

    i = 0;
    while (i != ntimes) {
      bareground_fluxesInvoke(ncells, Land, frac_veg_nosno, forc_u, forc_v, forc_q, forc_th, forc_hgt_u_patch, thm, thv,
                             t_grnd, qg, z0mg, dlrad, ulrad, forc_hgt_t_patch, forc_hgt_q_patch, z0hg, z0qg,
                             snl, forc_rho, soilbeta, dqgdT, htvp, t_h2osfc, qg_snow, qg_soil, qg_h2osfc, t_soisno,
                             forc_pbot, cgrnds, cgrndl, cgrnd, eflx_sh_grnd, eflx_sh_tot, eflx_sh_snow, eflx_sh_soil,
                             eflx_sh_h2osfc, qflx_evap_soi, qflx_evap_tot, qflx_ev_snow, qflx_ev_soil, qflx_ev_h2osfc,
                             t_ref2m, t_ref2m_r, q_ref2m, rh_ref2m, rh_ref2m_r);
      std::cout << "running, t = " << i << std::endl;
      i++;
    }

    auto altmax_indx = create<ArrayI1>("altmax_indx", ncells);
    auto altmax_lastyear_indx = create<ArrayI1>("altmax_lastyear_indx", ncells);

    auto max_dayl = create<ArrayD1>("max_dayl", ncells);
    auto dayl = create<ArrayD1>("dayl", ncells);
    auto tc_stress = create<ArrayD1>("tc_stres", ncells);
    auto forc_lwrad = create<ArrayD1>("forc_lwrad", ncells);
    auto t10 = create<ArrayD1>("t10", ncells);
    auto vcmaxcintsha = create<ArrayD1>("vcmaxcintsha", ncells);
    auto vcmaxcintsun = create<ArrayD1>("vcmaxcintsun", ncells);
    auto forc_pco2 = create<ArrayD1>("forc_pco2", ncells);
    auto forc_po2 = create<ArrayD1>("forc_po2", ncells);
    auto btran = create<ArrayD1>("btran", ncells);
    auto t_veg = create<ArrayD1>("t_veg", ncells);

    auto rootfr = create<ArrayD2>("rootfr", ncells, ELM::nlevgrnd);
    auto smpso = create<ArrayD2>("smpso", ncells, ELM::numpft);
    auto smpsc = create<ArrayD2>("smpsc", ncells, ELM::numpft);
    auto dleaf = create<ArrayD2>("dleaf", ncells, ELM::numpft);
    auto rootr = create<ArrayD2>("rootr", ncells, ELM::nlevgrnd);
    auto eff_porosity = create<ArrayD2>("eff_porosity", ncells, ELM::nlevgrnd);

    i = 0;
    while (i != ntimes) {

      canopyFluxesInvoke(
          ncells, Land, psnveg, t_soisno, h2osoi_ice, h2osoi_liq, dz, rootfr, sucsat, watsat, bsw,
          parsha_z, parsun_z, laisha_z, laisun_z, tlai_z, rootr, eff_porosity, nrad, snl, frac_veg_nosno, altmax_indx,
          altmax_lastyear_indx, frac_sno, forc_hgt_u_patch, thm, thv, max_dayl, dayl, tc_stress, elai, esai, emv, emg,
          qg, t_grnd, forc_t, forc_pbot, forc_lwrad, forc_u, forc_v, forc_q, forc_th, z0mg, dtime, forc_hgt_t_patch,
          forc_hgt_q_patch, fwet, fdry, laisun, laisha, forc_rho, snow_depth, soilbeta, frac_h2osfc, t_h2osfc, sabv,
          h2ocan, htop, t10, vcmaxcintsha, vcmaxcintsun, forc_pco2, forc_po2, qg_snow, qg_soil, qg_h2osfc, dqgdT, htvp,
          eflx_sh_grnd, eflx_sh_snow, eflx_sh_soil, eflx_sh_h2osfc, qflx_evap_soi, qflx_ev_snow, qflx_ev_soil,
          qflx_ev_h2osfc, dlrad, ulrad, cgrnds, cgrndl, cgrnd, t_ref2m, t_ref2m_r, q_ref2m, rh_ref2m, rh_ref2m_r,
          qflx_tran_veg, qflx_evap_veg, eflx_sh_veg, btran, displa, z0mv, z0hv, z0qv, t_veg);
      std::cout << "running, t = " << i << std::endl;
      i++;
    }
  }
  Kokkos::finalize();
  return 0;
}
