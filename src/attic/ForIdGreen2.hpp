/* pcmsolver_copyright_start */
/*
 *     PCMSolver, an API for the Polarizable Continuum Model
 *     Copyright (C) 2013 Roberto Di Remigio, Luca Frediani and contributors
 *
 *     This file is part of PCMSolver.
 *
 *     PCMSolver is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     PCMSolver is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with PCMSolver.  If not, see <http://www.gnu.org/licenses/>.
 *
 *     For information on the complete list of contributors to the
 *     PCMSolver API, see: <http://pcmsolver.github.io/pcmsolver-doc>
 */
/* pcmsolver_copyright_end */

#ifndef FORIDGREEN_HPP
#define FORIDGREEN_HPP

#include <stdexcept>
#include <string>

#include "Config.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/vector.hpp>

#include "GreenData.hpp"
#include "IGreensFunction.hpp"

/*! \file ForIdGreen.hpp
 *
 *  Metafunction for fake run-time selection of template arguments.
 *  D. Langr, P.Tvrdik, T. Dytrych and J. P. Draayer
 *  "Fake Run-Time Selection of Template Arguments in C++"
 *  in "Objects, Models, Components, Patterns"
 *  DOI: 10.1007/978-3-642-30561-0_11
 *  http://arxiv.org/abs/1306.5142
 *
 *  The idea is to iterate over sequences of types, S, beginning
 *  at type B and ending at type E. This heavily relies on the Boost
 *  MetaProgramming Library (Boost.MPL)
 *  We provide here an implementation covering the case were up to
 *  three template parameters have to be selected.
 *  According to the original reference, if D is the maximum number
 *  of template parameters to be selected at run-time, then the metaprogramming
 *  technique here described requires coding of (2D + 1) structs:
 *      - one primary template. The primary template has (4D + 1)
 *      template parameters, 4D of which type template.
 *      The primary template exhausts the first type sequence;
 *      - a first partial specialization, exhausting the second
 *      type sequence. This partial specialization has 4D template parameters,
 *      (4D - 1) of which type template;
 *      - a second partial specialization, exhausting the third type
 *      sequence. This partial specialization has (4D - 1) template parameters,
 *      (4D - 2) of which type templates;
 *      - a third partial specialization, never reached at run-time.
 *      This is needed to stop recursive instantiation at compile-time.
 *      It has (4D - 2) template parameters, (4D - 3) of which type template.
 *      - D ApplyFunctor helper structs, needed to wrap the functor.
 *  Once the template parameter has been resolved, a functor is
 *  applied. The return value of the functor is the Green's function
 *  we want to create with the template parameters resolved.
 *
 *  MPL metafunctions used:
 *      - boost::mpl::distance
 *      - boost::mpl::begin
 *      - boost::mpl::end
 *      - boost::mpl::find
 *      - boost::mpl::deref
 *      - boost::mpl::next
 */

namespace mpl = boost::mpl;

/*! \brief Returns a zero-based index of a type within a type sequence
 *  \tparam S type sequence
 *  \tparam T the type whose index will be returned
 */
template <typename S, typename T>
struct position : mpl::distance<typename mpl::begin<S>::type,
                           typename mpl::find<S, T>::type>::type { };

/*! Convenience wrapper to the functor
 *  \tparam D  dimension of the problem, i.e. the maximum number of type sequences allowed
 *  \tparam T1 type selected from S1
 *  \tparam T2 type selected from S2
 */
template <int D, typename T1, typename T2>
struct ApplyFunctor;

/*! \brief Iterates over type sequences either until the position of the actual type matches the
 *         desired id or until the end of the sequences is reached.
 *  \tparam D  dimension of the problem, i.e. the maximum number of type sequences allowed
 *  \tparam S1 type sequence number 1
 *  \tparam S2 type sequence number 2, by default empty
 *  \tparam B1 type of the first element in S1
 *  \tparam B2 type of the first element in S2
 *  \tparam E1 type of the last element in S1
 *  \tparam E2 type of the last element in S2
 *  \tparam T1 type selected from S1
 *  \tparam T2 type selected from S2
 *
 *  This is the primary template. As such, it has (4D + 1) template parameters, 4D
 *  of which type template parameters.
 *
 *  \note The mechanics of the code is also documented, since this is quite an intricate piece of code!
 */
