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
#include "classad/exprTree.h"

using namespace std;

BEGIN_NAMESPACE( classad )

ExprList::
ExprList()
{
	nodeKind = EXPR_LIST_NODE;
	return;
}

ExprList::
ExprList(const vector<ExprTree*>& exprs)
{
	nodeKind = EXPR_LIST_NODE;
    CopyList(exprs);
	return;
}

ExprList::
ExprList(const ExprList &other_list)
{
	nodeKind = EXPR_LIST_NODE;
    CopyFrom(other_list);
    return;
}

ExprList::
~ExprList()
{
	Clear( );
}

ExprList &ExprList::
operator=(const ExprList &other_list)
{
    if (this != &other_list) {
        CopyFrom(other_list);
    }
    return *this;
}

void ExprList::
Clear ()
{
	vector<ExprTree*>::iterator itr;
	for( itr = exprList.begin( ); itr != exprList.end( ); itr++ ) {
		delete *itr;
	}
	exprList.clear( );
}

ExprTree *ExprList::
Copy( ) const
{
	ExprList *newList = new ExprList;

	if (newList != NULL) {
        if (!newList->CopyFrom(*this)) {
            delete newList;
            newList = NULL;
        }
    }
	return newList;
}


bool ExprList::
CopyFrom(const ExprList &other_list)
{
    bool success;

    success = true;

    ExprTree::CopyFrom(other_list);

	vector<ExprTree*>::const_iterator itr;
	for( itr = other_list.exprList.begin( ); itr != other_list.exprList.end( ); itr++ ) {
        ExprTree *newTree;
		if( !( newTree = (*itr)->Copy( ) ) ) {
            success = false;
			CondorErrno = ERR_MEM_ALLOC_FAILED;
			CondorErrMsg = "";
            break;
		}
		exprList.push_back( newTree );
	}

    return success;

}

bool ExprList::
SameAs(const ExprTree *tree) const
{
    bool is_same;

    if (this == tree) {
        is_same = true;
    } else if (tree->GetKind() != EXPR_LIST_NODE) {
        is_same = false;
    } else {
        const ExprList *other_list;

        other_list = (const ExprList *) tree;

        if (exprList.size() != other_list->exprList.size()) {
            is_same = false;
        } else {
            vector<ExprTree*>::const_iterator itr1, itr2;

            is_same = true;
            itr1 = exprList.begin();
            itr2 = other_list->exprList.begin();
            while (itr1 != exprList.end()) {
                ExprTree *tree1, *tree2;

                tree1 = (*itr1);
                tree2 = (*itr2);

                if (!tree1->SameAs(tree2)) {
                    is_same = false;
                    break;
                }
                itr1++;
                itr2++;
            }
        }
    }
    return is_same;
}

bool operator==(ExprList &list1, ExprList &list2)
{
    return list1.SameAs(&list2);
}


void ExprList::
_SetParentScope( const ClassAd *parent )
{
	vector<ExprTree*>::iterator	itr;
	for( itr = exprList.begin( ); itr != exprList.end( ); itr++ ) {
		(*itr)->SetParentScope( parent );
	}
}

ExprList *ExprList::
MakeExprList( const vector<ExprTree*> &exprs )
{
	ExprList *el = new ExprList;
	if( !el ) {
		CondorErrno = ERR_MEM_ALLOC_FAILED;
		CondorErrMsg = "";
		el = NULL;
	} else {
        el->CopyList(exprs);
    }
    return el;
}

void ExprList::
GetComponents( vector<ExprTree*> &exprs ) const
{
	vector<ExprTree*>::const_iterator itr;
	exprs.clear( );
	for( itr=exprList.begin( ); itr!=exprList.end( ); itr++ ) {
		exprs.push_back( *itr );
	}
	return;
}

void ExprList::
insert(iterator it, ExprTree* t)
{
    exprList.insert(it, t);
	return;
}

void ExprList::
push_back(ExprTree* t)
{
    exprList.push_back(t);
	return;
}

void ExprList::
erase(iterator it)
{
    delete *it;
    exprList.erase(it);
	return;
}

void ExprList::
erase(iterator f, iterator l)
{
    for (iterator it = f; it != l; ++it) {
		delete *it;
    }
	
    exprList.erase(f,l);
	return;
}

bool ExprList::
_Evaluate (EvalState &, Value &val) const
{
	val.SetListValue ((ExprList *)this);
	return( true );
}

bool ExprList::
_Evaluate( EvalState &, Value &val, ExprTree *&sig ) const
{
	val.SetListValue((ExprList* )this);
	return( ( sig = Copy( ) ) );
}

