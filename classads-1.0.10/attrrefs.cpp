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


#include "classad/common.h"
#include "classad/classad.h"

using namespace std;

BEGIN_NAMESPACE( classad )

AttributeReference::
AttributeReference()
{
	nodeKind = ATTRREF_NODE;
	expr = NULL;
	absolute = false;
}


// a private ctor for use in significant expr identification
AttributeReference::
AttributeReference( ExprTree *tree, const string &attrname, bool absolut )
{
	nodeKind = ATTRREF_NODE;
	attributeStr = attrname;
	expr = tree;
	absolute = absolut;
}

AttributeReference::
AttributeReference(const AttributeReference &ref)
{
    CopyFrom(ref);
    return;
}

AttributeReference::
~AttributeReference()
{
	if( expr ) delete expr;
}

AttributeReference &AttributeReference::
operator=(const AttributeReference &ref)
{
    if (this != &ref) {
        CopyFrom(ref);
    }
    return *this;
}


ExprTree *AttributeReference::
Copy( ) const
{
	AttributeReference *newTree = new AttributeReference ();
	if (newTree == 0) {
		CondorErrno = ERR_MEM_ALLOC_FAILED;
		CondorErrMsg = "";
		return NULL;
	}

    if (!newTree->CopyFrom(*this)) {
        delete newTree;
        newTree = NULL;
    }

	return newTree;
}

bool AttributeReference::
CopyFrom(const AttributeReference &ref)
{
    bool success;

    success = true;

	attributeStr = ref.attributeStr;
	if( ref.expr && ( expr=ref.expr->Copy( ) ) == NULL ) {
        success = false;
	} else {
        ExprTree::CopyFrom(ref);
        absolute = ref.absolute;
    }
    return success;
}

bool AttributeReference::
SameAs(const ExprTree *tree) const
{
    bool is_same;

    if (tree->GetKind() != ATTRREF_NODE) {
        is_same = false;
    } else {
        const AttributeReference *other_ref = (const AttributeReference *) tree;
        
        if (   absolute     != other_ref->absolute
            || attributeStr != other_ref->attributeStr) {
            is_same = false;
        } else if (    (expr == NULL && other_ref->expr == NULL)
                    || (expr == other_ref->expr)
                    || (expr != NULL && other_ref->expr != NULL && expr->SameAs(other_ref->expr))) {
            // Will this check result in infinite recursion? How do I stop it? 
            is_same = true;
        } else {
            is_same = false;
        }
    }

    return is_same;
}

bool 
operator==(const AttributeReference &ref1, const AttributeReference &ref2)
{
    return ref1.SameAs(&ref2);
}

void AttributeReference::
_SetParentScope( const ClassAd *parent ) 
{
	if( expr ) expr->SetParentScope( parent );
}


void AttributeReference::
GetComponents( ExprTree *&tree, string &attr, bool &abs ) const
{
	tree = expr;
	attr = attributeStr;
	abs = absolute;
}


bool AttributeReference::
_Evaluate (EvalState &state, Value &val) const
{
	ExprTree	*tree; 
	ExprTree	*dummy;
	const ClassAd *curAd;
	bool		rval;

	// find the expression and the evalstate
	curAd = state.curAd;
	switch( FindExpr( state, tree, dummy, false ) ) {
		case EVAL_FAIL:
			return false;

		case EVAL_ERROR:
		case PROP_ERROR:
			val.SetErrorValue();
			state.curAd = curAd;
			return true;

		case EVAL_UNDEF:
		case PROP_UNDEF:
			val.SetUndefinedValue();
			state.curAd = curAd;
			return true;

		case EVAL_OK: 
		{
			if( state.depth_remaining <= 0 ) {
				val.SetErrorValue();
				state.curAd = curAd;
				return false;
			}
			state.depth_remaining--;

			rval = tree->Evaluate( state, val );

			state.depth_remaining++;

			state.curAd = curAd;

			return rval;
		}
		default:  CLASSAD_EXCEPT( "ClassAd:  Should not reach here" );
	}
	return false;
}


bool AttributeReference::
_Evaluate (EvalState &state, Value &val, ExprTree *&sig ) const
{
	ExprTree	*tree;
	ExprTree	*exprSig;
	const ClassAd *curAd;
	bool		rval;

	curAd = state.curAd;
	exprSig = NULL;
	rval 	= true;

	switch( FindExpr( state , tree , exprSig , true ) ) {
		case EVAL_FAIL:
			rval = false;
			break;

		case EVAL_ERROR:
		case PROP_ERROR:
			val.SetErrorValue( );
			break;

		case EVAL_UNDEF:
		case PROP_UNDEF:
			val.SetUndefinedValue( );
			break;

		case EVAL_OK:
		{
			if( state.depth_remaining <= 0 ) {
				val.SetErrorValue();
				state.curAd = curAd;
				return false;
			}
			state.depth_remaining--;

			rval = tree->Evaluate( state, val );

			state.depth_remaining++;

			break;
		}
		default:  CLASSAD_EXCEPT( "ClassAd:  Should not reach here" );
	}
	if(!rval || !(sig=new AttributeReference(exprSig,attributeStr,absolute))){
		if( rval ) {
			CondorErrno = ERR_MEM_ALLOC_FAILED;
			CondorErrMsg = "";
		}
		delete exprSig;
		sig = NULL;
		return( false );
	}
	state.curAd = curAd;
	return rval;
}


