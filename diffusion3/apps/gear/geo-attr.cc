#include	"geo-attr.hh"

// Attributes used in GEORouting
NRSimpleAttributeFactory<int> GeoBeaconTypeAttr(GEO_BEACON_TYPE_KEY, NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<double> GeoLongitudeAttr(GEO_LONGITUDE_KEY, NRAttribute::FLOAT64_TYPE);
NRSimpleAttributeFactory<double> GeoLatitudeAttr(GEO_LATITUDE_KEY, NRAttribute::FLOAT64_TYPE);
NRSimpleAttributeFactory<double> GeoRemainingEnergyAttr(GEO_REMAINING_ENERGY_KEY,
							NRAttribute::FLOAT64_TYPE);
NRSimpleAttributeFactory<int> GeoInterestModeAttr(GEO_INTEREST_MODE_KEY, NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<void *> GeoHeaderAttr(GEO_HEADER_KEY, NRAttribute::BLOB_TYPE);
NRSimpleAttributeFactory<void *> GeoH_valueAttr(GEO_H_VALUE_KEY, NRAttribute::BLOB_TYPE);
