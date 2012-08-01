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
#include "classad/lexerSource.h"
#include <iostream>
using namespace std;
BEGIN_NAMESPACE(classad)

/*--------------------------------------------------------------------
 *
 * FileLexerSource
 *
 *-------------------------------------------------------------------*/

FileLexerSource::FileLexerSource(FILE *file)
{
	SetNewSource(file);
	return;
}

FileLexerSource::~FileLexerSource()
{
	_file = NULL;
	return;
}

void FileLexerSource::SetNewSource(FILE *file)
{
	_file = file;
	return;
}

int 
FileLexerSource::ReadCharacter(void)
{
	int character;

	if (_file != NULL) {
		character = fgetc(_file);
	} else {
		character = -1;
	}
 	_previous_character = character;
	return character;
}

void 
FileLexerSource::UnreadCharacter(void)
{
	//fseek(_file, -1, SEEK_CUR);
	ungetc(_previous_character, _file);
	return;
}

bool 
FileLexerSource::AtEnd(void) const
{
	bool at_end;
	
	if (_file != NULL) {
		at_end = (feof(_file) != 0);
	} else {
		at_end = true;
	}
	return at_end;
}

/*--------------------------------------------------------------------
 *
 * InputStreamLexerSource
 *
 *-------------------------------------------------------------------*/

InputStreamLexerSource::InputStreamLexerSource(istream &stream) 
{
	SetNewSource(stream);
	return;
}

InputStreamLexerSource::~InputStreamLexerSource()
{
	return;
}

void InputStreamLexerSource::SetNewSource(istream &stream)
{
	_stream = &stream;
	return;
}

int 
InputStreamLexerSource::ReadCharacter(void)
{
	char real_character;
	int  character;

	if (_stream != NULL && !_stream->eof()) {
		_stream->get(real_character);
		character = (unsigned char)real_character;
	} else {
		character = -1;
	}
   _previous_character = character;
	return character;
}

void 
InputStreamLexerSource::UnreadCharacter(void)
{
	//doesn't work on cin
	//_stream->seekg(-1, ios::cur);
	_stream->putback(_previous_character);
	return;
}

bool 
InputStreamLexerSource::AtEnd(void) const
{
	bool at_end;
	
	if (_stream != NULL) {
		at_end = (_stream->eof());
	} else {
		at_end = true;
	}
	return at_end;
}

/*--------------------------------------------------------------------
 *
 * CharLexerSource
 *
 *-------------------------------------------------------------------*/

CharLexerSource::CharLexerSource(const char *string, int offset)
{
	SetNewSource(string, offset);
	return;
}

CharLexerSource::~CharLexerSource()
{
	return;
}

void CharLexerSource::SetNewSource(const char *string, int offset)
{
    _string = string;
    _offset = offset;
	return;
}

int 
CharLexerSource::ReadCharacter(void)
{
	int character;

	character = (unsigned char)_string[_offset];
	if (character == 0) {
		character = -1; 
	} else {
        _offset++;
	}

 	_previous_character = character;
	return character;
}

void 
CharLexerSource::UnreadCharacter(void)
{
	if (_offset > 0) {
		_offset--;
	}
	return;
}

bool 
CharLexerSource::AtEnd(void) const
{
	bool at_end;

    if (_string[_offset] == 0) {
        at_end = true;
    } else {
        at_end = false;
    }
	return at_end;
}

int 
CharLexerSource::GetCurrentLocation(void) const
{
	return _offset;
}

/*--------------------------------------------------------------------
 *
 * StringLexerSource
 *
 *-------------------------------------------------------------------*/

StringLexerSource::StringLexerSource(const string *string, int offset)
{
	SetNewSource(string, offset);
	return;
}

StringLexerSource::~StringLexerSource()
{
	return;
}
	
void StringLexerSource::SetNewSource(const string *string, int offset)
{
	_string = string;
	_offset = offset;
	return;
}

int 
StringLexerSource::ReadCharacter(void)
{
	int character;

	character = (unsigned char)(*_string)[_offset];
	if (character == 0) {
		character = -1;
	} else {
		_offset++;
	}
 	_previous_character = character;
	return character;
}

void 
StringLexerSource::UnreadCharacter(void)
{
	if (_offset > 0) {
		_offset--;
	}
	return;
}

bool 
StringLexerSource::AtEnd(void) const
{
	bool at_end;

	if ((*_string)[_offset] == 0) {
		at_end = true;
	} else {
		at_end = false;
	}
	return at_end;
}

int 
StringLexerSource::GetCurrentLocation(void) const
{
	return _offset;
}

END_NAMESPACE
