/*
 *
 *    Copyright (c) 2012-2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 *
 */

#include <getopt.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <vector>

#include "include/Parameters.h"
#include "include/macros.h"
#include "include/param-reader.h"
#include "include/outputroutines.h"
#include "include/Experiment.h"
/* build dependent variables */
#include "include/Config.h"

char *progname;

static void usage(int rc) {
  printf("\nUsage: %s [option]\n\n", progname);
  printf("Calculate transport box\n"
         "  -h, --help           usage information\n"
         "  -m, --modus          modus of laboratory\n"
         "  -V, --version\n\n");
  exit(rc);
}

/* main - do command line parsing and hence decides modus */
int main(int argc, char *argv[]) {
  char *p, *path;
  int opt, rc = 0;
  int modus;
    
  struct option longopts[] = {
    { "help",       no_argument,            0, 'h' },
    { "modus",      required_argument,      0, 'm' },
    { "version",    no_argument,            0, 'V' },
    { NULL,         0, 0, 0 }
  };

  /* strip any path to progname */
  progname = argv[0];
  if ((p = strrchr(progname, '/')) != NULL)
    progname = p + 1;
  printf("%s (%d)\n", progname, VERSION_MAJOR);

  /* XXX: make path configurable */
  size_t len = strlen("./") + 1;
  path = reinterpret_cast<char *>(malloc(len));
  snprintf(path, len, "./");

    while ((opt = getopt_long(argc, argv, "hm:V", longopts,
    NULL)) != -1) {
    switch (opt) {
    case 'h':
      usage(EXIT_SUCCESS);
      break;
    case 'm':
      modus=fabs(atoi(optarg));
      printf("Modus read in:%i\n", modus);

      break;
    case 'V':
      exit(EXIT_SUCCESS);
    default:
      usage(EXIT_FAILURE);
    }
    }


      auto experiment = Experiment::create(modus);
      
      experiment->config(path);

  
      
 /* Output IC values */
//  print_startup(*lab);
//  mkdir_data();
//  write_oscar_header();

  /* initialize random seed */
//  srand48(lab->seed());

  /* reducing cross section according to number of test particle */
//  if (lab->testparticles() > 1) {
//   printf("IC test particle: %i\n", lab->testparticles());
//    lab->set_cross_section(lab->cross_section() / lab->testparticles());
//    printf("Elastic cross section: %g [mb]\n", lab->cross_section());
//  }

  /* modus operandi */
//  if (lab->modus() == 1) {
//    Box *box = new Box(*lab);
//    lab = box;
//  } else if (lab->modus() == 2) {
//    Sphere *ball = new Sphere(*lab);
//    lab = ball;
//  }

  /* read specific config files */
//  lab->process_config(path);

  /* initiate particles */
//  Particles *particles = new Particles;
//  input_particles(particles, path);
//  input_decaymodes(particles, path);
//  CrossSections *cross_sections = new CrossSections;
//  cross_sections->add_elastic_parameter(lab->cross_section());
//  lab->initial_conditions(particles);

  /* record IC startup */
//  print_startup(*lab);
//  write_measurements_header(*particles);
//  print_header();
//  write_particles(*particles);

  /* the time evolution of the relevant subsystem */
//  rc = lab->evolve(particles, cross_sections);

  /* tear down */
//  free(path);
//  delete particles;
//  delete cross_sections;
 return rc;
      
 
}
