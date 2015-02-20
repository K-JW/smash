/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */
#include "unittest.h"

#include <cstdio>
#include <cstring>
#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <include/config.h>

#include "../include/binaryoutputcollisions.h"
#include "../include/binaryoutputparticles.h"
#include "../include/clock.h"
#include "../include/outputinterface.h"
#include "../include/particles.h"
#include "../include/processbranch.h"
#include "../include/random.h"

using namespace Smash;

static const bf::path testoutputpath = bf::absolute(SMASH_TEST_OUTPUT_PATH);
static auto random_value = Random::make_uniform_distribution(-15.0, +15.0);

TEST(directory_is_created) {
  bf::create_directory(testoutputpath);
  VERIFY(bf::exists(testoutputpath));
}

static const float mass_smashon = 0.123;
static std::string mass_str = std::to_string(mass_smashon);
static std::string width_str = "1.200";
static std::string pdg_str = "661";
static std::string smashon_str = "smashon " + mass_str + " "
    + width_str + " " + pdg_str + "\n";
static const int zero = 0;
static const int current_format_version = 2;

static ParticleData create_smashon_particle() {
  ParticleData particle = ParticleData{ParticleType::find(0x661)};
  particle.set_4momentum(mass_smashon, random_value(), random_value(),
                         random_value());
  particle.set_4position(FourVector(random_value(), random_value(),
                                    random_value(), random_value()));
  return particle;
}

/* A set of convienient functions to read binary */

static void read_binary(std::string &s, FILE *file) {
  std::int32_t size = s.size();
  VERIFY(std::fread(&size, sizeof(std::int32_t), 1, file) == 1);
  std::vector<char> buf(size);
  VERIFY(std::fread(&buf[0], 1, size, file) == static_cast<size_t>(size));
  s.assign(&buf[0], size);
}

static void read_binary(FourVector &v, FILE* file) {
  VERIFY(std::fread(v.begin(), sizeof(*v.begin()), 4, file) == 4);
}

static void read_binary(std::int32_t &x, FILE* file) {
  VERIFY(std::fread(&x, sizeof(x), 1, file) == 1);
}

/* Function to read and compare particle */
static bool compare_particle(const ParticleData &p, FILE* file) {
  int id, pdgcode;
  FourVector pos, mom;
  read_binary(mom, file);
  read_binary(pos, file);
  read_binary(pdgcode, file);
  read_binary(id, file);
  // std::cout << p.id() << " " << id << std::endl;
  // std::cout << p.pdgcode().get_decimal() << " " << pdgcode << std::endl;
  return (p.id() == id) && (p.pdgcode().get_decimal() == pdgcode) &&
         (pos == p.position()) && (mom == p.momentum());
}

/* function to read and compare particle block header */
static bool compare_particles_block_header(const int &npart, FILE* file) {
  int npart_read;
  char c_read;
  VERIFY(std::fread(&c_read, sizeof(char), 1, file) == 1);
  read_binary(npart_read, file);
  // std::cout << c_read << std::endl;
  // std::cout << npart_read << " " << npart << std::endl;
  return (c_read == 'p') && (npart_read == npart);
}

/* function to read and compare collision block header */
static bool compare_interaction_block_header(const int &nin,
                                             const int &nout,
                                             double rho,
                                             double weight,
                                             ProcessBranch::ProcessType process_type,
                                             FILE* file) {
  int nin_read, nout_read;
  double rho_read, weight_read;
  char c_read;
  VERIFY(std::fread(&c_read, sizeof(char), 1, file) == 1);
  read_binary(nin_read, file);
  read_binary(nout_read, file);
  VERIFY(std::fread(&rho_read, sizeof(double), 1, file) == 1);
  VERIFY(std::fread(&weight_read, sizeof(double), 1, file) == 1);
///  VERIFY(std::fread(&process_type_read, sizeof(int), 1, file == 1);
  // std::cout << c_read << std::endl;
  // std::cout << nin_read << " " << nin << std::endl;
  // std::cout << nout_read << " " << nout << std::endl;
  // std::cout << rho << std::endl;
  return (c_read == 'i') && (nin_read == nin) && (nout_read == nout)
      && (rho_read == rho) && (weight_read == weight);
}

/* function to read and compare event end line */
static bool compare_final_block_header(const int &ev, FILE* file) {
  int ev_read;
  char c_read;
  VERIFY(std::fread(&c_read, sizeof(char), 1, file) == 1);
  read_binary(ev_read, file);
  return (c_read == 'f') && (ev_read == ev);
}