bool ExprList::
_Flatten( EvalState &state, Value &, ExprTree *&tree, int* ) const
{
	vector<ExprTree*>::const_iterator	itr;
	ExprTree	*expr, *nexpr;
	Value		tempVal;
	ExprList	*newList;

	tree = NULL; // Just to be safe...  wenger 2003-12-11.

	if( ( newList = new ExprList( ) ) == NULL ) return false;

	for( itr = exprList.begin( ); itr != exprList.end( ); itr++ ) {
		expr = *itr;
		// flatten the constituent expression
		if( !expr->Flatten( state, tempVal, nexpr ) ) {
			delete newList;
			tree = NULL;
			return false;
		}

		// if only a value was obtained, convert to an expression
		if( !nexpr ) {
			nexpr = Literal::MakeLiteral( tempVal );
			if( !nexpr ) {
				CondorErrno = ERR_MEM_ALLOC_FAILED;
				CondorErrMsg = "";
				delete newList;
				return false;
			}
		}
		// add the new expression to the flattened list
		newList->exprList.push_back( nexpr );
	}

	tree = newList;

	return true;
}

void ExprList::CopyList(const vector<ExprTree*> &exprs)
{
	vector<ExprTree*>::const_iterator itr;

	for( itr=exprs.begin( ); itr!=exprs.end( ); itr++ ) {	
		exprList.push_back( *itr );
	}

	return;
}

// Iterator implementation
ExprListIterator::
ExprListIterator( )
{
	l = NULL;
}


ExprListIterator::
ExprListIterator( const ExprList* list )
{
	Initialize( list );
}


ExprListIterator::
~ExprListIterator( )
{
}


void ExprListIterator::
Initialize( const ExprList *el )
{
	// initialize eval state
	l = el;
	state.curAd = l->parentScope;
	state.SetRootScope( );

	// initialize list iterator
	itr = l->exprList.begin( );
}


void ExprListIterator::
ToFirst( )
{
	if( l ) itr = l->exprList.begin( );
}


void ExprListIterator::
ToAfterLast( )
{
	if( l ) itr = l->exprList.end( );
}


bool ExprListIterator::
ToNth( int n ) 
{
	if( l && n >= 0 && l->exprList.size( ) > (unsigned)n ) {
		itr = l->exprList.begin( ) + n;
		return( true );
	}
	return( false );
}


const ExprTree* ExprListIterator::
NextExpr( )
{
	if( l && itr != l->exprList.end( ) ) {
		itr++;
		return( itr==l->exprList.end() ? NULL : *itr );
	}
	return( NULL );
}


const ExprTree* ExprListIterator::
CurrentExpr( ) const
{
	return( l && itr != l->exprList.end( ) ? *itr : NULL );
}


const ExprTree* ExprListIterator::
PrevExpr( )
{
	if( l && itr != l->exprList.begin( ) ) {
		itr++;
		return( *itr );
	}
	return( NULL );
}


bool ExprListIterator::
GetValue( Value& val, const ExprTree *tree, EvalState *es )
{
	Value				cv;
	EvalState			*currentState;

	if( !tree ) return false;

	// if called from user code, es == NULL so we use &state instead
	currentState = es ? es : &state;

	if( currentState->depth_remaining <= 0 ) {
		val.SetErrorValue();
		return false;
	}
	currentState->depth_remaining--;

	const ClassAd *tmpScope = currentState->curAd;
	currentState->curAd = tree->GetParentScope();
	tree->Evaluate( *currentState, val );
	currentState->curAd = tmpScope;

	currentState->depth_remaining++;

	return true;
}


bool ExprListIterator::
NextValue( Value& val, EvalState *es )
{
	return GetValue( val, NextExpr( ), es );
}


bool ExprListIterator::
CurrentValue( Value& val, EvalState *es )
{
	return GetValue( val, CurrentExpr( ), es );
}


bool ExprListIterator::
PrevValue( Value& val, EvalState *es )
{
	return GetValue( val, PrevExpr( ), es );
}


bool ExprListIterator::
GetValue( Value& val, ExprTree*& sig, const ExprTree *tree, EvalState *es ) 
{
	Value				cv;
	EvalState			*currentState;

	if( !tree ) return false;

	// if called from user code, es == NULL so we use &state instead
	currentState = es ? es : &state;

	if( currentState->depth_remaining <= 0 ) {
		val.SetErrorValue();
		return false;
	}
	currentState->depth_remaining--;

	const ClassAd *tmpScope = currentState->curAd;
	currentState->curAd = tree->GetParentScope();
	tree->Evaluate( *currentState, val, sig );
	currentState->curAd = tmpScope;

	currentState->depth_remaining++;

	return true;
}


bool ExprListIterator::
NextValue( Value& val, ExprTree*& sig, EvalState *es )
{
	return GetValue( val, sig, NextExpr( ), es );
}


bool ExprListIterator::
CurrentValue( Value& val, ExprTree*& sig, EvalState *es )
{
	return GetValue( val, sig, CurrentExpr( ), es );
}


bool ExprListIterator::
PrevValue( Value& val, ExprTree*& sig, EvalState *es )
{
	return GetValue( val, sig, PrevExpr( ), es );
}


bool ExprListIterator::
IsAtFirst( ) const
{
	return( l && itr == l->exprList.begin( ) );
}


bool ExprListIterator::
IsAfterLast( ) const
{
	return( l && itr == l->exprList.end( ) );
}

END_NAMESPACE // classad
