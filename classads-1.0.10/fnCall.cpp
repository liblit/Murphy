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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "classad/common.h"
#include "classad/exprTree.h"
#include "classad/source.h"
#include "classad/sink.h"
#include "classad/util.h"

#if defined(WIN32) && !defined(USE_PCRE) && !defined(USE_POSIX_REGEX)
  #define USE_PCRE
  #define HAVE_PCRE_H
#endif

#if defined USE_POSIX_REGEX 
  #include <regex.h>
#elif defined USE_PCRE
  #ifdef HAVE_PCRE_H
    #include <pcre.h>
  #elif defined HAVE_PCRE_PCRE_H
    #include <pcre/pcre.h>
  #endif
#endif

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

using namespace std;

BEGIN_NAMESPACE( classad )

bool FunctionCall::initialized = false;

static bool doSplitTime(
    const Value &time, ClassAd * &splitClassAd);
static void absTimeToClassAd(
    const abstime_t &asecs, ClassAd * &splitClassAd);
static void relTimeToClassAd(
    double rsecs, ClassAd * &splitClassAd);
static void make_formatted_time(
    const struct tm &time_components, string &format, Value &result);

// start up with an argument list of size 4
FunctionCall::
FunctionCall( )
{
	nodeKind = FN_CALL_NODE;
	function = NULL;

	if( !initialized ) {
        FuncTable &functionTable = getFunctionTable();

		// load up the function dispatch table
			// type predicates
		functionTable["isundefined"	] = (void*)isType;
		functionTable["iserror"		] =	(void*)isType;
		functionTable["isstring"	] =	(void*)isType;
		functionTable["isinteger"	] =	(void*)isType;
		functionTable["isreal"		] =	(void*)isType;
		functionTable["islist"		] =	(void*)isType;
		functionTable["isclassad"	] =	(void*)isType;
		functionTable["isboolean"	] =	(void*)isType;
		functionTable["isabstime"	] =	(void*)isType;
		functionTable["isreltime"	] =	(void*)isType;

			// list membership
		functionTable["member"		] =	(void*)testMember;
		functionTable["identicalmember"	] =	(void*)testMember;

		// Some list functions, useful for lists as sets
		functionTable["size"        ] = (void*)size;
		functionTable["sum"         ] = (void*)sumAvg;
		functionTable["avg"         ] = (void*)sumAvg;
		functionTable["min"         ] = (void*)minMax;
		functionTable["max"         ] = (void*)minMax;
		functionTable["anycompare"  ] = (void*)listCompare;
		functionTable["allcompare"  ] = (void*)listCompare;

			// basic apply-like functions
		/*
		functionTable["sumfrom"		sumAvgFrom );
		functionTable["avgfrom"		sumAvgFrom );
		functionTable["maxfrom"		boundFrom );
		functionTable["minfrom"		boundFrom );
		*/

			// time management
        functionTable["time"        ] = (void*)epochTime;
        functionTable["currenttime"	] =	(void*)currentTime;
		functionTable["timezoneoffset"] =(void*)timeZoneOffset;
		functionTable["daytime"		] =	(void*)dayTime;
        //functionTable["makedate"	] =	(void*)makeDate;
		functionTable["getyear"		] =	(void*)getField;
		functionTable["getmonth"	] =	(void*)getField;
		functionTable["getdayofyear"] =	(void*)getField;
		functionTable["getdayofmonth"] =(void*)getField;
		functionTable["getdayofweek"] =	(void*)getField;
		functionTable["getdays"		] =	(void*)getField;
		functionTable["gethours"	] =	(void*)getField;
		functionTable["getminutes"	] =	(void*)getField;
		functionTable["getseconds"	] =	(void*)getField;
        functionTable["splittime"   ] = (void*)splitTime;
        functionTable["formattime"  ] = (void*)formatTime;
        //functionTable["indays"		] =	(void*)inTimeUnits;
		//functionTable["inhours"		] =	(void*)inTimeUnits;
		//functionTable["inminutes"	] =	(void*)inTimeUnits;
		//functionTable["inseconds"	] =	(void*)inTimeUnits;
		
			// string manipulation
		functionTable["strcat"		] =	(void*)strCat;
		functionTable["toupper"		] =	(void*)changeCase;
		functionTable["tolower"		] =	(void*)changeCase;
		functionTable["substr"		] =	(void*)subString;
        functionTable["strcmp"      ] = (void*)compareString;
        functionTable["stricmp"     ] = (void*)compareString;

			// pattern matching (regular expressions) 
#if defined USE_POSIX_REGEX || defined USE_PCRE
		functionTable["regexp"		] =	(void*)matchPattern;
        functionTable["regexpmember"] =	(void*)matchPatternMember;
		functionTable["regexps"     ] = (void*)substPattern;
#endif

			// conversion functions
		functionTable["int"			] =	(void*)convInt;
		functionTable["real"		] =	(void*)convReal;
		functionTable["string"		] =	(void*)convString;
		functionTable["bool"		] =	(void*)convBool;
		functionTable["absTime"		] =	(void*)convTime;
		functionTable["relTime"		] = (void*)convTime;

			// mathematical functions
		functionTable["floor"		] =	(void*)doMath;
		functionTable["ceil"		] =	(void*)doMath;
		functionTable["ceiling"		] =	(void*)doMath;
		functionTable["round"		] =	(void*)doMath;
        functionTable["random"      ] = (void*)random;

			// for compatibility with old classads:
		functionTable["ifThenElse"  ] = (void*)ifThenElse;
		functionTable["interval" ] = (void*)interval;
		functionTable["eval"] = (void*)eval;

		initialized = true;
	}
}

FunctionCall::
~FunctionCall ()
{
	for( ArgumentList::iterator i=arguments.begin(); i!=arguments.end(); i++ ) {
		delete (*i);
	}
}

FunctionCall::
FunctionCall(FunctionCall &functioncall)
{
    CopyFrom(functioncall);
    return;
}

FunctionCall & FunctionCall::
operator=(FunctionCall &functioncall)
{
    if (this != &functioncall) {
        CopyFrom(functioncall);
    }
    return *this;
}

ExprTree *FunctionCall::
Copy( ) const
{
	FunctionCall *newTree = new FunctionCall;

	if (!newTree) return NULL;

    if (!newTree->CopyFrom(*this)) {
        delete newTree;
        newTree = NULL;
    }
	return newTree;
}

bool FunctionCall::
CopyFrom(const FunctionCall &functioncall)
{
    bool     success;
	ExprTree *newArg;

    success      = true;
    ExprTree::CopyFrom(functioncall);
    functionName = functioncall.functionName;
	function     = functioncall.function;

	for (ArgumentList::const_iterator i = functioncall.arguments.begin(); 
         i != functioncall.arguments.end();
         i++ ) {
		newArg = (*i)->Copy( );
		if( newArg ) {
			arguments.push_back( newArg );
		} else {
            success = false;
            break;
		}
	}
    return success;
}

bool FunctionCall::
SameAs(const ExprTree *tree) const
{
    bool is_same;
    const FunctionCall *other_fn;
    
    if (this == tree) {
        is_same = true;
    } else if (tree->GetKind() != FN_CALL_NODE) {
        is_same = false;
    } else {
        other_fn = (const FunctionCall *) tree;
        
        if (functionName == other_fn->functionName
            && function == other_fn->function
            && arguments.size() == other_fn->arguments.size()) {
            
            is_same = true;
            ArgumentList::const_iterator a1 = arguments.begin();
            ArgumentList::const_iterator a2 = other_fn->arguments.begin();
            while (a1 != arguments.end()) {
                if (a2 == other_fn->arguments.end()) {
                    is_same = false;
                    break;
                } else if (!(*a1)->SameAs((*a2))) {
                    is_same = false;
                    break;
                }
				a1++;
				a2++;
            }
        } else {
            is_same = false;
        }
    }

    return is_same;
}

bool operator==(const FunctionCall &fn1, const FunctionCall &fn2)
{
    return fn1.SameAs(&fn2);
}

FunctionCall::FuncTable& FunctionCall::
getFunctionTable(void)
{
    static FuncTable functionTable;
    return functionTable;
}

void FunctionCall::RegisterFunction(
	string &functionName, 
	ClassAdFunc function)
{
    FuncTable &functionTable = getFunctionTable();

	if (functionTable.find(functionName) == functionTable.end()) {
		functionTable[functionName] = (void *) function;
	}
	return;
}

void FunctionCall::RegisterFunctions(
	ClassAdFunctionMapping *functions)
{
	if (functions != NULL) {
		while (functions->function != NULL) {
			RegisterFunction(functions->functionName, 
							 (ClassAdFunc) functions->function);
			functions++;
		}
	}
	return;
}

bool FunctionCall::RegisterSharedLibraryFunctions(
	const char *shared_library_path)
{
#ifdef HAVE_DLOPEN
	bool success;
	void *dynamic_library_handle;
		
	success = false;
	if (shared_library_path) {
		dynamic_library_handle = dlopen(shared_library_path, RTLD_LAZY|RTLD_GLOBAL);
		if (dynamic_library_handle) {
			ClassAdSharedLibraryInit init_function;

			init_function = (ClassAdSharedLibraryInit) dlsym(dynamic_library_handle, 
															 "Init");
			if (init_function != NULL) {
				ClassAdFunctionMapping *functions;
				
				functions = init_function();
				if (functions != NULL) {
					RegisterFunctions(functions);
					success = true;
					/*
					while (functions->apparentFunctionName != NULL) {
						void *function;
						string functionName = functions->apparentFunctionName;
						function = dlsym(dynamic_library_handle, 
										 functions->actualFunctionName);
						RegisterFunction(functionName,
										 (ClassAdFunc) function);
						success = true;
						functions++;
					}
					*/
				} else {
					CondorErrno  = ERR_CANT_LOAD_DYNAMIC_LIBRARY;
					CondorErrMsg = "Init function returned NULL.";
				}
			} else {
				CondorErrno  = ERR_CANT_LOAD_DYNAMIC_LIBRARY;
				CondorErrMsg = "Couldn't find Init() function.";
			}
		} else {
			CondorErrno  = ERR_CANT_LOAD_DYNAMIC_LIBRARY;
			CondorErrMsg = "Couldn't open shared library with dlopen.";
		}
	} else {
		CondorErrno  = ERR_CANT_LOAD_DYNAMIC_LIBRARY;
		CondorErrMsg = "No shared library was specified.";
	}

	return success;
#else /* end HAVE_DLOPEN */
    CondorErrno = ERR_CANT_LOAD_DYNAMIC_LIBRARY;
    CondorErrMsg = "Shared library support not available.";
    return false;
#endif /* end !HAVE_DLOPEN */
}

