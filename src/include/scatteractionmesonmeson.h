/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SCATTERACTIONMESONMESON_H_
#define SRC_INCLUDE_SCATTERACTIONMESONMESON_H_

#include "scatteraction.h"

namespace Smash {


/**
 * \ingroup action
 * ScatterActionMesonMeson is a special ScatterAction which represents the
 * scattering of two mesons.
 */
class ScatterActionMesonMeson : public ScatterAction {
 public:
  /* Inherit constructor. */
  using ScatterAction::ScatterAction;

 protected:
  /**
   * \ingroup logging
   * Writes information about this scatter action to the \p out stream.
   */
  void format_debug_output(std::ostream &out) const override;
};


}  // namespace Smash

#endif  // SRC_INCLUDE_SCATTERACTIONMESONMESON_H_
