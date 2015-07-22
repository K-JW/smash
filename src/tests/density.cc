/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include <map>
#include "unittest.h"
#include "setup.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "../include/boxmodus.h"
#include "../include/configuration.h"
#include "../include/cxx14compat.h"
#include "../include/density.h"
#include "../include/densityoutput.h"
#include "../include/experiment.h"
#include "../include/modusdefault.h"
#include "../include/nucleus.h"


using namespace Smash;

static const bf::path testoutputpath = bf::absolute(SMASH_TEST_OUTPUT_PATH);

TEST(directory_is_created) {
  bf::create_directories(testoutputpath);
  VERIFY(bf::exists(testoutputpath));
}

TEST(init_particle_types) {
  ParticleType::create_type_list(
      "# NAME MASS[GEV] WIDTH[GEV] PDG\n"
      "proton 0.938 0.0 2212\n"
      "neutron 0.938 0.0 2112\n");
}

static ParticleData create_proton(int id = -1) {
  return ParticleData{ParticleType::find(0x2212), id};
}


static ParticleData create_antiproton(int id = -1) {
  return ParticleData{ParticleType::find(-0x2212), id};
}

TEST(density_type) {
  //pions
  PdgCode pi0("111");
  PdgCode pi_plus("211");
  PdgCode pi_minus("-211");

  // verify that pions are recognized as pions
  COMPARE(density_factor(pi0, DensityType::pion), 1.f);
  COMPARE(density_factor(pi_plus, DensityType::pion), 1.f);
  COMPARE(density_factor(pi_minus, DensityType::pion), 1.f);

  // verify that pions are not recognized as baryons
  COMPARE(density_factor(pi0, DensityType::baryon), 0.f);

  // baryons
  PdgCode proton("2212");

  // verify that protons are recognized as baryons
  COMPARE(density_factor(proton, DensityType::baryon), 1.f);

  // verify that protons are not recognized as pions
  COMPARE(density_factor(proton, DensityType::pion), 0.f);

  // verify that all are recognized as particles
  VERIFY(density_factor(proton,   DensityType::hadron) == 1.f
      && density_factor(pi0,      DensityType::hadron) == 1.f
      && density_factor(pi_plus,  DensityType::hadron) == 1.f
      && density_factor(pi_minus, DensityType::hadron) == 1.f
      );
}

// create one particle moving along x axis and check density in comp. frame
// check if density in the comp. frame gets contracted as expected
TEST(density_value) {

  ParticleData part_x = create_proton();
  part_x.set_4momentum(FourVector(1.0, 0.95, 0.0, 0.0));
  part_x.set_4position(FourVector(0.0, 0.0, 0.0, 0.0));
  ParticleList P;
  P.push_back(part_x);
  ThreeVector r;
  const ExperimentParameters par = Smash::Test::default_parameters();
  double rho;
  DensityType bar_dens = DensityType::baryon;

  /* Got numbers executing mathematica code:
     rhoYZ = SetPrecision[Exp[-1.0/2.0]/(2*Pi)^(3/2), 16]
     rhoX = SetPrecision[Exp[-1.0/2.0/(1.0 - 0.95*0.95)]/(2*Pi)^(3/2), 16]
   */

  r = ThreeVector(1.0, 0.0, 0.0);
  rho = rho_eckart(r, P, par, bar_dens, false).first;
  COMPARE_ABSOLUTE_ERROR(rho, 0.0003763388107782538, 1.e-15);

  r = ThreeVector(0.0, 1.0, 0.0);
  rho = rho_eckart(r, P, par, bar_dens, false).first;
  COMPARE_ABSOLUTE_ERROR(rho, 0.03851083689074894, 1.e-15);

  r = ThreeVector(0.0, 0.0, 1.0);
  rho = rho_eckart(r, P, par, bar_dens, false).first;
  COMPARE_ABSOLUTE_ERROR(rho, 0.03851083689074894, 1.e-15);

}

TEST(density_eckart_special_cases) {
  /* This one checks one especially nasty case:
  Eckart rest frame of baryon density for proton and antiproton
  flying with the same speed in the opposite directions. */
  ParticleData pr = create_proton();
  ParticleData apr = create_antiproton();
  pr.set_4position(FourVector(0.0, 0.0, 0.0, 0.0));
  apr.set_4position(FourVector(0.0, 0.0, 0.0, 0.0));
  pr.set_4momentum(FourVector(1.0, 0.95, 0.0, 0.0));
  apr.set_4momentum(FourVector(1.0, -0.95, 0.0, 0.0));
  ParticleList P;
  P.push_back(pr);
  P.push_back(apr);
  ThreeVector r(1.0, 0.0, 0.0);
  const ExperimentParameters par = Smash::Test::default_parameters();
  double rho = rho_eckart(r, P, par, DensityType::baryon, false).first;
  COMPARE_ABSOLUTE_ERROR(rho, 0.0, 1.e-15) << rho;

  /* Now check for negative baryon density from antiproton */
  P.erase(P.begin()); // Remove proton
  rho = rho_eckart(r, P, par, DensityType::baryon, false).first;
  COMPARE_ABSOLUTE_ERROR(rho, -0.0003763388107782538, 1.e-15) << rho;
}

