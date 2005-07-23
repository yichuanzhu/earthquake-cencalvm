// -*- C++ -*-
//
// ======================================================================
//
//                           Brad T. Aagaard
//                        U.S. Geological Survey
//
// {LicenseText}
//
// ======================================================================
//

#include "AvgEngine.h" // implementation of class methods

#include "cencalvm/storage/Payload.h" // USES PayloadStruct
#include "cencalvm/storage/ErrorHandler.h" // USES ErrorHandler

extern "C" {
#include "etree.h"
}

#include <sstream> // USES std::ostringstream
#include <assert.h> // USES assert()
#include <iostream> // USES std::cout

std::string
printAddr(etree_addr_t* pAddr)
{
  std::ostringstream msg;
  msg << pAddr->x << " " << pAddr->y << " " << pAddr->z << " " << pAddr->level;
  return std::string(msg.str());
}

// ----------------------------------------------------------------------
const etree_tick_t cencalvm::average::AvgEngine::_LEFTMOSTONE =
  ~(~((etree_tick_t)0) >> 1);
const short cencalvm::average::AvgEngine::_NODATA = 999;

// ----------------------------------------------------------------------
// Default constructor
cencalvm::average::AvgEngine::AvgEngine(etree_t* dbOut,
					etree_t* dbIn,
				cencalvm::storage::ErrorHandler& errHandler) :
  _dbAvg(dbOut),
  _dbIn(dbIn),
  _pPendingOctants(0),
  _pendingSize(0),
  _errHandler(errHandler)
{ // constructor
  const int pendingSize = ETREE_MAXLEVEL + 1;
  _pPendingOctants = new OctantPendingStruct[pendingSize];
  for (int i=0; i < pendingSize; ++i) {
    _pPendingOctants[i].data.pSum = new cencalvm::storage::PayloadStruct;
    _pPendingOctants[i].data.numChildren = 0;
    _pPendingOctants[i].pAddr = new etree_addr_t;
    _pPendingOctants[i].processedChildren = 0x00;
    _pPendingOctants[i].isValid = false;
  } // if
  _pendingSize = pendingSize;
  
  _octantCounter.input = 0;
  _octantCounter.output = 0;
  _octantCounter.interior = 0;
  _octantCounter.inc_x = 0;
  _octantCounter.inc_y = 0;
  _octantCounter.inc_z = 0;
  _octantCounter.inc_xy = 0;
  _octantCounter.inc_yz = 0;
  _octantCounter.inc_xz = 0;
  _octantCounter.inc_xyz = 0;
  _octantCounter.inc_invalid = 0;  
} // constructor
  
// ----------------------------------------------------------------------
// Default destructor.
cencalvm::average::AvgEngine::~AvgEngine(void)
{ // destructor
  for (int i=0; i < _pendingSize; ++i) {
    delete _pPendingOctants[i].data.pSum; _pPendingOctants[i].data.pSum = 0;
    delete _pPendingOctants[i].pAddr; _pPendingOctants[i].pAddr = 0;
  } // for
  delete[] _pPendingOctants; _pPendingOctants = 0;
  _pendingSize = 0;
} // destructor

// ----------------------------------------------------------------------
// Fill in octants with averages of their children
void
cencalvm::average::AvgEngine::fillOctants(void)
{ // fillOctants
  etree_addr_t cursor;
  cursor.x = 0;
  cursor.y = 0;
  cursor.z = 0;
  cursor.level = ETREE_MAXLEVEL;

  int eof = etree_initcursor(_dbIn, cursor);
  if (eof) {
    _errHandler.error("Error occurred while initializing etree cursor.");
    return;
  } // if

  int err = etree_beginappend(_dbAvg, 1.0);
  if (0 != err) {
    _errHandler.error("Error occurred while trying to initiate appending "
		      "of etree.");
  } // if

  cencalvm::storage::PayloadStruct payload;
  while (!eof) {
    eof = etree_getcursor(_dbIn, &cursor, 0, &payload);
    if (eof) {
      _errHandler.error("Error occurred while trying to get payload at "
			"current cursor position.");
      return;
    } // if
    _averageOctant(&cursor, payload);
    if (cencalvm::storage::ErrorHandler::OK == _errHandler.status())
      eof = etree_advcursor(_dbIn);
    else
      return;
  } // while

  _finishProcessing();
  if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
    return;

  err = etree_endappend(_dbAvg);
  if (0 != err) {
    _errHandler.error("Error occurred while trying to terminate appending "
		      "of etree.");
    return;
  } // if
} // fillOctants

