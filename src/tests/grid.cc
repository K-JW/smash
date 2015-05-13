/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "setup.h"
#include "../include/grid.h"
#include "../include/logging.h"

#include <set>
#include <unordered_set>

using namespace Smash;

namespace std {
// helpers for printing sets/pairs on test failure
template <typename T, typename U>
ostream &operator<<(ostream &s, const pair<T, U> &p) {
  return s << '<' << p.first << '|' << p.second << '>';
}
template <typename T>
ostream &operator<<(ostream &s, const set<T> &set) {
  s << '{';
  for (auto &&x : set) {
    s << x << ' ';
  }
  return s << '}';
}
}  // namespace std

TEST(init) {
  set_default_loglevel(einhard::WARN);
  //create_all_loggers("Grid: DEBUG");
  Test::create_smashon_particletypes();
}

// Grid tests:
// - Test that the number of cells is determined correctly. I.e. what is
// expected for a specific volume of particles.
// - Test that the contents of each cell are correct.
// - Don't use random values! For a given set of positions an expected grid
// configuration can be written out. For random positions the expected grid must
// be determined programatically which can lead to errors in the the test code
// itself masking actual errors in the grid code.

TEST(grid_construction) {
  struct Parameter {
    // input:
    ParticleList particles;

    // expected results:
    std::size_t cellcount[3];  // per direction
    std::set<std::pair<int,int>> neighbors;
    std::vector<std::unordered_set<int>> ids;
  };
  auto &&make_particle = [](double x, double y, double z, int id) {
    return Test::smashon(Test::Position{0., x, y, z}, id);
  };
  for (const int testparticles : {1, 5, 20, 100}) {
    const double max_interaction_length =
        GridBase::min_cell_length(testparticles);
    for (const Parameter &param : std::vector<Parameter>{
             {{make_particle(0., 0., 0., 1), make_particle(1.9, 1.9, 1.9, 2)},
              {1, 1, 1},
              {},
              {{1, 2}}},
             {{make_particle(0, 0, 0, 1), make_particle(0, 0, 1, 2),
               make_particle(0, 0, 2, 3), make_particle(0, 1, 0, 4),
               make_particle(0, 1, 1, 5), make_particle(0, 1, 2, 6),
               make_particle(0, 2, 0, 7), make_particle(0, 2, 1, 8),
               make_particle(0, 2, 2, 9), make_particle(1, 0, 0, 10),
               make_particle(1, 0, 1, 11), make_particle(1, 0, 2, 12),
               make_particle(1, 1, 0, 13), make_particle(1, 1, 1, 14),
               make_particle(1, 1, 2, 15), make_particle(1, 2, 0, 16),
               make_particle(1, 2, 1, 17), make_particle(1, 2, 2, 18),
               make_particle(2, 0, 0, 19), make_particle(2, 0, 1, 20),
               make_particle(2, 0, 2, 21), make_particle(2, 1, 0, 22),
               make_particle(2, 1, 1, 23), make_particle(2, 1, 2, 24),
               make_particle(2, 2, 0, 25), make_particle(2, 2, 1, 26),
               make_particle(2, 2, 2, 27)},
              {3, 3, 3},
              {{1, 2}, {1, 4}, {1, 5}, {1, 10}, {1, 11}, {1, 13}, {1, 14},
               {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 10}, {2, 11}, {2, 12}, {2, 13}, {2, 14}, {2, 15},
               {3, 5}, {3, 6}, {3, 11}, {3, 12}, {3, 14}, {3, 15}, {4, 5},
               {4, 7}, {4, 8}, {4, 10}, {4, 11}, {4, 13}, {4, 14}, {4, 16}, {4, 17}, {5, 6},
               {5, 7}, {5, 8}, {5, 9}, {5, 10}, {5, 11}, {5, 12}, {5, 13}, {5, 14}, {5, 15}, {5, 16}, {5, 17}, {5, 18},
               {6, 8}, {6, 9}, {6, 11}, {6, 12}, {6, 14}, {6, 15}, {6, 17}, {6, 18},
               {7, 8}, {7, 13}, {7, 14}, {7, 16}, {7, 17},
               {8, 9}, {8, 13}, {8, 14}, {8, 15}, {8, 16}, {8, 17}, {8, 18},
               {9, 14}, {9, 15}, {9, 17}, {9, 18},
               {10, 11}, {10, 13}, {10, 14}, {10, 19}, {10, 20}, {10, 22}, {10, 23},
               {11, 12}, {11, 13}, {11, 14}, {11, 15}, {11, 19}, {11, 20}, {11, 21}, {11, 22}, {11, 23}, {11, 24},
               {12, 14}, {12, 15}, {12, 20}, {12, 21}, {12, 23}, {12, 24},
               {13, 14}, {13, 16}, {13, 17}, {13, 19}, {13, 20}, {13, 22}, {13, 23}, {13, 25}, {13, 26},
               {14, 15}, {14, 16}, {14, 17}, {14, 18}, {14, 19}, {14, 20}, {14, 21}, {14, 22}, {14, 23}, {14, 24}, {14, 25}, {14, 26}, {14, 27},
               {15, 17}, {15, 18}, {15, 20}, {15, 21}, {15, 23}, {15, 24}, {15, 26}, {15, 27},
               {16, 17}, {16, 22}, {16, 23}, {16, 25}, {16, 26},
               {17, 18}, {17, 22}, {17, 23}, {17, 24}, {17, 25}, {17, 26}, {17, 27},
               {18, 23}, {18, 24}, {18, 26}, {18, 27},
               {19, 20}, {19, 22}, {19, 23},
               {20, 21}, {20, 22}, {20, 23}, {20, 24},
               {21, 23}, {21, 24},
               {22, 23}, {22, 25}, {22, 26},
               {23, 24}, {23, 25}, {23, 26}, {23, 27},
               {24, 26}, {24, 27},
               {25, 26},
               {26, 27}},
              {{ 1}, {10}, {19}, { 4}, {13}, {22}, { 7}, {16}, {25},
               { 2}, {11}, {20}, { 5}, {14}, {23}, { 8}, {17}, {26},
               { 3}, {12}, {21}, { 6}, {15}, {24}, { 9}, {18}, {27}}},
         }) {
      ParticleList list;
      list.reserve(param.particles.size());
      for (auto p : param.particles) {
        p.set_4position(max_interaction_length * p.position());
        list.push_back(std::move(p));
      }
      Grid<GridOptions::Normal> grid(std::move(list), testparticles);
      auto idsIt = param.ids.begin();
      auto neighbors = param.neighbors;
      grid.iterate_cells([&](
          const ParticleList &search,
          const std::vector<const ParticleList *> &neighborLists) {
        auto ids = *idsIt++;
        for (const auto &p : search) {
          for (const auto &n : neighborLists) {
            for (const auto &p2 : *n) {
              COMPARE(neighbors.erase({std::min(p.id(), p2.id()),
                                       std::max(p.id(), p2.id())}),
                      1u)
                  << "<id|id>: <" << std::min(p.id(), p2.id()) << '|'
                  << std::max(p.id(), p2.id()) << '>';
            }
          }
          COMPARE(ids.erase(p.id()), 1u) << "p.id() = " << p.id();
        }
        COMPARE(ids.size(), 0u);
      });
      COMPARE(neighbors.size(), 0u) << neighbors;
    }
  }
}