// check that analytical and numerical results for gradient of density coincide
TEST(density_gradient) {
  // create two protons
  ParticleData part1 = create_proton();
  ParticleData part2 = create_proton();
  double mass = 0.938;
  // set momenta (just randomly):
  part1.set_4momentum(mass, ThreeVector(1.0, 4.0, 3.0));
  part2.set_4momentum(mass, ThreeVector(4.0, 2.0, 5.0));
  // set coordinates (not too far from each other)
  part1.set_4position(FourVector(0.0, -0.5, 0.0, 0.0));
  part2.set_4position(FourVector(0.0, 0.5, 0.0, 0.0));
  // make particle list out of them
  ParticleList P;
  P.push_back(part1);
  P.push_back(part2);

  const ExperimentParameters par = Smash::Test::default_parameters();
  ThreeVector r,dr;
  FourVector jmu;
  DensityType dtype = DensityType::baryon;

  ThreeVector num_grad, analit_grad;
  r = ThreeVector(0.0, 0.0, 0.0);
  std::pair<double, ThreeVector> rho_and_grad =
                             rho_eckart(r, P, par, dtype, true);
  double rho_r = rho_and_grad.first;

  // analytical gradient
  analit_grad = rho_and_grad.second;
  // numerical gradient
  dr = ThreeVector(1.e-4, 0.0, 0.0);
  double rho_rdr = rho_eckart(r + dr, P, par, dtype, false).first;
  num_grad.set_x1((rho_rdr - rho_r)/dr.x1());
  dr = ThreeVector(0.0, 1.e-4, 0.0);
  rho_rdr = rho_eckart(r + dr, P, par, dtype, false).first;
  num_grad.set_x2((rho_rdr - rho_r)/dr.x2());
  dr = ThreeVector(0.0, 0.0, 1.e-4);
  rho_rdr = rho_eckart(r + dr, P, par, dtype, false).first;
  num_grad.set_x3((rho_rdr - rho_r)/dr.x3());
  // compare them with: accuracy should not be worse than |dr|
  std::cout << num_grad << analit_grad << std::endl;
  COMPARE_ABSOLUTE_ERROR(num_grad.x1(), analit_grad.x1(), 1.e-4);
  COMPARE_ABSOLUTE_ERROR(num_grad.x2(), analit_grad.x2(), 1.e-4);
  COMPARE_ABSOLUTE_ERROR(num_grad.x3(), analit_grad.x3(), 1.e-4);
}

/*
   This test does not compare anything. It only prints density map versus
   time to vtk files, so that one can open it with paraview and make sure
   that Lorentz contraction works properly for density.
 */
/*
TEST(density_eckart_frame) {

  // create a particle list with particles moving in different directions.

  // proton that moves along the x axis:
  ParticleData proton_x = create_proton();
  // proton that moves along the y axis:
  ParticleData proton_y = create_proton();
  // proton that moves along the z axis:
  ParticleData proton_z = create_proton();
  // proton that moves in neither of xyz planes, v = v0:
  ParticleData proton = create_proton();
  // proton that moves in neither of xyz planes, v = - v0:
  ParticleData antiproton = create_antiproton();

  double mass = 0.938;
  // set momenta:
  proton_x.set_4momentum(mass, ThreeVector(2.0, 0.0, 0.0));
  proton_y.set_4momentum(mass, ThreeVector(0.0, -2.0, 0.0));
  proton_z.set_4momentum(mass, ThreeVector(0.0, 0.0, 2.0));
  proton.set_4momentum(mass, ThreeVector(0.4, 0.5, 0.7));
  antiproton.set_4momentum(mass, ThreeVector(-0.4, -0.5, -0.7));

  // set positions:
  proton_x.set_4position(FourVector(0.0, -5.0, 0.0, 0.0));
  proton_y.set_4position(FourVector(0.0, 0.0, 5.0, 0.0));
  proton_z.set_4position(FourVector(0.0, 0.0, 0.0, -5.0));
  proton.set_4position(FourVector(0.0,-4.0,-5.0,-7.0));
  antiproton.set_4position(FourVector(0.0, 4.0, 5.0, 7.0));

  // add particles (and make sure the particles get the correct ID):
  COMPARE(P.add_data(proton_x), 0);
  COMPARE(P.add_data(proton_y), 1);
  COMPARE(P.add_data(proton_z), 2);
  COMPARE(P.add_data(proton), 3);
  COMPARE(P.add_data(antiproton), 4);


  ModusDefault m;
  Particles Pdef;
  create_particle_list(Pdef);
  OutputsList out;
  // clock, output interval, cross-section, testparticles, gauss. sigma
  ExperimentParameters param = Smash::Test::default_parameters();
  double dx = 0.3;
  double dy = 0.3;
  double dz = 0.3;
  int nx = 20;
  int ny = 20;
  int nz = 20;
  double sigma = 0.8;
  ThreeVector r;
  DensityType bar_dens = baryon;
  ParticleList plist;

  for (auto it = 0; it < 30; it++) {
    plist = ParticleList(Pdef.data().begin(), Pdef.data().end());
    vtk_density_map( ("bdens.vtk." + std::to_string(it)).c_str(),
                     plist, sigma, bar_dens, 1,
                     nx, ny, nz, dx, dy, dz);
    m.propagate(&Pdef, param, out);
  }
}
*/