// ----------------------------------------------------------------------
// Print octant counting information to stream.
void
cencalvm::average::AvgEngine::printOctantInfo(void) const
{ // printOctantInfo
  std::cout
    << "Summary of etree averaging\n"
    << "Number of octants\n"
    << "  IN: " << _octantCounter.input << "\n"
    << "  OUT total: " << _octantCounter.output
    << ", interior: " << _octantCounter.interior << "\n"
    << "Incomplete octants\n"
    << "  x: " << _octantCounter.inc_x << "\n"
    << "  y: " << _octantCounter.inc_y << "\n"
    << "  z: " << _octantCounter.inc_z << "\n"
    << "  xy: " << _octantCounter.inc_xy << "\n"
    << "  yz: " << _octantCounter.inc_yz << "\n"
    << "  xz: " << _octantCounter.inc_xz << "\n"
    << "  xyz: " << _octantCounter.inc_xyz << "\n"
    << "  invalid: " << _octantCounter.inc_invalid << "\n"
    << std::endl;
} // printOctantInfo

// ----------------------------------------------------------------------
// Do average processing on octant.
void
cencalvm::average::AvgEngine::_averageOctant(etree_addr_t* pAddr,
			    const cencalvm::storage::PayloadStruct& payload)
{ // _processOctant
  std::cout << "_averageOctant pAddr:" << printAddr(pAddr) << std::endl;

  assert(0 != pAddr);

  // if at root, we are done
  if (pAddr->level <= 0)
    return;

  int pendingLevel = _findParent(pAddr);
  if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
    return;
  assert(pendingLevel >= 0 && pendingLevel == pAddr->level-1);

  std::cout << "ETREE (_averageOctant): Appending " << printAddr(pAddr) << " to database." << std::endl;

  int err = etree_append(_dbAvg, *pAddr, &payload);
  if (0 != err) {
    _errHandler.error("Error occurred while trying to append octant "
		      "to etree.");
  } // if
  ++_octantCounter.input;
  ++_octantCounter.output;

  _addToParent(&_pPendingOctants[pendingLevel], pAddr, payload);
  if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
    return;
  
  // if completed processing of octant, update higher levels
  while(pendingLevel >= 0 &&
	_pPendingOctants[pendingLevel].processedChildren == 0xFF) {
    _processOctant(pendingLevel);
    if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
      return;
    --pendingLevel;
  } // while
} // _processOctant

// ----------------------------------------------------------------------
// Complete processing
void
cencalvm::average::AvgEngine::_finishProcessing(void)
{ // _finishProcessing
  std::cout << "_finishProcessing" << std::endl;

  // Complete processing of the octants, begin at the bottom of the
  // tree and work towards the root
  for (int pendingLevel = _pendingSize-1; pendingLevel >= 0; --pendingLevel) {
    if (_pPendingOctants[pendingLevel].isValid) {
      _processOctant(pendingLevel);
      if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
	return;
    } // if
  } // for
} // _finishProcessing
  
// ----------------------------------------------------------------------
// Are octant addresses the same.
bool
cencalvm::average::AvgEngine::_sameAddr(etree_addr_t* pA,
					etree_addr_t* pB) const
{ // _sameAddr
  assert(0 != pA);
  assert(0 != pB);

  return (pA->x == pB->x &&
	  pA->y == pB->y &&
	  pA->z == pB->z &&
	  pA->level == pB->level);
} // _sameAddr
  