bool AttributeReference::
_Flatten( EvalState &state, Value &val, ExprTree*&ntree, int*) const
{
	ExprTree	*tree;
	ExprTree	*dummy;
	const ClassAd *curAd;
	bool		rval;

	ntree = NULL; // Just to be safe...  wenger 2003-12-11.

		// find the expression and the evalstate
	curAd = state.curAd;
	switch( FindExpr( state, tree, dummy, false ) ) {
		case EVAL_FAIL:
			return false;

		case EVAL_ERROR:
		case PROP_ERROR:
			val.SetErrorValue();
			state.curAd = curAd;
			return true;

		case EVAL_UNDEF:
		case PROP_UNDEF:
			if( expr && state.flattenAndInline ) {
				ExprTree *expr_ntree = NULL;
				Value expr_val;
				if( state.depth_remaining <= 0 ) {
					val.SetErrorValue();
					state.curAd = curAd;
					return false;
				}
				state.depth_remaining--;

				rval = expr->Flatten(state,expr_val,expr_ntree);

				state.depth_remaining++;

				if( rval && expr_ntree ) {
					ntree = MakeAttributeReference(expr_ntree,attributeStr);
					if( ntree ) {
						state.curAd = curAd;
						return true;
					}
				}
				delete expr_ntree;
			}
			if( !(ntree = Copy( ) ) ) {
				CondorErrno = ERR_MEM_ALLOC_FAILED;
				CondorErrMsg = "";
				state.curAd = curAd;
				return( false );
			}
			state.curAd = curAd;
			return true;

		case EVAL_OK:
		{
			if( state.depth_remaining <= 0 ) {
				val.SetErrorValue();
				state.curAd = curAd;
				return false;
			}
			state.depth_remaining--;

			rval = tree->Flatten( state, val, ntree );

			state.depth_remaining++;

			// don't inline if it didn't flatten to a value, and clear cache
			// do inline if FlattenAndInline was called
			if( ntree ) {
				if( state.flattenAndInline ) {	// NAC
					return true;   				// NAC
				}								// NAC
				delete ntree;
				ntree = Copy( );
				val.SetUndefinedValue( );
			}

			state.curAd = curAd;
			return rval;
		}
		default:  CLASSAD_EXCEPT( "ClassAd:  Should not reach here" );
	}
	return false;
}


int AttributeReference::
FindExpr(EvalState &state, ExprTree *&tree, ExprTree *&sig, bool wantSig) const
{
	const ClassAd *current=NULL;
	const ExprList *adList = NULL;
	Value	val;
	bool	rval;

	sig = NULL;

	// establish starting point for search
	if( expr == NULL ) {
		// "attr" and ".attr"
		current = absolute ? state.rootAd : state.curAd;
		if( absolute && ( current == NULL ) ) {	// NAC - circularity so no root
			return EVAL_FAIL;					// NAC
		}										// NAC
	} else {
		// "expr.attr"
		rval=wantSig?expr->Evaluate(state,val,sig):expr->Evaluate(state,val);
		if( !rval ) {
			return( EVAL_FAIL );
		}

		if( val.IsUndefinedValue( ) ) {
			return( PROP_UNDEF );
		} else if( val.IsErrorValue( ) ) {
			return( PROP_ERROR );
		}
		
		if( !val.IsClassAdValue( current ) && !val.IsListValue( adList ) ) {
			return( EVAL_ERROR );
		}
	}

	if( val.IsListValue( ) ) {
		vector< ExprTree *> eVector;
		const ExprTree *currExpr;
			// iterate through exprList and apply attribute reference
			// to each exprTree
		for(ExprListIterator itr(adList);!itr.IsAfterLast( );itr.NextExpr( )){
 			currExpr = itr.CurrentExpr( );
			if( currExpr == NULL ) {
				return( EVAL_FAIL );
			} else {
				AttributeReference *attrRef = NULL;
				attrRef = MakeAttributeReference( currExpr->Copy( ),
												  attributeStr,
												  false );
				val.Clear( );
					// Create new EvalState, within this scope, because
					// attrRef is only temporary, so we do not want to
					// cache the evaluated result in the outer state object.
				EvalState tstate;
				tstate.SetScopes(state.curAd);
				rval = wantSig ? attrRef->Evaluate( tstate, val, sig )
					: attrRef->Evaluate( tstate, val );
				delete attrRef;
				if( !rval ) {
					return( EVAL_FAIL );
				}
				
				ClassAd *evaledAd = NULL;
				const ExprList *evaledList = NULL;
				if( val.IsClassAdValue( evaledAd ) ) {
					eVector.push_back( evaledAd );
					continue;
				}
				else if( val.IsListValue( evaledList ) ) {
					eVector.push_back( evaledList->Copy( ) );
					continue;
				}
				else {
					eVector.push_back( Literal::MakeLiteral( val ) );
				}
			}
		}
		tree = ExprList::MakeExprList( eVector );
		ClassAd *newRoot = new ClassAd( );
		((ExprList*)tree)->SetParentScope( newRoot );
		return EVAL_OK;
	}
		// lookup with scope; this may side-affect state		

		/* ClassAd::alternateScope is intended for transitioning Condor from
		 * old to new ClassAds. It allows unscoped attribute references
		 * in expressions that can't be found in the local scope to be
		 * looked for in an alternate scope. In Condor, the alternate
		 * scope is the Target ad in matchmaking.
		 * Expect alternateScope to be removed from a future release.
		 */
	int rc = current->LookupInScope( attributeStr, tree, state );
	if ( !expr && rc == EVAL_UNDEF && current->alternateScope ) {
		rc = current->alternateScope->LookupInScope( attributeStr, tree, state );
	}
	return rc;
}


AttributeReference *AttributeReference::
MakeAttributeReference(ExprTree *tree, const string &attrStr, bool absolut)
{
	return( new AttributeReference( tree, attrStr, absolut ) );
}

END_NAMESPACE // classad
