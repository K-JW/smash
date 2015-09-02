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
float pp_total(double mandelstam_s);

/* np elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float np_elastic(double mandelstam_s);

/* np total cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float np_total(double mandelstam_s);

/* ppbar elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float ppbar_elastic(double mandelstam_s);

/* ppbar total cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float ppbar_total(double mandelstam_s);

/* K+ p elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float kplusp_elastic(double mandelstam_s);

/* K+ n elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float kplusn_elastic(double mandelstam_s);

/* K- p elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float kminusp_elastic(double mandelstam_s);

/* K- n elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float kminusn_elastic(double mandelstam_s);

/* K0 p elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float k0p_elastic(double mandelstam_s);

/* K0 n elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float k0n_elastic(double mandelstam_s);

/* Kbar0 p elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float kbar0p_elastic(double mandelstam_s);

/* Kbar0 n elastic cross section parametrization
 *
 * \fpPrecision Why \c double?
 */
float kbar0n_elastic(double mandelstam_s);

}  // namespace Smash

#endif  // SRC_INCLUDE_PARAMETRIZATIONS_H_