// ----------------------------------------------------------------------
// Create a new octant in the average database and append it to the
void
cencalvm::average::AvgEngine::_createOctant(etree_addr_t* pAddr)
{ // _createOctant
  std::cout << "_createOctant pAddr: " << printAddr(pAddr) << std::endl;

  assert(0 != pAddr);
  assert(0 != _pPendingOctants);

  const int pendingLevel = pAddr->level;
  assert(!_pPendingOctants[pendingLevel].isValid);

  cencalvm::storage::PayloadStruct payload;

  std::cout << "ETREE (createOctant): Appending " << printAddr(pAddr) << " to database." << std::endl;

  int err = etree_append(_dbAvg, *pAddr, &payload);
  if (0 != err) {
    _errHandler.error("Error occurred while appending new octant to etree.");
    return;
  } // if

  *_pPendingOctants[pendingLevel].pAddr = *pAddr;
  _pPendingOctants[pendingLevel].processedChildren = 0x00;
  _pPendingOctants[pendingLevel].isValid = true;
  _pPendingOctants[pendingLevel].data.numChildren = 0;
  _pPendingOctants[pendingLevel].data.pSum->Vp = 0;
  _pPendingOctants[pendingLevel].data.pSum->Vs = 0;
  _pPendingOctants[pendingLevel].data.pSum->Density = 0;
  _pPendingOctants[pendingLevel].data.pSum->Qp = 0;
  _pPendingOctants[pendingLevel].data.pSum->Qs = 0;
  _pPendingOctants[pendingLevel].data.pSum->DepthFreeSurf = 0;
  _pPendingOctants[pendingLevel].data.pSum->FaultBlock = 0;
  _pPendingOctants[pendingLevel].data.pSum->Zone = 0;

  ++_octantCounter.output;
  ++_octantCounter.interior;

  std::cout << "_createOctant: Created pending octant w/index " << pendingLevel << " at " << printAddr(pAddr) << std::endl;
} // _createOctant

// ----------------------------------------------------------------------
// Get bit associated with child's location relative to parent.
unsigned char
cencalvm::average::AvgEngine::_childOctantBit(etree_addr_t* pAddr) const
{ // _childOctantBit
  assert(0 != pAddr);

  const etree_tick_t mask = _LEFTMOSTONE >> pAddr->level;
  unsigned int shifts = (mask & pAddr->z) ? 4 : 0;
  shifts += (mask & pAddr->y) ? 2 : 0;
  shifts += (mask & pAddr->x) ? 1 : 0;

  return (1 << shifts);
} // _childOctantBit

// ----------------------------------------------------------------------
// Add contribution of octant to parent.
void
cencalvm::average::AvgEngine::_addToParent(OctantPendingStruct* pPendingParent,
					   etree_addr_t* pAddrChild,
			const cencalvm::storage::PayloadStruct& childPayload)
{ // _addToParent
  assert(0 != pPendingParent);
  assert(0 != pAddrChild);

  std::cout << "_addToParent: child is " << printAddr(pAddrChild)
	    << ", I am " << printAddr(pPendingParent->pAddr)
	    << std::endl;


  unsigned char childBit = _childOctantBit(pAddrChild);
  std::cout << "_addToParent: _childOctantBit: " << int(childBit) << std::endl;
  
  if (pPendingParent->processedChildren & childBit) {
    _errHandler.error("Consistency check for parent/child failed while "
		      "trying to add child's contribution to parent.");
    return;
  } // if

  pPendingParent->processedChildren |= childBit;

  ++pPendingParent->data.numChildren;
  pPendingParent->data.pSum->Vp += childPayload.Vp;
  pPendingParent->data.pSum->Vs += childPayload.Vs;
  pPendingParent->data.pSum->Density += childPayload.Density;
  pPendingParent->data.pSum->Qp += childPayload.Qp;
  pPendingParent->data.pSum->Qs += childPayload.Qs;
  pPendingParent->data.pSum->DepthFreeSurf += childPayload.DepthFreeSurf;

} // _addToParent
		    
