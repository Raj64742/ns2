#ifndef GEO_ATTR_HH
#define GEO_ATTR_HH

#include "nr.hh"
#include "attrs.hh"

// GEORouting Attribute Keys
#define	GEO_LONGITUDE_KEY        6000
#define	GEO_LATITUDE_KEY         6001
#define	GEO_REMAINING_ENERGY_KEY 6002
#define	GEO_BEACON_TYPE_KEY      6003
#define	GEO_INTEREST_MODE_KEY    6004
#define	GEO_INTEREST_HOPS_KEY    6005
#define	GEO_HEADER_KEY	         6006
#define	GEO_H_VALUE_KEY		 6007

extern NRSimpleAttributeFactory<double> GeoLongitudeAttr;
extern NRSimpleAttributeFactory<double> GeoLatitudeAttr;
extern NRSimpleAttributeFactory<double> GeoRemainingEnergyAttr;
extern NRSimpleAttributeFactory<int> GeoBeaconTypeAttr;
extern NRSimpleAttributeFactory<void *> GeoH_valueAttr;
extern NRSimpleAttributeFactory<int> GeoInterestModeAttr;
extern NRSimpleAttributeFactory<void *> GeoHeaderAttr;

#endif // GEO_ATTR_HH
