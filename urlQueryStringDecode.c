/**

Copyright 2023- Norman Raden

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

/*
 * This program translates URL query strings like "field1=value1&field2=value2&field3=value3&..." to:
 *
 *   field1="value1"
 *   field2="value2"
 *   field3="value3"
 *   [...]
 *
 *  which is a format more suitable for some scripting languages. Handles most of the character
 *  encodings specific to URL query strings.
 *
 *  Supports:
 *    - reading URL query strings from standard input.
 *    - reading URL query strings as arguments on the command line.
 *    - prepending a selected string before the fields.
 *
 *  Currently, non-alphanumeric characters in field's are replaced by underscores ('_').
 *  Value's are double-quoted with double-quote characters are prepended by escape characters ('\')
 *  that are a part of 'value'.
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

/*
 * Data structure 'characterDecodeState' is used to store the state for the state machine
 * that decodes URL query string's character encodings.
 */

typedef struct
{
  enum /* List of states */
  {
    SINGLE_CHAR,	/* Handling simple single characters. */
    FIRST_HEXADECIMAL,	/* Handling the first hexadecimal digit in a '%FS' percent-encoding. */
    SECOND_HEXADECIMAL	/* Handling the second hexadecimal digit in a '%FS' percent-encoding. */
  } charDecode;
  char encodedChar;	/* Storage for constructing the single character from the percent-encoding. */
} characterDecodeState;

/*
 * Function 'decodeHexDigit' simply returns the hexadecimal value inferred from character 'digit'.
 * If no legal value can be determined, then return '-1'.
 */

int decodeHexDigit(char digit)
{
  if(digit >= '0' && digit <= '9')
    return (digit - '0');
  else if(digit >= 'a' && digit <= 'f')
    return (digit - 'a' + 10);
  else if(digit >= 'A' && digit <= 'F')
    return (digit - 'A' + 10);
  else
    return -1;
}

/*
 * Function 'characterDecodeStateInitialize' initializes the state of URL query string character decoder.
 */

void characterDecodeStateInitialize(characterDecodeState *state)
{
  state->charDecode = SINGLE_CHAR;
  state->encodedChar = (char)0;
}

/*
 * Function 'decodeURLQueryStringAndPrint' receives URL query string characters one at a time via 'rawChar'
 * and writes the decoded and reformatted 'field=value' list directly to standard out. 'state' points to storage for
 * the state machine. 'rewriteCharacters' is a pointer to another function that further re-formats output characters.
 */

void decodeURLQueryStringAndPrint(char rawChar, characterDecodeState *state, void (*rewriteCharacters)(char))
{
  int digit;	/* Place to put a hexadecimal value needed to decode '%xx' percent-encodings. */

  switch(state->charDecode)
  {
    case SINGLE_CHAR:
      if(rawChar == '%')	/* Begin handling the percent-encoding '%xx'. */
      {
        state->charDecode = FIRST_HEXADECIMAL;
        state->encodedChar = (char)0;
      }
      else if(rawChar == '+')	/* Single '+' are decoded to ' ' characters, forward to 'rewriteCharacters'. */
      {
        rewriteCharacters(' ');
      }
      else	/* All other characters are forwarded to 'rewriteCharacters' for further processing. */
      {
        rewriteCharacters(rawChar);
      }
      break;

    case FIRST_HEXADECIMAL:
      digit = decodeHexDigit(rawChar);	/* Handle first hexadecimal character of '%xx' percent-encoding. */
      if(digit >= 0)
      {
        state->encodedChar = digit<<4;	/* If valid hexadecimal, add it to the resultant character. */
        state->charDecode = SECOND_HEXADECIMAL;	/* Move on to the next/last hexadecimal character. */
      }
      else
      {
        state->charDecode = SINGLE_CHAR;  /* Invalid '%xx' percent-encoding, treat them like single characters. */
        rewriteCharacters('%');
        rewriteCharacters(rawChar);
      }
      break;

    case SECOND_HEXADECIMAL:
      digit = decodeHexDigit(rawChar);	/* Handle second hexadecimal character of '%xx' percent-encoding. */
      if(digit >= 0)	/* If valid hexadecimal, add it to the resultant character. */
      {
        state->encodedChar |= digit;
        state->charDecode = SINGLE_CHAR;
        rewriteCharacters(state->encodedChar);
      }
      else
      {
        /*
         * Invalid second hexadecimal character, handle as a short '%x' percent-encoding and the second character
         * as a separate character. Hand both off to 'rewriteCharacters' for final processing.
         */
        state->charDecode = SINGLE_CHAR;
        rewriteCharacters(state->encodedChar>>4);
        rewriteCharacters(rawChar);
      }
      break;
  }
}

