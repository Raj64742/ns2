//
// gear_attr.cc   : GEAR Filter Attribute Instantiations
// authors        : Yan Yu and Fabio Silva
//
// Copyright (C) 2000-2002 by the University of Southern California
// Copyright (C) 2000-2002 by the University of California
// $Id: gear_attr.cc,v 1.1 2003/07/08 18:06:42 haldar Exp $
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
//

#include "gear_attr.hh"

// This file creates all attribute factories for attributes used in
// GEAR

NRSimpleAttributeFactory<int> GeoBeaconTypeAttr(GEO_BEACON_TYPE_KEY,
						NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<double> GeoLongitudeAttr(GEO_LONGITUDE_KEY,
						  NRAttribute::FLOAT64_TYPE);
NRSimpleAttributeFactory<double> GeoLatitudeAttr(GEO_LATITUDE_KEY,
						 NRAttribute::FLOAT64_TYPE);
NRSimpleAttributeFactory<double> GeoRemainingEnergyAttr(GEO_REMAINING_ENERGY_KEY,
							NRAttribute::FLOAT64_TYPE);
NRSimpleAttributeFactory<int> GeoInterestModeAttr(GEO_INTEREST_MODE_KEY,
						  NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<void *> GeoHeaderAttr(GEO_HEADER_KEY,
					       NRAttribute::BLOB_TYPE);
NRSimpleAttributeFactory<void *> GeoHeuristicValueAttr(GEO_H_VALUE_KEY,
						       NRAttribute::BLOB_TYPE);
