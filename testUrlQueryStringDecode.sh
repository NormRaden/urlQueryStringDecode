#!/bin/sh
#
# Perform basic verification.  Test a representative set of url query strings:
#

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

 RESULT=`echo "$1" | ${TOOL} -i`
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
# TODO: Add several more test cases.

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


# If all tests passed, inform us.

if [ "${TOOL_CORRECTNESS}" = "true" ]; then
 echo ${TOOL}" passes verification test."
else
 echo "ERROR: "${TOOL}" failed verification test."
fi