void FunctionCall::
_SetParentScope( const ClassAd* parent )
{
	for( ArgumentList::iterator i=arguments.begin(); i!=arguments.end(); i++ ) {
		(*i)->SetParentScope( parent );
	}
}
	

FunctionCall *FunctionCall::
MakeFunctionCall( const string &str, vector<ExprTree*> &args )
{
	FunctionCall *fc = new FunctionCall;
	if( !fc ) {
		vector<ExprTree*>::iterator i = args.begin( );
		while(i != args.end()) {
			delete *i;
			i++;
		}
		CondorErrno = ERR_MEM_ALLOC_FAILED;
		CondorErrMsg = "";
		return( NULL );
	}

    FuncTable &functionTable = getFunctionTable();
	FuncTable::iterator	itr = functionTable.find( str );

	if( itr != functionTable.end( ) ) {
		fc->function = (ClassAdFunc)itr->second;
	} else {
		fc->function = NULL;
	}

	fc->functionName = str;

	for( ArgumentList::iterator i = args.begin(); i != args.end( ); i++ ) {
		fc->arguments.push_back( *i );
	}
	return( fc );
}


void FunctionCall::
GetComponents( string &fn, vector<ExprTree*> &args ) const
{
	fn = functionName;
	for( ArgumentList::const_iterator i = arguments.begin(); 
			i != arguments.end( ); i++ ) {
		args.push_back( *i );
	}
}


bool FunctionCall::
_Evaluate (EvalState &state, Value &value) const
{
	if( function ) {
		return( (*function)( functionName.c_str( ), arguments, state, value ) );
	} else {
		value.SetErrorValue();
		return( true );
	}
}

bool FunctionCall::
_Evaluate( EvalState &state, Value &value, ExprTree *& tree ) const
{
	FunctionCall *tmpSig = new FunctionCall;
	Value		tmpVal;
	ExprTree	*argSig;
	bool		rval;

	if( !tmpSig ) {
		CondorErrno = ERR_MEM_ALLOC_FAILED;
		CondorErrMsg = "";
		return( false );
	}
	
	if( !_Evaluate( state, value ) ) {
		delete tmpSig;
		return false;
	}

	tmpSig->functionName = functionName;
	rval = true;
	for(ArgumentList::const_iterator i=arguments.begin();i!=arguments.end();
			i++) {
		rval = (*i)->Evaluate( state, tmpVal, argSig );
		if( rval ) tmpSig->arguments.push_back( argSig );
	}
	tree = tmpSig;

	if( !rval ) delete tree;
	return( rval );
}

bool FunctionCall::
_Flatten( EvalState &state, Value &value, ExprTree*&tree, int* ) const
{
	FunctionCall *newCall;
	ExprTree	*argTree;
	Value		argValue;
	bool		fold = true;

	tree = NULL; // Just to be safe...  wenger 2003-12-11.

	// if the function cannot be resolved, the value is "error"
	if( !function ) {
		value.SetErrorValue();
		tree = NULL;
		return true;
	}

	// create a residuated function call with flattened args
	if( ( newCall = new FunctionCall() ) == NULL ) {
		CondorErrno = ERR_MEM_ALLOC_FAILED;
		CondorErrMsg = "";
		return false;
	}
	newCall->functionName = functionName;
	newCall->function = function;

	// flatten the arguments
	for(ArgumentList::const_iterator i=arguments.begin();i!=arguments.end();
			i++ ) {
		if( (*i)->Flatten( state, argValue, argTree ) ) {
			if( argTree ) {
				newCall->arguments.push_back( argTree );
				fold = false;
				continue;
			} else {
				// Assert: argTree == NULL
				argTree = Literal::MakeLiteral( argValue );
				if( argTree ) {
					newCall->arguments.push_back( argTree );
					continue;
				}
			}
		} 

		// we get here only when something bad happens
		delete newCall;
		value.SetErrorValue();
		tree = NULL;
		return false;
	} 
	
	// assume all functions are "pure" (i.e., side-affect free)
	if( fold ) {
			// flattened to a value
		if(!(*function)(functionName.c_str(),arguments,state,value)) {
			delete newCall;
			return false;
		}
		tree = NULL;
		delete newCall;
	} else {
		tree = newCall;
	}

	return true;
}


bool FunctionCall::
isType (const char *name, const ArgumentList &argList, EvalState &state, 
	Value &val)
{
    Value   arg;

    // need a single argument
    if (argList.size() != 1) {
        val.SetErrorValue ();
        return( true );
    }

    // Evaluate the argument
    if( !argList[0]->Evaluate( state, arg ) ) {
		val.SetErrorValue( );
		return false;
	}

    // check if the value was of the required type
	if( strcasecmp( name, "isundefined" ) == 0 ) {
		val.SetBooleanValue( arg.IsUndefinedValue( ) );
	} else if( strcasecmp( name, "iserror" ) == 0 ) {
		val.SetBooleanValue( arg.IsErrorValue( ) );
	} else if( strcasecmp( name, "isinteger" ) == 0 ) {
		val.SetBooleanValue( arg.IsIntegerValue( ) );
	} else if( strcasecmp( name, "isstring" ) == 0 ) {
		val.SetBooleanValue( arg.IsStringValue( ) );
	} else if( strcasecmp( name, "isreal" ) == 0 ) {
		val.SetBooleanValue( arg.IsRealValue( ) );
	} else if( strcasecmp( name, "isboolean" ) == 0 ) {
		val.SetBooleanValue( arg.IsBooleanValue( ) );
	} else if( strcasecmp( name, "isclassad" ) == 0 ) {
		val.SetBooleanValue( arg.IsClassAdValue( ) );
	} else if( strcasecmp( name, "islist" ) == 0 ) {
		val.SetBooleanValue( arg.IsListValue( ) );
	} else if( strcasecmp( name, "isabstime" ) == 0 ) {
		val.SetBooleanValue( arg.IsAbsoluteTimeValue( ) );
	} else if( strcasecmp( name, "isreltime" ) == 0 ) {
		val.SetBooleanValue( arg.IsRelativeTimeValue( ) );
	} else {
		val.SetErrorValue( );
	}
	return( true );
}


bool FunctionCall::
testMember(const char *name,const ArgumentList &argList, EvalState &state, 
	Value &val)
{
    Value     		arg0, arg1, cArg;
    const ExprTree 	*tree;
	const ExprList	*el;
	bool			b;
	bool			useIS = ( strcasecmp( "identicalmember", name ) == 0 );

    // need two arguments
    if (argList.size() != 2) {
        val.SetErrorValue();
        return( true );
    }

    // Evaluate the arg list
    if( !argList[0]->Evaluate(state,arg0) || !argList[1]->Evaluate(state,arg1)){
		val.SetErrorValue( );
		return false;
	}

		// if the second arg (a list) is undefined, or the first arg is
		// undefined and we're supposed to test for strict comparison, the 
		// result is 'undefined'
    if( arg1.IsUndefinedValue() || ( !useIS && arg0.IsUndefinedValue() ) ) {
        val.SetUndefinedValue();
        return true;
    }

#ifdef FLEXIBLE_MEMBER
    if (arg0.IsListValue() && !arg1.IsListValue()) {
        Value swap;

        swap.CopyFrom(arg0);
        arg0.CopyFrom(arg1);
        arg1.CopyFrom(swap);
    }
#endif

		// arg1 must be a list; arg0 must be comparable
    if( !arg1.IsListValue() || arg0.IsListValue() || arg0.IsClassAdValue() ) {
        val.SetErrorValue();
        return true;
    }

		// if we're using strict comparison, arg0 can't be 'error'
	if( !useIS && arg0.IsErrorValue( ) ) {
		val.SetErrorValue( );
		return( true );
	}

    // check for membership
	arg1.IsListValue( el );
	ExprListIterator itr( el );
	while( ( tree = itr.CurrentExpr( ) ) ) {
		if( !tree->Evaluate( state, cArg ) ) {
			val.SetErrorValue( );
			return( false );
		}
		Operation::Operate(useIS ? Operation::IS_OP : Operation::EQUAL_OP, 
			cArg, arg0, val );
		if( val.IsBooleanValue( b ) && b ) {
			return true;
		}
		itr.NextExpr( );
	}
	val.SetBooleanValue( false );	

    return true;
}