TEST(fullhistory_format) {
  /* Set the most verbose option */
  Configuration&& op {bf::path {TEST_CONFIG_PATH} / "tests",
                       "test_binary_collisions.yaml"};

  /* Create an instance of binary output */
  BinaryOutputCollisions *bin_output =
                new BinaryOutputCollisions(testoutputpath, std::move(op));
  VERIFY(bf::exists(testoutputpath / "collisions_binary.bin"));

  /* create two smashon particles */
  ParticleType::create_type_list(
      "# NAME MASS[GEV] WIDTH[GEV] PDG\n" + smashon_str);
  Particles particles;
  ParticleData particle = create_smashon_particle();
  particles.add_data(particle);
  ParticleData second_particle = create_smashon_particle();
  particles.add_data(second_particle);

  int event_id = 0;
  /* Write initial state output: the two smashons we created */
  bin_output->at_eventstart(particles, event_id);

  /* Create interaction smashon + smashon -> smashon */
  std::vector<ParticleData> initial_particles, final_particles;
  initial_particles.push_back(particles.data(0));
  initial_particles.push_back(particles.data(1));
  particles.remove(0);
  particles.remove(1);
  ParticleData final_particle = create_smashon_particle();
  particles.add_data(final_particle);
  final_particles.push_back(particles.data(particles.id_max()));
  double rho = 0.123;
  double weight = 3.21;
  ProcessBranch::ProcessType process_type = ProcessBranch::NONE;
  bin_output->at_interaction(initial_particles, final_particles, rho, weight, process_type);

  /* Final state output */
  bin_output->at_eventend(particles, event_id);

  /*
   * Now we have an artificially generated binary output.
   * Let us try if we can read and understand it.
   */

  // Open file as a binary
  FILE * binF;
  binF = fopen((testoutputpath / "collisions_binary.bin").native().c_str(),
                                                                        "rb");
  // Header
  std::vector<char> buf(4);
  std::string magic, smash_version;
  int format_version_number;

  VERIFY(fread(&buf[0], 1, 4, binF) == 4);  // magic number
  magic.assign(&buf[0], 4);
  read_binary(format_version_number, binF);  // format version number
  read_binary(smash_version, binF);  // smash version

  VERIFY(magic == "SMSH");
  VERIFY(format_version_number == current_format_version);
  VERIFY(smash_version == VERSION_MAJOR);

  // particles at event atart: expect two smashons
  int nin, nout, npart;
  npart = 2;  // our two smashons
  VERIFY(compare_particles_block_header(npart, binF));
  VERIFY(compare_particle(initial_particles[0], binF));
  VERIFY(compare_particle(initial_particles[1], binF));

  // interaction:2 smashons -> 1 smashon
  nin = 2;
  nout = 1;
  VERIFY(compare_interaction_block_header(nin, nout, rho, weight, process_type, binF));
  VERIFY(compare_particle(initial_particles[0], binF));
  VERIFY(compare_particle(initial_particles[1], binF));
  VERIFY(compare_particle(final_particles[0], binF));

  // paricles at event end: one smashon
  npart = 1;
  VERIFY(compare_particles_block_header(npart, binF));
  VERIFY(compare_particle(final_particles[0], binF));

  // event end line
  VERIFY(compare_final_block_header(event_id, binF));
}

TEST(particles_format) {
  /* Set the most verbose option */
  Configuration&& op {bf::path {TEST_CONFIG_PATH} / "tests",
                       "test_binary_particles.yaml"};


  /* Create an instance of binary output */
  BinaryOutputParticles *bin_output =
                     new BinaryOutputParticles(testoutputpath, std::move(op));
  VERIFY(bf::exists(testoutputpath / "particles_binary.bin"));

  /* create two smashon particles */
  /*ParticleType::create_type_list( // already created in a previous test
       "# NAME MASS[GEV] WIDTH[GEV] PDG\n" + smashon_str); */
  Particles particles;
  ParticleData particle = create_smashon_particle();
  particles.add_data(particle);
  ParticleData second_particle = create_smashon_particle();
  particles.add_data(second_particle);

  int event_id = 0;
  /* Write initial state output: the two smashons we created */
  bin_output->at_eventstart(particles, event_id);

  /* Interaction smashon + smashon -> smashon */
  std::vector<ParticleData> initial_particles, final_particles;
  initial_particles.push_back(particles.data(0));
  initial_particles.push_back(particles.data(1));
  particles.remove(0);
  particles.remove(1);
  ParticleData final_particle = create_smashon_particle();
  particles.add_data(final_particle);
  final_particles.push_back(particles.data(particles.id_max()));
  Clock clock;

  bin_output->at_intermediate_time(particles, event_id, clock);

  /* Final state output */
  bin_output->at_eventend(particles, event_id);
  /*
   * Now we have an artificially generated binary output.
   * Let us try if we can read and understand it.
   */

  // Open file as a binary
  FILE * binF;
  binF = fopen((testoutputpath / "particles_binary.bin").native().c_str(),
                                                                        "rb");
  // Header
  std::vector<char> buf(4);
  std::string magic, smash_version;
  int format_version_number;

  VERIFY(fread(&buf[0], 1, 4, binF) == 4);  // magic number
  magic.assign(&buf[0], 4);
  read_binary(format_version_number, binF);  // format version number
  read_binary(smash_version, binF);  // smash version

  VERIFY(magic == "SMSH");
  VERIFY(format_version_number == current_format_version);
  VERIFY(smash_version == VERSION_MAJOR);

  int npart;
  // particles at event start: expect two smashons
  npart = 2;
  VERIFY(compare_particles_block_header(npart, binF));
  VERIFY(compare_particle(initial_particles[0], binF));
  VERIFY(compare_particle(initial_particles[1], binF));

  // Periodic output: already after interaction. One smashon expected.
  npart = 1;
  VERIFY(compare_particles_block_header(npart, binF));
  VERIFY(compare_particle(final_particles[0], binF));

  // paricles at event end: nothing expected, because only_final option is off

  // after end of event
  VERIFY(compare_final_block_header(event_id, binF));
}