/*
 * Functions used to perform additional formatting on decoded characters before sending to standard out:
 */

void passThroughCharacters(char c)	/* Pass through characters unaltered. (currently unused) */
{
  putchar(c);
}

void escapeDoubleQuoteCharacters(char c)	/* Prepend '"' with '\', otherwise pass through. */
{
  if(c == '"')
    putchar('\\');

  putchar(c);
}

void replaceNonAlphaNumericsWithUnderscores(char c)	/* Pass through all alpha-numeric characters, otherwise replace with '_'. */
{
  if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
    putchar(c);
  else
    putchar('_');
}

/*
 * Data structure 'parseURLQueryStringState' used to store the state of the parser that handles
 * field and value pairs in URL query strings.
 */

typedef struct
{
  enum	/* List of states */
  {
    FIELD_TOKEN_PENDING,	/* Expecting a field */
    FIELD_TOKEN,	/* Process a field */
    VALUE_TOKEN,	/* Process a value */
    UNKNOWN_TOKEN	/* Handle a sequence of invalid characters */
  } tokenDecode;
} parseURLQueryStringState;

/*
 * Function 'parseURLQueryStringInitialize' initializes the state of URL query string parser.
 */

void parseURLQueryStringInitialize(parseURLQueryStringState *state)
{
  state->tokenDecode = FIELD_TOKEN_PENDING;
}

/*
 * Function 'parseURLQueryString' receives URL query string characters one at a time via 'c' and parses,
 * decodes characters encodings, applies additional reformatting and writes the translated 'field=value'
 * to standard out. 'parseState' points to storage for the parser state machine. 'characterState' points
 * to storage for character decoder state machine. 'prependToToken' is a string that is inserted before
 * the field tokens.
 */

void parseURLQueryString(int c, parseURLQueryStringState *parseState, characterDecodeState *characterState, const char *prependBeforeToken)
{
  switch(parseState->tokenDecode)
  {
    case FIELD_TOKEN_PENDING:
      if(c == '=')	/* Field token beginning with '=' is invalid. */
      {
        parseState->tokenDecode = UNKNOWN_TOKEN;
      }
      else if (!((c == '&') || (c == EOF)))	/* If first character isn't an '&' and EOF, begin handling the field. */
      {
        parseState->tokenDecode = FIELD_TOKEN;
        if(prependBeforeToken)	/* Is there a string to prepend before the first token of the field? */
        {
          printf(prependBeforeToken);	/* Insert a string right before the first character of the field token. */
        }
        decodeURLQueryStringAndPrint(c, characterState, replaceNonAlphaNumericsWithUnderscores);
      }
      break;

    case FIELD_TOKEN:
      if(c == '=')	/* Switch from handling a field to handling a value. */
      {
         putchar('=');
         putchar('"');
         parseState->tokenDecode = VALUE_TOKEN;
      }
      else if((c == '&') || (c == EOF))
      /* We received '&' instead of '=', handle the field-only case gracefully. */
      {
        putchar('\n');
        parseState->tokenDecode = FIELD_TOKEN_PENDING;
      }
      else
      /*
       * Decode characters that are part of the field and forward to
       * 'replaceNonAlphaNumericsWithUnderscores()' for reformatting.
       */
      {
        decodeURLQueryStringAndPrint(c, characterState, replaceNonAlphaNumericsWithUnderscores);
      }
      break;

    case VALUE_TOKEN:
      if((c == '&') || (c == EOF))	/* Finished up handling a value. */
      {
        putchar('"');
        putchar('\n');
        parseState->tokenDecode = FIELD_TOKEN_PENDING;
      }
      else
      /* Decode characters that are part of the value and forward to 'escapeDoubleQuoteCharacters()' for reformatting. */
      {
        decodeURLQueryStringAndPrint(c, characterState, escapeDoubleQuoteCharacters);
      }
      break;

    case UNKNOWN_TOKEN:
      if(c == '&')	/* Silently discard an invalid token until we re-synchronize. */
      {
        parseState->tokenDecode = FIELD_TOKEN_PENDING;
      }
      break;
  }
}


