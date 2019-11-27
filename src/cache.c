//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//
#include "cache.h"

//
// TODO:Student Information
//
const char *studentName = "Mayunk Kulkarni";
const char *studentID   = "A53285335";
const char *email       = "mkulkarn@eng.ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//
uint32_t icache[1<<16];
uint32_t ilru[1<<16];
uint32_t dcache[1<<16];
uint32_t dlru[1<<16];
uint32_t l2cache[1<<16];
uint32_t l2lru[1<<16];
uint32_t icacheMask;
uint32_t dcacheMask;
uint32_t l2cacheMask;
uint32_t l2AssocBits;
uint32_t dAssocBits;
uint32_t iAssocBits;
uint32_t icacheBlockBits;
uint32_t icacheIndexBits;
uint32_t dcacheBlockBits;
uint32_t dcacheIndexBits;
uint32_t l2cacheBlockBits;
uint32_t l2cacheIndexBits;
//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
	uint32_t addrTag = ((addr >> icacheBlockBits) >> icacheIndexBits);
	uint32_t icacheIndex = (( addr / (blocksize * icacheAssoc) ) % icacheSets) * icacheAssoc;
	if(icacheHit(addrTag, icacheIndex)){
		return icacheHitTime;
	} else {
		return icacheMiss(addrTag, icacheIndex);
	}
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  uint32_t addrTag = ((addr >> dcacheBlockBits) >> dcacheIndexBits);
	uint32_t dcacheIndex = (( addr / (blocksize * dcacheAssoc) ) % dcacheSets) * dcacheAssoc;
	if(dcacheHit(addrTag, dcacheIndex)){
		return dcacheHitTime;
	} else {
		return dCacheMiss(addrTag, dcacheIndex);
	}
 
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
	uint32_t l2addrTag = ((addr >> l2cacheBlockBits) >> dcacheIndexBits);
	uint32_t l2cacheIndex - ((addr / (blocksize * l2cacheAssoc) ) % l2cacheSets) * l2cacheAssoc;
	if(l2cacheHit(addrTag, l2cacheIndex)){
		return l2cacheHitTime;
	} else {
		return l2cacheMiss(l2addrTag, l2cacheIndex);
	}
}

// l2 cache hit function
bool l2cacheHit(uint32_t addrTag, uint32_t l2cacheIndex)
{
	bool hit = false;

	for(int i=0; i<icacheAssoc; i++) {
		if(addrTag == l2cache[l2cacheIndex + i]){
			hit = true;
			uint32_t lcachelruIndex = (addr/(blocksize*l2cacheAssoc)) % l2cacheSets;
			l2lruUpdate(l2cachelruIndex, i);
			break;
		}
	}
	return hit;
}

// l2 cache miss function
uint32_t l2cacheMiss(uint32_t addrTag, uint32_t l2cacheIndex)
{
	uint32_t missPenalty = memspeed;
	uint32_t l2cachelruIndex = (addr/(blocksize*l2cacheAssoc)) % l2cacheSets;
	uint32_t indexOffset = l2lru[l2cachelruIndex + l2cacheAssoc - 1]; // lru cache block!
	uint32_t l2cacheIndex = ((addr/(blocksize*l2cacheAssoc))% l2cacheSets)*l2cacheAssoc;
	if(inclusive) {
		uint32_t badTag = l2cache[l2cacheIndex + indexOffset];
		if(badTag != 0){
			invalidate(badTag); // badTag will always be longer than tag size of l1 caches as they are lower assoc compared to l2
		}
	}
	l2cache[l2cacheIndex + indexOffset] = addrTag;
	l2lruUpdate(l2cachelruIndex, l2cacheAssoc - 1);
	return missPenalty + l2cacheHitTime;

}

// invalidation scheme: search and eliminate tag in i/d cache
void invalidate(uint32_t badTag)
{
	uint32_t dcacheBadTag = (badTag >> (l2AssocBits-dAssocBits));
	uint32_t icacheBadTag = (badTag >> (l2AssocBits-iAssocBits));
	for(int i=0; i < dcacheSets * dcacheAssoc; i++)
	{
		if(dcache[i] == dcacheBadTag){
			dcache[i] = 0;
			break;
		}
	}
  
	for(int i=0; i < icacheSets * icacheAssoc; i++)
	{
		if(icache[i] == icacheBadTag){
			icache[i] = 0;
			break;
		}
	}

}

// icache Hit function
bool icacheHit(uint32_t addrTag, uint32_t icacheIndex)
{
	bool hit = false;

  for(int i=0; i<icacheAssoc; i++) {
		if(addrTag == icache[icacheIndex+i]){
			hit = true;
			uint32_t icachelruIndex = (addr/(blocksize*icacheAssoc)) % icacheSets;
			ilruUpdate(icachelruIndex, i);
			break;
		}
	}
	return hit;
}

// icache Miss Function
uint32_t icacheMiss(uint32_t addrTag, uint32_t icacheIndex)
{
  uint32_t missPenalty = l2cache_access(addr);
	uint32_t icachelruIndex = (addr/(blocksize*icacheAssoc)) % icacheSets;
	uint32_t indexOffset = ilru[icachelruIndex + icacheAssoc - 1]; // lru cache block!
	uint32_t icacheIndex = ((addr/(blocksize*icacheAssoc))% icacheSets)*icacheAssoc;
	icache[icacheIndex + indexOffset] = addrTag;
	ilruUpdate(icachelruIndex, icacheAssoc - 1);
	return missPenalty + icacheHitTime;
}

// ilru updating function 
void ilruUpdate(uint32_t icachelruIndex, uint32_t mru)
{
	  int temp = ilru[icachelruIndex+mru];
	  for(int i=mru; i>0; i--) {
		 	ilru[icachelruIndex+i] = ilru[icachelruIndex+i-1];
	  }
	  ilru[icachelruIndex] = temp;
}

// dcache hit function
bool dcacheHit(uint32_t addrTag, uint32_t dcacheIndex)
{
	bool hit = false;

  for(int i=0; i<dcacheAssoc; i++) {
		if(addrTag == dcache[dcacheIndex+i]){
			hit = true;
			uint32_t dcachelruIndex = (addr/(blocksize*dcacheAssoc)) % dcacheSets;
			dlruUpdate(dcachelruIndex, i);
			break;
		}
	}
	return hit;
}

// dcache miss function
uint32_t dcacheMiss(uint32_t addrTag, uint32_t dcacheIndex)
{
  uint32_t missPenalty = l2cache_access(addr);
	uint32_t dcachelruIndex = (addr/(blocksize*dcacheAssoc)) % dcacheSets;
	uint32_t indexOffset = dlru[dcachelruIndex + dcacheAssoc - 1]; // lru cache block!
	uint32_t dcacheIndex = ((addr/(blocksize*dcacheAssoc))% dcacheSets)*dcacheAssoc;
	dcache[dcacheIndex + indexOffset] = addrTag;
	dlruUpdate(dcachelruIndex, dcacheAssoc - 1);
	return missPenalty + dcacheHitTime;
}

// dlru update function
void dlruUpdate(uint32_t dcachelruIndex, uint32_t mru)
{
	  int temp = dlru[dcachelruIndex+mru];
	  for(int i=mru; i>0; i--) {
		 	dlru[dcachelruIndex+i] = dlru[dcachelruIndex+i-1];
	  }
	  dlru[dcachelruIndex] = temp;
}