template <int D,
          typename S1,
          typename S2 = mpl::vector<>,
          typename B1 = typename mpl::begin<S1>::type,
          typename B2 = typename mpl::begin<S2>::type,
          typename E1 = typename mpl::end<S1>::type,
          typename E2 = typename mpl::end<S2>::type,
          typename T1 = typename mpl::deref<B1>::type,
          typename T2 = typename mpl::deref<B2>::type
         >
struct for_id_impl
{
    /*! Executes given functor with selected template arguments
     *  \param[in] f the creational functor to be applied
     *  \param[in] data the data needed to create the correct Green's function
     *  \param[in] id1 the type of derivative
     *  \param[in] id2 index for the second type
     *  \tparam T the type of the Green's function
     */
    template <typename T>
    static IGreensFunction * execute(T & f, const greenData & data, int id1, int id2 = 0) {
        if (position<S1, typename mpl::deref<B1>::type>::value == id1) { // Desired type in S1 found
            if (1 == D) { // One-dimensional, we're done!
                return ApplyFunctor<D, typename mpl::deref<B1>::type, T2>::apply(f, data);
            } else { // Resolve second and third dimensions
                // Call first partial specialization of for_id_impl
                // B1 is not "passed", since the type desired from S1 has been resolved
                // The resolved type is "saved" into T1 and "passed" to the first partial specialization
                return (for_id_impl<D, S1, S2, E1, B2,
                        E1, E2, typename mpl::deref<B1>::type>::execute(f, data, id1, id2));
            }
        } else if (1 == mpl::distance<B1, E1>::value) { // Desired type NOT found in S1
            throw std::invalid_argument("Invalid derivative type (id1 = " +
                    boost::lexical_cast<std::string>(id1) + ") in for_id metafunction.");
        } else { // First type not resolved, but S1 type sequence not exhausted
            // Call for_id_impl primary template with type of B1 moved to the next type in S1
            return (for_id_impl<D, S1, S2,
                    typename mpl::next<B1>::type, B2,
                    E1, E2, T1>::execute(f, data, id1)); //, T2>::execute(f, data, id1, id2));
        }
    }
};

/*! \brief Iterates over type sequences either until the position of the actual type matches the
 *         desired id or until the end of the sequences is reached.
 *  \tparam D  dimension of the problem, i.e. the maximum number of type sequences allowed
 *  \tparam S1 type sequence number 1
 *  \tparam S2 type sequence number 2
 *  \tparam B2 type of the first element in S2
 *  \tparam E1 type of the last element in S1
 *  \tparam E2 type of the last element in S2
 *  \tparam T1 type selected from S1
 *  \tparam T2 type selected from S2
 *
 *  This is the fist partial specialization. As such, it has 4D template parameters, (4D - 1)
 *  of which type template parameters.
 *
 *  \note The mechanics of the code is also documented, since this is quite an intricate piece of code!
 */
template <int D,
          typename S1, typename S2,
                       typename B2,
          typename E1, typename E2,
          typename T1, typename T2>