template <typename Container, typename T>
typename Container::const_iterator find(const Container &c, const T &value) {
  return std::find(std::begin(c), std::end(c), value);
}

namespace std {
template <typename T, typename U>
std::ostream &operator<<(std::ostream &s,
                         const std::vector<std::pair<T, U>> &l) {
  for (auto &&p : l) {
    s << "\n<" << p.first << ", " << p.second << '>';
  }
  return s;
}
}  // namespace std

TEST(periodic_grid) {
  using Test::Position;
  using Test::Momentum;
  for (const int testparticles : {1, 5}) {
    for (const int nparticles : {1,/* 5, 20, 75,*/ 124, 125}) {
      const double max_interaction_length =
          GridBase::min_cell_length(testparticles);
      constexpr float length = 10;
      ParticleList list;
      auto random_value = Random::make_uniform_distribution(0., 9.99);
      for (auto n = nparticles; n; --n) {
        list.push_back(Test::smashon(
            Position{0., random_value(), random_value(), random_value()},
            Momentum{Test::smashon_mass,
                     {random_value(), random_value(), random_value()}},
            n));
      }
      Grid<GridOptions::PeriodicBoundaries> grid(
          make_pair(std::array<float, 3>{0, 0, 0},
                    std::array<float, 3>{length, length, length}),
          ParticleList(list),  // make a temp copy which gets moved
          testparticles);

      // stores the neighbor pairs found via the grid:
      std::vector<std::pair<ParticleData, ParticleData>> neighbor_pairs;

      grid.iterate_cells([&](
          const ParticleList &search,
          const std::vector<const ParticleList *> &neighborLists) {
        // combine all neighbor particles into a single list
        ParticleList combinedNeighbors;
        for (auto &&neighbors : neighborLists) {
          if (neighbors) {
            for (auto &&n : *neighbors) {
              combinedNeighbors.push_back(n);
            }
          }
        }

        // for each particle in neighborLists, find the same particle in list
        auto &&compareDiff = [](float d) {
          if (d < 0.) {
            FUZZY_COMPARE(d, -length);
          } else if (d > 0.) {
            FUZZY_COMPARE(d, length);
          } else {
            COMPARE(d, 0.);
          }
        };
        for (const ParticleData &p : combinedNeighbors) {
          const auto it = find(list, p);
          VERIFY(it != list.end());
          COMPARE(it->id(), p.id());
          if (it->position() != p.position()) {
            // then the cell was wrapped around
            const auto diff = it->position() - p.position();
            COMPARE(diff[0], 0.);
            compareDiff(diff[1]);
            compareDiff(diff[2]);
            compareDiff(diff[3]);
            VERIFY(diff != FourVector(0, 0, 0, 0));
          }
        }

        // for each particle in search, find the same particle in list
        for (const ParticleData &p : search) {
          const auto it = find(list, p);
          VERIFY(it != list.end());
          COMPARE(it->id(), p.id());
          COMPARE(it->position(), p.position());
        }

        // for each particle in search, search through the complete list of
        // neighbors to find those closer than 2.5fm
        for (const ParticleData &p : search) {
          for (const ParticleData &q : search) {
            if (p == q) {
              continue;
            }
            const auto sqrDistance =
                (p.position().threevec() - q.position().threevec()).sqr();
            if (sqrDistance <=
                max_interaction_length * max_interaction_length) {
              const auto pair =
                  p.id() < q.id() ? std::make_pair(p, q) : std::make_pair(q, p);
              neighbor_pairs.emplace_back(std::move(pair));
            }
          }
          for (const ParticleData &q : combinedNeighbors) {
            VERIFY(!(p == q)) << "\np: " << p << "\nq: " << q << '\n' << search
                              << '\n' << combinedNeighbors;
            const auto sqrDistance =
                (p.position().threevec() - q.position().threevec()).sqr();
            if (sqrDistance <=
                max_interaction_length * max_interaction_length) {
              auto pair =
                  p.id() < q.id() ? std::make_pair(p, q) : std::make_pair(q, p);
              const auto it = find(neighbor_pairs, pair);
              COMPARE(it, neighbor_pairs.end()) << "\np: " << p << "\nq: " << q
                                                << '\n' << neighbor_pairs;
              neighbor_pairs.emplace_back(std::move(pair));
            }
          }
        }
      });

      // Now search through the original list to verify the grid search found
      // everything.
      auto &&wrap = [&length](ParticleData p, int i) {
        auto pos = p.position();
        if (i > 0) {
          pos[i] += length;
        } else {
          pos[-i] -= length;
        }
        p.set_4position(pos);
        return p;
      };
      for (ParticleData p0 : list) {
        ParticleList p_periodic;
        p_periodic.push_back(p0);
        if (p0.position()[3] < max_interaction_length) {
          p_periodic.push_back(wrap(p0, 3));
        }
        if (p0.position()[3] > length - max_interaction_length) {
          p_periodic.push_back(wrap(p0, -3));
        }
        if (p0.position()[2] < max_interaction_length) {
          p_periodic.push_back(wrap(p0, 2));
          if (p0.position()[3] < max_interaction_length) {
            p_periodic.push_back(wrap(p0, 3));
          }
          if (p0.position()[3] > length - max_interaction_length) {
            p_periodic.push_back(wrap(p0, -3));
          }
        }
        if (p0.position()[2] > length - max_interaction_length) {
          p_periodic.push_back(wrap(p0, -2));
          if (p0.position()[3] < max_interaction_length) {
            p_periodic.push_back(wrap(p0, 3));
          }
          if (p0.position()[3] > length - max_interaction_length) {
            p_periodic.push_back(wrap(p0, -3));
          }
        }
        if (p0.position()[1] < max_interaction_length) {
          p_periodic.push_back(wrap(p0, 1));
          if (p0.position()[3] < max_interaction_length) {
            p_periodic.push_back(wrap(p0, 3));
          }
          if (p0.position()[3] > length - max_interaction_length) {
            p_periodic.push_back(wrap(p0, -3));
          }
          if (p0.position()[2] < max_interaction_length) {
            p_periodic.push_back(wrap(p0, 2));
            if (p0.position()[3] < max_interaction_length) {
              p_periodic.push_back(wrap(p0, 3));
            }
            if (p0.position()[3] > length - max_interaction_length) {
              p_periodic.push_back(wrap(p0, -3));
            }
          }
          if (p0.position()[2] > length - max_interaction_length) {
            p_periodic.push_back(wrap(p0, -2));
            if (p0.position()[3] < max_interaction_length) {
              p_periodic.push_back(wrap(p0, 3));
            }
            if (p0.position()[3] > length - max_interaction_length) {
              p_periodic.push_back(wrap(p0, -3));
            }
          }
        }
        if (p0.position()[1] > length - max_interaction_length) {
          p_periodic.push_back(wrap(p0, -1));
          if (p0.position()[3] < max_interaction_length) {
            p_periodic.push_back(wrap(p0, 3));
          }
          if (p0.position()[3] > length - max_interaction_length) {
            p_periodic.push_back(wrap(p0, -3));
          }
          if (p0.position()[2] < max_interaction_length) {
            p_periodic.push_back(wrap(p0, 2));
            if (p0.position()[3] < max_interaction_length) {
              p_periodic.push_back(wrap(p0, 3));
            }
            if (p0.position()[3] > length - max_interaction_length) {
              p_periodic.push_back(wrap(p0, -3));
            }
          }
          if (p0.position()[2] > length - max_interaction_length) {
            p_periodic.push_back(wrap(p0, -2));
            if (p0.position()[3] < max_interaction_length) {
              p_periodic.push_back(wrap(p0, 3));
            }
            if (p0.position()[3] > length - max_interaction_length) {
              p_periodic.push_back(wrap(p0, -3));
            }
          }
        }

        for (const ParticleData &p : p_periodic) {
          for (const ParticleData &q : list) {
            if (p == q) {
              continue;
            }
            const auto sqrDistance =
                (p.position().threevec() - q.position().threevec()).sqr();
            if (sqrDistance <=
                max_interaction_length * max_interaction_length) {
              // (p,q) must be in neighbor_pairs
              auto pair =
                  p.id() > q.id() ? std::make_pair(q, p) : std::make_pair(p, q);
              const auto it = find(neighbor_pairs, pair);
              VERIFY(it != neighbor_pairs.end())
                  << "\ntestparticles: " << testparticles
                  << "\nnparticles: " << nparticles << "\np: " << p
                  << "\nq: " << q;  // << "\n" << neighbor_pairs;
            }
          }
        }
      }
    }
  }
}
