/**

Copyright 2023- Norman Raden

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/


#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct
{
  enum
  {
    SINGLE_CHAR,
    FIRST_HEXADECIMAL,
    SECOND_HEXADECIMAL
  } charDecode;
  char encodedChar;
} characterDecodeState;

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

void characterDecodeStateInitialize(characterDecodeState *state)
{
  state->charDecode = SINGLE_CHAR;
  state->encodedChar = (char)0;
}

void decodeURLQueryStringAndPrint(char rawChar, characterDecodeState *state, void (*rewriteCharacters)(char))
{
  int digit;

  switch(state->charDecode)
  {
    case SINGLE_CHAR:
      if(rawChar == '%')
      {
        state->charDecode = FIRST_HEXADECIMAL;
        state->encodedChar = (char)0;
      }
      else if(rawChar == '+')
      {
        rewriteCharacters(' ');
      }
      else
      {
        rewriteCharacters(rawChar);
      }
      break;

    case FIRST_HEXADECIMAL:
      digit = decodeHexDigit(rawChar);
      if(digit >= 0)
      {
        state->encodedChar = digit<<4;
        state->charDecode = SECOND_HEXADECIMAL;
      }
      else
      {
        state->charDecode = SINGLE_CHAR;  /* Error in decode */
        rewriteCharacters('%');
        rewriteCharacters(rawChar);
      }
      break;

    case SECOND_HEXADECIMAL:
      digit = decodeHexDigit(rawChar);
      if(digit >= 0)
      {
        state->encodedChar |= digit;
        state->charDecode = SINGLE_CHAR;
        rewriteCharacters(state->encodedChar);
      }
      else
      {
        state->charDecode = SINGLE_CHAR;  /* Error in decode */
        rewriteCharacters(state->encodedChar>>4);
        rewriteCharacters(rawChar);
      }
      break;
  }
}

void passThroughCharacters(char c)
{
  putchar(c);
}

void escapeDoubleQuoteCharacters(char c)
{
  if(c == '"')
    putchar('\\');

  putchar(c);
}

void replaceNonAlphaNumericsWithUnderscores(char c)
{
  if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
    putchar(c);
  else
    putchar('_');
}

typedef struct
{
  enum
  {
    IDENTIFIER_TOKEN_PENDING,
    IDENTIFIER_TOKEN,
    VALUE_TOKEN,
    UNKNOWN_TOKEN
  } tokenDecode;
} parseURLQueryStringState;

void parseURLQueryStringInitialize(parseURLQueryStringState *state)
{
  state->tokenDecode = IDENTIFIER_TOKEN_PENDING;
}

void parseURLQueryString(int c, parseURLQueryStringState *parseState, characterDecodeState *characterState)
{
  switch(parseState->tokenDecode)
  {
    case IDENTIFIER_TOKEN_PENDING:
      if(c == '=')
      {
        parseState->tokenDecode = UNKNOWN_TOKEN;
      }
      else if (!((c == '&') || (c == -1)))
      {
        parseState->tokenDecode = IDENTIFIER_TOKEN;
        decodeURLQueryStringAndPrint(c, characterState, replaceNonAlphaNumericsWithUnderscores);
      }
      break;
    case IDENTIFIER_TOKEN:
      if(c == '=')
      {
         putchar('=');
         putchar('"');
         parseState->tokenDecode = VALUE_TOKEN;
      }
      else if((c == '&') || (c == -1))
      {
        putchar('\n');
        parseState->tokenDecode = IDENTIFIER_TOKEN_PENDING;
      }
      else
      {
        decodeURLQueryStringAndPrint(c, characterState, replaceNonAlphaNumericsWithUnderscores);
      }
      break;

    case VALUE_TOKEN: 
      if((c == '&') || (c == -1))
      {
        putchar('"');
        putchar('\n');
        parseState->tokenDecode = IDENTIFIER_TOKEN_PENDING;
      }
      else
      {
        decodeURLQueryStringAndPrint(c, characterState, escapeDoubleQuoteCharacters);
      }
      break;

    case UNKNOWN_TOKEN:
      if((c == '&'))
      {
        parseState->tokenDecode = IDENTIFIER_TOKEN_PENDING;
      }
      break;
  }
}

/* NOTE: For interfacing in bash:
 *   source /dev/stdin <<< `command`
 */

int main(int argc, char** argv)
{
  int c;
  char *urlString;

  bool performedAction = false;

  parseURLQueryStringState parse;
  parseURLQueryStringInitialize(&parse);

  characterDecodeState decode;
  characterDecodeStateInitialize(&decode);

  int option;

  while ((option = getopt(argc, argv, "is:")) != -1)
  {
    switch(option)
    {
      case 'i':
        while((c = getchar()) != EOF)
          parseURLQueryString(c, &parse, &decode);
        parseURLQueryString(-1, &parse, &decode);
        performedAction = true;
        break;
      case 's':
        for(urlString = optarg; *urlString != '\0'; urlString++)
          parseURLQueryString(*urlString, &parse, &decode);
        parseURLQueryString(-1, &parse, &decode);
        performedAction = true;
        break;
    }
  }
  if(performedAction == false)
  {
    fprintf(stderr, "Usage: %s [-i | -s <urlquery> ]\n", argv[0]);
    fprintf(stderr, "\t-i  Read url query string from stdio and parse\n\t-s <url query string>  Parse url query string parameter\n");
    return(-1);
  }
  else
    return(0);
}

