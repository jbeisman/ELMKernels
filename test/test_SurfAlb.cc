
#include "SurfaceAlbedo.h"
#include "SnowSNICAR.h"
#include "ELMConstants.h"
#include "LandType.h"
#include "read_test_input.hh"
#include "array.hh"

#include <iostream>
#include <string>

/*    
need to reconcile rhol/rhos taul/taus dimensionality
ReadPFT has 1d var[numpft] and these kernels use 2d var[numrad][numpft]

Need to read a few more NC files:
mss_cnc vars from AerosolMod, everything from SnowOptics_Init, soil color (isoicol, albsat, albdry) from SurfaceAlbedoInitTimeConst

tests SurfaceAlbedo kernels: 
SurfRadZeroFluxes()
SurfRadAbsorbed()
SurfRadLayers()
SurfRadReflected()

the following data comes from the files SurfaceAlbedo_IN.txt and SurfaceAlbedo_OUT.txt located in test/data

should probably pre-select constants that vary at the pft or cell level - things like albdry, albsat,
rhol, rhos, taul, taus, xl


snl_top
snl_btm
snl
nrad
ncan
flg_nosnl
snw_rds
snw_rds_lcl
coszen
elai
frac_sno
esai
tlai
tsai
vcmaxcintsun
vcmaxcintsha
t_veg
fwet
xl
t_grnd
mu_not
mss_cnc_bcphi
mss_cnc_bcpho
mss_cnc_dst1
mss_cnc_dst2
mss_cnc_dst3
mss_cnc_dst4
albsod
albsoi
albgrd
albgri
albd
albi
fabd
fabd_sun
fabd_sha
fabi
fabi_sun
fabi_sha
ftdd
ftid
ftii
flx_absdv
flx_absdn
flx_absiv
flx_absin
albsnd
albsni
tlai_z
tsai_z
fsun_z
fabd_sun_z
fabd_sha_z
fabi_sun_z
fabi_sha_z
rhol
rhos
taul
taus
albsat
albdry
h2osoi_vol
mss_cnc_aer_in_fdb
flx_absd_snw
flx_absi_snw
flx_abs_lcl
h2osno
ss_alb_oc1
asm_prm_oc1
ext_cff_mss_oc1
ss_alb_oc2
asm_prm_oc2
ext_cff_mss_oc2
ss_alb_dst1
asm_prm_dst1
ext_cff_mss_dst1
ss_alb_dst2
asm_prm_dst2
ext_cff_mss_dst2
ss_alb_dst3
asm_prm_dst3
ext_cff_mss_dst3
ss_alb_dst4
asm_prm_dst4
ext_cff_mss_dst4
ss_alb_snw_drc
asm_prm_snw_drc
ext_cff_mss_snw_drc
ss_alb_snw_dfs
asm_prm_snw_dfs
ext_cff_mss_snw_dfs
ss_alb_bc1
asm_prm_bc1
ext_cff_mss_bc1
ss_alb_bc2
asm_prm_bc2
ext_cff_mss_bc2
albout_lcl
flx_slrd_lcl
flx_slri_lcl
h2osoi_liq
h2osoi_ice
h2osoi_ice_lcl
h2osoi_liq_lcl
bcenh
g_star
omega_star
tau_star
*/

using ArrayI1 = ELM::Array<int, 1>;
using ArrayI2 = ELM::Array<int, 2>;
using ArrayD1 = ELM::Array<double, 1>;
using ArrayD2 = ELM::Array<double, 2>;
using ArrayD3 = ELM::Array<double, 3>;

template <class Array_t> Array_t create(const std::string &name, int D0) { return Array_t(name, D0); }
template <class Array_t> Array_t create(const std::string &name, int D0, int D1) { return Array_t(name, D0, D1); }
template <class Array_t> Array_t create(const std::string &name, int D0, int D1, int D2) { return Array_t(name, D0, D1, D2); }
template <class Array_t, typename Scalar_t> void assign(Array_t &arr, Scalar_t val) { ELM::deep_copy(arr, val); }

