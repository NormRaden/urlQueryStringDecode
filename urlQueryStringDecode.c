/**

Copyright 2023 Norman Raden

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/


#include <stdio.h>
#include <unistd.h>

typedef struct
{
  enum tokenDecodeState
  {
    IDENTIFIER,
    VALUE,
    NOIDENTIFIERVALUE
  } tokenDecode;
  int i;
} parseState;

typedef struct
{
  enum charDecodeState
  {
    PLAIN,
    FIRSTHEX,
    FINALHEX
  } charDecode;
  char encodedChar;
} decodeState;

void passThrough(char c)
{
  putchar(c);
}

void escapeSpecialCharacters(char c)
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

void decodedPrint(char rawChar, decodeState *state, void (*handleChar)(char))
{
  int digit;

  switch(state->charDecode)
  {
    case PLAIN:
      if(rawChar == '%')
      {
        state->charDecode = FIRSTHEX;
        state->encodedChar = (char)0;
      }
      else if(rawChar == '+')
      {
        handleChar(' ');
      }
      else
      {
        handleChar(rawChar);
      }
      break;

    case FIRSTHEX:
      digit = decodeHexDigit(rawChar);
      if(digit >= 0)
      {
        state->encodedChar = digit<<4;
        state->charDecode = FINALHEX;
      }
      else
      {
        state->charDecode = PLAIN;  /* Error in decode */
        handleChar('%');
        handleChar(rawChar);
      }
      break;

    case FINALHEX:
      digit = decodeHexDigit(rawChar);
      if(digit >= 0)
      {
        state->encodedChar |= digit;
        state->charDecode = PLAIN;
        handleChar(state->encodedChar);
      }
      else
      {
        state->charDecode = PLAIN;  /* Error in decode */
        handleChar(state->encodedChar>>4);
        handleChar(rawChar);
      }
      break;
  }
}

void parseURLQueryString(int c, parseState *p, decodeState *d)
{
  switch(p->tokenDecode)
  {
    case IDENTIFIER:
      if(c == '=')
      {
        if(p->i > 0)
        {
           putchar('=');
           putchar('"');
           p->tokenDecode = VALUE;
        }
        else
        {
          p->tokenDecode = NOIDENTIFIERVALUE;
        }
        p->i = -1;
      }
      else if((c == '&') || (c == -1) || (c == '\n'))
      {
        if(p->i > 0)
          putchar('\n');
        p->tokenDecode = IDENTIFIER;
        p->i = -1;
      }
      else
      {
        decodedPrint(c, d, replaceNonAlphaNumericsWithUnderscores);
      }
      break;

    case VALUE: 
      if((c == '&') || (c == -1) || (c == '\n'))
      {
        if(p->i > 0)
        {
          putchar('"');
          putchar('\n');
        }
        p->tokenDecode = IDENTIFIER;
        p->i = -1;
      }
      else
      {
        decodedPrint(c, d, escapeSpecialCharacters);
      }
      break;

    case NOIDENTIFIERVALUE:
      if((c == '&') || (c == '\n'))
      {
        p->tokenDecode = IDENTIFIER;
        p->i = -1;
      }
      break;
  }
  p->i++;
}

/* NOTE: For interfacing in bash:
 *   source /dev/stdin <<< `command`
 */

int main(int argc, char** argv)
{
  int c;
  char *e;

  int doanything = 0;

  parseState parse;
  parse.tokenDecode = IDENTIFIER;
  parse.i = 0;

  decodeState decode;
  decode.charDecode = PLAIN;

  int opt;

  while ((opt = getopt(argc, argv, "is:")) != -1)
  {
    switch(opt)
    {
      case 'i':
        while((c = getchar()) != EOF)
          parseURLQueryString(c, &parse, &decode);
        parseURLQueryString(-1, &parse, &decode);
        doanything = -1;
        break;
      case 's':
        e = optarg;

        for(parse.i=0;*e != '\0'; e++)
          parseURLQueryString(*e, &parse, &decode);
        parseURLQueryString(-1, &parse, &decode);
        doanything = -1;
        break;
    }
  }
  if(doanything == 0)
  {
    fprintf(stderr, "Usage: %s [-i | -s <urlquery> ]\n", argv[0]);
    fprintf(stderr, "\t-i  Read url query string from stdio and parse\n\t-s <url query string>  Parse url query string parameter\n");
    return(-1);
  }
  else
    return(0);
}