// ----------------------------------------------------------------------
// Update pending octant.
void 
cencalvm::average::AvgEngine::_updateOctant(etree_addr_t* pAddr,
		       const cencalvm::storage::PayloadStruct& childPayload)
{ // _updateOctant
  std::cout << "_updateOctant pAddr:" << printAddr(pAddr) << std::endl;
  assert(0 != pAddr);

  std::cout << "ETREE (_updateOctant): updating " << printAddr(pAddr) << std::endl;

  int err = etree_update(_dbAvg, *pAddr, &childPayload);
  if (0 != err) {
    _errHandler.error("Could not update octant in etree database.");
    return;
  } // if
} // _updateOctant

// ----------------------------------------------------------------------
// Process pending octant.
void
cencalvm::average::AvgEngine::_processOctant(const int pendingLevel)
{ // _processOctant
  std::cout << "_processOctant pendingLevel: " << pendingLevel << std::endl;

  static const unsigned char X0_Y0_Z0 = 0x01;
  static const unsigned char X1_Y0_Z0 = 0x02;
  static const unsigned char X0_Y1_Z0 = 0x04;
  static const unsigned char X1_Y1_Z0 = 0x08;
  static const unsigned char X0_Y0_Z1 = 0x10;
  static const unsigned char X1_Y0_Z1 = 0x20;
  static const unsigned char X0_Y1_Z1 = 0x40;
  //static const unsigned char X1_Y1_Z1 = 0x80;
  static const unsigned char INC_FULL = 0xFF;
  static const unsigned char INC_X = X0_Y0_Z0 | X0_Y1_Z0 | X0_Y0_Z1 | X0_Y1_Z1;
  static const unsigned char INC_Y = X0_Y0_Z0 | X1_Y0_Z0 | X0_Y0_Z1 | X1_Y0_Z1;
  static const unsigned char INC_Z = X0_Y0_Z0 | X1_Y0_Z0 | X0_Y1_Z0 | X1_Y1_Z0;
  static const unsigned char INC_XY = X0_Y0_Z0 | X0_Y0_Z1;
  static const unsigned char INC_YZ = X0_Y0_Z0 | X1_Y0_Z0;
  static const unsigned char INC_XZ = X0_Y0_Z0 | X0_Y1_Z0;
  static const unsigned char INC_XYZ = X0_Y0_Z0;

  assert(0 <= pendingLevel && pendingLevel < _pendingSize);

  OctantPendingStruct& pendingOctant = _pPendingOctants[pendingLevel];

  // Determine number of complete octants
  int numComplete = 0;
  switch(pendingOctant.processedChildren)
    { // switch
    case INC_FULL :
      numComplete = 8;
      break;
    case INC_X :
      numComplete = 4;
      ++_octantCounter.inc_x;
      break;
    case INC_Y :
      numComplete = 4;
      ++_octantCounter.inc_y;
      break;
    case INC_Z :
      numComplete = 4;
      ++_octantCounter.inc_z;
      break;
    case INC_XY :
      numComplete = 2;
      ++_octantCounter.inc_xy;
      break;
    case INC_YZ :
      numComplete = 2;
      ++_octantCounter.inc_yz;
      break;
    case INC_XZ :
      numComplete = 2;
      ++_octantCounter.inc_xz;
      break;
    case INC_XYZ :
      numComplete = 1;
      ++_octantCounter.inc_xyz;
      break;
    default :
      ++_octantCounter.inc_invalid;
      _errHandler.log("Found invalid incomplete octant.");
      // manually determine number of children
      for (int i=0, childBit=1, numComplete=0; i < 8; ++i, childBit<<=1)
	if (pendingOctant.processedChildren & childBit)
	  ++numComplete;
    } // switch

  cencalvm::storage::PayloadStruct payload;
  const int numChildren = pendingOctant.data.numChildren;
  payload.Vp = pendingOctant.data.pSum->Vp / numChildren;
  payload.Vs = pendingOctant.data.pSum->Vs / numChildren;
  payload.Density = pendingOctant.data.pSum->Density / numChildren;
  payload.Qp = pendingOctant.data.pSum->Qp / numChildren;
  payload.Qs = pendingOctant.data.pSum->Qs / numChildren;
  payload.DepthFreeSurf = pendingOctant.data.pSum->DepthFreeSurf / numChildren;
  payload.FaultBlock = _NODATA;
  payload.Zone = _NODATA;

  std::cout << "OCTANT "
	    << pendingOctant.pAddr->x << " "
	    << pendingOctant.pAddr->y << " "
	    << pendingOctant.pAddr->z << " "
	    << pendingOctant.pAddr->level
	    << ", Vp: " << payload.Vp
	    << ", Vs: " << payload.Vs
	    << ", Density: " << payload.Density
	    << ", Qp: " << payload.Qp
	    << ", Qs: " << payload.Qs
	    << ", DepthFreeSurf: " << payload.DepthFreeSurf
	    << ", FaultBlock: " << payload.FaultBlock
	    << ", Zone: " << payload.Zone
	    << std::endl;

  _updateOctant(pendingOctant.pAddr, payload);
  if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
    return;
  if (pendingLevel > 0) {
    _addToParent(&_pPendingOctants[pendingLevel-1], pendingOctant.pAddr,
		 payload);
    if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
      return;
  } // if

  // Flag octant as processed
  _pPendingOctants[pendingLevel].isValid = false;
} // _processOctant

