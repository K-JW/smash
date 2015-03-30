/*
 *
 *    Copyright (c) 2013-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */
#ifndef SRC_INCLUDE_PARAMETRIZATIONS_H_
#define SRC_INCLUDE_PARAMETRIZATIONS_H_

namespace Smash {

/* pp elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float pp_elastic(double mandelstam_s);

/* pp total cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float pp_total(double p_lab);

/* np elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float np_elastic(double mandelstam_s);

/* np total cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float np_total(double p_lab);

/* ppbar elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float ppbar_elastic(double mandelstam_s);

/* ppbar total cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float ppbar_total(double p_lab);

}  // namespace Smash

#endif  // SRC_INCLUDE_PARAMETRIZATIONS_H_
