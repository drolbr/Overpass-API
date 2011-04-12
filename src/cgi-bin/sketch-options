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

<h2>Example file:</h2>

<pre>&lt;?xml version=&quot;1.0&quot; encoding=&quot;UTF-8&quot;?&gt;
&lt;options&gt;</pre>

The necessary XML header.

<pre>&lt;translate expr=&quot;to&quot; to=&quot;['to' in Italian]&quot;/&gt;
&lt;translate expr=&quot;from&quot; to=&quot;['from' in Italian]&quot;/&gt;
&lt;translate expr=&quot;or&quot; to=&quot;['or' in Italian]&quot;/&gt;
&lt;translate expr=&quot;and&quot; to=&quot;['and' in Italian]&quot;/&gt;
&lt;translate expr=&quot;operates&quot; to=&quot;operates&quot;/&gt;
&lt;translate expr=&quot;monday_short&quot; to=&quot;mo&quot;/&gt;
&lt;translate expr=&quot;tuesday_short&quot; to=&quot;tu&quot;/&gt;
&lt;translate expr=&quot;wednesday_short&quot; to=&quot;we&quot;/&gt;
&lt;translate expr=&quot;thursday_short&quot; to=&quot;th&quot;/&gt;
&lt;translate expr=&quot;friday_short&quot; to=&quot;fr&quot;/&gt;
&lt;translate expr=&quot;saturday_short&quot; to=&quot;sa&quot;/&gt;
&lt;translate expr=&quot;sunday_short&quot; to=&quot;su&quot;/&gt;</pre>

These words appear literally in the diagrams and can be replaced by
suitable translations.

<pre>&lt;reduce expr=&quot;Via &quot; to=&quot;&quot;/&gt;
&lt;reduce expr=&quot;Straße&quot; to=&quot;Str.&quot;/&gt;
&lt;reduce expr=&quot;straße&quot; to=&quot;str.&quot;/&gt;</pre>

The first one is an ommission of the prefix <em>Via</em> which is common in Italian.
The other two are examples for typical German abbrevations for the German transltion
of street.

<pre>&lt;width px=&quot;1200&quot;/&gt;
&lt;height px=&quot;400&quot;/&gt;
&lt;stop-font-size px=&quot;10&quot;/&gt;
&lt;force rows=&quot;1&quot;/&gt;
&lt;max-correspondences per-line=&quot;6&quot; below=&quot;0&quot;/&gt;</pre>

These correspond to the options that can be directly passed to sketch-line. They are
explained at the <a href=\"../public_transport.html\">public transport page</a>.

<pre>&lt;correspondences limit=&quot;100&quot;/&gt;</pre>

Enables the display of correspondences and sets the maximum walking distance to change
to 100 meters.

<pre>&lt;display k=&quot;amenity&quot; v=&quot;bank&quot; text=&quot;bank&quot;/&gt;
&lt;display k=&quot;park_ride&quot; text=&quot;P+R&quot;/&gt;</pre>

Displays at every stop where a nearby node is tagged as &quot;amenity&quot;=&quot;bank&quot;
the text &quot;bank&quot;. A node is nearby if it is less than the corredpondence walking distance from the stop. The second line lets display at every stop where a nearby node is tagged as &quot;park_ride&quot; with any value the text &quot;P+R&quot;.

<pre>&lt;/options&gt;</pre>

The necessary footer.

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