// ----------------------------------------------------------------------
// Find parent octant.
int
cencalvm::average::AvgEngine::_findParent(etree_addr_t* pAddr)
{ // _findParent
  std::cout << "_findParent pAddr: " << printAddr(pAddr) << std::endl;

  static const unsigned char INC_FULL = 0xFF;

  assert(0 != pAddr);
  assert(pAddr->level >= 0);
  assert(pAddr->level <= _pendingSize);

  if (0 == pAddr->level)
    return -1;

  int pendingLevel = pAddr->level - 1;
  while (pendingLevel >= 0 && !_pPendingOctants[pendingLevel].isValid)
    --pendingLevel;
  
  etree_addr_t parent;
  while(pendingLevel >= 0 && _pPendingOctants[pendingLevel].isValid) {
    const etree_tick_t mask = ((signed) _LEFTMOSTONE) >> pendingLevel;
    parent.x = pAddr->x & mask;
    parent.y = pAddr->y & mask;
    parent.z = pAddr->z & mask;
    parent.level = pendingLevel;
    parent.t = 0;
    parent.type = ETREE_INTERIOR;

    // check for incomplete parents while accounting for domains that
    // are not "square". If the octant at the current level is fully
    // processed or it is not a parent of the current octant, then
    // flush it to the database.
    if (!_sameAddr(&parent, _pPendingOctants[pendingLevel].pAddr) ||
	INC_FULL == _pPendingOctants[pendingLevel].processedChildren) {
      _processOctant(pendingLevel);
      if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
	return -1;
      --pendingLevel;
    } else
      break;
  } // while
  
  for (++pendingLevel; pendingLevel < pAddr->level; ++pendingLevel) {
    const etree_tick_t mask = ((signed) _LEFTMOSTONE) >> pendingLevel;
    parent.x = pAddr->x & mask;
    parent.y = pAddr->y & mask;
    parent.z = pAddr->z & mask;
    parent.level = pendingLevel;
    parent.t = 0;
    parent.type = ETREE_INTERIOR;
 
    _createOctant(&parent);
    if (cencalvm::storage::ErrorHandler::OK != _errHandler.status())
      return -1;
  } // for

  return pendingLevel-1;
} // _findParent

// version
// $Id$

// End of file 