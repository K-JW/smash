/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "tests/unittest.h"
#include "include/configuration.h"

#include <boost/filesystem.hpp>

TEST(create_object) {
  Configuration conf(TEST_CONFIG_PATH);
}

TEST(check_config_general_contents) {
  Configuration conf(TEST_CONFIG_PATH);

  std::string modus = conf.read({"General", "MODUS"        });
  COMPARE(modus, "Collider");
  COMPARE(double(conf.read({"General", "EPS"          })), 0.01);
  COMPARE(int   (conf.read({"General", "STEPS"        })), 1000);
  COMPARE(int   (conf.read({"General", "UPDATE"       })), 10);
  COMPARE(int   (conf.read({"General", "RANDOMSEED"   })), 1);
  COMPARE(double(conf.read({"General", "SIGMA"        })), 10.0);
  COMPARE(int   (conf.read({"General", "TESTPARTICLES"})), 1);
  COMPARE(int   (conf.read({"General", "NEVENTS"      })), 1);
}

TEST(check_config_collider_contents) {
  Configuration conf(TEST_CONFIG_PATH);
  COMPARE(int   (conf.read({"Modi", "Collider", "PROJECTILE"})), 211);
  COMPARE(int   (conf.read({"Modi", "Collider", "TARGET"    })), -211);
  COMPARE(double(conf.read({"Modi", "Collider", "SQRTS"     })), 1.0);
}

TEST(test_take) {
  Configuration conf(TEST_CONFIG_PATH);
  double d = conf.take({"Modi", "Sphere", "RADIUS"});
  COMPARE(d, 5.);
}

TEST(test_take_multiple) {
  Configuration conf(TEST_CONFIG_PATH);
  Configuration modi = conf["Modi"];
  double d = modi.take({"Box", "LENGTH"});
  COMPARE(d, 10.);
  d = modi.take({"Box", "TEMPERATURE"});
  COMPARE(d, 0.2);
  int i = modi.take({"Box", "INITIAL_CONDITION"});
  COMPARE(i, 1);
}

TEST_CATCH(take_incorrect_type, Configuration::IncorrectTypeInAssignment) {
  Configuration conf(TEST_CONFIG_PATH);
  Configuration modi = conf["Modi"];
  int i = modi.take({"Sphere", "RADIUS"});
  COMPARE(i, 5);
}

TEST(take_always_converts_to_string) {
  Configuration conf(TEST_CONFIG_PATH);
  Configuration modi = conf["Modi"];
  std::string s = modi.take({"Sphere", "RADIUS"});
  COMPARE(s, "5.0");
}

TEST(has_value) {
  Configuration conf(TEST_CONFIG_PATH);
  Configuration modi = conf["Modi"];
  VERIFY(modi.has_value({"Sphere", "RADIUS"}));
  VERIFY(modi.has_value({"Sphere", "RADIUS"}));
}

TEST(take_removes_entry) {
  Configuration conf(TEST_CONFIG_PATH);
  Configuration modi = conf["Modi"];
  VERIFY(modi.has_value({"Sphere", "RADIUS"}));
  modi.take({"Sphere", "RADIUS"});
  VERIFY(!modi.has_value({"Sphere", "RADIUS"}));
}

TEST(check_unused_report) {
  std::string reference;
  Configuration conf(TEST_CONFIG_PATH);
  Configuration modi = conf["Modi"];
  conf.take({"particles"});
  conf.take({"decaymodes"});
  conf.take({"General", "MODUS"});
  conf.take({"General", "EPS"});
  conf.take({"General", "STEPS"});
  conf.take({"General", "UPDATE"});
  conf.take({"General", "RANDOMSEED"});
  conf.take({"General", "SIGMA"});
  conf.take({"General", "TESTPARTICLES"});
  conf.take({"General", "NEVENTS"});
  modi.take({"Box", "LENGTH"});
  modi.take({"Box", "TEMPERATURE"});
  modi.take({"Box", "INITIAL_CONDITION"});
  modi.take({"Nucleus"});
  reference =
      "Modi:\n  Collider:\n    SQRTS: 1.0\n    TARGET: -211\n    PROJECTILE: "
      "211\n  Sphere:\n    RADIUS: 5.0";
  COMPARE(conf.unused_values_report(), reference);

  modi.take({"Sphere", "RADIUS"});
  reference = "Modi:\n  Collider:\n    SQRTS: 1.0\n    TARGET: -211\n    PROJECTILE: 211";
  COMPARE(conf.unused_values_report(), reference);

  modi.take({"Collider", "PROJECTILE"});
  reference = "Modi:\n  Collider:\n    SQRTS: 1.0\n    TARGET: -211";
  COMPARE(conf.unused_values_report(), reference);

  modi.take({"Collider", "SQRTS"});
  reference = "Modi:\n  Collider:\n    TARGET: -211";
  COMPARE(conf.unused_values_report(), reference);

  modi.take({"Collider", "TARGET"});
  reference = "{}";
  COMPARE(conf.unused_values_report(), reference);
}

TEST(test_config_read) {
  Configuration conf(TEST_CONFIG_PATH);
  int nevents = conf.read({"General", "NEVENTS"});
  COMPARE(nevents, 1);
  nevents = conf.read({"General", "NEVENTS"});
  COMPARE(nevents, 1);
  nevents = conf.take({"General", "NEVENTS"});
  COMPARE(nevents, 1);
}

TEST(test_sub_config_objects) {
  Configuration conf(TEST_CONFIG_PATH);
  Configuration general = conf["General"];
  const Configuration box = conf["Modi"]["Box"];
  VERIFY(general.has_value({"NEVENTS"}));
  int nevents = general.read({"NEVENTS"});
  VERIFY(general.has_value({"NEVENTS"}));
  COMPARE(nevents, 1);
  nevents = general.take({"NEVENTS"});
  VERIFY(!general.has_value({"NEVENTS"}));
  COMPARE(nevents, 1);
  COMPARE(double(box.read({"LENGTH"})), 10.);
}

TEST(check_setting_new_value) {
  Configuration conf(TEST_CONFIG_PATH);
  VERIFY(!conf.has_value({"Test"}));
  conf["Test"] = 1.;
  VERIFY(conf.has_value({"Test"}));
  COMPARE(double(conf.read({"Test"})), 1.);
}

TEST(merge_override) {
  Configuration conf(TEST_CONFIG_PATH);
  COMPARE(int(conf.read({"General", "STEPS"  })), 1000);
  COMPARE(int(conf.read({"General", "NEVENTS"})), 1);
  conf.merge_yaml("General: { NEVENTS: 2 }");
  COMPARE(int(conf.read({"General", "STEPS"  })), 1000);
  COMPARE(int(conf.read({"General", "NEVENTS"})), 2);
}

TEST(remove_all_but) {
  Configuration conf(TEST_CONFIG_PATH);
  Configuration modi = conf["Modi"];
  modi.remove_all_but("Sphere");
  conf.remove_all_but("Modi");
  COMPARE(conf.to_string(), "Modi:\n  Sphere:\n    RADIUS: 5.0");
}
