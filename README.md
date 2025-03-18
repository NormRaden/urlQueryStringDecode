# urlQueryStringDecode

## DESCRIPTION

`urlQueryStringDecode` is a simple tool that translates URL query strings into a list of variable assignments suitable for Bash scripts. The URL query string is the part of a URL that follows the first question mark character ("?"):

 https://some.domainname.tld/resource?<URL query string\>

This URL query string decoder translates "<field1\>=<value1\>&<field2\>=<value2\>&...&<fieldN\>=<valueN\>" to:

 field1="value1"\
 field2="value2"\
 .\
 .\
 .\
 fieldN="valueN"


Additionally, a field without a value is handled slightly differently in that the equals character is omitted. For example, "...&<field\>&..." would be translated to:

.\
.\
.\
 field\
.\
.\
.

Non-alphanumeric characters that are part of the field/variable are mapped to underscores ("_"), while quote characters ('\"') that are part of the value are "escaped" by prepending a backslash ("\\"). Character sequences "%<hex digits\>" and "+" in the URL query are handled appropriately. For example, the URL query string:

```
date=2001-01-01&data.element1=nil&data.element2=empty+set&auth+type=pass&pass=A-3!dn=s6.d&result^code=ERROR&blob=dj832"-!%263k*e%25N,`dc~1$a&save
```

would translate to:

```
date="2001-01-01"
data_element1="nil"
data_element2="empty set"
auth_type="pass"
pass="A-3!dn=s6.d"
result_code="ERROR"
blob="dj832\"-!&3k*e%N,`dc~1$a"
save
```

## BUILD

To build 'urlQueryStringDecode':

```
# make
```

## VERIFICATION

A minimal verification script is available to test the correctness of urlQueryStringDecode to catch most errors introduced during development.

To run the verification script, invoke Makefile target 'verify' via:

```
# make verify
```

## USAGE

urlQueryStringDecode supports two ways to receive the URL query string:

Using option '-i' or '--input' reads a URL query string from standard input and writes the list of field/variable assignments to standard output:

```
# printURLquery | ./urlQueryStringDecode --input | handleAssignmentList
```

Using option '-s' or '--string' reads a URL query string from the next command line argument and writes the list of field/variable assignments to standard output:

```
# ./urlQueryStringDecode --string "<URL query string\>" | handleAssignmentList
```

Only one -i/--input or -s/--string option is supported for each invocation of urlQueryStringDecode.

Additionally, there is an optional -p/--prefix option that reads a prefix to prepend before fields from the next command line argument. This option should proceed the -i/--input or -s/--string option.

In a http server Bash script:

```
 #!/bin/sh
 #
 # Translate the URL query string from standard input to a variable assignment list and add it to the shell script's environment:

 source /dev/stdin <<< `urlQueryStringDecode --input`

 # Rest of script...
``` 
 
Or:

```
 #!/bin/sh
 #
 # Translate the URL query string from ${QUERY_STRING} to a variable assignment list and add it to the shell script's environment:

 source /dev/stdin <<< `urlQueryStringDecode --string "${QUERY_STRING}"`

 # Rest of script...
```

## PURPOSE

Created to assist in handling API decodes within simple Bash scripts.

