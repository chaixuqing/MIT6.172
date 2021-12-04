// Copyright (c) 2015 MIT License by 6.172 Staff

/////////////////////////////////////////////////////////////////////////////
//
// Rémi Coulom
//
// December, 2004
//
/////////////////////////////////////////////////////////////////////////////
#include "./CDistributionCollection.h"
#include "./CCDistribution.h"

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
CDistributionCollection::CDistributionCollection(int PlayersInit,
                                                 int Size,
                                                 double Min,
                                                 double Max)
    : CDiscretization(Size, Min, Max),
      Players(PlayersInit) {
  ppDistribution = new CCDistribution*[Players];
  for (int i = Players; --i >= 0;)
    ppDistribution[i] = new CCDistribution(Size, Min, Max);
}

/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
CDistributionCollection::~CDistributionCollection() {
  for (int i = Players; --i >= 0;)
    delete ppDistribution[i];
  delete[] ppDistribution;
}
