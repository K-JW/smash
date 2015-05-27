/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/grid.h"

#include <stdexcept>

#include "include/algorithms.h"
#include "include/fourvector.h"
#include "include/logging.h"
#include "include/particledata.h"
#include "include/threevector.h"

namespace std {
template <typename T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
  auto column = out.tellp();
  out << "{ ";
  for (const auto &x : v) {
    if (out.tellp() - column >= 100) {
      out << '\n';
      column = out.tellp();
    }
    out << x << ' ';
  }
  return out << '}';
}

template <typename T>
std::ostream &operator<<(std::ostream &out, const std::initializer_list<T> &v) {
  auto column = out.tellp();
  out << "{ ";
  for (const auto &x : v) {
    if (out.tellp() - column >= 100) {
      out << '\n';
      column = out.tellp();
    }
    out << x << ' ';
  }
  return out << '}';
}

template <typename T, std::size_t N>
std::ostream &operator<<(std::ostream &out, const std::array<T, N> &a) {
  auto column = out.tellp();
  out << "{ ";
  for (const auto &x : a) {
    if (out.tellp() - column >= 100) {
      out << '\n';
      column = out.tellp();
    }
    out << x << ' ';
  }
  return out << '}';
}
}  // namespace std

namespace Smash {

////////////////////////////////////////////////////////////////////////////////
// GridBase

std::pair<std::array<float, 3>, std::array<float, 3>>
GridBase::find_min_and_length(const ParticleList &all_particles) {
  std::pair<std::array<float, 3>, std::array<float, 3>> r;

  // intialize min and max position arrays with the position of the first
  // particle in the list
  const auto first = all_particles.begin()->position().threevec();
  r.first = {{static_cast<float>(first[0]), static_cast<float>(first[1]),
              static_cast<float>(first[2])}};
  r.second = r.first;
  for (const auto &p : all_particles) {
    const auto pos = p.position().threevec();
    r.first[0] = std::min(r.first[0], static_cast<float>(pos[0]));
    r.first[1] = std::min(r.first[1], static_cast<float>(pos[1]));
    r.first[2] = std::min(r.first[2], static_cast<float>(pos[2]));
    r.second[0] = std::max(r.second[0], static_cast<float>(pos[0]));
    r.second[1] = std::max(r.second[1], static_cast<float>(pos[1]));
    r.second[2] = std::max(r.second[2], static_cast<float>(pos[2]));
  }
  r.second[0] = r.second[0] - r.first[0];
  r.second[1] = r.second[1] - r.first[1];
  r.second[2] = r.second[2] - r.first[2];
  return r;
}

template <GridOptions O>
std::tuple<std::array<float, 3>, std::array<int, 3>>
Grid<O>::determine_cell_sizes(size_type particle_count,
                              const std::array<float, 3> &length,
                              const int testparticles) {
  std::tuple<std::array<float, 3>, std::array<int, 3> > r;
  auto &index_factor = std::get<0>(r);
  auto &number_of_cells = std::get<1>(r);

  float max_interaction_length = min_cell_length(testparticles);

  // The number of cells is determined by the min and max coordinates where
  // particles are positioned and the maximal interaction length (which equals
  // the length of a cell).
  // But don't let the number of cells exceed the actual number of particles.
  // That would be overkill. Let max_cells³ ≤ particle_count (conversion to
  // int truncates).
  // Consider that particle placement into cells uses half-open intervals. Thus
  // a cell includes particles in [0, a[. The next cell [a, 2a[. And so on. This
  // is important for calculating the number of cells. If length * index_factor
  // (equivalent to length / max_interaction_length) is integral, then
  // length * index_factor + 1 determines the number of required cells. That's
  // because the last cell will then store particles in the interval
  // [length, length + max_interaction_length[. The code below achieves this
  // effect by rounding down (floor) and adding 1 afterwards.
  // --------------------
  // TODO(mkretz):
  // The last cell in each direction can be smaller than
  // max_interaction_length. In that case periodic boundaries will not work
  // correctly. Thus, we need to reduce the number of cells in that direction
  // by one and make the last cell larger. This basically merges a smaller
  // boundary cell into a full cell inside the grid.
  const int max_cells = std::cbrt(particle_count);
  for (std::size_t i = 0; i < number_of_cells.size(); ++i) {
    index_factor[i] = 1.f / max_interaction_length;
    number_of_cells[i] =
        static_cast<int>(std::floor(length[i] * index_factor[i])) + 1;
    if (number_of_cells[i] > max_cells) {
      number_of_cells[i] = max_cells;
      index_factor[i] = (max_cells - 0.1f)  // -0.1 for safety margin
                        / length[i];
    }
    if (O == GridOptions::PeriodicBoundaries && number_of_cells[i] == 1) {
      number_of_cells[i] = 2;
      index_factor[i] = 1.999f / length[i];
    }
  }
  return r;
}

////////////////////////////////////////////////////////////////////////////////
// Grid<Options>

template <GridOptions Options>
inline typename Grid<Options>::size_type Grid<Options>::make_index(
    size_type x, size_type y, size_type z) const {
  return (z * number_of_cells_[1] + y) * number_of_cells_[0] + x;
}

template <GridOptions Options>
inline typename Grid<Options>::size_type Grid<Options>::make_index(
    const ThreeVector &position) const {
  return make_index(
      std::floor((static_cast<float>(position[0]) - min_position_[0]) *
                 index_factor_[0]),
      std::floor((static_cast<float>(position[1]) - min_position_[1]) *
                 index_factor_[1]),
      std::floor((static_cast<float>(position[2]) - min_position_[2]) *
                 index_factor_[2]));
}

template <GridOptions O>
void Grid<O>::build_cells(ParticleList &&all_particles,
                          const std::array<float, 3> &length) {
  const auto &log = logger<LogArea::Grid>();
  if (all_of(number_of_cells_, [](size_type n) { return n <= 2; })) {
    // dilute limit:
    // the grid would have <= 2x2x2 cells, meaning every particle has to be
    // compared with every other particle anyway. Then we can just as well
    // fall back to not using the grid at all
    log.debug("There would only be ", number_of_cells_,
              " cells. Therefore the Grid falls back to a single cell / "
              "particle list.");
    number_of_cells_ = {1, 1, 1};
    cells_.reserve(1);
    cells_.emplace_back(std::move(all_particles));
  } else {
    // construct a normal grid
    log.debug("min: ", min_position_, "\nlength: ", length, "\ncells: ",
              number_of_cells_, "\nindex_factor: ", index_factor_);

    // After the grid parameters are determined, we can start placing the
    // particles in cells.
    cells_.resize(number_of_cells_[0] * number_of_cells_[1] *
                  number_of_cells_[2]);

    for (const auto &p : all_particles) {
      const auto idx = make_index(p.position().threevec());
#ifndef NDEBUG
      if (idx >= size_type(cells_.size())) {
        log.fatal(source_location,
                  "\nan out-of-bounds access would be necessary for the "
                  "particle ",
                  p, "\nfor a grid with the following parameters:\nmin: ",
                  min_position_, "\nlength: ", length, "\ncells: ",
                  number_of_cells_, "\nindex_factor: ", index_factor_,
                  "\ncells_.size: ", cells_.size(), "\nrequested index: ", idx);
        throw std::runtime_error("out-of-bounds grid access on construction");
      }
#endif
      cells_[idx].push_back(p);
    }
  }

  log.debug(cells_);
}

template <GridOptions Options>
void Grid<Options>::iterate_cells(
    const std::function<void(const ParticleList &)> &search_cell_callback,
    const std::function<void(const ParticleList &, const ParticleList &)> &
        neighbor_cell_callback) const {
}

template void Grid<GridOptions::Normal>::iterate_cells(
    const std::function<void(const ParticleList &)> &,
    const std::function<void(const ParticleList &, const ParticleList &)> &)
    const;
template void Grid<GridOptions::PeriodicBoundaries>::iterate_cells(
    const std::function<void(const ParticleList &)> &,
    const std::function<void(const ParticleList &, const ParticleList &)> &)
    const;

template std::tuple<std::array<float, 3>, std::array<int, 3>> Grid<
    GridOptions::Normal>::determine_cell_sizes(size_type,
                                               const std::array<float, 3> &,
                                               const int);
template std::tuple<std::array<float, 3>, std::array<int, 3>>
    Grid<GridOptions::PeriodicBoundaries>::determine_cell_sizes(
        size_type, const std::array<float, 3> &, const int);

template void Grid<GridOptions::Normal>::build_cells(
    ParticleList &&, const std::array<float, 3> &);
template void Grid<GridOptions::PeriodicBoundaries>::build_cells(
    ParticleList &&, const std::array<float, 3> &);
}  // namespace Smash