/*
bool FunctionCall::
sumAvgFrom (char *name, const ArgumentList &argList, EvalState &state, Value &val)
{
	Value		caVal, listVal;
	ExprTree	*ca;
	Value		tmp, result;
	ExprList	*el;
	ClassAd		*ad;
	bool		first = true;
	int			len;
	bool		onlySum = ( strcasecmp( "sumfrom", name ) == 0 );

	// need two arguments
	if( argList.getlast() != 1 ) {
		val.SetErrorValue();
		return true;
	}

	// first argument must Evaluate to a list
	if( !argList[0]->Evaluate( state, listVal ) ) {
		val.SetErrorValue( );
		return( false );
	} else if( listVal.IsUndefinedValue() ) {
		val.SetUndefinedValue( );
		return( true );
	} else if( !listVal.IsListValue( el ) ) {
		val.SetErrorValue();
		return( true );
	}

	el->Rewind();
	result.SetUndefinedValue();
	while( ( ca = el->Next() ) ) {
		if( !ca->Evaluate( state, caVal ) ) {
			val.SetErrorValue( );
			return( false );
		} else if( !caVal.IsClassAdValue( ad ) ) {
			val.SetErrorValue();
			return( true );
		} else if( !ad->EvaluateExpr( argList[1], tmp ) ) {
			val.SetErrorValue( );
			return( false );
		}
		if( first ) {
			result.CopyFrom( tmp );
			first = false;
		} else {
			Operation::Operate( ADDITION_OP, result, tmp, result );
		}
		tmp.Clear();
	}

		// if the sumFrom( ) function was called, don't need to find average
	if( onlySum ) {
		val.CopyFrom( result );
		return( true );
	}


	len = el->Number();
	if( len > 0 ) {
		tmp.SetRealValue( len );
		Operation::Operate( DIVISION_OP, result, tmp, result );
	} else {
		val.SetUndefinedValue();
	}

	val.CopyFrom( result );
	return true;
}


bool FunctionCall::
boundFrom (char *fn, const ArgumentList &argList, EvalState &state, Value &val)
{
	Value		caVal, listVal, cmp;
	ExprTree	*ca;
	Value		tmp, result;
	ExprList	*el;
	ClassAd		*ad;
	bool		first = true, b=false, min;

	// need two arguments
	if( argList.getlast() != 1 ) {
		val.SetErrorValue();
		return( true );
	}

	// first argument must Evaluate to a list
	if( !argList[0]->Evaluate( state, listVal ) ) {
		val.SetErrorValue( );
		return( false );
	} else if( listVal.IsUndefinedValue( ) ) {
		val.SetUndefinedValue( );
		return( true );
	} else if( !listVal.IsListValue( el ) ) {
		val.SetErrorValue();
		return( true );
	}

	// fn is either "min..." or "max..."
	min = ( tolower( fn[1] ) == 'i' );

	el->Rewind();
	result.SetUndefinedValue();
	while( ( ca = el->Next() ) ) {
		if( !ca->Evaluate( state, caVal ) ) {
			val.SetErrorValue( );
			return false;
		} else if( !caVal.IsClassAdValue( ad ) ) {
			val.SetErrorValue();
			return( true );
		} else if( !ad->EvaluateExpr( argList[1], tmp ) ) {
			val.SetErrorValue( );
			return( true );
		}
		if( first ) {
			result.CopyFrom( tmp );
			first = false;
		} else {
			Operation::Operate(min?LESS_THAN_OP:GREATER_THAN_OP,tmp,result,cmp);
			if( cmp.IsBooleanValue( b ) && b ) {
				result.CopyFrom( tmp );
			}
		}
	}

	val.CopyFrom( result );
	return true;
}
*/

// The size of a list, ClassAd, etc. 
bool FunctionCall::
size(const char *, const ArgumentList &argList, 
	 EvalState &state, Value &val)
{
	Value             arg;
	const ExprList    *listToSize;
    ClassAd           *classadToSize;
	int			      length;

	// we accept only one argument
	if (argList.size() != 1) {
		val.SetErrorValue();
		return( true );
	}
	
	if (!argList[0]->Evaluate(state, arg)) {
		val.SetErrorValue();
		return false;
	} else if (arg.IsUndefinedValue()) {
		val.SetUndefinedValue();
		return true;
	} else if (arg.IsListValue(listToSize)) {
        length = listToSize->size();
        val.SetIntegerValue(length);
        return true;
	} else if (arg.IsClassAdValue(classadToSize)) {
        length = classadToSize->size();
        val.SetIntegerValue(length);
        return true;
    } else if (arg.IsStringValue(length)) {
        val.SetIntegerValue(length);
        return true;
    } else {
        val.SetErrorValue();
        return true;
    }
}

bool FunctionCall::
sumAvg(const char *name, const ArgumentList &argList, 
	   EvalState &state, Value &val)
{
	Value             listElementValue, listVal;
	const ExprTree    *listElement;
	Value             numElements, result;
	const ExprList    *listToSum;
	ExprListIterator  listIterator;
	bool		      first;
	int			      len;
	bool              onlySum = (strcasecmp("sum", name) == 0 );

	// we accept only one argument
	if (argList.size() != 1) {
		val.SetErrorValue();
		return( true );
	}
	
	// argument must Evaluate to a list
	if (!argList[0]->Evaluate(state, listVal)) {
		val.SetErrorValue();
		return false;
	} else if (listVal.IsUndefinedValue()) {
		val.SetUndefinedValue();
		return true;
	} else if (!listVal.IsListValue((const ExprList *&)listToSum)) {
		val.SetErrorValue();
		return( true );
	}

	onlySum = (strcasecmp("sum", name) == 0 );
	listIterator.Initialize(listToSum);
	result.SetUndefinedValue();
	len = 0;
	first = true;

	// Walk over each element in the list, and sum.
	for (listElement = listIterator.CurrentExpr();
		 listElement != NULL;
		 listElement = listIterator.NextExpr()) {
		if (listElement != NULL) {
			len++;
			// Make sure this element is a number.
			if (!listElement->Evaluate(state, listElementValue)) {
				val.SetErrorValue();
				return false;
			} else if (   !listElementValue.IsRealValue() 
						  && !listElementValue.IsIntegerValue()) {
				val.SetErrorValue();
				return true;
			}

			// Either take the number if it's the first, 
			// or add to the running sum.
			if (first) {
				result.CopyFrom(listElementValue);
				first = false;
			} else {
				Operation::Operate(Operation::ADDITION_OP, result, 
								   listElementValue, result);
			}
		}
	}

    // if the sum() function was called, we don't need to find the average
    if (onlySum) {
		val.CopyFrom(result);
		return true;
	}

	if (len > 0) {
		numElements.SetRealValue(len);
		Operation::Operate(Operation::DIVISION_OP, result, 
						   numElements, result);
	} else {
		val.SetUndefinedValue();
	}

	val.CopyFrom( result );
	return true;
}


bool FunctionCall::
minMax(const char *fn, const ArgumentList &argList, 
	   EvalState &state, Value &val)
{
	Value		       listElementValue, listVal, cmp;
	const ExprTree     *listElement;
	Value              result;
	const ExprList     *listToBound;
	ExprListIterator   listIterator;
    bool		       first = true, b = false;
	Operation::OpKind  comparisonOperator;

	// we accept only one argument
	if (argList.size() != 1) {
		val.SetErrorValue();
		return true;
	}

	// first argument must Evaluate to a list
	if(!argList[0]->Evaluate(state, listVal)) {
		val.SetErrorValue();
		return false;
	} else if (listVal.IsUndefinedValue()) {
		val.SetUndefinedValue();
		return true;
	} else if (!listVal.IsListValue((const ExprList *&)listToBound)) {
		val.SetErrorValue();
		return true;
	}

	// fn is either "min..." or "max..."
	if (tolower(fn[1]) == 'i') {
		comparisonOperator = Operation::LESS_THAN_OP;
	} else {
		comparisonOperator = Operation::GREATER_THAN_OP;
	}

	listIterator.Initialize(listToBound);
	result.SetUndefinedValue();

	// Walk over the list, calculating the bound the whole way.
	for (listElement = listIterator.CurrentExpr();
		 listElement != NULL;
		 listElement = listIterator.NextExpr()) {
		if (listElement != NULL) {

			// For this element of the list, make sure it is 
			// acceptable.
			if(!listElement->Evaluate(state, listElementValue)) {
				val.SetErrorValue();
				return false;
			} else if (   !listElementValue.IsRealValue() 
						  && !listElementValue.IsIntegerValue()) {
				val.SetErrorValue();
				return true;
			}
			
			// If it's the first element, copy it to the bound,
			// otherwise compare to decide what to do.
			if (first) {
				result.CopyFrom(listElementValue);
				first = false;
			} else {
				Operation::Operate(comparisonOperator, listElementValue, 
								   result, cmp);
				if (cmp.IsBooleanValue(b) && b) {
					result.CopyFrom(listElementValue);
				}
			}
		}
	}

	val.CopyFrom(result);
	return true;
}