/*
 * Function 'main' translates a URL query string to a list of corresponding <field>=<value> lines.
 *
 * Syntax: urlQueryStringDecode
 *          -p, --prefix '<prefix string>'	Sets a prefix that is prepended before the generated <field>=<value> lines.
 *			-i, --input						Reads query string from standard input.
 *			-s, --string '<query string>'	Reads query string from command line.
 */

int main(int argc, char** argv)
{
  int c;
  char *urlString;
  char *prefixBeforeToken = NULL;

  /* Track whether a useful action was performed. */
  bool performedAction = false;

  /* Allocate and initialize query string parser state. */
  parseURLQueryStringState parse;
  parseURLQueryStringInitialize(&parse);

  /* Allocate and initialize query string character decoder state. */
  characterDecodeState decode;
  characterDecodeStateInitialize(&decode);

  /* Declare some long options to use instead of the short options. */
  struct option long_options[] =
  {
    {"input", no_argument, 0, 'i'},
    {"string", required_argument, 0, 's'},
    {"prefix", required_argument, 0, 'p'},
    {0, 0, 0, 0}
  };

  int option;

  /* Look for options '-i'/'--input' or '-s'/'--string'. */
  while ((option = getopt_long(argc, argv, "is:p:", long_options, NULL)) != -1)
  {
    if(performedAction == true) /* Limit to only handling the first -i or -s option. */
    {
      fprintf(stderr, "WARNING: Only the first -i/--input or -s/--string option is processed. Ignoring additional -i/--input or -s/--string options.\n");
      return(0);
    }

    switch(option)
    {
      case 'i':
        /* For option '-i', read each character from standard input till EOF, feeding each in to 'parseURLQueryString' */
        while((c = getchar()) != EOF)
          parseURLQueryString(c, &parse, &decode, prefixBeforeToken);
        parseURLQueryString(-1, &parse, &decode, prefixBeforeToken);
        performedAction = true;
        break;
      case 's':
        /* For option '-s', read next command line argument, feed each character in to 'parseURLQueryString' */
        for(urlString = optarg; *urlString != '\0'; urlString++)
          parseURLQueryString(*urlString, &parse, &decode, prefixBeforeToken);
        parseURLQueryString(-1, &parse, &decode, prefixBeforeToken);
        performedAction = true;
        break;
      case 'p':
        /* For option '-p', use string pointed at by the next command line argument as the string to prepend before all generated <field>=<value> */
        prefixBeforeToken = optarg;
        break;
    }
  }
  if(performedAction == false)	/* Neither option -s or -i was selected, give usage information and exit. */
  {
    fprintf(stderr, "Usage: %s [ -p <field prefix> | --prefix <field prefix> ] [-i | --input | -s <url query string> | --string <url query string> ]\n", argv[0]);
    fprintf(stderr, "\tOnly handles one -i/--input or -s/--string option per invocation.\n");
    fprintf(stderr, "\t-i, --input   Read url query string from standard input and parse\n\t-s, --string <url query string>  Parse url query string parameter\n");
    fprintf(stderr, "\t-p, --prefix  Prepend a string before generated <field>=<value> pairs. This option should proceed the -i/--input or -s/--string option.\n");
    return(-1);
  }
  else
    return(0);
}

