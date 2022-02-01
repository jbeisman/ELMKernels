
#pragma once

#include <string>
#include <array>

#include "array.hh"
#include "utils.hh"
#include "read_input.hh"

namespace ELM::snicar_data {

struct SnicarData {
// use this for now - I don't want to template this class
using ArrayD1 = ELM::Array<double, 1>;
using ArrayD2 = ELM::Array<double, 2>;
using ArrayD3 = ELM::Array<double, 3>;

// arrays to store snicar parameters
ArrayD1 ss_alb_oc1;
ArrayD1 asm_prm_oc1;
ArrayD1 ext_cff_mss_oc1;
ArrayD1 ss_alb_oc2;
ArrayD1 asm_prm_oc2;
ArrayD1 ext_cff_mss_oc2;
ArrayD1 ss_alb_dst1;
ArrayD1 asm_prm_dst1;
ArrayD1 ext_cff_mss_dst1;
ArrayD1 ss_alb_dst2;
ArrayD1 asm_prm_dst2;
ArrayD1 ext_cff_mss_dst2;
ArrayD1 ss_alb_dst3;
ArrayD1 asm_prm_dst3;
ArrayD1 ext_cff_mss_dst3;
ArrayD1 ss_alb_dst4;
ArrayD1 asm_prm_dst4;
ArrayD1 ext_cff_mss_dst4;
ArrayD2 ss_alb_snw_drc;
ArrayD2 asm_prm_snw_drc;
ArrayD2 ext_cff_mss_snw_drc;
ArrayD2 ss_alb_snw_dfs;
ArrayD2 asm_prm_snw_dfs;
ArrayD2 ext_cff_mss_snw_dfs;
ArrayD2 ss_alb_bc1;
ArrayD2 asm_prm_bc1;
ArrayD2 ext_cff_mss_bc1;
ArrayD2 ss_alb_bc2;
ArrayD2 asm_prm_bc2;
ArrayD2 ext_cff_mss_bc2;
ArrayD3 bcenh;

SnicarData();
};

// read and manipulate data
template <typename ArrayT, typename T, size_t D>
void read_and_fill_array(const Comm_type &comm, const std::string &filename, const std::string &varname,
                 Array<T, D> &arr_for_read, ArrayT& arrout);

// read all fields in SnicarData
void read_snicar_data(const Comm_type &comm, const std::string& filename, SnicarData *snicar_data);

} // namespace ELM::snicar_data

#include "snicar_data_impl.hh"