bool FunctionCall::
listCompare(
	const char         *fn, 
	const ArgumentList &argList, 
	EvalState          &state, 
	Value              &val)
{
	Value		       listElementValue, listVal, compareVal;
	Value              stringValue;
	const ExprTree     *listElement;
	const ExprList     *listToCompare;
	ExprListIterator   listIterator;
    bool		       needAllMatch;
	string             comparison_string;
	Operation::OpKind  comparisonOperator;

	// We take three arguments:
	// The operator to use, as a string.
	// The list
	// The thing we are comparing against.
	if (argList.size() != 3) {
		val.SetErrorValue();
		return true;
	}

	// The first argument must be a string
	if (!argList[0]->Evaluate(state, stringValue)) {
		val.SetErrorValue();
		return false;
	} else if (stringValue.IsUndefinedValue()) {
		val.SetUndefinedValue();
		return true;
	} else if (!stringValue.IsStringValue(comparison_string)) {
		val.SetErrorValue();
		return true;
	}
	
	// Decide which comparison to do, or give an error
	if (comparison_string == "<") {
		comparisonOperator = Operation::LESS_THAN_OP;
	} else if (comparison_string == "<=") {
		comparisonOperator = Operation::LESS_OR_EQUAL_OP;
	} else if (comparison_string == "!=") {
		comparisonOperator = Operation::NOT_EQUAL_OP;
	} else if (comparison_string == "==") {
		comparisonOperator = Operation::EQUAL_OP;
	} else if (comparison_string == ">") {
		comparisonOperator = Operation::GREATER_THAN_OP;
	} else if (comparison_string == ">=") {
		comparisonOperator = Operation::GREATER_OR_EQUAL_OP;
	} else if (comparison_string == "is") {
		comparisonOperator = Operation::META_EQUAL_OP;
	} else if (comparison_string == "isnt") {
		comparisonOperator = Operation::META_NOT_EQUAL_OP;
	} else {
		val.SetErrorValue();
		return true;
	}

	// The second argument must Evaluate to a list
	if (!argList[1]->Evaluate(state, listVal)) {
		val.SetErrorValue();
		return false;
	} else if (listVal.IsUndefinedValue()) {
		val.SetUndefinedValue();
		return true;
	} else if (!listVal.IsListValue((const ExprList *&)listToCompare)) {
		val.SetErrorValue();
		return true;
	}

	// The third argument is something to compare against.
	if (!argList[2]->Evaluate(state, compareVal)) {
		val.SetErrorValue();
		return false;
	} else if (listVal.IsUndefinedValue()) {
		val.SetUndefinedValue();
		return true;
	}

	// Finally, we decide what to do exactly, based on our name.
	if (!strcasecmp(fn, "anycompare")) {
		needAllMatch = false;
		val.SetBooleanValue(false);
	} else {
		needAllMatch = true;
		val.SetBooleanValue(true);
	}

	listIterator.Initialize(listToCompare);

	// Walk over the list
	for (listElement = listIterator.CurrentExpr();
		 listElement != NULL;
		 listElement = listIterator.NextExpr()) {
		if (listElement != NULL) {

			// For this element of the list, make sure it is 
			// acceptable.
			if(!listElement->Evaluate(state, listElementValue)) {
				val.SetErrorValue();
				return false;
			} else {
				Value  compareResult;
				bool   b;

				Operation::Operate(comparisonOperator, listElementValue,
								   compareVal, compareResult);
				if (!compareResult.IsBooleanValue(b)) {
					if (compareResult.IsUndefinedValue()) {
						if (needAllMatch) {
							val.SetBooleanValue(false);
							return true;
						}
					} else {
						val.SetErrorValue();
						return false;
					}
					return true;
				} else if (b) {
					if (!needAllMatch) {
						val.SetBooleanValue(true);
						return true;
					}
				} else {
					if (needAllMatch) {
						// we failed, because it didn't match
						val.SetBooleanValue(false);
						return true;
					}
				}
			}
		}
	}

	if (needAllMatch) {
		// They must have all matched, because nothing failed,
		// which would have returned.
		val.SetBooleanValue(true);
	} else {
		// Nothing must have matched, since we would have already
		// returned.
		val.SetBooleanValue(false);
	}

	return true;
}

bool FunctionCall::
epochTime (const char *, const ArgumentList &argList, EvalState &, Value &val)
{

		// no arguments
	if( argList.size( ) > 0 ) {
		val.SetErrorValue( );
		return( true );
	}

    val.SetIntegerValue(time(NULL));
	return( true );
}

bool FunctionCall::
currentTime (const char *, const ArgumentList &argList, EvalState &, Value &val)
{

		// no arguments
	if( argList.size( ) > 0 ) {
		val.SetErrorValue( );
		return( true );
	}

	Literal *time_literal = Literal::MakeAbsTime(NULL);
    if (time_literal != NULL) {
        time_literal->GetValue(val);
        delete time_literal;
        return true;
    } else {
        return false;
    }
}


bool FunctionCall::
timeZoneOffset (const char *, const ArgumentList &argList, EvalState &, 
	Value &val)
{
		// no arguments
	if( argList.size( ) > 0 ) {
		val.SetErrorValue( );
		return( true );
	}
	
	val.SetRelativeTimeValue( (time_t) timezone_offset( time(NULL), false ) );
	return( true );
}

bool FunctionCall::
dayTime (const char *, const ArgumentList &argList, EvalState &, Value &val)
{
	time_t 		now;
	struct tm 	lt;
	if( argList.size( ) > 0 ) {
		val.SetErrorValue( );
		return( true );
	}
	time( &now );
	if( now == -1 ) {
		val.SetErrorValue( );
		return( false );
	}

	getLocalTime(&now, &lt);

	val.SetRelativeTimeValue((time_t) ( lt.tm_hour*3600 + lt.tm_min*60 + lt.tm_sec) );
	return( true );
}

#if 0

bool FunctionCall::
makeDate( const char*, const ArgumentList &argList, EvalState &state, 
	Value &val )
{
	Value 	arg0, arg1, arg2;
	int		dd, mm, yy;
	time_t	clock;
	struct	tm	tms;
	char	buffer[64];
	string	month;
	abstime_t abst;

		// two or three arguments
	if( argList.size( ) < 2 || argList.size( ) > 3 ) {
		val.SetErrorValue( );
		return( true );
	}

		// evaluate arguments
	if( !argList[0]->Evaluate( state, arg0 ) || 
		!argList[1]->Evaluate( state, arg1 ) ) {
		val.SetErrorValue( );
		return( false );
	}

		// get current time in tm struct
	if( time( &clock ) == -1 ) {
		val.SetErrorValue( );
		return( false );
	}
	getLocalTime( &clock, &tms );

		// evaluate optional third argument
	if( argList.size( ) == 3 ) {
		if( !argList[2]->Evaluate( state, arg2 ) ) {
			val.SetErrorValue( );
			return( false );
		}
	} else {
			// use the current year (tm_year is years since 1900)
		arg2.SetIntegerValue( tms.tm_year + 1900 );
	}
		
		// check if any of the arguments are undefined
	if( arg0.IsUndefinedValue( ) || arg1.IsUndefinedValue( ) || 
		arg2.IsUndefinedValue( ) ) {
		val.SetUndefinedValue( );
		return( true );
	}

		// first and third arguments must be integers (year should be >= 1970)
	if( !arg0.IsIntegerValue( dd ) || !arg2.IsIntegerValue( yy ) || yy < 1970 ){
		val.SetErrorValue( );
		return( true );
	}

		// the second argument must be integer or string
	if( arg1.IsIntegerValue( mm ) ) {
		if( sprintf( buffer, "%d %d %d", dd, mm, yy ) > 63 
			|| strptime( buffer, "%d %m %Y", tms ) == NULL )
		{
			val.SetErrorValue( );
			return( true );
		}
	} else if( arg1.IsStringValue( month ) ) {
		if( sprintf( buffer, "%d %s %d", dd, month.c_str( ), yy ) > 63 ||
				strptime( buffer, "%d %b %Y", &tms ) == NULL ) {
			val.SetErrorValue( );
			return( true );
		}
	} else {
		val.SetErrorValue( );
		return( true );
	}

		// convert the struct tm -> time_t -> absolute time
	clock = mktime( &tms );
	if(clock == -1) {
		val.SetErrorValue( );
		return( true );
	}
	abst.secs = clock;	
	abst.offset = timezone_offset();
	// alter absolute time parameters based on current day-light saving status
	if(tms->tm_isdst > 0) { 
		abst.offset += 3600;
		abst.secs -= 3600;
	}
	else {
		abst.secs += 3600;
	}
	val.SetAbsoluteTimeValue( abst );
	return( true );
}

#endif

bool FunctionCall::
getField(const char* name, const ArgumentList &argList, EvalState &state, 
	Value &val )
{
	Value 	arg;
	abstime_t asecs;
	time_t rsecs;
	time_t	clock;
	struct  tm tms;

	if( argList.size( ) != 1 ) {
		val.SetErrorValue( );
		return( true );
	}

	if( !argList[0]->Evaluate( state, arg ) ) {
		val.SetErrorValue( );
		return false;	
	}

	if( arg.IsAbsoluteTimeValue( asecs ) ) {
	 	clock = asecs.secs;
		getLocalTime( &clock, &tms );
		if( strcasecmp( name, "getyear" ) == 0 ) {
			 // tm_year is years since 1900 --- make it y2k compliant :-)
			val.SetIntegerValue( tms.tm_year + 1900 );
		} else if( strcasecmp( name, "getmonth" ) == 0 ) {
			val.SetIntegerValue( tms.tm_mon + 1 );
		} else if( strcasecmp( name, "getdayofyear" ) == 0 ) {
			val.SetIntegerValue( tms.tm_yday );
		} else if( strcasecmp( name, "getdayofmonth" ) == 0 ) {
			val.SetIntegerValue( tms.tm_mday );
		} else if( strcasecmp( name, "getdayofweek" ) == 0 ) {
			val.SetIntegerValue( tms.tm_wday );
		} else if( strcasecmp( name, "gethours" ) == 0 ) {
			val.SetIntegerValue( tms.tm_hour );
		} else if( strcasecmp( name, "getminutes" ) == 0 ) {
			val.SetIntegerValue( tms.tm_min );
		} else if( strcasecmp( name, "getseconds" ) == 0 ) {
			val.SetIntegerValue( tms.tm_sec );
		} else if( strcasecmp( name, "getdays" ) == 0 ||
			strcasecmp( name, "getuseconds" ) == 0 ) {
				// not meaningful for abstimes
			val.SetErrorValue( );
			return( true );
		} else {
			CLASSAD_EXCEPT( "Should not reach here" );
			val.SetErrorValue( );
			return( false );
		}
		return( true );
	} else if( arg.IsRelativeTimeValue( rsecs ) ) {
		if( strcasecmp( name, "getyear" ) == 0  	||
			strcasecmp( name, "getmonth" ) == 0  	||
			strcasecmp( name, "getdayofmonth" )== 0 ||
			strcasecmp( name, "getdayofweek" ) == 0 ||
			strcasecmp( name, "getdayofyear" ) == 0 ) {
				// not meaningful for reltimes
			val.SetErrorValue( );
			return( true );
		} else if( strcasecmp( name, "getdays" ) == 0 ) {
			val.SetIntegerValue( rsecs / 86400 );
		} else if( strcasecmp( name, "gethours" ) == 0 ) {
			val.SetIntegerValue( (rsecs % 86400 ) / 3600 );
		} else if( strcasecmp( name, "getminutes" ) == 0 ) {
			val.SetIntegerValue( ( rsecs % 3600 ) / 60 );
		} else if( strcasecmp( name, "getseconds" ) == 0 ) {
			val.SetIntegerValue( rsecs % 60 );
		} else {
			CLASSAD_EXCEPT( "Should not reach here" );
			val.SetErrorValue( );
			return( false );
		}
		return( true );
	}

	val.SetErrorValue( );
	return( true );
}

