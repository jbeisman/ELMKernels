#ifndef ELM_UTILS_READERS_HH_
#define ELM_UTILS_READERS_HH_

#include "../utils/array.hh"

// Example of what the reader interface might look like?
namespace ELM {
namespace IO {

//
// Returns shape in file, as a pair (N_TIMES, NX_GLOBAL, NY_GLOBAL)
//
std::tuple<std::size_t, std::size_t, std::size_t>
get_dimensions(const std::string& fname,
               std::size_t start_year, std::size_t start_months, std::size_t n_months);


//
// Read a variable from the phenology file.
//
// Assumes shape(arr) == (N_MONTHS, NX_LOCAL, NY_LOCAL, N_PFT)
//  
void
read_phenology(const MPI_Comm& comm,
               const std::string& fname, const std::string& phenology_type,
               size_t start_year, size_t start_month,
               size_t i_beg, size_t j_beg, ELM::Utils::Array<double,4>& arr);


//
// Read a variable from the forcing files.
//
// Assumes shape(arr) == (N_TIMES, NX_LOCAL, NY_LOCAL )
//
void
read_forcing(const MPI_Comm& comm,
             const std::string& fname, const std::string& forcing_type,
             size_t start_year, size_t start_month,
             size_t i_beg, size_t j_beg, ELM::Utils::Array<double,3>& arr);

} // namespace
} // namespace


#endif