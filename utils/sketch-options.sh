#!/bin/bash

BUF=$QUERY_STRING\&

# echo "Content-Type: text/plain; charset=utf-8"
# echo

SKETCH_PARAMS=
ACTION=
STYLE=
DATA=

while [[ -n $BUF ]]; do
{
  KEY=`echo $BUF | awk '{ print substr($0,0,match($0,"=")); }'`
  BUF=`echo $BUF | awk '{ print substr($0,match($0,"=")+1); }'`
  VALUE=`echo $BUF | awk '{ print substr($0,0,match($0,"\&")); }'`
  BUF=`echo $BUF | awk '{ print substr($0,match($0,"\&")+1); }'`
  if [[ $KEY == "action" ]]; then
    ACTION=$VALUE
  elif [[ $KEY == "style" ]]; then
    STYLE=$VALUE
  elif [[ $KEY == "data" ]]; then
    DATA=$VALUE
  fi
};
done

echo "Content-Type: text/html; charset=utf-8\n"
echo

if [[ $ACTION == "edit" ]]; then
{
  echo -e "
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\
    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">
<head/>\
<body>\

<h1>Edit style \"$STYLE\"</h1>

<form action=\"sketch-options\" method=\"get\" accept-charset=\"UTF-8\">
  <textarea name=\"data\" rows=\"10\" cols=\"80\">"
  if [[ -r /opt/osm_why_api/options/sketch-line.$STYLE ]]; then
    ../bin/escape_xml </opt/osm_why_api/options/sketch-line.$STYLE
  fi
  echo -e "\
</textarea>
  <input type=\"hidden\" name=\"style\" value=\"$STYLE\"/>
  <input type=\"hidden\" name=\"action\" value=\"commit\"/>
  <input type=\"submit\"/>
</form>

</body>\
"
};
elif [[ $ACTION == "commit" ]]; then
{
  echo $DATA | ../bin/uncgi >/opt/osm_why_api/options/sketch-line.$STYLE
  
  echo -e "
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\
    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">
<head/>\
<body>\

<h1>Edit style \"$STYLE\"</h1>

The style has successfully been saved.

</body>\
"
};
fi