bool FunctionCall::
splitTime(const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value 	arg;
    ClassAd *split;

	if( argList.size( ) != 1 ) {
		result.SetErrorValue( );
		return true;
	}

	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return false;	
	}

    if (!arg.IsClassAdValue() && doSplitTime(arg, split)) {
        result.SetClassAdValue(split);
    } else {
        result.SetErrorValue();
    }
    return true;
}

bool FunctionCall::
formatTime(const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value 	   time_arg;
    Value      format_arg;
    time_t     epoch_time;
	struct  tm time_components;
    ClassAd    *splitClassAd;
    string     format;
    int        number_of_args;
    bool       did_eval;

    memset(&time_components, 0, sizeof(time_components));

    did_eval = true;
    number_of_args = argList.size();
    if (number_of_args == 0) {
        time(&epoch_time);
        getLocalTime(&epoch_time, &time_components);
        format = "%c";
        make_formatted_time(time_components, format, result);
    } else if (number_of_args < 3) {
        // The first argument should be our time and should
        // not be a relative time.
        if (!argList[0]->Evaluate(state, time_arg)) {
            did_eval = false;
        } else if (time_arg.IsRelativeTimeValue()) {
            result.SetErrorValue();
        } else if (!doSplitTime(time_arg, splitClassAd)) {
            result.SetErrorValue();
        } else {
            if (!splitClassAd->EvaluateAttrInt("Seconds", time_components.tm_sec)) {
                time_components.tm_sec = 0;
            }
            if (!splitClassAd->EvaluateAttrInt("Minutes", time_components.tm_min)) {
                time_components.tm_min = 0;
            }
            if (!splitClassAd->EvaluateAttrInt("Hours", time_components.tm_hour)) {
                time_components.tm_hour = 0;
            }
            if (!splitClassAd->EvaluateAttrInt("Day", time_components.tm_mday)) {
                time_components.tm_mday = 0;
            }
            if (!splitClassAd->EvaluateAttrInt("Month", time_components.tm_mon)) {
                time_components.tm_mon = 0;
            } else {
                time_components.tm_mon--;
            }
            if (!splitClassAd->EvaluateAttrInt("Year", time_components.tm_year)) {
                time_components.tm_year = 0;
            } else {
                time_components.tm_year -= 1900;;
            }

            // Note that we are doing our own calculations to get the weekday and year day.
            // Why are we doing our own calculations instead of using something like gmtime or 
            // localtime? That's because we are dealing with something that might not be in 
            // our timezone or GM time, but some other timezone.
            day_numbers(time_components.tm_year + 1900, time_components.tm_mon+1, time_components.tm_mday,
                        time_components.tm_wday, time_components.tm_yday);

            // The second argument, if provided, must be a string
            if (number_of_args == 1) {
                format = "%c";
                make_formatted_time(time_components, format, result);
            } else {
                if (!argList[1]->Evaluate(state, format_arg)) {
                    did_eval = false;
                } else {
                    if (!format_arg.IsStringValue(format)) {
                        result.SetErrorValue();
                    } else {
                        make_formatted_time(time_components, format, result);
                    }
                }
            }
            delete splitClassAd;
        }
    } else {
        did_eval = false;
    }

    if (!did_eval) {
        result.SetErrorValue();
    }
    return did_eval;
}

#if 0

bool FunctionCall::
inTimeUnits(const char*name,const ArgumentList &argList,EvalState &state, 
	Value &val )
{
	Value 	arg;
	abstime_t	asecs;
	asecs.secs = 0;
	asecs.offset = 0;
	time_t rsecs=0;
	double	secs=0.0;

    if( argList.size( ) != 1 ) {
        val.SetErrorValue( );
        return( true );
    }

    if( !argList[0]->Evaluate( state, arg ) ) {
        val.SetErrorValue( );
        return false;
    }

		// only handle times
	if( !arg.IsAbsoluteTimeValue(asecs) && 
		!arg.IsRelativeTimeValue(rsecs) ) {
		val.SetErrorValue( );
		return( true );
	}

	if( arg.IsAbsoluteTimeValue( ) ) {
		secs = asecs.secs;
	} else if( arg.IsRelativeTimeValue( ) ) {	
		secs = rsecs;
	}

	if (strcasecmp( name, "indays" ) == 0 ) {
		val.SetRealValue( secs / 86400.0 );
		return( true );
	} else if( strcasecmp( name, "inhours" ) == 0 ) {
		val.SetRealValue( secs / 3600.0 );
		return( true );
	} else if( strcasecmp( name, "inminutes" ) == 0 ) {
		val.SetRealValue( secs / 60.0 );
	} else if( strcasecmp( name, "inseconds" ) == 0 ) {
		val.SetRealValue( secs );
		return( true );
	}

	val.SetErrorValue( );
	return( true );
}

#endif

	// concatenate all arguments (expected to be strings)
bool FunctionCall::
strCat( const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	ClassAdUnParser	unp;
	string			buf, s;
	bool			errorFlag=false, undefFlag=false, rval=true;

	for( int i = 0 ; (unsigned)i < argList.size() ; i++ ) {
		Value  val;
        Value  stringVal;

		s = "";
		if( !( rval = argList[i]->Evaluate( state, val ) ) ) {
			break;
		}

        if (val.IsStringValue(s)) {
            buf += s;
        } else {
            convertValueToStringValue(val, stringVal);
            if (stringVal.IsUndefinedValue()) {
                undefFlag = true;
                break;
            } else if (stringVal.IsErrorValue()) {
                errorFlag = true;
                result.SetErrorValue();
                break;
            } else if (stringVal.IsStringValue(s)) {
                buf += s;
            } else {
                errorFlag = true;
                break;
            }
        }
    }
	
    // failed evaluating some argument
	if( !rval ) {
		result.SetErrorValue( );
		return( false );
	}
		// type error
	if( errorFlag ) {
		result.SetErrorValue( );
		return( true );
	} 
		// some argument was undefined
	if( undefFlag ) {
		result.SetUndefinedValue( );
		return( true );
	}

	result.SetStringValue( buf );
	return( true );
}

bool FunctionCall::
changeCase(const char*name,const ArgumentList &argList,EvalState &state,
	Value &result)
{
	Value 		val, stringVal;
	string		str;
	bool		lower = ( strcasecmp( name, "tolower" ) == 0 );
	int			len;

    // only one argument 
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return true;
	}

    // check for evaluation failure
	if( !argList[0]->Evaluate( state, val ) ) {
		result.SetErrorValue( );
		return false;
	}

    if (!val.IsStringValue(str)) {
        convertValueToStringValue(val, stringVal);
        if (stringVal.IsUndefinedValue()) {
            result.SetUndefinedValue( );
            return true;
        } else if (stringVal.IsErrorValue()) {
            result.SetErrorValue();
            return true;
        } else if (!stringVal.IsStringValue(str)) {
            result.SetErrorValue();
            return true;
        }
	}

	len = str.size( );
	for( int i=0; i <= len; i++ ) {
		str[i] = lower ? tolower( str[i] ) : toupper( str[i] );
	}

	result.SetStringValue( str );
	return( true );
}

bool FunctionCall::
subString( const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value 	arg0, arg1, arg2;
	string	buf;
	int		offset, len=0, alen;

		// two or three arguments
	if( argList.size() < 2 || argList.size() > 3 ) {
		result.SetErrorValue( );
		return( true );
	}

		// Evaluate all arguments
	if( !argList[0]->Evaluate( state, arg0 ) ||
		!argList[1]->Evaluate( state, arg1 ) ||
		( argList.size( ) > 2 && !argList[2]->Evaluate( state, arg2 ) ) ) {
		result.SetErrorValue( );
		return( false );
	}

		// strict on undefined
	if( arg0.IsUndefinedValue( ) || arg1.IsUndefinedValue( ) ||
		(argList.size() > 2 && arg2.IsUndefinedValue( ) ) ) {
		result.SetUndefinedValue( );
		return( true );
	}

		// arg0 must be string, arg1 must be int, arg2 (if given) must be int
	if( !arg0.IsStringValue( buf ) || !arg1.IsIntegerValue( offset )||
		(argList.size( ) > 2 && !arg2.IsIntegerValue( len ) ) ) {
		result.SetErrorValue( );
		return( true );
	}

		// perl-like substr; negative offsets and lengths count from the end
		// of the string
	alen = buf.size( );
	if( offset < 0 ) { 
		offset = alen + offset; 
	} else if( offset >= alen ) {
		offset = alen;
	}
	if( len <= 0 ) {
		len = alen - offset + len;
		if( len < 0 ) {
			len = 0;
		}
	} else if( len > alen - offset ) {
		len = alen - offset;
	}

	// to make sure that if length is specified as 0 explicitly
	// then, len is set to 0
	if(argList.size( ) == 3) {
	  int templen;
	  arg2.IsIntegerValue( templen );
	  if(templen == 0)
	    len = 0;
	}

		// allocate storage for the string
	string str;

	str.assign( buf, offset, len );
	result.SetStringValue( str );
	return( true );
}

