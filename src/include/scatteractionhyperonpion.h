/*
 *
 *    Copyright (c) 2015-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SCATTERACTIONHYPERONPION_H_
#define SRC_INCLUDE_SCATTERACTIONHYPERONPION_H_

#include "scatteraction.h"
#include "scatteractionbaryonmeson.h"

namespace smash {

/**
 * \ingroup action
 * ScatterActionHyperonPion is a special ScatterActionBaryonMeson which
 * represents the
 * scattering of a hyperon and a pion.
 */
class ScatterActionHyperonPion : public ScatterActionBaryonMeson {
 public:
  /* Inherit constructor. */
  using ScatterActionBaryonMeson::ScatterActionBaryonMeson;

 protected:
  /**
   * \ingroup logging
   * Writes information about this scatter action to the \p out stream.
   */
  void format_debug_output(std::ostream &out) const override;
};

}  // namespace smash

#endif  // SRC_INCLUDE_SCATTERACTIONHYPERONPION_H_
