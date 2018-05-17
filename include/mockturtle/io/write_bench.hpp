/* mockturtle: C++ logic network library
 * Copyright (C) 2018  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file write_bench.hpp
  \brief Write networks to BENCH format

  \author Mathias Soeken
*/

#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include <fmt/format.h>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Writes network in BENCH format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required network functions:**
 * - `is_constant`
 * - `is_pi`
 * - `is_complemented`
 * - `get_node`
 * - `node_to_index`
 * - `node_function`
 *
 * \param ntk Network
 * \param os Output stream
 */
template<class Ntk>
void write_bench( Ntk const& ntk, std::ostream& os )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

  ntk.foreach_pi( [&]( auto const& n ) {
    os << fmt::format( "INPUT(n{})\n", ntk.node_to_index( n ) );
  } );

  ntk.foreach_po( [&]( auto const& s ) {
    os << fmt::format( "OUTPUT(n{}{})\n",
                       ntk.node_to_index( ntk.get_node( s ) ),
                       ntk.is_complemented( s ) ? "_inv" : "" );
  } );

  ntk.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return; /* continue */

    auto func = ntk.node_function( n );
    std::string children;
    auto first = true;
    ntk.foreach_fanin( n, [&]( auto const& c, auto i ) {
      if ( ntk.is_complemented( c ) )
      {
        kitty::flip_inplace( func, i );
      }
      if ( first )
      {
        first = false;
      }
      else
      {
        children += ", ";
      }

      children += fmt::format( "n{}", ntk.node_to_index( ntk.get_node( c ) ) );
    } );

    os << fmt::format( "n{} = LUT 0x{} ({})\n",
                       ntk.node_to_index( n ),
                       kitty::to_hex( func ), children );
  } );

  /* inverted outputs */
  ntk.foreach_po( [&]( auto const& s ) {
    if ( ntk.is_complemented( s ) )
    {
      os << fmt::format( "n{0}_inv = LUT 0x1 (n{0})\n", ntk.node_to_index( ntk.get_node( s ) ) );
    }
  } );

  /* constant outputs */
  ntk.foreach_po( [&]( auto const& s ) {
    if ( ntk.is_constant( ntk.get_node( s ) ) )
    {
      os << fmt::format( "n{} = {}\n",
                         ntk.node_to_index( ntk.get_node( s ) ),
                         ntk.is_complemented( s ) ? "vdd" : "gnd" );
    }
  } );

  os << std::flush;
}

/*! \brief Writes network in BENCH format into a file
 *
 * **Required network functions:**
 * - `is_constant`
 * - `is_pi`
 * - `is_complemented`
 * - `get_node`
 * - `node_to_index`
 * - `node_function`
 *
 * \param ntk Network
 * \param filename Filename
 */
template<class Ntk>
void write_bench( Ntk const& ntk, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_bench( ntk, os );
  os.close();
}

} /* namespace mockturtle */