bool FunctionCall::
compareString( const char*name, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value 	arg0, arg1;
    Value   arg0_s, arg1_s;

    // Must have two arguments
	if(argList.size() != 2) {
		result.SetErrorValue( );
		return( true );
	}

    // Evaluate both arguments
	if(!argList[0]->Evaluate(state, arg0) ||
       !argList[1]->Evaluate(state, arg1)) {
		result.SetErrorValue();
		return false;
	}

    // If either argument is undefined, then the result is
    // undefined.
	if(arg0.IsUndefinedValue() || arg1.IsUndefinedValue()) {
		result.SetUndefinedValue( );
		return true;
    }

    string  s0, s1;
    if (   convertValueToStringValue(arg0, arg0_s)
        && convertValueToStringValue(arg1, arg1_s)
        && arg0_s.IsStringValue(s0)
        && arg1_s.IsStringValue(s1)) {

        int  order;
        
        if (strcasecmp(name, "strcmp") == 0) {
            order = strcmp(s0.c_str(), s1.c_str());
            if (order < 0) order = -1;
            else if (order > 0) order = 1;
        } else {
            order = strcasecmp(s0.c_str(), s1.c_str());
            if (order < 0) order = -1;
            else if (order > 0) order = 1;
        }
        result.SetIntegerValue(order);
    } else {
        result.SetErrorValue();
    }

	return( true );
}

bool FunctionCall::
convInt( const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value	arg;

		// takes exactly one argument
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}

    convertValueToIntegerValue(arg, result);
    return true;
}


bool FunctionCall::
convReal( const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value  arg;

    // takes exactly one argument
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}

    convertValueToRealValue(arg, result);
    return true;
}

bool FunctionCall::
convString(const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value	arg;

		// takes exactly one argument
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}

    convertValueToStringValue(arg, result);
    return true;
}

bool FunctionCall::
convBool( const char*, const ArgumentList &argList, EvalState &state, 
	Value &result )
{
	Value	arg;

		// takes exactly one argument
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}

	switch( arg.GetType( ) ) {
		case Value::UNDEFINED_VALUE:
			result.SetUndefinedValue( );
			return( true );

		case Value::ERROR_VALUE:
		case Value::CLASSAD_VALUE:
		case Value::LIST_VALUE:
		case Value::ABSOLUTE_TIME_VALUE:
			result.SetErrorValue( );
			return( true );

		case Value::BOOLEAN_VALUE:
			result.CopyFrom( arg );
			return( true );

		case Value::INTEGER_VALUE:
			{
				int ival;
				arg.IsIntegerValue( ival );
				result.SetBooleanValue( ival != 0 );
				return( true );
			}

		case Value::REAL_VALUE:
			{
				double rval;
				arg.IsRealValue( rval );
				result.SetBooleanValue( rval != 0.0 );
				return( true );
			}

		case Value::STRING_VALUE:
			{
				string buf;
				arg.IsStringValue( buf );
				if( strcasecmp( "false", buf.c_str( ) ) || buf == "" ) {
					result.SetBooleanValue( false );
				} else {
					result.SetBooleanValue( true );
				}
				return( true );
			}

		case Value::RELATIVE_TIME_VALUE:
			{
				time_t rsecs;
				arg.IsRelativeTimeValue( rsecs );
				result.SetBooleanValue( rsecs != 0 );
				return( true );
			}

		default:
			CLASSAD_EXCEPT( "Should not reach here" );
	}
	return( false );
}




bool FunctionCall::
convTime(const char* name,const ArgumentList &argList,EvalState &state,
	Value &result)
{
	Value	arg, arg2;
	bool	relative = ( strcasecmp( "reltime", name ) == 0 );
	bool    secondarg = false; // says whether a 2nd argument exists
	int     arg2num;

    if (argList.size() == 0 && !relative) {
        // absTime with no arguments returns the current time. 
        return currentTime(name, argList, state, result);
    }
	if(( argList.size() < 1 )  || (argList.size() > 2)) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}
	if(argList.size() == 2) { // we have a 2nd argument
		secondarg = true;
		if( !argList[1]->Evaluate( state, arg2 ) ) {
			result.SetErrorValue( );
			return( false );
		}
		int ivalue2 = 0;
		double rvalue2 = 0;
		time_t rsecs = 0;
		if(relative) {// 2nd argument is N/A for reltime
			result.SetErrorValue( );
			return( true );
		}
		// 2nd arg should be integer, real or reltime
		else if (arg2.IsIntegerValue(ivalue2)) {
			arg2num = ivalue2;
		}
		else if (arg2.IsRealValue(rvalue2)) {
			arg2num = (int)rvalue2;
		}
		else if(arg2.IsRelativeTimeValue(rsecs)) {
			arg2num = rsecs;
		}
		else {
			result.SetErrorValue( );
			return( true );
		}
	} else {
        secondarg = false;
        arg2num = 0;
    }

	switch( arg.GetType( ) ) {
		case Value::UNDEFINED_VALUE:
			result.SetUndefinedValue( );
			return( true );

		case Value::ERROR_VALUE:
		case Value::CLASSAD_VALUE:
		case Value::LIST_VALUE:
		case Value::BOOLEAN_VALUE:
			result.SetErrorValue( );
			return( true );

		case Value::INTEGER_VALUE:
			{
				int ivalue;
				arg.IsIntegerValue( ivalue );
				if( relative ) {
					result.SetRelativeTimeValue( (time_t) ivalue );
				} else {
				  abstime_t atvalue;
				  atvalue.secs = ivalue;
				  if(secondarg)   //2nd arg is the offset in secs
				    atvalue.offset = arg2num;
				  else   // the default offset is the current timezone
				    atvalue.offset = Literal::findOffset(atvalue.secs);
				  
				  if(atvalue.offset == -1) {
				    result.SetErrorValue( );
				    return( false );
				  }
				  else
				    result.SetAbsoluteTimeValue( atvalue );
				}
				return( true );
			}

		case Value::REAL_VALUE:
			{
				double	rvalue;

				arg.IsRealValue( rvalue );
				if( relative ) {
					result.SetRelativeTimeValue( rvalue );
				} else {
				  	  abstime_t atvalue;
					  atvalue.secs = (int)rvalue;
					  if(secondarg)         //2nd arg is the offset in secs
					    atvalue.offset = arg2num;
					  else    // the default offset is the current timezone
					    atvalue.offset = Literal::findOffset(atvalue.secs);
					  if(atvalue.offset == -1) {
					    result.SetErrorValue( );
					    return( false );
					  }
					  else	
					    result.SetAbsoluteTimeValue( atvalue );
				}
				return( true );
			}

		case Value::STRING_VALUE:
			{
			  //should'nt come here
			  // a string argument to this function is transformed to a literal directly
			}

		case Value::ABSOLUTE_TIME_VALUE:
			{
				abstime_t secs;
				arg.IsAbsoluteTimeValue( secs );
				if( relative ) {
					result.SetRelativeTimeValue( secs.secs );
				} else {
					result.CopyFrom( arg );
				}
				return( true );
			}

		case Value::RELATIVE_TIME_VALUE:
			{
				if( relative ) {
					result.CopyFrom( arg );
				} else {
					time_t secs;
					arg.IsRelativeTimeValue( secs );
					abstime_t atvalue;
					atvalue.secs = secs;
					if(secondarg)    //2nd arg is the offset in secs
						atvalue.offset = arg2num;
					else      // the default offset is the current timezone
						atvalue.offset = Literal::findOffset(atvalue.secs);	
					if(atvalue.offset == -1) {
						result.SetErrorValue( );
						return( false );
					}
					else
					  result.SetAbsoluteTimeValue( atvalue );
				}
				return( true );
			}

		default:
			CLASSAD_EXCEPT( "Should not reach here" );
			return( false );
	}
}


bool FunctionCall::
doMath( const char* name,const ArgumentList &argList,EvalState &state,
	Value &result )
{
	Value	arg;
    Value   realValue;

		// takes exactly one argument
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}

    if (arg.GetType() == Value::INTEGER_VALUE) {
        result.CopyFrom(arg);
    } else {
        if (!convertValueToRealValue(arg, realValue)) {
            result.SetErrorValue();
        } else {
            double rvalue;
            realValue.IsRealValue(rvalue);
            if (strcasecmp("floor", name) == 0) {
                result.SetIntegerValue((int) floor(rvalue));
            } else if (   strcasecmp("ceil", name)    == 0 
                       || strcasecmp("ceiling", name) == 0) {
                result.SetIntegerValue((int) ceil(rvalue));
            } else if( strcasecmp("round", name) == 0) {
                result.SetIntegerValue((int) rint(rvalue));
            } else {
                result.SetErrorValue( );
            }
        }
    }
    return true;
}

bool FunctionCall::
random( const char*,const ArgumentList &argList,EvalState &state,
	Value &result )
{
	Value	arg;
    int     int_max;
    double  double_max;
    int     random_int;
    double  random_double;

    // takes exactly one argument
	if( argList.size() > 1 ) {
		result.SetErrorValue( );
		return( true );
	} else if ( argList.size() == 0 ) {
		arg.SetRealValue( 1.0 );
	} else if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}

    if (arg.IsIntegerValue(int_max)) {
        random_int = get_random_integer() % int_max;
        result.SetIntegerValue(random_int);
    } else if (arg.IsRealValue(double_max)) {
        random_double = double_max * get_random_real();
        result.SetRealValue(random_double);
    } else {
        result.SetErrorValue();
    }

    return true;
}

