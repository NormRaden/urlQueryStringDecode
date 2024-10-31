#!/bin/sh
#
# Perform basic verification.  Test a representative set of url query strings:
#
# RUN THIS VERIFICATION SCRIPT BEFORE EACH COMMIT!

TOOL=./urlQueryStringDecode
TOOL_CORRECTNESS=true

# Function to test both input options against the same pair of url query string and correct result.
# Let us know about any discrepancies.

function testUrlQueryStringDecoder ()
{
 local RESULT

# Test the '-s' option:

 RESULT=`${TOOL} -s "$1"`
 if [ "${RESULT}" != "$2" ]; then
  echo "ERROR: "${TOOL}" -s '"$1"' generated:"
  echo
  echo "${RESULT}"
  echo
  TOOL_CORRECTNESS=false
 fi

# Test the '-i' option:

 RESULT=`echo -n "$1" | ${TOOL} -i`
 if [ "${RESULT}" != "$2" ]; then
  echo "ERROR: echo '"$1"' | "${TOOL}" -i generated:"
  echo
  echo "${RESULT}"
  echo
  TOOL_CORRECTNESS=false
 fi
}

# Is the tool accessible?

if [ ! -x ${TOOL} ]; then
 echo "ERROR: "${TOOL}" is not accessible!"
 exit
fi


# List of select url query strings and correct outputs to try:
# TODO: Add more test cases.

testUrlQueryStringDecoder 'AAA=foba&B=42&abort' \
\
'AAA="foba"
B="42"
abort'

testUrlQueryStringDecoder 'date=2001-01-01&data.element1=nil&data.element2=empty+set&auth+type=pass&pass=A-3!dn=s6.d&result^code=ERROR&blob=dj832"-!%263k*e%25N,`dc~1$a&save' \
\
'date="2001-01-01"
data_element1="nil"
data_element2="empty set"
auth_type="pass"
pass="A-3!dn=s6.d"
result_code="ERROR"
blob="dj832\"-!&3k*e%N,`dc~1$a"
save'

testUrlQueryStringDecoder '&name=JOE&&place=EXETER&cid=293704264&&' \
\
'name="JOE"
place="EXETER"
cid="293704264"'

testUrlQueryStringDecoder 'done=&=&a=1&b=2&&aa&ab=&ac=z' \
\
'done=""
a="1"
b="2"
aa
ab=""
ac="z"'

testUrlQueryStringDecoder '&name=John&&&=sdsdfsdf&location=here&&=&&end=' \
\
'name="John"
location="here"
end=""'

testUrlQueryStringDecoder 'a=1&b=2&b=null=none&b=none=null&=none=null&&=null=none&command=load=file=final' \
\
'a="1"
b="2"
b="null=none"
b="none=null"
command="load=file=final"'

testUrlQueryStringDecoder '&&&===&&&===&&&=&=&======&&&&&' \
\
''

testUrlQueryStringDecoder '==dfs545g&&=gdsfg34256=fdgsdfg=&=f434534....%*#$0&=' \
\
''

testUrlQueryStringDecoder 'customerID=#394BC30A4&random=23489..hfsd3^4^@$`!#0)*(<;,>:?]}[-_~' \
\
'customerID="#394BC30A4"
random="23489..hfsd3^4^@$`!#0)*(<;,>:?]}[-_~"'

testUrlQueryStringDecoder 'a="""""&b="123"' \
\
'a="\"\"\"\"\""
b="\"123\""'

testUrlQueryStringDecoder 'EXTRAS=%20%21%22%23%24%25%26%28%29%2A%2B%2C%2D%2E%2F%3A%3B%3C%3D%3E%3F%5B%5C%5E%5F%7B%7C%7D%7E&extras=%20%21%22%23%24%25%26%28%29%2a%2b%2c%2d%2e%2f%3a%3b%3c%3d%3e%3f%5b%5c%5e%5f%7b%7c%7d%7e' \
\
'EXTRAS=" !\"#$%&()*+,-./:;<=>?[\^_{|}~"
extras=" !\"#$%&()*+,-./:;<=>?[\^_{|}~"'

testUrlQueryStringDecoder '' \
\
''


#
# If all tests passed, inform us.
#

if [ "${TOOL_CORRECTNESS}" = "true" ]; then
 echo ${TOOL}" passes verification test."
else
 echo "ERROR: "${TOOL}" failed verification test."
fi

