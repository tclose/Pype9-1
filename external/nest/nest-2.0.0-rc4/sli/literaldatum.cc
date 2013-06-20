/*
 *  literaldatum.cc
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#include "namedatum.h"
#include "datumconverter.h"

void LiteralDatum::pprint(std::ostream &out) const
{
  out << '/';
  print(out);   
}

void LiteralDatum::use_converter(DatumConverter &converter)
{
  converter.convert_me(*this); // call visit with our own type here
                  // this will call the approproate implementation of the derived class
}