bool FunctionCall::
ifThenElse( const char* /* name */,const ArgumentList &argList,EvalState &state,
	Value &result )
{
	Value	arg1;
	bool    arg1_bool = false;;

		// takes exactly three arguments
	if( argList.size() != 3 ) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg1 ) ) {
		result.SetErrorValue( );
		return( false );
	}

	switch( arg1.GetType() ) {
	case Value::BOOLEAN_VALUE:
		if( !arg1.IsBooleanValue(arg1_bool) ) {
			result.SetErrorValue();
			return( false );
		}
		break;
	case Value::INTEGER_VALUE: {
		int intval;
		if( !arg1.IsIntegerValue(intval) ) {
			result.SetErrorValue();
			return( false );
		}
		arg1_bool = intval != 0;
		break;
	}
	case Value::REAL_VALUE: {
		double realval;
		if( !arg1.IsRealValue(realval) ) {
			result.SetErrorValue();
			return( false );
		}
		arg1_bool = realval != 0.0;
		break;
	}

	case Value::UNDEFINED_VALUE:
		result.SetUndefinedValue();
		return( true );

	case Value::ERROR_VALUE:
	case Value::CLASSAD_VALUE:
	case Value::LIST_VALUE:
	case Value::STRING_VALUE:
	case Value::ABSOLUTE_TIME_VALUE:
	case Value::RELATIVE_TIME_VALUE:
	case Value::NULL_VALUE:
		result.SetErrorValue();
		return( true );
	}

	if( arg1_bool ) {
		if( !argList[1]->Evaluate( state, result ) ) {
			result.SetErrorValue( );
			return( false );
		}
	}
	else {
		if( !argList[2]->Evaluate( state, result ) ) {
			result.SetErrorValue( );
			return( false );
		}
	}

    return true;
}

bool FunctionCall::
eval( const char* /* name */,const ArgumentList &argList,EvalState &state,
	  Value &result )
{
	Value arg,strarg;

		// takes exactly one argument
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return true;
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return false;
	}

	string s;
    if( !convertValueToStringValue(arg, strarg) ||
		!strarg.IsStringValue( s ) )
	{
		result.SetErrorValue();
		return true;
	}

	ClassAdParser parser;
	ExprTree *expr = NULL;
	if( !parser.ParseExpression( s.c_str(), expr, true ) || !expr ) {
		if( expr ) {
			delete expr;
		}
		result.SetErrorValue();
		return true;
	}

	expr->SetParentScope( state.curAd );

	bool eval_ok = expr->Evaluate( result );

	delete expr;

	if( !eval_ok ) {
		result.SetErrorValue();
		return false;
	}
	return true;
}

bool FunctionCall::
interval( const char* /* name */,const ArgumentList &argList,EvalState &state,
	Value &result )
{
	Value	arg,intarg;
	int tot_secs;

		// takes exactly one argument
	if( argList.size() != 1 ) {
		result.SetErrorValue( );
		return( true );
	}
	if( !argList[0]->Evaluate( state, arg ) ) {
		result.SetErrorValue( );
		return( false );
	}
	if( !convertValueToIntegerValue(arg,intarg) ) {
		result.SetErrorValue( );
		return( true );
	}

	if( !intarg.IsIntegerValue(tot_secs) ) {
		result.SetErrorValue( );
		return( true );
	}

	int days,hours,min,secs;
	days = tot_secs / (3600*24);
	tot_secs %= (3600*24);
	hours = tot_secs / 3600;
	tot_secs %= 3600;
	min = tot_secs / 60;
	secs = tot_secs % 60;

	char strval[25];
	if ( days != 0 ) {
		sprintf(strval,"%d+%02d:%02d:%02d", days, abs(hours), abs(min), abs(secs) );
	} else if ( hours != 0 ) {
		sprintf(strval,"%d:%02d:%02d", hours, abs(min), abs(secs) );
	} else if ( min != 0 ) {
		sprintf(strval,"%d:%02d", min, abs(secs) );
	} else {
		sprintf(strval,"%d", secs );
	}
	result.SetStringValue(strval);

    return true;
}

#if defined USE_POSIX_REGEX || defined USE_PCRE
static bool regexp_helper(const char *pattern, const char *target,
                          const char *replace,
                          bool have_options, string options_string,
                          Value &result);

bool FunctionCall::
substPattern( const char*,const ArgumentList &argList,EvalState &state,
	Value &result )
{
    bool        have_options;
	Value 		arg0, arg1, arg2, arg3;
	const char	*pattern=NULL, *target=NULL, *replace=NULL;
    string      options_string;

		// need three or four arguments: pattern, string, replace, optional settings
	if( argList.size() != 3 && argList.size() != 4) {
		result.SetErrorValue( );
		return( true );
	}
    if (argList.size() == 3) {
        have_options = false;
    } else {
        have_options = true;
    }

		// Evaluate args
	if( !argList[0]->Evaluate( state, arg0 ) || 
		!argList[1]->Evaluate( state, arg1 ) ||
		!argList[2]->Evaluate( state, arg2 ) ) {
		result.SetErrorValue( );
		return( false );
	}
    if( have_options && !argList[3]->Evaluate( state, arg3 ) ) {
		result.SetErrorValue( );
		return( false );
    }

		// if any arg is error, the result is error
	if( arg0.IsErrorValue( ) || arg1.IsErrorValue( ) || arg2.IsErrorValue() ) {
		result.SetErrorValue( );
		return( true );
	}
    if( have_options && arg3.IsErrorValue( ) ) {
        result.SetErrorValue( );
        return( true );
    }

		// if any arg is undefined, the result is undefined
	if( arg0.IsUndefinedValue( ) || arg1.IsUndefinedValue( ) || arg2.IsUndefinedValue() ) {
		result.SetUndefinedValue( );
		return( true );
	}
    if( have_options && arg3.IsUndefinedValue( ) ) {
		result.SetUndefinedValue( );
		return( true );
    } else if ( have_options && !arg3.IsStringValue( options_string ) ) {
        result.SetErrorValue( );
        return( true );
    }

		// if either argument is not a string, the result is an error
	if( !arg0.IsStringValue( pattern ) || !arg1.IsStringValue( target ) || !arg2.IsStringValue( replace ) ) {
		result.SetErrorValue( );
		return( true );
	}
    return regexp_helper(pattern, target, replace, have_options, options_string, result);
}

bool FunctionCall::
matchPattern( const char*,const ArgumentList &argList,EvalState &state,
	Value &result )
{
    bool        have_options;
	Value 		arg0, arg1, arg2;
	const char	*pattern=NULL, *target=NULL;
    string      options_string;

		// need two or three arguments: pattern, string, optional settings
	if( argList.size() != 2 && argList.size() != 3) {
		result.SetErrorValue( );
		return( true );
	}
    if (argList.size() == 2) {
        have_options = false;
    } else {
        have_options = true;
    }

		// Evaluate args
	if( !argList[0]->Evaluate( state, arg0 ) || 
		!argList[1]->Evaluate( state, arg1 ) ) {
		result.SetErrorValue( );
		return( false );
	}
    if( have_options && !argList[2]->Evaluate( state, arg2 ) ) {
		result.SetErrorValue( );
		return( false );
    }

		// if either arg is error, the result is error
	if( arg0.IsErrorValue( ) || arg1.IsErrorValue( ) ) {
		result.SetErrorValue( );
		return( true );
	}
    if( have_options && arg2.IsErrorValue( ) ) {
        result.SetErrorValue( );
        return( true );
    }

		// if either arg is undefined, the result is undefined
	if( arg0.IsUndefinedValue( ) || arg1.IsUndefinedValue( ) ) {
		result.SetUndefinedValue( );
		return( true );
	}
    if( have_options && arg2.IsUndefinedValue( ) ) {
		result.SetUndefinedValue( );
		return( true );
    } else if ( have_options && !arg2.IsStringValue( options_string ) ) {
        result.SetErrorValue( );
        return( true );
    }

		// if either argument is not a string, the result is an error
	if( !arg0.IsStringValue( pattern ) || !arg1.IsStringValue( target ) ) {
		result.SetErrorValue( );
		return( true );
	}
    return regexp_helper(pattern, target, NULL, have_options, options_string, result);
}

bool FunctionCall::
matchPatternMember( const char*,const ArgumentList &argList,EvalState &state,
	Value &result )
{
    bool            have_options;
	Value 		    arg0, arg1, arg2;
	const char	    *pattern=NULL, *target=NULL;
    const ExprList	*target_list;
    string          options_string;

    // need two or three arguments: pattern, list, optional settings
	if( argList.size() != 2 && argList.size() != 3) {
		result.SetErrorValue( );
		return( true );
	}
    if (argList.size() == 2) {
        have_options = false;
    } else {
        have_options = true;
    }

		// Evaluate args
	if( !argList[0]->Evaluate( state, arg0 ) || 
		!argList[1]->Evaluate( state, arg1 ) ) {
		result.SetErrorValue( );
		return( false );
	}
    if( have_options && !argList[2]->Evaluate( state, arg2 ) ) {
		result.SetErrorValue( );
		return( false );
    }

		// if either arg is error, the result is error
	if( arg0.IsErrorValue( ) || arg1.IsErrorValue( ) ) {
		result.SetErrorValue( );
		return( true );
	}
    if( have_options && arg2.IsErrorValue( ) ) {
        result.SetErrorValue( );
        return( true );
    }

		// if either arg is undefined, the result is undefined
	if( arg0.IsUndefinedValue( ) || arg1.IsUndefinedValue( ) ) {
		result.SetUndefinedValue( );
		return( true );
	}
    if( have_options && arg2.IsUndefinedValue( ) ) {
		result.SetUndefinedValue( );
		return( true );
    } else if ( have_options && !arg2.IsStringValue( options_string ) ) {
        result.SetErrorValue( );
        return( true );
    }

		// if the arguments are not of the correct types, the result is an error
	if( !arg0.IsStringValue( pattern ) || !arg1.IsListValue( target_list ) ) {
		result.SetErrorValue( );
		return( true );
	}
    result.SetBooleanValue(false);
    
    ExprTree *target_expr;
    ExprList::const_iterator list_iter = target_list->begin();
    while (list_iter != target_list->end()) {
        Value target_value;
        Value have_match_value;
        target_expr = *list_iter;
        if (target_expr != NULL) {
            if( !target_expr->Evaluate(state, target_value)) {
                result.SetErrorValue();
                return true;
            }
            if (!target_value.IsStringValue(target)) {
                result.SetErrorValue();
                return true;
            } else {
                bool have_match;
                bool success = regexp_helper(pattern, target, NULL, have_options, options_string, have_match_value);
                if (!success) {
                    result.SetErrorValue();
                    return true;
                } else {
                    if (have_match_value.IsBooleanValue(have_match) && have_match) {
                        result.SetBooleanValue(true);
                        return true;
                    }
                }
            }
        } else {
            result.SetErrorValue();
            return(false);
        }
        list_iter++;
    }
    return true;
}