int main(int argc, char **argv) {

  // data files 
  const std::string data_dir("/Users/80x/Software/elm_kernels/test/data/");
  const std::string snowoptics_file = data_dir + "SnowOptics_IN.txt";
  const std::string input_file = data_dir + "SurfaceAlbedo_IN.txt";
  const std::string output_file = data_dir + "SurfaceAlbedo_OUT.txt";

  // hardwired
  ELM::LandType Land;
  Land.ltype = 1;
  Land.ctype = 1;
  Land.vtype = 12;
  int n_grid_cells = 1;
  double dtime = 1800.0;
  int idx = 0;


  // ELM state variables
  // required for SurfaceAlbedo kernels
  // I1
  auto snl_top = create<ArrayI1>("snl_top", n_grid_cells);
  auto snl_btm = create<ArrayI1>("snl_btm", n_grid_cells);
  auto snl = create<ArrayI1>("snl", n_grid_cells);
  auto nrad = create<ArrayI1>("nrad", n_grid_cells);
  auto ncan = create<ArrayI1>("ncan", n_grid_cells);
  auto flg_nosnl = create<ArrayI1>("flg_nosnl", n_grid_cells);

  // I2
  auto snw_rds_lcl = create<ArrayI2>("snw_rds_lcl", n_grid_cells, ELM::nlevsno);

  // D1
  auto coszen = create<ArrayD1>("coszen", n_grid_cells);
  auto elai = create<ArrayD1>("elai", n_grid_cells);
  auto frac_sno = create<ArrayD1>("frac_sno", n_grid_cells);
  auto esai = create<ArrayD1>("esai", n_grid_cells);
  auto tlai = create<ArrayD1>("tlai", n_grid_cells);
  auto tsai = create<ArrayD1>("tsai", n_grid_cells);
  auto vcmaxcintsun = create<ArrayD1>("vcmaxcintsun", n_grid_cells);
  auto vcmaxcintsha = create<ArrayD1>("vcmaxcintsha", n_grid_cells);
  auto t_veg = create<ArrayD1>("t_veg", n_grid_cells);
  auto fwet = create<ArrayD1>("fwet", n_grid_cells);
  auto xl = create<ArrayD1>("xl", ELM::numpft);
  auto t_grnd = create<ArrayD1>("t_grnd", n_grid_cells);
  auto mu_not = create<ArrayD1>("mu_not", n_grid_cells);

  // D2
  auto snw_rds = create<ArrayD2>("snw_rds", n_grid_cells, ELM::nlevsno);
  auto mss_cnc_bcphi = create<ArrayD2>("mss_cnc_bcphi", n_grid_cells, ELM::nlevsno);
  auto mss_cnc_bcpho = create<ArrayD2>("mss_cnc_bcpho", n_grid_cells, ELM::nlevsno);
  auto mss_cnc_dst1 = create<ArrayD2>("mss_cnc_dst1", n_grid_cells, ELM::nlevsno);
  auto mss_cnc_dst2 = create<ArrayD2>("mss_cnc_dst2", n_grid_cells, ELM::nlevsno);
  auto mss_cnc_dst3 = create<ArrayD2>("mss_cnc_dst3", n_grid_cells, ELM::nlevsno);
  auto mss_cnc_dst4 = create<ArrayD2>("mss_cnc_dst4", n_grid_cells, ELM::nlevsno);
  auto albsod = create<ArrayD2>("albsod", n_grid_cells, ELM::numrad);
  auto albsoi = create<ArrayD2>("albsoi", n_grid_cells, ELM::numrad);
  auto albgrd = create<ArrayD2>("albgrd", n_grid_cells, ELM::numrad);
  auto albgri = create<ArrayD2>("albgri", n_grid_cells, ELM::numrad);
  auto albd = create<ArrayD2>("albd", n_grid_cells, ELM::numrad);
  auto albi = create<ArrayD2>("albi", n_grid_cells, ELM::numrad);
  auto fabd = create<ArrayD2>("fabd", n_grid_cells, ELM::numrad);
  auto fabd_sun = create<ArrayD2>("fabd_sun", n_grid_cells, ELM::numrad);
  auto fabd_sha = create<ArrayD2>("fabd_sha", n_grid_cells, ELM::numrad);
  auto fabi = create<ArrayD2>("fabi", n_grid_cells, ELM::numrad);
  auto fabi_sun = create<ArrayD2>("fabi_sun", n_grid_cells, ELM::numrad);
  auto fabi_sha = create<ArrayD2>("fabi_sha", n_grid_cells, ELM::numrad);
  auto ftdd = create<ArrayD2>("ftdd", n_grid_cells, ELM::numrad);
  auto ftid = create<ArrayD2>("ftid", n_grid_cells, ELM::numrad);
  auto ftii = create<ArrayD2>("ftii", n_grid_cells, ELM::numrad);
  auto flx_absdv = create<ArrayD2>("flx_absdv", n_grid_cells, ELM::nlevsno+1);
  auto flx_absdn = create<ArrayD2>("flx_absdn", n_grid_cells, ELM::nlevsno+1);
  auto flx_absiv = create<ArrayD2>("flx_absiv", n_grid_cells, ELM::nlevsno+1);
  auto flx_absin = create<ArrayD2>("flx_absin", n_grid_cells, ELM::nlevsno+1);
  auto albsnd = create<ArrayD2>("albsnd", n_grid_cells, ELM::numrad);
  auto albsni = create<ArrayD2>("albsni", n_grid_cells, ELM::numrad);
  auto tlai_z = create<ArrayD2>("tlai_z", n_grid_cells, ELM::nlevcan);
  auto tsai_z = create<ArrayD2>("tsai_z", n_grid_cells, ELM::nlevcan);
  auto fsun_z = create<ArrayD2>("fsun_z", n_grid_cells, ELM::nlevcan);
  auto fabd_sun_z = create<ArrayD2>("fabd_sun_z", n_grid_cells, ELM::nlevcan);
  auto fabd_sha_z = create<ArrayD2>("fabd_sha_z", n_grid_cells, ELM::nlevcan);
  auto fabi_sun_z = create<ArrayD2>("fabi_sun_z", n_grid_cells, ELM::nlevcan);
  auto fabi_sha_z = create<ArrayD2>("fabi_sha_z", n_grid_cells, ELM::nlevcan);

  // these need to be transposed
  auto rhol = create<ArrayD2>("rhol", ELM::numrad, ELM::numpft);
  auto rhos = create<ArrayD2>("rhos", ELM::numrad, ELM::numpft);
  auto taul = create<ArrayD2>("taul", ELM::numrad, ELM::numpft);
  auto taus = create<ArrayD2>("taus", ELM::numrad, ELM::numpft);

  auto albsat = create<ArrayD2>("albsat", n_grid_cells, ELM::numrad);
  auto albdry = create<ArrayD2>("albdry", n_grid_cells, ELM::numrad);
  auto h2osoi_vol = create<ArrayD2>("h2osoi_vol", n_grid_cells, ELM::nlevgrnd);

  // D3
  auto mss_cnc_aer_in_fdb = create<ArrayD3>("mss_cnc_aer_in_fdb", n_grid_cells, ELM::nlevsno, ELM::SNICAR::sno_nbr_aer);
  auto flx_absd_snw = create<ArrayD3>("flx_absd_snw", n_grid_cells, ELM::nlevsno+1, ELM::numrad);
  auto flx_absi_snw = create<ArrayD3>("flx_absi_snw", n_grid_cells, ELM::nlevsno+1, ELM::numrad);
  auto flx_abs_lcl = create<ArrayD3>("flx_abs_lcl", n_grid_cells, ELM::nlevsno+1, ELM::SNICAR::numrad_snw);
  //auto flx_abs = create<ArrayD3>("flx_abs", n_grid_cells, ELM::nlevsno+1, ELM::numrad);

  // remaining variables required for SNICAR kernels
  // D1
  auto h2osno = create<ArrayD1>("h2osno", n_grid_cells);
  auto ss_alb_oc1 = create<ArrayD1>("ss_alb_oc1", ELM::SNICAR::numrad_snw);
  auto asm_prm_oc1 = create<ArrayD1>("asm_prm_oc1", ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_oc1 = create<ArrayD1>("ext_cff_mss_oc1", ELM::SNICAR::numrad_snw);
  auto ss_alb_oc2 = create<ArrayD1>("ss_alb_oc2", ELM::SNICAR::numrad_snw);
  auto asm_prm_oc2 = create<ArrayD1>("asm_prm_oc2", ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_oc2 = create<ArrayD1>("ext_cff_mss_oc2", ELM::SNICAR::numrad_snw);
  auto ss_alb_dst1 = create<ArrayD1>("ss_alb_dst1", ELM::SNICAR::numrad_snw);
  auto asm_prm_dst1 = create<ArrayD1>("asm_prm_dst1", ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_dst1 = create<ArrayD1>("ext_cff_mss_dst1", ELM::SNICAR::numrad_snw);
  auto ss_alb_dst2 = create<ArrayD1>("ss_alb_dst2", ELM::SNICAR::numrad_snw);
  auto asm_prm_dst2 = create<ArrayD1>("asm_prm_dst2", ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_dst2 = create<ArrayD1>("ext_cff_mss_dst2", ELM::SNICAR::numrad_snw);
  auto ss_alb_dst3 = create<ArrayD1>("ss_alb_dst3", ELM::SNICAR::numrad_snw);
  auto asm_prm_dst3 = create<ArrayD1>("asm_prm_dst3", ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_dst3 = create<ArrayD1>("ext_cff_mss_dst3", ELM::SNICAR::numrad_snw);
  auto ss_alb_dst4 = create<ArrayD1>("ss_alb_dst4", ELM::SNICAR::numrad_snw);
  auto asm_prm_dst4 = create<ArrayD1>("asm_prm_dst4", ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_dst4 = create<ArrayD1>("ext_cff_mss_dst4", ELM::SNICAR::numrad_snw);

  // D2
  auto ss_alb_snw_drc = create<ArrayD2>("ss_alb_snw_drc", ELM::SNICAR::numrad_snw, ELM::SNICAR::idx_Mie_snw_mx);
  auto asm_prm_snw_drc = create<ArrayD2>("asm_prm_snw_drc", ELM::SNICAR::numrad_snw, ELM::SNICAR::idx_Mie_snw_mx);
  auto ext_cff_mss_snw_drc = create<ArrayD2>("ext_cff_mss_snw_drc", ELM::SNICAR::numrad_snw, ELM::SNICAR::idx_Mie_snw_mx);
  auto ss_alb_snw_dfs = create<ArrayD2>("ss_alb_snw_dfs", ELM::SNICAR::numrad_snw, ELM::SNICAR::idx_Mie_snw_mx);
  auto asm_prm_snw_dfs = create<ArrayD2>("asm_prm_snw_dfs", ELM::SNICAR::numrad_snw, ELM::SNICAR::idx_Mie_snw_mx);
  auto ext_cff_mss_snw_dfs = create<ArrayD2>("ext_cff_mss_snw_dfs", ELM::SNICAR::numrad_snw, ELM::SNICAR::idx_Mie_snw_mx);
  auto ss_alb_bc1 = create<ArrayD2>("ss_alb_bc1", ELM::SNICAR::idx_bc_nclrds_max+1, ELM::SNICAR::numrad_snw);
  auto asm_prm_bc1 = create<ArrayD2>("asm_prm_bc1", ELM::SNICAR::idx_bc_nclrds_max+1, ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_bc1 = create<ArrayD2>("ext_cff_mss_bc1", ELM::SNICAR::idx_bc_nclrds_max+1, ELM::SNICAR::numrad_snw);
  auto ss_alb_bc2 = create<ArrayD2>("ss_alb_bc2", ELM::SNICAR::idx_bc_nclrds_max+1, ELM::SNICAR::numrad_snw);
  auto asm_prm_bc2 = create<ArrayD2>("asm_prm_bc2", ELM::SNICAR::idx_bc_nclrds_max+1, ELM::SNICAR::numrad_snw);
  auto ext_cff_mss_bc2 = create<ArrayD2>("ext_cff_mss_bc2", ELM::SNICAR::idx_bc_nclrds_max+1, ELM::SNICAR::numrad_snw);
  auto albout_lcl = create<ArrayD2>("albout_lcl", n_grid_cells, ELM::SNICAR::numrad_snw);
  //auto albout = create<ArrayD2>("albout", n_grid_cells, ELM::numrad);
  auto flx_slrd_lcl = create<ArrayD2>("flx_slrd_lcl", n_grid_cells, ELM::SNICAR::numrad_snw);
  auto flx_slri_lcl = create<ArrayD2>("flx_slri_lcl", n_grid_cells, ELM::SNICAR::numrad_snw);
  auto h2osoi_liq = create<ArrayD2>("h2osoi_liq", n_grid_cells, ELM::nlevsno + ELM::nlevgrnd);
  auto h2osoi_ice = create<ArrayD2>("h2osoi_ice", n_grid_cells, ELM::nlevsno + ELM::nlevgrnd);
  auto h2osoi_ice_lcl = create<ArrayD2>("h2osoi_ice_lcl", n_grid_cells, ELM::nlevsno);
  auto h2osoi_liq_lcl = create<ArrayD2>("h2osoi_liq_lcl", n_grid_cells, ELM::nlevsno);

  // D3
  auto bcenh = create<ArrayD3>("bcenh", ELM::SNICAR::idx_bcint_icerds_max+1, ELM::SNICAR::idx_bc_nclrds_max+1, ELM::SNICAR::numrad_snw);
  auto g_star = create<ArrayD3>("g_star", n_grid_cells, ELM::SNICAR::numrad_snw, ELM::nlevsno);
  auto omega_star = create<ArrayD3>("omega_star", n_grid_cells, ELM::SNICAR::numrad_snw, ELM::nlevsno);
  auto tau_star = create<ArrayD3>("tau_star", n_grid_cells, ELM::SNICAR::numrad_snw, ELM::nlevsno);
  
  // provide basic input to allow loops to run
  assign(coszen, 0.9);
  assign(h2osno, 0.5);








  // input and output utility class objects
  ELM::IO::ELMtestinput snowoptics(snowoptics_file);
  // get input and output state for time t
  snowoptics.getState(0);


  snowoptics.parseState(ss_alb_oc1);
  snowoptics.parseState(asm_prm_oc1);
  snowoptics.parseState(ext_cff_mss_oc1);
  snowoptics.parseState(ss_alb_oc2);
  snowoptics.parseState(asm_prm_oc2);
  snowoptics.parseState(ext_cff_mss_oc2);
  snowoptics.parseState(ss_alb_dst1);
  snowoptics.parseState(asm_prm_dst1);
  snowoptics.parseState(ext_cff_mss_dst1);
  snowoptics.parseState(ss_alb_dst2);
  snowoptics.parseState(asm_prm_dst2);
  snowoptics.parseState(ext_cff_mss_dst2);
  snowoptics.parseState(ss_alb_dst3);
  snowoptics.parseState(asm_prm_dst3);
  snowoptics.parseState(ext_cff_mss_dst3);
  snowoptics.parseState(ss_alb_dst4);
  snowoptics.parseState(asm_prm_dst4);
  snowoptics.parseState(ext_cff_mss_dst4);
  snowoptics.parseState(ss_alb_snw_drc);
  snowoptics.parseState(asm_prm_snw_drc);
  snowoptics.parseState(ext_cff_mss_snw_drc);
  snowoptics.parseState(ss_alb_snw_dfs);
  snowoptics.parseState(asm_prm_snw_dfs);
  snowoptics.parseState(ext_cff_mss_snw_dfs);
  snowoptics.parseState(ss_alb_bc1);
  snowoptics.parseState(asm_prm_bc1);
  snowoptics.parseState(ext_cff_mss_bc1);
  snowoptics.parseState(ss_alb_bc2);
  snowoptics.parseState(asm_prm_bc2);
  snowoptics.parseState(ext_cff_mss_bc2);
  snowoptics.parseState(bcenh);

  // input and output utility class objects
  ELM::IO::ELMtestinput in(input_file);
  ELM::IO::ELMtestinput out(output_file);

  for (std::size_t t = 2; t < 49; ++t) {

    // get input and output state for time t
    in.getState(t);
    out.getState(t);


    // parse input state and assign to variables
    in.parseState(snl);
    in.parseState(elai);
    in.parseState(esai);
    in.parseState(tlai);
    in.parseState(tsai);
    in.parseState(coszen);
    in.parseState(frac_sno);
    in.parseState(h2osno);
    in.parseState(t_veg);
    in.parseState(fwet);
    in.parseState(mss_cnc_bcphi);
    in.parseState(mss_cnc_bcpho);
    in.parseState(mss_cnc_dst1);
    in.parseState(mss_cnc_dst2);
    in.parseState(mss_cnc_dst3);
    in.parseState(mss_cnc_dst4);
    in.parseState(albsod);
    in.parseState(albsoi);
    in.parseState(albsnd);
    in.parseState(albsni);
    in.parseState(h2osoi_liq);
    in.parseState(h2osoi_ice);
    in.parseState(snw_rds);
    in.parseState(xl);
    in.parseState(h2osoi_vol);
    in.parseState(albsat);
    in.parseState(albdry);
    in.parseState(rhol);
    in.parseState(rhos);
    in.parseState(taul);
    in.parseState(taus);
    in.parseState(albgrd);
    in.parseState(albgri);
    in.parseState(flx_absdv);
    in.parseState(flx_absdn);
    in.parseState(flx_absiv);
    in.parseState(flx_absin);
    in.parseState(tlai_z);
    in.parseState(tsai_z);
    in.parseState(fsun_z);
    in.parseState(fabd_sun_z);
    in.parseState(fabd_sha_z);
    in.parseState(fabi_sun_z);
    in.parseState(fabi_sha_z);
    in.parseState(albd);
    in.parseState(ftid);
    in.parseState(ftdd);
    in.parseState(fabd);
    in.parseState(fabd_sun);
    in.parseState(fabd_sha);
    in.parseState(albi);
    in.parseState(ftii);
    in.parseState(fabi);
    in.parseState(fabi_sun);
    in.parseState(fabi_sha);


  ELM::SurfaceAlbedo::InitTimestep(Land.urbpoi, elai[idx], mss_cnc_bcphi[idx], mss_cnc_bcpho[idx], 
    mss_cnc_dst1[idx], mss_cnc_dst2[idx], mss_cnc_dst3[idx], mss_cnc_dst4[idx],
    vcmaxcintsun[idx], vcmaxcintsha[idx], albsod[idx], albsoi[idx], albgrd[idx], albgri[idx], albd[idx], 
    albi[idx], fabd[idx], fabd_sun[idx], fabd_sha[idx], fabi[idx], fabi_sun[idx], fabi_sha[idx], 
    ftdd[idx], ftid[idx], ftii[idx], flx_absdv[idx], flx_absdn[idx], flx_absiv[idx], flx_absin[idx], 
    mss_cnc_aer_in_fdb[idx]);

  ELM::SurfaceAlbedo::SoilAlbedo(
    Land, snl[idx], t_grnd[idx], coszen[idx], 
    h2osoi_vol[idx], albsat[idx], albdry[idx],
    albsod[idx], albsoi[idx]);

{
    int flg_slr_in = 1;

    ELM::SNICAR::InitTimestep (Land.urbpoi,
      flg_slr_in, coszen[idx], h2osno[idx], snl[idx], h2osoi_liq[idx], h2osoi_ice[idx],
      snw_rds[idx], snl_top[idx], snl_btm[idx], flx_abs_lcl[idx], flx_absd_snw[idx],
      flg_nosnl[idx], h2osoi_ice_lcl[idx], h2osoi_liq_lcl[idx], snw_rds_lcl[idx], mu_not[idx], flx_slrd_lcl[idx],
      flx_slri_lcl[idx]);


    ELM::SNICAR::SnowAerosolMieParams(Land.urbpoi, flg_slr_in, snl_top[idx], snl_btm[idx], coszen[idx], h2osno[idx], snw_rds_lcl[idx],
      h2osoi_ice_lcl[idx], h2osoi_liq_lcl[idx], ss_alb_oc1, asm_prm_oc1, ext_cff_mss_oc1, ss_alb_oc2, asm_prm_oc2, ext_cff_mss_oc2, ss_alb_dst1,
      asm_prm_dst1, ext_cff_mss_dst1, ss_alb_dst2, asm_prm_dst2, ext_cff_mss_dst2, ss_alb_dst3, asm_prm_dst3, ext_cff_mss_dst3, ss_alb_dst4, asm_prm_dst4,
      ext_cff_mss_dst4, ss_alb_snw_drc, asm_prm_snw_drc, ext_cff_mss_snw_drc, ss_alb_snw_dfs, asm_prm_snw_dfs, ext_cff_mss_snw_dfs, ss_alb_bc1,
      asm_prm_bc1, ext_cff_mss_bc1, ss_alb_bc2, asm_prm_bc2, ext_cff_mss_bc2, bcenh, mss_cnc_aer_in_fdb[idx], g_star[idx], omega_star[idx], tau_star[idx]);


    ELM::SNICAR::SnowRadiativeTransfer(Land.urbpoi, flg_slr_in,flg_nosnl[idx], snl_top[idx], snl_btm[idx], coszen[idx], h2osno[idx], mu_not[idx],
      flx_slrd_lcl[idx], flx_slri_lcl[idx], albsoi[idx], g_star[idx], omega_star[idx], tau_star[idx], albout_lcl[idx], flx_abs_lcl[idx]);


    ELM::SNICAR::SnowAlbedoRadiationFlux(Land.urbpoi, flg_slr_in, snl_top[idx], coszen[idx], mu_not[idx], h2osno[idx], snw_rds_lcl[idx], albsoi[idx],
      albout_lcl[idx], flx_abs_lcl[idx], albsnd[idx], flx_absd_snw[idx]);
  }


  {
    int flg_slr_in = 2;

    ELM::SNICAR::InitTimestep (Land.urbpoi,
      flg_slr_in, coszen[idx], h2osno[idx], snl[idx], h2osoi_liq[idx], h2osoi_ice[idx],
      snw_rds[idx], snl_top[idx], snl_btm[idx], flx_abs_lcl[idx], flx_absi_snw[idx],
      flg_nosnl[idx], h2osoi_ice_lcl[idx], h2osoi_liq_lcl[idx], snw_rds_lcl[idx], mu_not[idx], flx_slrd_lcl[idx],
      flx_slri_lcl[idx]);


    ELM::SNICAR::SnowAerosolMieParams(Land.urbpoi, flg_slr_in, snl_top[idx], snl_btm[idx], coszen[idx], h2osno[idx], snw_rds_lcl[idx],
      h2osoi_ice_lcl[idx], h2osoi_liq_lcl[idx], ss_alb_oc1, asm_prm_oc1, ext_cff_mss_oc1, ss_alb_oc2, asm_prm_oc2, ext_cff_mss_oc2, ss_alb_dst1,
      asm_prm_dst1, ext_cff_mss_dst1, ss_alb_dst2, asm_prm_dst2, ext_cff_mss_dst2, ss_alb_dst3, asm_prm_dst3, ext_cff_mss_dst3, ss_alb_dst4, asm_prm_dst4,
      ext_cff_mss_dst4, ss_alb_snw_drc, asm_prm_snw_drc, ext_cff_mss_snw_drc, ss_alb_snw_dfs, asm_prm_snw_dfs, ext_cff_mss_snw_dfs, ss_alb_bc1,
      asm_prm_bc1, ext_cff_mss_bc1, ss_alb_bc2, asm_prm_bc2, ext_cff_mss_bc2, bcenh, mss_cnc_aer_in_fdb[idx], g_star[idx], omega_star[idx], tau_star[idx]);


    ELM::SNICAR::SnowRadiativeTransfer(Land.urbpoi, flg_slr_in,flg_nosnl[idx], snl_top[idx], snl_btm[idx], coszen[idx], h2osno[idx], mu_not[idx], 
      flx_slrd_lcl[idx], flx_slri_lcl[idx], albsoi[idx], g_star[idx], omega_star[idx], tau_star[idx], albout_lcl[idx], flx_abs_lcl[idx]);


    ELM::SNICAR::SnowAlbedoRadiationFlux(Land.urbpoi, flg_slr_in, snl_top[idx], coszen[idx], mu_not[idx], h2osno[idx], snw_rds_lcl[idx], albsoi[idx],
      albout_lcl[idx], flx_abs_lcl[idx], albsni[idx], flx_absi_snw[idx]);
  }


  ELM::SurfaceAlbedo::GroundAlbedo(Land.urbpoi, coszen[idx], frac_sno[idx], albsod[idx], 
    albsoi[idx], albsnd[idx], albsni[idx], albgrd[idx], albgri[idx]);


  ELM::SurfaceAlbedo::SnowAbsorptionFactor(Land, coszen[idx], frac_sno[idx], albsod[idx], albsoi[idx], albsnd[idx], albsni[idx], flx_absd_snw[idx], 
    flx_absi_snw[idx], flx_absdv[idx], flx_absdn[idx], flx_absiv[idx], flx_absin[idx]);


  ELM::SurfaceAlbedo::CanopyLayerLAI(Land.urbpoi, elai[idx], esai[idx], tlai[idx], tsai[idx], nrad[idx], ncan[idx], tlai_z[idx], tsai_z[idx],
    fsun_z[idx], fabd_sun_z[idx], fabd_sha_z[idx], fabi_sun_z[idx], fabi_sha_z[idx]);


  ELM::SurfaceAlbedo::TwoStream(Land, nrad[idx], coszen[idx], t_veg[idx], fwet[idx], elai[idx], esai[idx], xl, tlai_z[idx], tsai_z[idx], 
    albgrd[idx], albgri[idx], rhol, rhos, taul, taus, vcmaxcintsun[idx], vcmaxcintsha[idx], albd[idx], ftid[idx], ftdd[idx], 
    fabd[idx], fabd_sun[idx], fabd_sha[idx], albi[idx], ftii[idx], fabi[idx], fabi_sun[idx], fabi_sha[idx], fsun_z[idx], fabd_sun_z[idx], 
    fabd_sha_z[idx], fabi_sun_z[idx], fabi_sha_z[idx]);


    snowoptics.compareOutput(ss_alb_oc1);
    snowoptics.compareOutput(asm_prm_oc1);
    snowoptics.compareOutput(ext_cff_mss_oc1);
    snowoptics.compareOutput(ss_alb_oc2);
    snowoptics.compareOutput(asm_prm_oc2);
    snowoptics.compareOutput(ext_cff_mss_oc2);
    snowoptics.compareOutput(ss_alb_dst1);
    snowoptics.compareOutput(asm_prm_dst1);
    snowoptics.compareOutput(ext_cff_mss_dst1);
    snowoptics.compareOutput(ss_alb_dst2);
    snowoptics.compareOutput(asm_prm_dst2);
    snowoptics.compareOutput(ext_cff_mss_dst2);
    snowoptics.compareOutput(ss_alb_dst3);
    snowoptics.compareOutput(asm_prm_dst3);
    snowoptics.compareOutput(ext_cff_mss_dst3);
    snowoptics.compareOutput(ss_alb_dst4);
    snowoptics.compareOutput(asm_prm_dst4);
    snowoptics.compareOutput(ext_cff_mss_dst4);
    snowoptics.compareOutput(ss_alb_snw_drc);
    snowoptics.compareOutput(asm_prm_snw_drc);
    snowoptics.compareOutput(ext_cff_mss_snw_drc);
    snowoptics.compareOutput(ss_alb_snw_dfs);
    snowoptics.compareOutput(asm_prm_snw_dfs);
    snowoptics.compareOutput(ext_cff_mss_snw_dfs);
    snowoptics.compareOutput(ss_alb_bc1);
    snowoptics.compareOutput(asm_prm_bc1);
    snowoptics.compareOutput(ext_cff_mss_bc1);
    snowoptics.compareOutput(ss_alb_bc2);
    snowoptics.compareOutput(asm_prm_bc2);
    snowoptics.compareOutput(ext_cff_mss_bc2);
    snowoptics.compareOutput(bcenh);


    // compare kernel output to ELM output state
    out.compareOutput(snl);
    out.compareOutput(elai);
    out.compareOutput(esai);
    out.compareOutput(tlai);
    out.compareOutput(tsai);
    out.compareOutput(coszen);
    out.compareOutput(frac_sno);
    out.compareOutput(h2osno);
    out.compareOutput(t_veg);
    out.compareOutput(fwet);
    out.compareOutput(mss_cnc_bcphi);
    out.compareOutput(mss_cnc_bcpho);
    out.compareOutput(mss_cnc_dst1);
    out.compareOutput(mss_cnc_dst2);
    out.compareOutput(mss_cnc_dst3);
    out.compareOutput(mss_cnc_dst4);
    out.compareOutput(albsod);
    out.compareOutput(albsoi);
    out.compareOutput(albsnd);
    out.compareOutput(albsni);
    out.compareOutput(h2osoi_liq);
    out.compareOutput(h2osoi_ice);
    out.compareOutput(snw_rds);
    out.compareOutput(xl);
    out.compareOutput(h2osoi_vol);
    out.compareOutput(albsat);
    out.compareOutput(albdry);
    out.compareOutput(rhol);
    out.compareOutput(rhos);
    out.compareOutput(taul);
    out.compareOutput(taus);
    out.compareOutput(albgrd);
    out.compareOutput(albgri);
    out.compareOutput(flx_absdv);
    out.compareOutput(flx_absdn);
    out.compareOutput(flx_absiv);
    out.compareOutput(flx_absin);
    out.compareOutput(tlai_z);
    out.compareOutput(tsai_z);
    out.compareOutput(fsun_z);
    out.compareOutput(fabd_sun_z);
    out.compareOutput(fabd_sha_z);
    out.compareOutput(fabi_sun_z);
    out.compareOutput(fabi_sha_z);
    out.compareOutput(albd);
    out.compareOutput(ftid);
    out.compareOutput(ftdd);
    out.compareOutput(fabd);
    out.compareOutput(fabd_sun);
    out.compareOutput(fabd_sha);
    out.compareOutput(albi);
    out.compareOutput(ftii);
    out.compareOutput(fabi);
    out.compareOutput(fabi_sun);
    out.compareOutput(fabi_sha);
  }
  return 0;
}