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


#ifndef _MyString_H_
#define _MyString_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>  // for isspace and friends

#define HAVE_VASPRINTF  // at least true on Linux


class MyString;

/** The MyString class is a C++ representation of a string. It was
 * written before we could reliably use the standard string class.
 * For an example of how to use it, see test_mystring.C.
 *
 * A warning to anyone changing this code: as currently implemented,
 * an "empty" MyString can have two different possible internal
 * representations depending on its history.  Sometimes Data == NULL,
 * sometimes Data[0] == '\0'.  So don't assume one or the other...  We
 * should change this to never having NULL, but there is worry that
 * someone depends on this behavior.  */
class MyString 
{
  
 public:

	// ----------------------------------------
	//       Constructors and destructors      
	// ----------------------------------------
	/**@name Constructors and Destructors */
	//@{
	
	/** Default constructor  */
	MyString();  

	/** Constructor to make an integer string. For example, if you pass
	 *  50, you get the string "50".*/
	MyString(int i);

	/** Constructor to make a copy of a null-terminated character string. */
	MyString(const char* S);

	/** Copy constructor */
	MyString(const MyString& S);

	/** Destructor */
	~MyString();
    //@}

	// ----------------------------------------
	//               Accessors
	// ----------------------------------------
	/**@name Accessors */
	//@{

	/** Returns length of string */
	int Length()          const { return Len;                }

	/** Returns true if the string is empty, false otherwise */
	bool IsEmpty() const { return (Len == 0); }

	/** Returns space reserved for string */
	int Capacity()        const { return capacity;           }

	/** Returns a strdup()ed C string. */
	char *StrDup() const { return strdup( Value() );         }

	/** Returns string. 
		Note that it never returns NULL, but will return an 
	    empty string instead. */
	const char *Value()   const { return (Data ? Data : ""); }

	/** Returns a single character from the string. Returns 0
	 *  if out of bounds. */
	char operator[](int pos) const;

	/** Returns a single character from the string. Returns 0
	 *  if out of bounds. */
	const char& operator[](int pos);

	/** Sets the character at the given position to the given value,
	 *  if the position is within the string.  Setting the character
	 *  to '\0' truncates the string to end at that position. */
	void setChar(int pos, char value);


	//@}

	// ----------------------------------------
	//           Assignment Operators
	// ----------------------------------------
	/**@name Assignment Operators */
	//@{ 

	/** Copies a MyString into the object */
	MyString& operator=(const MyString& S);

	/** Copies a null-terminated character string into the object */
	MyString& operator=( const char *s );
	//@}

	// ----------------------------------------
	//           Memory Management
	// ----------------------------------------
	/**@name Memory Management */
	//@{ 
	/** This is like calling malloc: it makes sure the capacity of the 
	 *  string is sz bytes, and it copies whatever is in the string into
	 *  the memory. It will truncate the string if you decrease the size. 
	 *  You don't normally need to call this. */
	bool reserve(const int sz);

	/** This is like calling malloc, but more interesting: it makes
	 *  sure the capacity of the string is at least sz bytes, and
	 *  preferably twice sz bytes. It copies whatever is in the string
	 *  into the memory. It will truncate the string if you decrease
	 *  the size.  You don't normally need to call this--it's used to
	 *  make appending to a string more efficient.  */
	bool reserve_at_least(const int sz);
    //@}

	// ----------------------------------------
	//           Concatenating strings
	// ----------------------------------------
	/**@name Appending strings */
	//@{ 
	
	/** Appends a MyString */
	MyString& operator+=(const MyString& S);

	/** Appends a null-termianted string */
	MyString& operator+=(const char *s);

	/** Appends a single character. Note that this isn't as
		inefficient as you might think, because it won't merely
		increase the size of the string by one and copy into it, so
		you can append a bunch of characters at a time and it will act
		reasonably.  */
	MyString& operator+=(const char ); 

	/** Appends the string version of the given int */
	MyString& operator+=(int i);

	/** Appends the string version of the given unsigned int */
	MyString& operator+=(unsigned int ui);

	/** Appends the string version of the given long int */
	MyString& operator+=(long l);

	/** Appends the string version of the given double */
	MyString& operator+=(double d);


	/** Returns a new string that is S1 followed by S2 */
	friend MyString operator+(const MyString &S1, const MyString &S2); 
	//@}

	// ----------------------------------------
	//           Miscellaneous functions
	// ----------------------------------------
	/**@name Miscellaneous functions */
	//@{ 

	/** Returns a new MyString that is the portion of the string from 
	 *  pos1 to pos2 (including both pos1 and pos2). 
	 *  The first character in the string is position 0. 
	 */
	MyString Substr(int pos1, int pos2) const;
    
	/** Returns a new MyString. Q is a string of characters that need
     *  to be escaped, and the "escape" is the character to put before
     *  each character. For example, if you pass "abc" and '\' and the
     *  original string is "Alain", you will get "Al\ain" in a new
     *  string.  */
	MyString EscapeChars(const MyString& Q, const char escape) const;

	/** Returns the position at which a character is first found. Begins
	 * counting from FirstPos. Returns -1 if it's not found. 
	 */
	int FindChar(int Char, int FirstPos=0) const;

	/** Calculates a hash function on the string. */
	unsigned int Hash() const;

