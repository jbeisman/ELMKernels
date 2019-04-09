#include <array>
#include <sstream>
#include <iterator>
#include <exception>
#include <string>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <Kokkos_Core.hpp>
#include "utils.hh"
#include "readers.hh"
#include "CanopyHydrology_cpp.hh" 


namespace ELM {
namespace Utils {

static const int n_months = 12;
static const int n_pfts = 17;
static const int n_max_times = 31 * 24 * 2; // max days per month times hours per
                                            // day * half hour timestep
static const int n_grid_cells = 24;

using MatrixState = MatrixStatic<n_grid_cells, n_pfts>;
using MatrixForc = MatrixStatic<n_max_times,n_grid_cells>;


} // namespace
} // namespace


int main(int argc, char ** argv)
{
  using ELM::Utils::n_months;
  using ELM::Utils::n_pfts;
  using ELM::Utils::n_grid_cells;
  using ELM::Utils::n_max_times;
  
  // fixed magic parameters for now
  const int ctype = 1;
  const int ltype = 1;
  const bool urbpoi = false;
  const bool do_capsnow = false;
  const int frac_veg_nosno = 1;
  int n_irrig_steps_left = 0;

  const double dewmx = 0.1;
  double dtime = 1800.0;

  Kokkos::initialize( argc, argv );
  {

  typedef Kokkos::View<double**>  ViewMatrixType;
  typedef Kokkos::Cuda ExecSpace;
  typedef Kokkos::Cuda MemSpace;
  typedef Kokkos::RangePolicy<ExecSpace> range_policy;
 
  ViewMatrixType elai( "elai", n_months, n_pfts );
  ViewMatrixType esai( "esai", n_months, n_pfts );
  ViewMatrixType::HostMirror h_elai = Kokkos::create_mirror_view( elai );
  ViewMatrixType::HostMirror h_esai = Kokkos::create_mirror_view( esai );

  // phenology state
  // ELM::Utils::MatrixState elai;
  // ELM::Utils::MatrixState esai;
  ELM::Utils::read_phenology("../links/surfacedataWBW.nc", n_months, n_pfts, 0, h_elai, h_esai);
  ELM::Utils::read_phenology("../links/surfacedataBRW.nc", n_months, n_pfts, n_months, h_elai, h_esai);

  // forcing state
  // ELM::Utils::MatrixForc forc_rain;
  // ELM::Utils::MatrixForc forc_snow;
  // ELM::Utils::MatrixForc forc_air_temp;

  ViewMatrixType forc_rain( "forc_rain", n_max_times,n_grid_cells );
  ViewMatrixType forc_snow( "forc_snow", n_max_times,n_grid_cells );
  ViewMatrixType forc_air_temp( "forc_air_temp", n_max_times,n_grid_cells );
  ViewMatrixType::HostMirror h_forc_rain = Kokkos::create_mirror_view( forc_rain );
  ViewMatrixType::HostMirror h_forc_snow = Kokkos::create_mirror_view( forc_snow );
  ViewMatrixType::HostMirror h_forc_air_temp = Kokkos::create_mirror_view( forc_air_temp );
  const int n_times = ELM::Utils::read_forcing("../links/forcing", n_max_times, 0, n_grid_cells, h_forc_rain, h_forc_snow, h_forc_air_temp);

  ELM::Utils::MatrixForc forc_irrig; forc_irrig = 0.;
  
  // output state by the grid cell
  // auto qflx_prec_intr = std::array<double,n_grid_cells>();
  // auto qflx_irrig = std::array<double,n_grid_cells>();
  // auto qflx_prec_grnd = std::array<double,n_grid_cells>();
  // auto qflx_snwcp_liq = std::array<double,n_grid_cells>();
  // auto qflx_snwcp_ice = std::array<double,n_grid_cells>();
  // auto qflx_snow_grnd_patch = std::array<double,n_grid_cells>();
  // auto qflx_rain_grnd = std::array<double,n_grid_cells>();
  // auto qflx_prec_intr = ELM::Utils::MatrixState();
  // auto qflx_irrig = ELM::Utils::MatrixState();
  // auto qflx_prec_grnd = ELM::Utils::MatrixState();
  // auto qflx_snwcp_liq = ELM::Utils::MatrixState();
  // auto qflx_snwcp_ice = ELM::Utils::MatrixState();
  // auto qflx_snow_grnd_patch = ELM::Utils::MatrixState();
  // auto qflx_rain_grnd = ELM::Utils::MatrixState();
  ViewMatrixType qflx_prec_intr( "qflx_prec_intr", n_grid_cells, n_pfts );
  ViewMatrixType qflx_irrig( "qflx_irrig", n_grid_cells, n_pfts  );
  ViewMatrixType qflx_prec_grnd( "qflx_prec_grnd", n_grid_cells, n_pfts  );
  ViewMatrixType qflx_snwcp_liq( "qflx_snwcp_liq", n_grid_cells, n_pfts );
  ViewMatrixType qflx_snwcp_ice ( "qflx_snwcp_ice ", n_grid_cells, n_pfts  );
  ViewMatrixType qflx_snow_grnd_patch( "qflx_snow_grnd_patch", n_grid_cells, n_pfts  );
  ViewMatrixType qflx_rain_grnd( "qflx_rain_grnd", n_grid_cells, n_pfts  );
  ViewMatrixType::HostMirror h_qflx_prec_intr = Kokkos::create_mirror_view( qflx_prec_intr );
  ViewMatrixType::HostMirror h_qflx_irrig = Kokkos::create_mirror_view( qflx_irrig);
  ViewMatrixType::HostMirror h_qflx_prec_grnd = Kokkos::create_mirror_view( qflx_prec_grnd);
  ViewMatrixType::HostMirror h_qflx_snwcp_liq = Kokkos::create_mirror_view(  qflx_snwcp_liq);
  ViewMatrixType::HostMirror h_qflx_snwcp_ice = Kokkos::create_mirror_view( qflx_snwcp_ice   );
  ViewMatrixType::HostMirror h_qflx_snow_grnd_patch = Kokkos::create_mirror_view( qflx_snow_grnd_patch );
  ViewMatrixType::HostMirror h_qflx_rain_grnd = Kokkos::create_mirror_view(  qflx_rain_grnd  );

  // output state by the pft
  ViewMatrixType h2o_can( "h2o_can", n_grid_cells, n_pfts );
  //auto h2o_can = ELM::Utils::MatrixState(); 
  ViewMatrixType::HostMirror h_h2o_can = Kokkos::create_mirror_view( h2o_can );
  //h_h2o_can = 0.;
  auto h2o_can1 = ELM::Utils::MatrixState(); 
  // Array<int64_t, 2> a = h_h2o_can;

  //   const int64_t begin0 = h_h2o_can.begin();
  //   const int64_t end0= h_h2o_can.end();
  Kokkos::deep_copy( elai, h_elai);
  Kokkos::deep_copy( esai, h_esai);
  Kokkos::deep_copy( forc_rain, h_forc_rain);
  Kokkos::deep_copy( forc_snow, h_forc_snow);
  Kokkos::deep_copy( forc_air_temp, h_forc_air_temp);
  Kokkos::deep_copy( qflx_prec_intr, h_qflx_prec_intr);
  Kokkos::deep_copy( qflx_irrig, h_qflx_irrig);
  Kokkos::deep_copy( qflx_prec_grnd, h_qflx_prec_grnd);
  Kokkos::deep_copy( qflx_snwcp_liq, h_qflx_snwcp_liq);
  Kokkos::deep_copy( qflx_snwcp_ice, h_qflx_snwcp_ice);
  Kokkos::deep_copy( qflx_snow_grnd_patch, h_qflx_snow_grnd_patch);
  Kokkos::deep_copy( qflx_rain_grnd, h_qflx_rain_grnd);
  Kokkos::deep_copy( h2o_can, h_h2o_can);

  

  std::cout << "Time\t Total Canopy Water\t Min Water\t Max Water" << std::endl;
  auto min_max = std::minmax_element(h2o_can1.begin(), h2o_can1.end());
  std::cout << std::setprecision(16)
            << 0 << "\t" << std::accumulate(h2o_can1.begin(), h2o_can1.end(), 0.)
            << "\t" << *min_max.first
            << "\t" << *min_max.second << std::endl;

  



  
  // main loop
  // -- the timestep loop cannot/should not be parallelized
  for (size_t t = 0; t != n_times; ++t) {

    // // grid cell and/or pft loop can be parallelized
    // Kokkos::parallel_for(range_policy(0,n_grid_cells), KOKKOS_LAMBDA ( size_t g ) {
    // //for (size_t g = 0; g != n_grid_cells; ++g) {
    //   //Kokkos::parallel_for(range_policy(0,n_pfts), KOKKOS_LAMBDA ( size_t p ) { 
    //     for (size_t p = 0; p != n_pfts; ++p) {
    //     // NOTE: this currently punts on what to do with the qflx variables!
    //     // Surely they should be either accumulated or stored on PFTs as well.
    //     // --etc

    // Kokkos::parallel_for("CanopyHydrology_Interception", Kokkos::MDRangePolicy<Kokkos::Rank<2,Kokkos::Iterate::Left>>({0,0},{n_grid_cells,n_pfts}),
    //    KOKKOS_LAMBDA (size_t g, size_t p) {
    Kokkos::parallel_for("InitA", n_grid_cells, KOKKOS_LAMBDA (const size_t& g) {
      for (size_t p = 0; p != n_pfts; ++p) {
        ELM::CanopyHydrology_Interception(dtime,
                forc_rain(t,g), forc_snow(t,g), forc_irrig(t,g),
                ltype, ctype, urbpoi, do_capsnow,
                elai(g,p), esai(g,p), dewmx, frac_veg_nosno,
                h2o_can(g,p), n_irrig_steps_left,
                qflx_prec_intr(g,p), qflx_irrig(g,p), qflx_prec_grnd(g,p),
                qflx_snwcp_liq(g,p), qflx_snwcp_ice(g,p),
                qflx_snow_grnd_patch(g,p), qflx_rain_grnd(g,p));

                // qflx_prec_intr[g], qflx_irrig[g], qflx_prec_grnd[g],
                // qflx_snwcp_liq[g], qflx_snwcp_ice[g],
                // qflx_snow_grnd_patch[g], qflx_rain_grnd[g]);
        //printf("%i %i %16.8g %16.8g %16.8g %16.8g %16.8g %16.8g\n", g, p, forc_rain(t,g), forc_snow(t,g), elai(g,p), esai(g,p), h2o_can(g,p), qflx_prec_intr[g]);
      //}//)
      }
    });

    auto min_max = std::minmax_element(h2o_can1.begin(), h2o_can1.end());
    std::cout << std::setprecision(16)
              << t+1 << "\t" << std::accumulate(h2o_can1.begin(), h2o_can1.end(), 0.)
              << "\t" << *min_max.first
              << "\t" << *min_max.second << std::endl;

  }
  }
  Kokkos::finalize();
  return 0;
}