static bool regexp_helper(
    const char *pattern,
    const char *target,
	const char *replace,
    bool       have_options,
    string     options_string,
    Value      &result)
{
    int         options;
	int			status;

#if defined (USE_POSIX_REGEX)
	regex_t		re;

	const int MAX_REGEX_GROUPS=11;
	regmatch_t pmatch[MAX_REGEX_GROUPS];
	size_t      nmatch = MAX_REGEX_GROUPS;

    options = REG_EXTENDED;
	if( !replace ) {
		options |= REG_NOSUB;
	}
    if( have_options ){
        // We look for the options we understand, and ignore
        // any others that we might find, hopefully allowing
        // forwards compatibility.
        if ( options_string.find( 'i' ) != string::npos ) {
            options |= REG_ICASE;
        }
    }

		// compile the patern
	if( regcomp( &re, pattern, options ) != 0 ) {
			// error in pattern
		result.SetErrorValue( );
		return( true );
	}

		// test the match
	status = regexec( &re, target, nmatch, pmatch, 0 );

		// dispose memory created by regcomp()
	regfree( &re );

	if( status == 0 && replace ) {
		string group_buffers[MAX_REGEX_GROUPS];
		char const *groups[MAX_REGEX_GROUPS];
		int ngroups = MAX_REGEX_GROUPS;
		int i;

		for(i=0;i<MAX_REGEX_GROUPS;i++) {
			regoff_t rm_so = pmatch[i].rm_so;
			regoff_t rm_eo = pmatch[i].rm_eo;
			if( rm_so >= 0 ) {
				group_buffers[i].append(target,rm_so,rm_eo-rm_so);
				groups[i] = group_buffers[i].c_str();
			}
			else {
				groups[i] = NULL;
			}
		}

		string output;
		bool replace_success = true;

		while (*replace) {
			if (*replace == '\\') {
				if (isdigit(replace[1])) {
					int offset = replace[1] - '0';
					replace++;
					if( offset >= ngroups || !groups[offset] ) {
						replace_success = false;
						break;
					}
					output += groups[offset];
				} else {
					output += '\\';
				}
			} else {
				output += *replace;
			}
			replace++;
		}

		if( replace_success ) {
			result.SetStringValue( output );
		}
		else {
			result.SetErrorValue( );
		}
		return( true );
	}
	else if( status == REG_NOMATCH && replace ) {
		result.SetStringValue( "" );
		return( true );
	}

		// check for success/failure
	if( status == 0 ) {
		result.SetBooleanValue( true );
		return( true );
	} else if( status == REG_NOMATCH ) {
		result.SetBooleanValue( false );
		return( true );
	} else {
			// some error; we could possibly return 'false' here ...
		result.SetErrorValue( );
		return( true );
	}
#elif defined (USE_PCRE)
    const char  *error_message;
    int         error_offset;
    pcre        *re;

    options     = 0;
    if( have_options ){
        // We look for the options we understand, and ignore
        // any others that we might find, hopefully allowing
        // forwards compatibility.
        if ( options_string.find( 'i' ) != string::npos ) {
            options |= PCRE_CASELESS;
        } 
        if ( options_string.find( 'm' ) != string::npos ) {
            options |= PCRE_MULTILINE;
        }
        if ( options_string.find( 's' ) != string::npos ) {
            options |= PCRE_DOTALL;
        }
        if ( options_string.find( 'x' ) != string::npos ) {
            options |= PCRE_EXTENDED;
        }
    }

    re = pcre_compile( pattern, options, &error_message,
                      &error_offset, NULL );
    if ( re == NULL ){
			// error in pattern
		result.SetErrorValue( );
    } else {
		int group_count;
		pcre_fullinfo(re, NULL, PCRE_INFO_CAPTURECOUNT, &group_count);
		int oveccount = 3 * (group_count + 1); // +1 for the string itself
		int * ovector = (int *) malloc(oveccount * sizeof(int));


        status = pcre_exec(re, NULL, target, strlen(target),
                           0, 0, ovector, oveccount);
        if (status >= 0) {
            result.SetBooleanValue( true );
        } else {
            result.SetBooleanValue( false );
        }

		pcre_free(re);

		if( replace && status<0 ) {
			result.SetStringValue( "" );
		}
		else if( replace ) {

			const char **groups = NULL;
			string output;
			int ngroups = status;
			bool replace_success = true;

			if( pcre_get_substring_list(target,ovector,ngroups,&groups)!=0 ) {
				result.SetErrorValue( );
				replace_success = false;
			}
			else while (*replace) {
				if (*replace == '\\') {
					if (isdigit(replace[1])) {
						int offset = replace[1] - '0';
						replace++;
						if( offset >= ngroups ) {
							replace_success = false;
							break;
						}
						output += groups[offset];
					} else {
						output += '\\';
					}
				} else {
					output += *replace;
				}
				replace++;
			}

			pcre_free_substring_list( groups );

			if( replace_success ) {
				result.SetStringValue( output );
			}
			else {
				result.SetErrorValue( );
			}
		}

		free( ovector );
    }
    return true;
#endif
}

#endif /* defined USE_POSIX_REGEX || defined USE_PCRE */

static bool 
doSplitTime(const Value &time, ClassAd * &splitClassAd)
{
    bool             did_conversion;
    int              integer;
    double           real;
    abstime_t        asecs;
    double           rsecs;
    const ClassAd    *classad;

    did_conversion = true;
    if (time.IsIntegerValue(integer)) {
        asecs.secs = integer;
        asecs.offset = timezone_offset( asecs.secs, false );
        absTimeToClassAd(asecs, splitClassAd);
    } else if (time.IsRealValue(real)) {
        asecs.secs = (int) real;
        asecs.offset = timezone_offset( asecs.secs, false );
        absTimeToClassAd(asecs, splitClassAd);
    } else if (time.IsAbsoluteTimeValue(asecs)) {
        absTimeToClassAd(asecs, splitClassAd);
    } else if (time.IsRelativeTimeValue(rsecs)) {
        relTimeToClassAd(rsecs, splitClassAd);
    } else if (time.IsClassAdValue(classad)) {
        splitClassAd = new ClassAd;
        splitClassAd->CopyFrom(*classad);
    } else {
        did_conversion = false;
    }
    return did_conversion;
}

static void 
absTimeToClassAd(const abstime_t &asecs, ClassAd * &splitClassAd)
{
	time_t	  clock;
    struct tm tms;

    splitClassAd = new ClassAd;

    clock = asecs.secs + asecs.offset;
    getGMTime( &clock, &tms );

    splitClassAd->InsertAttr("Type", "AbsoluteTime");
    splitClassAd->InsertAttr("Year", tms.tm_year + 1900);
    splitClassAd->InsertAttr("Month", tms.tm_mon + 1);
    splitClassAd->InsertAttr("Day", tms.tm_mday);
    splitClassAd->InsertAttr("Hours", tms.tm_hour);
    splitClassAd->InsertAttr("Minutes", tms.tm_min);
    splitClassAd->InsertAttr("Seconds", tms.tm_sec);
    // Note that we convert the timezone from seconds to minutes.
    splitClassAd->InsertAttr("Offset", asecs.offset);
    
    return;
}

static void 
relTimeToClassAd(double rsecs, ClassAd * &splitClassAd) 
{
    int		days, hrs, mins;
    double  secs;
    bool    is_negative;

    if( rsecs < 0 ) {
        rsecs = -rsecs;
        is_negative = true;
    } else {
        is_negative = false;
    }
    days = (int) rsecs;
    hrs  = days % 86400;
    mins = hrs  % 3600;
    secs = (mins % 60) + (rsecs - floor(rsecs));
    days = days / 86400;
    hrs  = hrs  / 3600;
    mins = mins / 60;
    
    if (is_negative) {
        if (days > 0) {
            days = -days;
        } else if (hrs > 0) {
            hrs = -hrs;
        } else if (mins > 0) {
            mins = -mins;
        } else {
            secs = -secs;
        }
    }
    
    splitClassAd = new ClassAd;
    splitClassAd->InsertAttr("Type", "RelativeTime");
    splitClassAd->InsertAttr("Days", days);
    splitClassAd->InsertAttr("Hours", hrs);
    splitClassAd->InsertAttr("Minutes", mins);
    splitClassAd->InsertAttr("Seconds", secs);
    
    return;
}

static void
make_formatted_time(const struct tm &time_components, string &format,
                    Value &result)
{
    char output[1024]; // yech
    strftime(output, 1023, format.c_str(), &time_components);
    result.SetStringValue(output);
    return;
}

END_NAMESPACE // classad