struct for_id_impl<D, S1, S2, E1, B2, E1, E2, T1, T2>
{
    /*! Executes given functor with selected template arguments
     *  \param[in] f the creational functor to be applied
     *  \param[in] data the data needed to create the correct Green's function
     *  \param[in] id1 the type of derivative
     *  \param[in] id2 index for the second type
     *  \tparam T the type of the Green's function
     */
    template <typename T>
    static IGreensFunction * execute(T & f, const greenData & data, int id1, int id2 = 0) {
        if (position<S2, typename mpl::deref<B2>::type>::value == id2) { // Desired type in S2 found
            return ApplyFunctor<D, T1, typename mpl::deref<B2>::type>::apply(f, data);
        } else if (1 == mpl::distance<B2, E2>::value) { // Desired type NOT found in S2
            throw std::invalid_argument("Invalid derivative type (id2 = " +
                                        boost::lexical_cast<std::string>(id2) + ") in for_id metafunction.");
        } else { // Second type not resolved, but S2 type sequence not exhausted
            // Call for_id_impl first partial specialization with type of B2 moved to the next type in S2
            return (for_id_impl<D, S1, S2, E1,
                                typename mpl::next<B2>::type,
                                E1, E2, T1>::execute(f, data, id1, id2)); //, T2>::execute(f, data, id1, id2));
        }
    }
};

/*! \brief Iterates over type sequences either until the position of the actual type matches the
 *         desired id or until the end of the sequences is reached.
 *  \tparam D  dimension of the problem, i.e. the maximum number of type sequences allowed
 *  \tparam S1 type sequence number 1
 *  \tparam S2 type sequence number 2
 *  \tparam E1 type of the last element in S1
 *  \tparam E2 type of the last element in S2
 *  \tparam T1 type selected from S1
 *  \tparam T2 type selected from S2
 *
 *  This is the third partial specialization. As such, it has (4D - 2) template parameters, (4D - 3)
 *  of which type template parameters.
 *  It is never reached at run-time, it is needed to stop the recursive instantiation at compile-time.
 */
template <int D,
          typename S1, typename S2,
          typename E1, typename E2,
          typename T1, typename T2>
struct for_id_impl<D, S1, S2, E1, E2, E1, E2, T1, T2>
{
    /*! Executes given functor with selected template arguments
     *  \tparam T the type of the Green's function
     */
    template <typename T>
    static IGreensFunction * execute(T & /* f */, const greenData & /* data */,
                                     int /* id1 */, int /* id2 */ = 0, int /* id3 */ = 0) {
        return nullptr;
    }
};

/**@{ Wrappers to the functor */
/*! Wrapper to the two-dimensional case
 *  \tparam T1 type selected from S1
 *  \tparam T2 type selected from S2
 */
template <typename T1, typename T2>
struct ApplyFunctor<2, T1, T2>
{
    template <typename T>
    static IGreensFunction * apply(T & f, const greenData & data) {
        return (f.template operator()<T1, T2>(data));
    }
};

/*! Wrapper to the one-dimensional case
 *  \tparam T1 type selected from S1
 *  \tparam T2 type selected from S2
 */
template <typename T1, typename T2>
struct ApplyFunctor<1, T1, T2>
{
    template <typename T>
    static IGreensFunction * apply(T & f, const greenData & data) {
        return (f.template operator()<T1>(data));
    }
};
/**@}*/

/**@{ Wrappers to the primary template for the metafunction */
/*! Wrapper for the two-dimensional case.
 *  \tparam S1 type sequence 1
 *  \tparam S2 type sequence 2
 *  \tparam T  type of the functor
 *  \param[in,out] f functor
 *  \param[in] data input arguments for functor
 *  \param[in] id1  index of the first template argument, selected in S1
 *  \param[in] id2  index of the second template argument, selected in S2
 */
template <typename S1, typename S2, typename T>
IGreensFunction * for_id(T & f, const greenData & data, int id1, int id2) {
    return for_id_impl<2, S1, S2>::execute(f, data, id1, id2);
}

/*! Wrapper for the one-dimensional case.
 *  \tparam S1 type sequence 1
 *  \tparam T  type of the functor
 *  \param[in,out] f functor
 *  \param[in] data input arguments for functor
 *  \param[in] id1  index of the first template argument, selected in S1
 */
template <typename S1, typename T>
IGreensFunction * for_id(T & f, const greenData & data, int id1) {
    return for_id_impl<1, S1>::execute(f, data, id1);
}
/**@}*/

#endif // FORIDGREEN_HPP