/*
  Generates nuclei and prints their density profiles to vtk files
*/
/*
TEST(nucleus_density) {
  std::string configfilename = "densconfig.yaml";
  bf::ofstream(testoutputpath / configfilename) << "x: 0\ny: 0\nz: 0\n";
  VERIFY(bf::exists(testoutputpath / configfilename));

  // Lead nuclei with 1000 test-particles
  std::map<PdgCode, int> lead_list = {{0x2212, 79}, {0x2112, 118}};
  int Ntest = 1000;
  Nucleus lead;
  lead.fill_from_list(lead_list, Ntest);
  lead.set_parameters_automatic();
  lead.arrange_nucleons();

  // make particle list out of generated nucleus
  Particles p;
  lead.copy_particles(&p);
  ParticleList plist = p.copy_to_vector();

  // write density profile to file, time-consuming!
  DensityType dens_type = DensityType::baryon;
  double sigma = 0.5; // fm
//  vtk_density_map("lead_density.vtk", plist, sigma, dens_type, Ntest,
//                     20, 20, 20, 0.5, 0.5, 0.5);
  ThreeVector lstart(-10.0, 0.0, 0.0);
  ThreeVector lend(10.0, 0.0, 0.0);
  const int npoints = 100;

  Configuration&& conf{testoutputpath, configfilename};
  ExperimentParameters par = Smash::Test::default_parameters(Ntest);
  std::unique_ptr<DensityOutput> out = make_unique<DensityOutput>(testoutputpath, std::move(conf));
  out->density_along_line("lead_densityX.dat", plist, par, dens_type,
                          lstart, lend, npoints);

  lstart = ThreeVector(0.0, -10.0, 0.0);
  lend = ThreeVector(0.0, 10.0, 0.0);
  out->density_along_line("lead_densityY.dat", plist, par, dens_type,
                          lstart, lend, npoints);

  lstart = ThreeVector(0.0, 0.0, -10.0);
  lend = ThreeVector(0.0, 0.0, 10.0);
  out->density_along_line("lead_densityZ.dat", plist, par, dens_type,
                          lstart, lend, npoints);
}*/

/*TEST(box_density) {
ParticleType::create_type_list(
    "# NAME MASS[GEV] WIDTH[GEV] PDG\n"
    "proton 0.938 0.0 2212\n");
const int Ntest = 1000;
const float L = 10.0f;
Configuration conf(TEST_CONFIG_PATH);
conf["Modus"] = "Box";
conf.take({"Modi", "Box", "Init_Multiplicities"});
conf["Modi"]["Box"]["Init_Multiplicities"]["2212"] = 1000;
conf["Modi"]["Box"]["Length"] = L;
const ExperimentParameters par = Smash::Test::default_parameters(Ntest);
std::unique_ptr<BoxModus> b = make_unique<BoxModus>(conf["Modi"], par);
Particles P;
b->initial_conditions(&P, par);
ParticleList plist = P.copy_to_vector();
conf["Output"]["Density"]["x"] = 0.0;
conf["Output"]["Density"]["y"] = 0.0;
conf["Output"]["Density"]["z"] = 0.0;
std::unique_ptr<DensityOutput> out = make_unique<DensityOutput>(testoutputpath,
                                         conf["Output"]["Density"]);
const ThreeVector lstart = ThreeVector(0.0, 0.0, 0.0);
const ThreeVector lend = ThreeVector(L, L, L);
const int npoints = 100;
ExperimentParameters par = Smash::Test::default_parameters(Ntest);
out->density_along_line("box_density.dat", plist, par, DensityType::baryon,
                        lstart, lend, npoints);
}*/
