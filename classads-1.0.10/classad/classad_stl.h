/***************************************************************
 *
 * Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/


#ifndef __CLASSAD_CLASSAD_STL_H__
#define __CLASSAD_CLASSAD_STL_H__

#include <map>
#include <list>

#ifdef WIN32
#include <hash_map>
#else
#include <ext/hash_map>
#endif

#ifdef WIN32
#define classad_hash_map stdext::hash_map
#else
#define classad_hash_map __gnu_cxx::hash_map
#endif
#define classad_map   std::map 
#define classad_slist std::list

#endif /* __CLASSAD_CLASSAD_STL_H__ */