	/** Returns the zero-based index of the first character of a
     *  substring, if it is contained within the MyString. Begins
     *  looking at iStartPost.  Returns -1 if pszToFind is not
	 *  found, 0 if pszToFind is empty. */
	int find(const char *pszToFind, int iStartPos = 0) const;


	/** Fills a MyString with what you would have gotten from sprintf.
	 *  It's safe though, and it will accept whatever you print into it. 
	 *  Assuming, of course, that you don't run out of memory. 
	 *  The returns true if it succeeded, false otherwise.
	 */
	bool sprintf(const char *format, ...);
	bool vsprintf(const char *format, va_list args);


	/** Like sprintf, but this appends to existing data. */
	bool sprintf_cat(const char *format, ...);
	bool vsprintf_cat(const char *format, va_list args);


	void lower_case(void);
	void upper_case(void);

	/** If the last character in the string is a newline, remove
		it (by setting it to '\0' and decrementing Len).
		If the newline is preceeded by a '\r', remove that as well.
		@return True if we removed a newline, false if not
	*/  
	bool chomp( void );

	/** Trim leading and trailing whitespace from this string.
	*/
	void trim( void );

	/** Remove all the whitespace from this string
	*/
	void compressSpaces( void );

	//@}

	// ----------------------------------------
	//           Comparisons
	// ----------------------------------------
	/**@name Comparisons */
	//@{ 
	/** Compare two MyStrings to see if they are same */
	friend int operator==(const MyString& S1, const MyString& S2);

	/** Compare a MyString with a null-terminated C string to see if
        they are the same.  */
	friend int operator==(const MyString& S1, const char     *S2);
	friend int operator==(const char     *S1, const MyString& S2);

	/** Compare two MyStrings to see if they are different. */
	friend int operator!=(const MyString& S1, const MyString& S2);

	/** Compare a MyString with a null-terminated C string to see if
        they are different.  */
	friend int operator!=(const MyString& S1, const char     *S2);
	friend int operator!=(const char     *S1, const MyString& S2);

	/** Compare two MyStrings to see if the first is less than the
        second.  */
	friend int operator< (const MyString& S1, const MyString& S2);

	/** Compare two MyStrings to see if the first is less than or
        equal to the second.  */
	friend int operator<=(const MyString& S1, const MyString& S2);

	/** Compare two MyStrings to see if the first is greater than the
        second.  */
	friend int operator> (const MyString& S1, const MyString& S2);

	/** Compare two MyStrings to see if the first is greater than or
        equal to the second.  */
	friend int operator>=(const MyString& S1, const MyString& S2);
	//@}

	// ----------------------------------------
	//           I/O
	// ----------------------------------------
  
	/** Safely read from the given file until we've hit a newline or
		an EOF.  We use fgets() in a loop to make sure we've read data
		until we find a newline.  If the buffer wasn't big enough and
		there's more to read, we fgets() again and append the results
		to ourselves.  If we hit EOF right away, we return false, and
		the value of this MyString is unchanged.  If we read any data
		at all, that's now the value of this MyString, and we return
		true.  If we hit EOF before a newline, we still return true,
		so don't assume a newline just because this returns true.
		@param fp The file you want to read from
		@returns True if we found data, false if we're at the EOF 
	 */
	bool readLine( FILE* fp, bool append = false);

	// ----------------------------------------
	//           Tokenize (safe replacement for strtok())
	// ----------------------------------------
	/**@name Tokenize */
	//@{ 

	/** Initialize the tokenizing of this string.  */
	void Tokenize();

	/** Get the next token, with tokens separated by the characters
	    in delim.  Note that the value of delim may change from call to
		call.
		WARNING: changing the value of this object between a call to
		Tokenize() and a call to GetNextToken() will result in an error
		(incorrect value from GetNextToken()).
	    */
	const char *GetNextToken(const char *delim, bool skipBlankTokens);
	//@}

private:

	/** Returns string. Note that it may return NULL */
	const char *GetCStr() const { return Data;               }

    void init();
	void append_str( const char *s, int s_len );
	void assign_str( const char *s, int s_len );

  char* Data;	// array containing the C string of this MyString's value
  char dummy;	// used for '\0' char in operator[] when the index
  				// is past the end of the string (effectively it's
				// a const, but compiler doesn't like that)
  int Len;		// the length of the string
  int capacity;	// capacity of the data array, not counting null terminator

  char *tokenBuf;
  char *nextToken;
  
};

unsigned int MyStringHash( const MyString &str );


// YourSensitiveString is a case-sensitive string class that holds a
// string pointer (no copying or freeing) for use in a HashTable.

class YourSensitiveString {
public:
	YourSensitiveString() : m_str(0) {}
	YourSensitiveString(char const *str) {
		m_str = str;
	}
	bool operator ==(const YourSensitiveString &rhs) {
		if (m_str == rhs.m_str) return true;
		if ((!m_str) || (!rhs.m_str)) return false;
		return strcmp(m_str,rhs.m_str) == 0;
	}
	void operator =(char const *str) {
		m_str = str;
	}
	static unsigned int hashFunction(const YourSensitiveString &s) {
		// hash function for strings
		// Chris Torek's world famous hashing function
		unsigned int hash = 0;
		if (!s.m_str) return 7; // Least random number

		const char *p = s.m_str;
		while (*p) {
			hash = (hash<<5)+hash + (unsigned char)*p;
			p++;
		}

		return hash;
	}


private:
	char const *m_str;
};


#endif
