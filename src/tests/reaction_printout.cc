/*
 *
 *    Copyright (c) 2016
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "setup.h"

#include "../include/isoparticletype.h"
#include "../include/particletype.h"
#include "../include/particledata.h"
#include "../include/scatteraction.h"
#include "../include/scatteractionbaryonbaryon.h"
#include "../include/scatteractionbaryonmeson.h"
#include "../include/scatteractionmesonmeson.h"
#include "../include/scatteractionnucleonkaon.h"
#include "../include/scatteractionnucleonnucleon.h"

using namespace Smash;

static ScatterActionPtr construct_scatter_action(const ParticleData &a,
                                                 const ParticleData &b) {
  const auto &pdg_a = a.pdgcode();
  const auto &pdg_b = b.pdgcode();
  const float time = 1.0f;  // It does not matter here anyway
  ScatterActionPtr act;
  if (a.is_baryon() && b.is_baryon()) {
    if (pdg_a.is_nucleon() && pdg_b.is_nucleon()) {
      act = make_unique<ScatterActionNucleonNucleon>(a, b, time);
    } else {
      act = make_unique<ScatterActionBaryonBaryon>(a, b, time);
    }
  } else if (a.is_baryon() || b.is_baryon()) {
    if ((pdg_a.is_nucleon() && pdg_b.is_kaon()) ||
        (pdg_b.is_nucleon() && pdg_a.is_kaon())) {
      act = make_unique<ScatterActionNucleonKaon>(a, b, time);
    } else {
      act = make_unique<ScatterActionBaryonMeson>(a, b, time);
    }
  } else {
    act = make_unique<ScatterActionMesonMeson>(a, b, time);
  }
  return act;
}

static void remove_substr(std::string& s, const std::string& p) { 
  std::string::size_type n = p.length();
  for (std::string::size_type i = s.find(p); i != std::string::npos; i = s.find(p)) {
    s.erase(i, n);
  }
}

static std::string isoclean(std::string s) {
  remove_substr(s, "⁺");
  remove_substr(s, "⁻");
  remove_substr(s, "⁰");
  return s;
}

TEST(init_particle_types) {
  Test::create_actual_particletypes();
}

TEST(init_decaymodes) {
  Test::create_actual_decaymodes();
}

TEST(printout_possible_channels) {
  const size_t N_isotypes = IsoParticleType::list_all().size();
  const size_t N_pairs = N_isotypes * (N_isotypes - 1) / 2;
  // We do not consider decays here, only 2->n processes, where n > 1, 
  constexpr bool two_to_one = false;
  // We do not set all elastic cross-sections to fixed value
  constexpr float elastic_parameter = -1.0f;
  constexpr bool two_to_two = true, strings_switch = true;
  std::cout << N_isotypes << " iso-particle types." << std::endl;
  std::cout << "They can make " << N_pairs << " pairs." << std::endl;
  
  for (const IsoParticleType &A_isotype : IsoParticleType::list_all()) {
    for (const IsoParticleType &B_isotype : IsoParticleType::list_all()) {
      if (&A_isotype > &B_isotype) {
        continue;
      }
      bool any_nonzero_cs = false;
      std::vector<std::string> r_list;
      for (const ParticleTypePtr A_type : A_isotype.get_states()) {
        for (const ParticleTypePtr B_type : B_isotype.get_states()) {
          if (A_type > B_type) {
            continue;
          }
          ParticleData A(*A_type), B(*B_type);
          A.set_4momentum(A.pole_mass(), 1.0, 0.0, 0.0);
          B.set_4momentum(B.pole_mass(), -1.0, 0.0, 0.0);
          ScatterActionPtr act = construct_scatter_action(A, B);
          act->add_all_processes(elastic_parameter, two_to_one,
                                 two_to_two, strings_switch);
          const float total_cs = act->cross_section();
          if (total_cs <= 0.0) {
            continue;
          }
          any_nonzero_cs = true;
          for (const auto& channel : act->collision_channels()) {
            std::string r = A_type->name() + B_type->name()
                          + std::string("->")
                          + channel->particle_types()[0]->name()
                          + channel->particle_types()[1]->name();
            r_list.push_back(isoclean(r));
          }  
        }
      }
      std::sort(r_list.begin(), r_list.end());
      r_list.erase(std::unique(r_list.begin(), r_list.end()), r_list.end() );
      if (any_nonzero_cs) {
        for (auto r : r_list) {
          std::cout << r << ", ";
        }
        std::cout << std::endl;
      }
    }
  }
}
