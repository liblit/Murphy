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


#ifndef __CLASSAD_EXPR_TREE_H__
#define __CLASSAD_EXPR_TREE_H__

#include "classad/classad_stl.h"
#include "classad/common.h"
#include "classad/value.h"

BEGIN_NAMESPACE( classad )


// forward declarations
class ExprTree;
class ClassAd;
class MatchClassAd;

class EvalState {
	public:
		EvalState( );
		~EvalState( );

		void SetRootScope( );
		void SetScopes( const ClassAd* );

		int depth_remaining; // max recursion depth - current depth

		const ClassAd *rootAd;
		const ClassAd *curAd;

		bool		flattenAndInline;	// NAC

		// Cache_to_free are the things in the cache that must be
		// freed when this gets deleted. The problem is that we put
		// two kinds of things into the cache: some that must be
		// freed, and some that must not be freed. We keep track of
		// the ones that must be freed separately.  Memory managment
		// is a pain! We should all use languages that do memory
		// management for you.
		//EvalCache   cache_to_delete; 
};

/** A node of the expression tree, which may be a literal, attribute reference,
	function call, classad, expression list, or an operator applied to other
	ExprTree operands.
	@see NodeKind
*/
class ExprTree 
{
  	public:
			/// The kinds of nodes in expression trees
		enum NodeKind {
	    	/// Literal node (string, integer, real, boolean, undefined, error)
	    	LITERAL_NODE,
			/// Attribute reference node (attr, .attr, expr.attr) 
			ATTRREF_NODE,
			/// Expression operation node (unary, binary, ternary)/
			OP_NODE,
			/// Function call node 
	   		FN_CALL_NODE,
			/// ClassAd node 
			CLASSAD_NODE,
			/// Expression list node 
			EXPR_LIST_NODE
		};

		/// Virtual destructor
		virtual ~ExprTree ();

		/** Sets the lexical parent scope of the expression, which is used to 
				determine the lexical scoping structure for resolving attribute
				references. (However, the semantic parent may be different from 
				the lexical parent if a <tt>super</tt> attribute is specified.) 
				This method is automatically called when expressions are 
				inserted into ClassAds, and should thus be called explicitly 
				only when evaluating expressions which haven't been inserted 
				into a ClassAd.
			@param p The parent ClassAd.
		*/
		void SetParentScope( const ClassAd* p );

		/** Gets the parent scope of the expression.
		 	@return The parent scope of the expression.
		*/
		const ClassAd *GetParentScope( ) const { return( parentScope ); }

		/** Makes a deep copy of the expression tree
		 * 	@return A deep copy of the expression, or NULL on failure.
		*/
		virtual ExprTree *Copy( ) const = 0;

		/** Gets the node kind of this expression node.
			@return The node kind.
			@see NodeKind
		*/
		NodeKind GetKind (void) const { return nodeKind; }

		/// A debugging method; send expression to stdout
		void Puke( ) const;

        /** Evaluate this tree
         *  @param state The current state
         *  @param val   The result of the evaluation
         *  @return true on success, false on failure
         */
		bool Evaluate( EvalState &state, Value &val ) const; 
		
        /** Evaluate this tree.
         *  This only works if the expression is currently part of a ClassAd.
         *  @param val   The result of the evaluation
         *  @return true on success, false on failure
         */
		bool Evaluate( Value& v ) const;

        /** Is this ExprTree the same as the tree?
         *  @return true if it is the same, false otherwise
         */
        virtual bool SameAs(const ExprTree *tree) const = 0;

  	protected:
		ExprTree ();

        /** Fill in this ExprTree with the contents of the other ExprTree.
         *  @return true if the copy succeeded, false otherwise.
         */
        void CopyFrom(const ExprTree &literal);

		bool Evaluate( Value& v, ExprTree*& t ) const;
		bool Flatten( Value& val, ExprTree*& tree) const;

		bool Flatten( EvalState&, Value&, ExprTree*&, int* = NULL ) const;
		bool Evaluate( EvalState &, Value &, ExprTree *& ) const;

		const ClassAd	*parentScope;
		NodeKind		nodeKind;

		enum {
			EVAL_FAIL,
			EVAL_OK,
			EVAL_UNDEF,
			PROP_UNDEF,
			EVAL_ERROR,
			PROP_ERROR
		};


  	private:
		friend class Operation;
		friend class AttributeReference;
		friend class FunctionCall;
		friend class FunctionTable;
		friend class ExprList;
		friend class ExprListIterator;
		friend class ClassAd; 

		/// Copy constructor
        ExprTree(const ExprTree &tree);
        ExprTree &operator=(const ExprTree &literal);

		virtual void _SetParentScope( const ClassAd* )=0;
		virtual bool _Evaluate( EvalState&, Value& ) const=0;
		virtual bool _Evaluate( EvalState&, Value&, ExprTree*& ) const=0;
		virtual bool _Flatten( EvalState&, Value&, ExprTree*&, int* )const=0;

        friend bool operator==(const ExprTree &tree1, const ExprTree &tree2);
};

std::ostream& operator<<(std::ostream &os, const ExprTree &expr);

END_NAMESPACE // classad

#include "classad/literals.h"
#include "classad/attrrefs.h"
#include "classad/operators.h"
#include "classad/exprList.h"
#include "classad/classad.h"
#include "classad/fnCall.h"

#endif//__CLASSAD_EXPR_TREE_H__
